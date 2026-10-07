/*
 * ZyxelSshClient.cxx
 *
 * Copyright (C) 2026, Charles Chiou
 */

#include "ZyxelSshClient.hxx"
#include "Config.hxx"

#include <iostream>
#include <fstream>
#include <sstream>
#include <iomanip>
#include <cstring>
#include <chrono>
#include <thread>
#include <cctype>
#include <cerrno>
#include <fcntl.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <sys/stat.h>

#include <openssl/crypto.h>
#include <libssh2.h>

ZyxelSshClient::ZyxelSshClient()
    : _port(22),
      _socketFd(-1),
      _session(nullptr),
      _channel(nullptr),
      _state(SshClientState::DISCONNECTED),
      _promptState(PromptState::UNKNOWN),
      _policyInactiveAck(false),
      _cancelled(false),
      _hasTransport(false) {
    _pinPath = Config::resolveHomePath("~/.config/netmon/router_hostkey.pin");
}

ZyxelSshClient::~ZyxelSshClient() {
    std::lock_guard<std::mutex> lock(_sshMutex);
    disconnectUnlocked();
}

void ZyxelSshClient::configure(const std::string &host, int port,
                              const std::string &user,
                              const std::string &pinPath) {
    std::lock_guard<std::mutex> lock(_sshMutex);
    _host = host;
    _port = port > 0 ? port : 22;
    _user = user;
    if (!pinPath.empty()) {
        _pinPath = Config::resolveHomePath(pinPath);
    }
}

void ZyxelSshClient::resetForTesting() {
    std::lock_guard<std::mutex> lock(_sshMutex);
    disconnectUnlocked();
    _hasTransport = false;
    _transport = ZyshTransport();
    _host.clear();
    _host.shrink_to_fit();
    _user.clear();
    _user.shrink_to_fit();
    _pinPath.clear();
    _pinPath.shrink_to_fit();
    _hostname.clear();
    _hostname.shrink_to_fit();
    _lastMatchedPrompt.clear();
    _lastMatchedPrompt.shrink_to_fit();
    _promptState = PromptState::UNKNOWN;
    _policyInactiveAck = false;
    _state = SshClientState::DISCONNECTED;
    _cancelled.store(false);
}

void ZyxelSshClient::cancelActiveCommand() {
    _cancelled.store(true);
}

bool ZyxelSshClient::isCancelled() const {
    return _cancelled.load();
}

bool ZyxelSshClient::isConnected() const {
    std::lock_guard<std::mutex> lock(_sshMutex);
    if (_state != SshClientState::AUTHENTICATED) {
        return false;
    }
    if (_hasTransport) {
        return true;
    }
    return (_session != nullptr && _channel != nullptr);
}

SshClientState ZyxelSshClient::getState() const {
    return _state.load();
}

static std::string digestToHex(const unsigned char *digest, size_t len) {
    std::ostringstream oss;
    for (size_t i = 0; i < len; ++i) {
        oss << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(digest[i]);
    }
    return oss.str();
}

SshResult ZyxelSshClient::verifyPinHex(const std::string &currentHex) {
    if (_pinPath.empty() || currentHex.size() != 64) {
        return SshResult::ERR_HOSTKEY_REJECTED;
    }

    std::ifstream pinFile(_pinPath);
    if (!pinFile.is_open()) {
        return SshResult::ERR_HOSTKEY_REJECTED;
    }

    std::string pinnedHex;
    pinFile >> pinnedHex;
    pinFile.close();
    while (!pinnedHex.empty() && (pinnedHex.back() == '\r' || pinnedHex.back() == '\n' || pinnedHex.back() == ' ')) {
        pinnedHex.pop_back();
    }
    if (pinnedHex.size() != 64) {
        return SshResult::ERR_HOSTKEY_MISMATCH;
    }
    if (CRYPTO_memcmp(currentHex.c_str(), pinnedHex.c_str(), 64) != 0) {
        std::cerr << "ZyxelSshClient: Host key mismatch! Expected: "
                  << pinnedHex << " but server presented: " << currentHex << std::endl;
        return SshResult::ERR_HOSTKEY_MISMATCH;
    }
    return SshResult::SUCCESS;
}

SshResult ZyxelSshClient::verifyHostKey(LIBSSH2_SESSION *session) {
    const char *fingerprint = libssh2_hostkey_hash(session, LIBSSH2_HOSTKEY_HASH_SHA256);
    if (!fingerprint) {
        return SshResult::ERR_HOSTKEY_MISMATCH;
    }
    std::string currentHex = digestToHex(reinterpret_cast<const unsigned char *>(fingerprint), 32);
    return verifyPinHex(currentHex);
}

SshResult ZyxelSshClient::writeHostKeyPin(const std::string &path, const std::string &sha256Hex) {
    if (path.empty() || sha256Hex.size() != 64) {
        return SshResult::ERR_HOSTKEY_REJECTED;
    }
    for (char c : sha256Hex) {
        if (!std::isxdigit(static_cast<unsigned char>(c))) {
            return SshResult::ERR_HOSTKEY_REJECTED;
        }
    }

    std::string::size_type slash = path.find_last_of('/');
    if (slash != std::string::npos) {
        std::string dir = path.substr(0, slash);
        if (!dir.empty() && mkdir(dir.c_str(), 0700) != 0 && errno != EEXIST) {
            return SshResult::ERR_HOSTKEY_REJECTED;
        }
    }

    int fd = open(path.c_str(), O_WRONLY | O_CREAT | O_EXCL | O_NOFOLLOW, 0600);
    if (fd < 0) {
        return SshResult::ERR_HOSTKEY_REJECTED;
    }
    ssize_t written = ::write(fd, sha256Hex.data(), sha256Hex.size());
    int syncRc = fsync(fd);
    ::close(fd);
    if (written != static_cast<ssize_t>(sha256Hex.size()) || syncRc != 0) {
        unlink(path.c_str());
        return SshResult::ERR_HOSTKEY_REJECTED;
    }
    return SshResult::SUCCESS;
}

SshResult ZyxelSshClient::connect(const std::string &password) {
    std::lock_guard<std::mutex> lock(_sshMutex);

    if (_host.empty() || _user.empty()) {
        return SshResult::ERR_CONNECT_FAILED;
    }

    disconnectUnlocked();
    if (_pinPath.empty()) {
        _state = SshClientState::DISCONNECTED;
        return SshResult::ERR_HOSTKEY_REJECTED;
    }
    if (_hasTransport) {
        return connectTransportUnlocked(password);
    }
    _state = SshClientState::CONNECTING;

    // Resolve host address
    struct addrinfo hints, *res = nullptr;
    std::memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET; // IPv4 only for router management
    hints.ai_socktype = SOCK_STREAM;

    std::string portStr = std::to_string(_port);
    if (getaddrinfo(_host.c_str(), portStr.c_str(), &hints, &res) != 0 || !res) {
        _state = SshClientState::ERROR_DISCONNECTED;
        return SshResult::ERR_CONNECT_FAILED;
    }

    _socketFd = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
    if (_socketFd < 0) {
        freeaddrinfo(res);
        _state = SshClientState::ERROR_DISCONNECTED;
        return SshResult::ERR_CONNECT_FAILED;
    }

    // Set non-blocking for connect with 5s timeout
    int flags = fcntl(_socketFd, F_GETFL, 0);
    fcntl(_socketFd, F_SETFL, flags | O_NONBLOCK);

    int connRet = ::connect(_socketFd, res->ai_addr, res->ai_addrlen);
    if (connRet < 0 && errno == EINPROGRESS) {
        fd_set wfds;
        FD_ZERO(&wfds);
        FD_SET(_socketFd, &wfds);
        struct timeval tv;
        tv.tv_sec = 5;
        tv.tv_usec = 0;
        int selRet = select(_socketFd + 1, nullptr, &wfds, nullptr, &tv);
        if (selRet <= 0) {
            close(_socketFd);
            _socketFd = -1;
            freeaddrinfo(res);
            _state = SshClientState::ERROR_DISCONNECTED;
            return SshResult::ERR_CONNECT_FAILED;
        }
        int soErr = 0;
        socklen_t len = sizeof(soErr);
        getsockopt(_socketFd, SOL_SOCKET, SO_ERROR, &soErr, &len);
        if (soErr != 0) {
            close(_socketFd);
            _socketFd = -1;
            freeaddrinfo(res);
            _state = SshClientState::ERROR_DISCONNECTED;
            return SshResult::ERR_CONNECT_FAILED;
        }
    } else if (connRet < 0) {
        close(_socketFd);
        _socketFd = -1;
        freeaddrinfo(res);
        _state = SshClientState::ERROR_DISCONNECTED;
        return SshResult::ERR_CONNECT_FAILED;
    }
    freeaddrinfo(res);

    // Restore blocking mode on socket (libssh2 handles blocking timeouts)
    fcntl(_socketFd, F_SETFL, flags);

    // Initialize libssh2 session
    _session = libssh2_session_init();
    if (!_session) {
        close(_socketFd);
        _socketFd = -1;
        _state = SshClientState::ERROR_DISCONNECTED;
        return SshResult::ERR_HANDSHAKE_FAILED;
    }

    libssh2_session_set_blocking(_session, 1);
    libssh2_session_set_timeout(_session, 5000);

    int rc = libssh2_session_handshake(_session, _socketFd);
    if (rc != 0) {
        disconnectUnlocked();
        _state = SshClientState::ERROR_DISCONNECTED;
        return SshResult::ERR_HANDSHAKE_FAILED;
    }

    // Verify host key fingerprint before sending credentials
    SshResult hostKeyRes = verifyHostKey(_session);
    if (hostKeyRes != SshResult::SUCCESS) {
        disconnectUnlocked();
        _state = SshClientState::DEGRADED_HOSTKEY_MISMATCH;
        return SshResult::ERR_HOSTKEY_MISMATCH;
    }

    // Authenticate with router password and cleanse temporary memory
    std::vector<char> passBuf(password.begin(), password.end());
    passBuf.push_back('\0');
    rc = libssh2_userauth_password(_session, _user.c_str(), passBuf.data());
    OPENSSL_cleanse(passBuf.data(), passBuf.size());

    if (rc != 0) {
        disconnectUnlocked();
        _state = SshClientState::DEGRADED_AUTH_FAILED;
        return SshResult::ERR_AUTH_FAILED;
    }

    int keepaliveOn = 1;
    setsockopt(_socketFd, SOL_SOCKET, SO_KEEPALIVE, &keepaliveOn, sizeof(keepaliveOn));
    int keepIdle = 30;
    int keepIntvl = 10;
    int keepCnt = 3;
    setsockopt(_socketFd, IPPROTO_TCP, TCP_KEEPIDLE, &keepIdle, sizeof(keepIdle));
    setsockopt(_socketFd, IPPROTO_TCP, TCP_KEEPINTVL, &keepIntvl, sizeof(keepIntvl));
    setsockopt(_socketFd, IPPROTO_TCP, TCP_KEEPCNT, &keepCnt, sizeof(keepCnt));
    libssh2_keepalive_config(_session, 1, 30);

    // Open channel session
    _channel = libssh2_channel_open_session(_session);
    if (!_channel) {
        disconnectUnlocked();
        _state = SshClientState::ERROR_DISCONNECTED;
        return SshResult::ERR_CHANNEL_FAILED;
    }

    // Request 256-column PTY with max height (65535 rows) to avoid line wrapping and paging
    rc = libssh2_channel_request_pty_ex(_channel, "vt100", 5, nullptr, 0, 256, 65535, 0, 0);
    if (rc != 0) {
        rc = libssh2_channel_request_pty_ex(_channel, "vt100", 5, nullptr, 0, 256, 24, 0, 0);
    }
    if (rc != 0) {
        disconnectUnlocked();
        _state = SshClientState::ERROR_DISCONNECTED;
        return SshResult::ERR_PTY_FAILED;
    }

    // Request interactive shell
    rc = libssh2_channel_shell(_channel);
    if (rc != 0) {
        disconnectUnlocked();
        _state = SshClientState::ERROR_DISCONNECTED;
        return SshResult::ERR_CHANNEL_FAILED;
    }

    _state = SshClientState::AUTHENTICATED;

    // Switch to non-blocking mode for interactive prompt and cancel polling
    libssh2_session_set_blocking(_session, 0);

    // Drain initial login banner until prompt
    std::string initOutput;
    drainUntilPrompt(initOutput, 5000);

    // Suppress pagination (unlocked call since _sshMutex is held by connect)
    std::string termOut;
    executeCommandUnlocked("terminal length 0", termOut, 5000);

    return SshResult::SUCCESS;
}

void ZyxelSshClient::disconnect() {
    std::lock_guard<std::mutex> lock(_sshMutex);
    disconnectUnlocked();
}

void ZyxelSshClient::dropSessionUnlocked() {
    disconnectUnlocked();
    _promptState = PromptState::UNKNOWN;
    _policyInactiveAck = false;
    _hostname.clear();
    _lastMatchedPrompt.clear();
}

void ZyxelSshClient::disconnectUnlocked() {
    if (_hasTransport && _transport.close) {
        _transport.close();
    }
    if (_channel) {
        libssh2_channel_close(_channel);
        libssh2_channel_free(_channel);
        _channel = nullptr;
    }
    if (_session) {
        libssh2_session_disconnect(_session, "NetMon disconnecting");
        libssh2_session_free(_session);
        _session = nullptr;
    }
    if (_socketFd >= 0) {
        close(_socketFd);
        _socketFd = -1;
    }
    _promptState = PromptState::UNKNOWN;
    _policyInactiveAck = false;
    _hostname.clear();
    _lastMatchedPrompt.clear();
    if (_state != SshClientState::DEGRADED_AUTH_FAILED &&
        _state != SshClientState::DEGRADED_HOSTKEY_MISMATCH) {
        _state = SshClientState::DISCONNECTED;
    }
}

std::string ZyxelSshClient::stripAnsiEscapes(const std::string &input) {
    std::string result;
    result.reserve(input.size());

    size_t i = 0;
    while (i < input.size()) {
        if (input[i] == '\033') {
            // ANSI escape sequence
            if (i + 1 < input.size() && input[i + 1] == '[') {
                i += 2;
                while (i < input.size() && !((input[i] >= 'a' && input[i] <= 'z') ||
                                            (input[i] >= 'A' && input[i] <= 'Z') ||
                                            input[i] == '~')) {
                    ++i;
                }
                if (i < input.size()) {
                    ++i; // consume terminator
                }
            } else if (i + 1 < input.size() && input[i + 1] == ']') {
                // OSC sequence
                i += 2;
                while (i < input.size() && input[i] != '\007' && input[i] != '\033') {
                    ++i;
                }
                if (i < input.size() && input[i] == '\007') {
                    ++i;
                }
            } else {
                i += 2;
            }
        } else if (input[i] == '\r') {
            // Normalize CRLF to LF
            if (i + 1 < input.size() && input[i + 1] == '\n') {
                result.push_back('\n');
                i += 2;
            } else {
                ++i;
            }
        } else {
            result.push_back(input[i]);
            ++i;
        }
    }
    return result;
}

static bool isHostToken(const std::string &host) {
    if (host.empty()) {
        return false;
    }
    for (char c : host) {
        if (!((c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') ||
              (c >= '0' && c <= '9') || c == '.' || c == '_' || c == '-')) {
            return false;
        }
    }
    return true;
}

static bool isPromptMode(const std::string &mode) {
    if (mode == "config" || mode == "secure-policy") {
        return true;
    }
    const std::string prefix = "config-";
    if (mode.size() <= prefix.size() || mode.compare(0, prefix.size(), prefix) != 0) {
        return false;
    }
    for (size_t i = prefix.size(); i < mode.size(); ++i) {
        char c = mode[i];
        if (!((c >= 'a' && c <= 'z') || (c >= '0' && c <= '9') || c == '-')) {
            return false;
        }
    }
    return true;
}

static std::string promptLineOf(const std::string &buffer) {
    std::string cleaned = ZyxelSshClient::stripAnsiEscapes(buffer);
    while (!cleaned.empty() && (cleaned.back() == ' ' || cleaned.back() == '\t' ||
                                cleaned.back() == '\r' || cleaned.back() == '\n')) {
        cleaned.pop_back();
    }
    std::string::size_type nl = cleaned.find_last_of('\n');
    if (nl != std::string::npos) {
        cleaned = cleaned.substr(nl + 1);
    }
    while (!cleaned.empty() && (cleaned.back() == ' ' || cleaned.back() == '\t' || cleaned.back() == '\r')) {
        cleaned.pop_back();
    }
    return cleaned;
}

static bool parsePromptLine(const std::string &hostname, const std::string &line, PromptState &stateOut) {
    if (!isHostToken(hostname) || line.size() < hostname.size() + 1) {
        return false;
    }
    if (line.compare(0, hostname.size(), hostname) != 0) {
        return false;
    }
    std::string rest = line.substr(hostname.size());
    std::string mode;
    if (!rest.empty() && rest[0] == '(') {
        std::string::size_type end = rest.find(')');
        if (end == std::string::npos || end < 2) {
            return false;
        }
        mode = rest.substr(1, end - 1);
        rest = rest.substr(end + 1);
        if (!isPromptMode(mode)) {
            return false;
        }
    }
    if (rest != ">" && rest != "#") {
        return false;
    }
    if (mode.empty()) {
        stateOut = (rest == "#") ? PromptState::ROOT : PromptState::USER;
    } else if (mode == "config") {
        stateOut = PromptState::CONFIG;
    } else if (mode == "secure-policy") {
        stateOut = PromptState::POLICY_SUBMODE;
    } else {
        stateOut = PromptState::OTHER_SUBMODE;
    }
    return true;
}

bool ZyxelSshClient::matchPrompt(const std::string &hostname,
                                const std::string &buffer,
                                std::string &matchedPrompt,
                                PromptState &stateOut) {
    std::string line = promptLineOf(buffer);
    if (!parsePromptLine(hostname, line, stateOut)) {
        stateOut = PromptState::UNKNOWN;
        return false;
    }
    matchedPrompt = line;
    return true;
}

bool ZyxelSshClient::classifyPromptLine(const std::string &buffer, PromptState &stateOut) {
    std::string line = promptLineOf(buffer);
    if (line.size() < 2) {
        stateOut = PromptState::UNKNOWN;
        return false;
    }
    char end = line.back();
    if (end != '>' && end != '#') {
        stateOut = PromptState::UNKNOWN;
        return false;
    }
    std::string host;
    std::string::size_type paren = line.find('(');
    if (paren == std::string::npos) {
        host = line.substr(0, line.size() - 1);
    } else if (paren > 0) {
        host = line.substr(0, paren);
    }
    if (!parsePromptLine(host, line, stateOut)) {
        stateOut = PromptState::UNKNOWN;
        return false;
    }
    return true;
}

bool ZyxelSshClient::acceptPrompt(const std::string &buffer) {
    std::string line = promptLineOf(buffer);
    if (line.empty()) {
        return false;
    }
    if (_hostname.empty()) {
        char end = line.back();
        if ((end == '>' || end == '#') && line.find('(') == std::string::npos) {
            std::string host = line.substr(0, line.size() - 1);
            if (isHostToken(host)) {
                _hostname = host;
            }
        }
    }
    PromptState state = PromptState::UNKNOWN;
    std::string matched;
    if (!matchPrompt(_hostname, buffer, matched, state)) {
        return false;
    }
    _lastMatchedPrompt = matched;
    _promptState = state;
    return true;
}

void ZyxelSshClient::notePolicyAck(const std::string &command) {
    if (_promptState == PromptState::POLICY_SUBMODE && command == "no activate") {
        _policyInactiveAck = true;
    } else if (_promptState != PromptState::POLICY_SUBMODE) {
        _policyInactiveAck = false;
    }
}

std::string ZyxelSshClient::stripCommandEcho(const std::string &buffer, const std::string &commandSent) {
    std::string cleaned = stripAnsiEscapes(buffer);
    if (commandSent.empty() || cleaned.empty()) {
        return cleaned;
    }

    // If buffer begins with the echoed command line, remove it
    size_t firstLineEnd = cleaned.find('\n');
    if (firstLineEnd != std::string::npos) {
        std::string firstLine = cleaned.substr(0, firstLineEnd);
        // Trim trailing \r or spaces
        while (!firstLine.empty() && (firstLine.back() == '\r' || firstLine.back() == ' ')) {
            firstLine.pop_back();
        }
        if (firstLine == commandSent) {
            return cleaned.substr(firstLineEnd + 1);
        }
    } else if (cleaned == commandSent) {
        return "";
    }

    return cleaned;
}

std::string ZyxelSshClient::stripTrailingPrompt(const std::string &buffer) {
    std::string cleaned = stripAnsiEscapes(buffer);
    if (cleaned.empty()) {
        return cleaned;
    }

    std::string line = promptLineOf(buffer);
    PromptState ignored = PromptState::UNKNOWN;
    std::string host;
    if (!line.empty() && (line.back() == '>' || line.back() == '#')) {
        std::string body = line.substr(0, line.size() - 1);
        std::string::size_type open = body.find('(');
        host = (open == std::string::npos) ? body : body.substr(0, open);
    }
    if (parsePromptLine(host, line, ignored)) {
        std::string::size_type pos = cleaned.rfind(line);
        if (pos != std::string::npos) {
            cleaned = cleaned.substr(0, pos);
        }
    }

    while (!cleaned.empty() && (cleaned.back() == ' ' || cleaned.back() == '\t' ||
                                cleaned.back() == '\r' || cleaned.back() == '\n')) {
        cleaned.pop_back();
    }

    return cleaned;
}

bool ZyxelSshClient::isConfigLocked(const std::string &buffer) {
    return (buffer.find("% Configuration is locked") != std::string::npos);
}

bool ZyxelSshClient::isSyntaxError(const std::string &buffer) {
    if (buffer.find("% Invalid") != std::string::npos ||
        buffer.find("% Incomplete command") != std::string::npos ||
        buffer.find("% Object name is invalid") != std::string::npos ||
        buffer.find("% Syntax error") != std::string::npos ||
        buffer.find("% Ambiguous command") != std::string::npos ||
        buffer.find("% Bad parameter") != std::string::npos) {
        return true;
    }
    return false;
}

std::string ZyxelSshClient::sanitizeReason(const std::string &rawReason) {
    std::string sanitized;
    sanitized.reserve(rawReason.size());
    for (char c : rawReason) {
        if ((c >= 'a' && c <= 'z') ||
            (c >= 'A' && c <= 'Z') ||
            (c >= '0' && c <= '9') ||
            c == ' ' || c == '-' || c == '_' || c == '.' || c == ':') {
            sanitized.push_back(c);
            if (sanitized.size() >= 63) {
                break;
            }
        }
    }
    return sanitized;
}

std::string ZyxelSshClient::sanitizeIpToObjectName(const std::string &ip) {
    std::string name = "NETMON_BLK_";
    for (char c : ip) {
        if (c == '.') {
            name.push_back('_');
        } else if ((c >= '0' && c <= '9') || (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z')) {
            name.push_back(c);
        }
    }
    return name;
}

SshResult ZyxelSshClient::drainUntilPromptWithReader(
    const ChannelReader &reader,
    const ChannelWriter &writer,
    std::string &outputOut,
    int timeoutMs) {
    outputOut.clear();

    std::string rawBuffer;
    char chunk[512];
    auto startTime = std::chrono::steady_clock::now();

    while (true) {
        if (_cancelled.load()) {
            _cancelled.store(false);
            outputOut = stripTrailingPrompt(stripAnsiEscapes(rawBuffer));
            dropSessionUnlocked();
            return SshResult::ERR_INTERRUPTED;
        }

        auto now = std::chrono::steady_clock::now();
        int elapsedMs = std::chrono::duration_cast<std::chrono::milliseconds>(now - startTime).count();
        if (elapsedMs > timeoutMs) {
            dropSessionUnlocked();
            return SshResult::ERR_TIMEOUT;
        }

        bool eof = false;
        ssize_t n = reader(chunk, sizeof(chunk) - 1, eof);
        if (n > 0) {
            chunk[n] = '\0';
            rawBuffer.append(chunk, n);

            // Handle --More-- pagination prompt by sending space to advance
            while (true) {
                size_t morePos = rawBuffer.find("--More--");
                if (morePos == std::string::npos) {
                    break;
                }
                size_t startPos = morePos;
                if (startPos > 0 && rawBuffer[startPos - 1] == '\r') {
                    startPos--;
                }
                if (startPos >= 4 && rawBuffer[startPos - 4] == '\x1b' && rawBuffer[startPos - 3] == '[') {
                    startPos -= 4;
                } else if (startPos >= 3 && rawBuffer[startPos - 3] == '\x1b' && rawBuffer[startPos - 2] == '[') {
                    startPos -= 3;
                }
                size_t endPos = morePos + 8;
                if (endPos + 2 < rawBuffer.size() && rawBuffer[endPos] == '\x1b' && rawBuffer[endPos + 1] == '[') {
                    size_t k = endPos + 2;
                    while (k < rawBuffer.size() && k < endPos + 8 && (rawBuffer[k] == '?' || (rawBuffer[k] >= '0' && rawBuffer[k] <= '9') || rawBuffer[k] == ';')) {
                        k++;
                    }
                    if (k < rawBuffer.size()) {
                        endPos = k + 1;
                    }
                }
                while (endPos < rawBuffer.size() && (rawBuffer[endPos] == '\r' || rawBuffer[endPos] == '\b' || rawBuffer[endPos] == ' ')) {
                    endPos++;
                }
                rawBuffer.erase(startPos, endPos - startPos);
                if (writer) {
                    writer(" ", 1);
                }
            }

            if (acceptPrompt(rawBuffer)) {
                outputOut = stripAnsiEscapes(rawBuffer);
                return SshResult::SUCCESS;
            }
        } else if (n == LIBSSH2_ERROR_EAGAIN || (n == 0 && !eof)) {
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        } else if (n < 0) {
            dropSessionUnlocked();
            return SshResult::ERR_CHANNEL_FAILED;
        } else {
            if (acceptPrompt(rawBuffer)) {
                outputOut = stripAnsiEscapes(rawBuffer);
            }
            dropSessionUnlocked();
            if (!outputOut.empty()) {
                return SshResult::SUCCESS;
            }
            return SshResult::ERR_CHANNEL_FAILED;
        }
    }
}

SshResult ZyxelSshClient::drainUntilPrompt(std::string &outputOut, int timeoutMs) {
    if (_hasTransport) {
        auto reader = [this](char *buf, size_t buflen, bool &eofOut) -> ssize_t {
            if (!_transport.read) {
                eofOut = true;
                return -1;
            }
            return _transport.read(buf, buflen, eofOut);
        };
        auto writer = [this](const char *data, size_t len) {
            if (_transport.write) {
                _transport.write(data, len);
            }
        };
        return drainUntilPromptWithReader(reader, writer, outputOut, timeoutMs);
    }

    if (!_channel) {
        return SshResult::ERR_DISCONNECTED;
    }

    auto reader = [this](char *buf, size_t buflen, bool &eofOut) -> ssize_t {
        if (!_channel) {
            eofOut = true;
            return -1;
        }
        ssize_t n = libssh2_channel_read(_channel, buf, buflen);
        eofOut = (libssh2_channel_eof(_channel) != 0);
        return n;
    };

    auto writer = [this](const char *data, size_t len) {
        if (_channel) {
            libssh2_channel_write(_channel, data, len);
        }
    };

    return drainUntilPromptWithReader(reader, writer, outputOut, timeoutMs);
}

SshResult ZyxelSshClient::drainUntilPromptForTesting(
    const ChannelReader &reader,
    const ChannelWriter &writer,
    std::string &outputOut,
    int timeoutMs) {
    std::lock_guard<std::mutex> lock(_sshMutex);
    return drainUntilPromptWithReader(reader, writer, outputOut, timeoutMs);
}

SshResult ZyxelSshClient::executeCommandUnlocked(const std::string &command,
                                                std::string &outputOut,
                                                int timeoutMs) {
    outputOut.clear();
    _cancelled.store(false);

    if (_state != SshClientState::AUTHENTICATED || (!_hasTransport && !_channel)) {
        return SshResult::ERR_DISCONNECTED;
    }

    PromptState promptBefore = _promptState;

    char discard[512];
    if (_hasTransport && _transport.read) {
        for (int i = 0; i < 8; ++i) {
            bool eof = false;
            ssize_t n = _transport.read(discard, sizeof(discard), eof);
            if (n < 0 || eof) {
                dropSessionUnlocked();
                return SshResult::ERR_CHANNEL_FAILED;
            }
            if (n == 0) {
                break;
            }
        }
    } else if (_channel) {
        while (libssh2_channel_read(_channel, discard, sizeof(discard)) > 0) {}
    }

    std::string cmdToSend = command + "\n";
    size_t totalWritten = 0;
    while (totalWritten < cmdToSend.size()) {
        ssize_t written = -1;
        if (_hasTransport && _transport.write) {
            written = _transport.write(cmdToSend.c_str() + totalWritten,
                                      cmdToSend.size() - totalWritten);
        } else if (_channel) {
            written = libssh2_channel_write(_channel,
                                            cmdToSend.c_str() + totalWritten,
                                            cmdToSend.size() - totalWritten);
        }
        if (written > 0) {
            totalWritten += static_cast<size_t>(written);
        } else if (written == LIBSSH2_ERROR_EAGAIN) {
            std::this_thread::sleep_for(std::chrono::milliseconds(5));
        } else {
            dropSessionUnlocked();
            return SshResult::ERR_CHANNEL_FAILED;
        }
    }

    SshResult res = drainUntilPrompt(outputOut, timeoutMs);
    if (res == SshResult::SUCCESS) {
        outputOut = stripCommandEcho(outputOut, command);
        if (_promptState == PromptState::POLICY_SUBMODE && promptBefore != PromptState::POLICY_SUBMODE) {
            _policyInactiveAck = false;
        }
        notePolicyAck(command);
        if (isConfigLocked(outputOut)) {
            return SshResult::ERR_LOCKED;
        }
    }
    return res;
}

SshResult ZyxelSshClient::executeCommand(const std::string &command,
                                        std::string &outputOut,
                                        int timeoutMs) {
    std::lock_guard<std::mutex> lock(_sshMutex);
    return executeCommandUnlocked(command, outputOut, timeoutMs);
}

SshResult ZyxelSshClient::unwindToRootPromptUnlocked(int maxAttempts) {
    if (_state != SshClientState::AUTHENTICATED || (!_hasTransport && !_channel)) {
        return SshResult::ERR_DISCONNECTED;
    }
    if (_promptState == PromptState::POLICY_SUBMODE && !_policyInactiveAck) {
        return SshResult::ERR_UNSAFE_UNWIND;
    }

    for (int attempt = 0; attempt < maxAttempts; ++attempt) {
        if (_promptState == PromptState::ROOT) {
            return SshResult::SUCCESS;
        }

        std::string out;
        ssize_t written = -1;
        if (_hasTransport && _transport.write) {
            written = _transport.write("exit\n", 5);
        } else if (_channel) {
            written = libssh2_channel_write(_channel, "exit\n", 5);
        }
        if (written < 0) {
            dropSessionUnlocked();
            return SshResult::ERR_CHANNEL_FAILED;
        }
        SshResult drained = drainUntilPrompt(out, 2000);
        if (drained != SshResult::SUCCESS) {
            return drained;
        }
    }

    if (_promptState == PromptState::ROOT) {
        return SshResult::SUCCESS;
    }
    return SshResult::ERR_EXEC_FAILED;
}

SshResult ZyxelSshClient::unwindToRootPrompt(int maxAttempts) {
    std::lock_guard<std::mutex> lock(_sshMutex);
    return unwindToRootPromptUnlocked(maxAttempts);
}

std::string ZyxelSshClient::getLastMatchedPrompt() const {
    std::lock_guard<std::mutex> lock(_sshMutex);
    return _lastMatchedPrompt;
}

PromptState ZyxelSshClient::getPromptState() const {
    std::lock_guard<std::mutex> lock(_sshMutex);
    return _promptState;
}

bool ZyxelSshClient::policyInactiveAcknowledged() const {
    std::lock_guard<std::mutex> lock(_sshMutex);
    return _policyInactiveAck;
}

bool ZyxelSshClient::sendKeepalive() {
    std::lock_guard<std::mutex> lock(_sshMutex);
    if (_state != SshClientState::AUTHENTICATED) {
        return false;
    }
    bool alive = false;
    if (_hasTransport && _transport.keepalive) {
        alive = _transport.keepalive();
    } else if (_session) {
        int secondsToNext = 0;
        int rc = libssh2_keepalive_send(_session, &secondsToNext);
        alive = (rc == 0);
    }
    if (!alive) {
        dropSessionUnlocked();
        return false;
    }
    return true;
}

void ZyxelSshClient::setTransport(ZyshTransport transport) {
    std::lock_guard<std::mutex> lock(_sshMutex);
    if (_state == SshClientState::AUTHENTICATED) {
        disconnectUnlocked();
    }
    _transport = std::move(transport);
    _hasTransport = true;
}

SshResult ZyxelSshClient::commitHostKeyPin(const std::string &sha256Hex) {
    std::lock_guard<std::mutex> lock(_sshMutex);
    return writeHostKeyPin(_pinPath, sha256Hex);
}

SshResult ZyxelSshClient::connectTransportUnlocked(const std::string &password) {
    if (!_transport.open) {
        _state = SshClientState::ERROR_DISCONNECTED;
        return SshResult::ERR_CONNECT_FAILED;
    }
    _state = SshClientState::CONNECTING;
    std::string hostKey;
    SshResult opened = _transport.open(_host, _port, _user, password, hostKey);
    if (opened != SshResult::SUCCESS) {
        dropSessionUnlocked();
        _state = SshClientState::ERROR_DISCONNECTED;
        return opened;
    }
    SshResult pinned = verifyPinHex(hostKey);
    if (pinned != SshResult::SUCCESS) {
        dropSessionUnlocked();
        if (pinned == SshResult::ERR_HOSTKEY_MISMATCH) {
            _state = SshClientState::DEGRADED_HOSTKEY_MISMATCH;
        }
        return pinned;
    }

    _state = SshClientState::AUTHENTICATED;
    std::string banner;
    SshResult drained = drainUntilPrompt(banner, 5000);
    if (drained != SshResult::SUCCESS || _hostname.empty()) {
        dropSessionUnlocked();
        return SshResult::ERR_CHANNEL_FAILED;
    }
    std::string termOut;
    SshResult termed = executeCommandUnlocked("terminal length 0", termOut, 5000);
    if (termed != SshResult::SUCCESS) {
        dropSessionUnlocked();
        return termed;
    }
    return SshResult::SUCCESS;
}

SshResult ZyxelSshClient::probeHostKey(std::string &sha256HexOut) {
    std::lock_guard<std::mutex> lock(_sshMutex);
    sha256HexOut.clear();
    if (_host.empty()) {
        return SshResult::ERR_CONNECT_FAILED;
    }
    disconnectUnlocked();

    if (_hasTransport && _transport.open) {
        std::string key;
        SshResult opened = _transport.open(_host, _port, _user, "", key);
        if (_transport.close) {
            _transport.close();
        }
        if (opened != SshResult::SUCCESS || key.size() != 64) {
            _state = SshClientState::DISCONNECTED;
            return SshResult::ERR_HOSTKEY_REJECTED;
        }
        sha256HexOut = key;
        _state = SshClientState::DISCONNECTED;
        return SshResult::SUCCESS;
    }

    _state = SshClientState::CONNECTING;
    struct addrinfo hints, *res = nullptr;
    std::memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    std::string portStr = std::to_string(_port);
    if (getaddrinfo(_host.c_str(), portStr.c_str(), &hints, &res) != 0 || !res) {
        _state = SshClientState::ERROR_DISCONNECTED;
        return SshResult::ERR_CONNECT_FAILED;
    }
    _socketFd = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
    if (_socketFd < 0) {
        freeaddrinfo(res);
        _state = SshClientState::ERROR_DISCONNECTED;
        return SshResult::ERR_CONNECT_FAILED;
    }
    int flags = fcntl(_socketFd, F_GETFL, 0);
    fcntl(_socketFd, F_SETFL, flags | O_NONBLOCK);
    int connRet = ::connect(_socketFd, res->ai_addr, res->ai_addrlen);
    if (connRet < 0 && errno == EINPROGRESS) {
        fd_set wfds;
        FD_ZERO(&wfds);
        FD_SET(_socketFd, &wfds);
        struct timeval tv;
        tv.tv_sec = 5;
        tv.tv_usec = 0;
        int selRet = select(_socketFd + 1, nullptr, &wfds, nullptr, &tv);
        int soErr = 0;
        socklen_t len = sizeof(soErr);
        if (selRet > 0) {
            getsockopt(_socketFd, SOL_SOCKET, SO_ERROR, &soErr, &len);
        }
        if (selRet <= 0 || soErr != 0) {
            freeaddrinfo(res);
            disconnectUnlocked();
            _state = SshClientState::ERROR_DISCONNECTED;
            return SshResult::ERR_CONNECT_FAILED;
        }
    } else if (connRet < 0) {
        freeaddrinfo(res);
        disconnectUnlocked();
        _state = SshClientState::ERROR_DISCONNECTED;
        return SshResult::ERR_CONNECT_FAILED;
    }
    freeaddrinfo(res);
    fcntl(_socketFd, F_SETFL, flags);

    _session = libssh2_session_init();
    if (!_session) {
        disconnectUnlocked();
        _state = SshClientState::ERROR_DISCONNECTED;
        return SshResult::ERR_HANDSHAKE_FAILED;
    }
    libssh2_session_set_blocking(_session, 1);
    libssh2_session_set_timeout(_session, 5000);
    if (libssh2_session_handshake(_session, _socketFd) != 0) {
        disconnectUnlocked();
        _state = SshClientState::ERROR_DISCONNECTED;
        return SshResult::ERR_HANDSHAKE_FAILED;
    }
    const char *fingerprint = libssh2_hostkey_hash(_session, LIBSSH2_HOSTKEY_HASH_SHA256);
    if (!fingerprint) {
        disconnectUnlocked();
        _state = SshClientState::DISCONNECTED;
        return SshResult::ERR_HOSTKEY_REJECTED;
    }
    sha256HexOut = digestToHex(reinterpret_cast<const unsigned char *>(fingerprint), 32);
    disconnectUnlocked();
    _state = SshClientState::DISCONNECTED;
    return SshResult::SUCCESS;
}

/*
 * Local variables:
 * mode: C++
 * c-file-style: "BSD"
 * c-basic-offset: 4
 * tab-width: 4
 * indent-tabs-mode: nil
 * End:
 */
