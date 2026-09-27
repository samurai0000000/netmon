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
#include <regex>
#include <cstring>
#include <chrono>
#include <thread>
#include <fcntl.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
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
      _state(SshClientState::DISCONNECTED) {
    _pinPath = Config::resolveHomePath("~/.config/netmon/router_hostkey.pin");
}

ZyxelSshClient::~ZyxelSshClient() {
    disconnect();
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
    disconnect();
    std::lock_guard<std::mutex> lock(_sshMutex);
    _host.clear();
    _host.shrink_to_fit();
    _user.clear();
    _user.shrink_to_fit();
    _pinPath.clear();
    _pinPath.shrink_to_fit();
    _lastMatchedPrompt.clear();
    _lastMatchedPrompt.shrink_to_fit();
    _state = SshClientState::DISCONNECTED;
}

bool ZyxelSshClient::isConnected() const {
    std::lock_guard<std::mutex> lock(_sshMutex);
    return (_state == SshClientState::AUTHENTICATED && _session != nullptr && _channel != nullptr);
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

SshResult ZyxelSshClient::verifyHostKey(LIBSSH2_SESSION *session) {
    const char *fingerprint = libssh2_hostkey_hash(session, LIBSSH2_HOSTKEY_HASH_SHA256);
    if (!fingerprint) {
        return SshResult::ERR_HOSTKEY_MISMATCH;
    }

    std::string currentHex = digestToHex(reinterpret_cast<const unsigned char *>(fingerprint), 32);

    if (_pinPath.empty()) {
        return SshResult::SUCCESS;
    }

    // Check if pin file exists
    std::ifstream pinFile(_pinPath);
    if (pinFile.is_open()) {
        std::string pinnedHex;
        pinFile >> pinnedHex;
        pinFile.close();

        // Strip whitespace
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

    // Trust On First Use (TOFU): write pin file with 0600 permissions
    size_t lastSlash = _pinPath.find_last_of('/');
    if (lastSlash != std::string::npos) {
        std::string dir = _pinPath.substr(0, lastSlash);
        struct stat st;
        if (stat(dir.c_str(), &st) != 0) {
            mkdir(dir.c_str(), 0700);
        }
    }

    std::string tmpPinPath = _pinPath + ".tmp";
    int fd = open(tmpPinPath.c_str(), O_WRONLY | O_CREAT | O_TRUNC, 0600);
    if (fd >= 0) {
        fchmod(fd, 0600);
        ssize_t written = write(fd, currentHex.c_str(), currentHex.size());
        fsync(fd);
        close(fd);
        if (written == static_cast<ssize_t>(currentHex.size())) {
            rename(tmpPinPath.c_str(), _pinPath.c_str());
        } else {
            unlink(tmpPinPath.c_str());
        }
    }

    return SshResult::SUCCESS;
}

SshResult ZyxelSshClient::connect(const std::string &password) {
    std::lock_guard<std::mutex> lock(_sshMutex);

    if (_host.empty() || _user.empty()) {
        return SshResult::ERR_CONNECT_FAILED;
    }

    disconnect();
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
        disconnect();
        _state = SshClientState::ERROR_DISCONNECTED;
        return SshResult::ERR_HANDSHAKE_FAILED;
    }

    // Verify host key fingerprint before sending credentials
    SshResult hostKeyRes = verifyHostKey(_session);
    if (hostKeyRes != SshResult::SUCCESS) {
        disconnect();
        _state = SshClientState::DEGRADED_HOSTKEY_MISMATCH;
        return SshResult::ERR_HOSTKEY_MISMATCH;
    }

    // Authenticate with router password and cleanse temporary memory
    std::vector<char> passBuf(password.begin(), password.end());
    passBuf.push_back('\0');
    rc = libssh2_userauth_password(_session, _user.c_str(), passBuf.data());
    OPENSSL_cleanse(passBuf.data(), passBuf.size());

    if (rc != 0) {
        disconnect();
        _state = SshClientState::DEGRADED_AUTH_FAILED;
        return SshResult::ERR_AUTH_FAILED;
    }

    // Open channel session
    _channel = libssh2_channel_open_session(_session);
    if (!_channel) {
        disconnect();
        _state = SshClientState::ERROR_DISCONNECTED;
        return SshResult::ERR_CHANNEL_FAILED;
    }

    // Request 256-column PTY to avoid line wrapping
    rc = libssh2_channel_request_pty_ex(_channel, "vt100", 5, nullptr, 0, 256, 24, 0, 0);
    if (rc != 0) {
        disconnect();
        _state = SshClientState::ERROR_DISCONNECTED;
        return SshResult::ERR_PTY_FAILED;
    }

    // Request interactive shell
    rc = libssh2_channel_shell(_channel);
    if (rc != 0) {
        disconnect();
        _state = SshClientState::ERROR_DISCONNECTED;
        return SshResult::ERR_CHANNEL_FAILED;
    }

    _state = SshClientState::AUTHENTICATED;

    // Drain initial login banner until prompt
    std::string initOutput;
    drainUntilPrompt(initOutput, 5000);

    // Suppress pagination (unlocked call since _sshMutex is held by connect)
    std::string termOut;
    executeCommandUnlocked("terminal length 0", termOut, 5000);

    return SshResult::SUCCESS;
}

void ZyxelSshClient::disconnect() {
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

bool ZyxelSshClient::matchPrompt(const std::string &buffer, std::string &matchedPrompt) {
    std::string cleaned = stripAnsiEscapes(buffer);
    if (cleaned.empty()) {
        return false;
    }

    // Match prompt strictly at trailing end of buffer
    // Handles: Router#, Router>, Router(config)#, Router(config-policy-control)#, usg-flex-200#
    static const std::regex promptRegex(R"((?:[\r\n]|^)[\w.-]+(?:\([A-Za-z0-9_.-]+\))?[>#]\s*$)");
    std::smatch match;
    if (std::regex_search(cleaned, match, promptRegex)) {
        matchedPrompt = match.str();
        return true;
    }
    return false;
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
            if (sanitized.size() >= 64) {
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

SshResult ZyxelSshClient::drainUntilPrompt(std::string &outputOut, int timeoutMs) {
    outputOut.clear();
    if (!_channel) {
        return SshResult::ERR_DISCONNECTED;
    }

    std::string rawBuffer;
    char chunk[512];
    auto startTime = std::chrono::steady_clock::now();

    while (true) {
        auto now = std::chrono::steady_clock::now();
        int elapsedMs = std::chrono::duration_cast<std::chrono::milliseconds>(now - startTime).count();
        if (elapsedMs > timeoutMs) {
            return SshResult::ERR_TIMEOUT;
        }

        ssize_t n = libssh2_channel_read(_channel, chunk, sizeof(chunk) - 1);
        if (n > 0) {
            chunk[n] = '\0';
            rawBuffer.append(chunk, n);

            // Handle --More-- pagination prompt by sending space to advance
            if (rawBuffer.find("--More--") != std::string::npos) {
                libssh2_channel_write(_channel, " ", 1);
            }

            std::string prompt;
            if (matchPrompt(rawBuffer, prompt)) {
                _lastMatchedPrompt = prompt;
                outputOut = stripAnsiEscapes(rawBuffer);
                return SshResult::SUCCESS;
            }
        } else if (n == LIBSSH2_ERROR_EAGAIN) {
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        } else if (n < 0) {
            return SshResult::ERR_CHANNEL_FAILED;
        } else {
            // EOF reached
            outputOut = stripAnsiEscapes(rawBuffer);
            return SshResult::SUCCESS;
        }
    }
}

SshResult ZyxelSshClient::executeCommandUnlocked(const std::string &command,
                                                std::string &outputOut,
                                                int timeoutMs) {
    outputOut.clear();

    if (_state != SshClientState::AUTHENTICATED || !_channel) {
        return SshResult::ERR_DISCONNECTED;
    }

    std::string cmdToSend = command + "\n";
    ssize_t written = libssh2_channel_write(_channel, cmdToSend.c_str(), cmdToSend.size());
    if (written <= 0) {
        disconnect();
        return SshResult::ERR_CHANNEL_FAILED;
    }

    SshResult res = drainUntilPrompt(outputOut, timeoutMs);
    if (res == SshResult::SUCCESS) {
        outputOut = stripCommandEcho(outputOut, command);
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
    if (_state != SshClientState::AUTHENTICATED || !_channel) {
        return SshResult::ERR_DISCONNECTED;
    }

    for (int attempt = 0; attempt < maxAttempts; ++attempt) {
        // If current prompt is root (e.g. "Router#" or "Router>"), we are at root
        if (!_lastMatchedPrompt.empty() &&
            _lastMatchedPrompt.find("(config") == std::string::npos &&
            (_lastMatchedPrompt.find('#') != std::string::npos ||
             _lastMatchedPrompt.find('>') != std::string::npos)) {
            return SshResult::SUCCESS;
        }

        std::string out;
        libssh2_channel_write(_channel, "exit\n", 5);
        drainUntilPrompt(out, 2000);
    }

    // If still in submode, send Ctrl+C to abort
    libssh2_channel_write(_channel, "\x03\n", 2);
    std::string finalOut;
    drainUntilPrompt(finalOut, 2000);

    return SshResult::SUCCESS;
}

SshResult ZyxelSshClient::unwindToRootPrompt(int maxAttempts) {
    std::lock_guard<std::mutex> lock(_sshMutex);
    return unwindToRootPromptUnlocked(maxAttempts);
}

bool ZyxelSshClient::sendKeepalive() {
    std::lock_guard<std::mutex> lock(_sshMutex);
    if (_state != SshClientState::AUTHENTICATED || !_session) {
        return false;
    }
    int secondsToNext = 0;
    int rc = libssh2_keepalive_send(_session, &secondsToNext);
    return (rc == 0);
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
