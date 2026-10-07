/*
 * AiSecurityClearance.cxx
 *
 * Copyright (C) 2026, Charles Chiou
 */

#include "AiSecurityClearance.hxx"
#include "ZyxelDriver.hxx"
#include "ZyxelSshClient.hxx"
#include "NcursesConsole.hxx"
#include "Config.hxx"

#include <algorithm>
#include <cctype>
#include <sstream>
#include <cstring>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <arpa/inet.h>
#include <poll.h>
#include <future>
#include <nlohmann/json.hpp>

// AiSecurityClassifier and clearance enum string conversions are implemented in AiSecurityClassifier.cxx


// -----------------------------------------------------------------
// AiSecurityClearanceManager implementation
// -----------------------------------------------------------------

AiSecurityClearanceManager &AiSecurityClearanceManager::getInstance() {
    static AiSecurityClearanceManager instance;
    return instance;
}

std::string AiSecurityClearanceManager::formatConnId(uint64_t id) {
    char buf[32];
    if (id <= 9999) {
        snprintf(buf, sizeof(buf), "conn-%04llu", (unsigned long long)id);
    } else {
        snprintf(buf, sizeof(buf), "conn-%llu", (unsigned long long)id);
    }
    return std::string(buf);
}

uint64_t AiSecurityClearanceManager::parseConnId(const std::string &str) {
    if (str.empty()) return 0;
    if (str.rfind("conn-", 0) != 0) {
        return 0; // Strictly enforce "conn-" prefix (zero bare integers from UI)
    }
    std::string s = str.substr(5);
    if (s.empty()) return 0;
    for (char c : s) {
        if (!std::isdigit(static_cast<unsigned char>(c))) {
            return 0;
        }
    }
    char *end = nullptr;
    unsigned long long val = strtoull(s.c_str(), &end, 10);
    if (!end || *end != '\0' || val == 0) return 0;
    return static_cast<uint64_t>(val);
}

AiSecurityClearanceManager::AiSecurityClearanceManager()
    : _bindAddress("0.0.0.0"),
      _port(3885),
      _running(false),
      _listenFd(-1),
      _nextConnId(1),
      _pendingApprovalConnId(0),
      _activeLevel2ConnId(0),
      _sessionOwnerConnId(0),
      _routerBusyConnId(0) {
}

AiSecurityClearanceManager::~AiSecurityClearanceManager() {
    stop();
}

bool AiSecurityClearanceManager::start(const std::string &bindAddress, uint16_t port) {
    std::lock_guard<std::recursive_mutex> lock(_mutex);
    if (_running.load()) {
        return false;
    }

    _bindAddress = bindAddress.empty() ? "0.0.0.0" : bindAddress;
    _port = (port == 0) ? 3885 : port;

    _listenFd = socket(AF_INET, SOCK_STREAM, 0);
    if (_listenFd < 0) {
        return false;
    }

    int opt = 1;
    setsockopt(_listenFd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    struct sockaddr_in sin;
    memset(&sin, 0, sizeof(sin));
    sin.sin_family = AF_INET;
    sin.sin_port = htons(_port);
    if (inet_pton(AF_INET, _bindAddress.c_str(), &sin.sin_addr) <= 0) {
        sin.sin_addr.s_addr = htonl(INADDR_ANY);
    }

    if (bind(_listenFd, (struct sockaddr *)&sin, sizeof(sin)) < 0) {
        close(_listenFd);
        _listenFd = -1;
        return false;
    }

    if (listen(_listenFd, 8) < 0) {
        close(_listenFd);
        _listenFd = -1;
        return false;
    }

    _running.store(true);
    _listenerThread = std::thread(&AiSecurityClearanceManager::listenerWorker, this);
    _routerWorkerThread = std::thread(&AiSecurityClearanceManager::routerWorker, this);
    return true;
}

void AiSecurityClearanceManager::stop() {
    if (!_running.load()) {
        return;
    }
    _running.store(false);
    _routerQueueCv.notify_all();

    {
        std::lock_guard<std::mutex> qLock(_routerQueueMutex);
        while (!_routerQueue.empty()) {
            auto item = _routerQueue.front();
            _routerQueue.pop();
            if (item) {
                RouterWorkResult r;
                r.sshResult = static_cast<int>(SshResult::ERR_INTERRUPTED);
                r.output = "Interrupted";
                r.matchedPrompt = "-";
                try {
                    item->promise.set_value(r);
                } catch (...) {}
            }
        }
    }

    {
        std::lock_guard<std::recursive_mutex> lock(_mutex);
        if (_listenFd >= 0) {
            shutdown(_listenFd, SHUT_RDWR);
            close(_listenFd);
            _listenFd = -1;
        }
        for (auto &kv : _connections) {
            if (kv.second.socketFd >= 0) {
                shutdown(kv.second.socketFd, SHUT_RDWR);
            }
        }
    }

    if (_listenerThread.joinable()) {
        _listenerThread.join();
    }
    if (_routerWorkerThread.joinable()) {
        _routerWorkerThread.join();
    }

    {
        std::lock_guard<std::mutex> tLock(_clientThreadsMutex);
        for (auto &rec : _clientThreads) {
            if (rec.th.joinable()) {
                rec.th.join();
            }
        }
        std::vector<ClientThreadRecord>().swap(_clientThreads);
    }

    {
        std::lock_guard<std::recursive_mutex> lock(_mutex);
        _connections.clear();
        _pendingApprovalConnId = 0;
        _activeLevel2ConnId = 0;
        _sessionOwnerConnId = 0;
        _routerBusyConnId = 0;
    }
}

bool AiSecurityClearanceManager::isListening() const {
    return _running.load();
}

void AiSecurityClearanceManager::listenerWorker() {
    while (_running.load()) {
        struct pollfd pfd;
        pfd.fd = _listenFd;
        pfd.events = POLLIN;
        pfd.revents = 0;

        int pr = poll(&pfd, 1, 500);
        if (pr <= 0 || !(pfd.revents & POLLIN)) {
            std::lock_guard<std::mutex> tLock(_clientThreadsMutex);
            for (auto it = _clientThreads.begin(); it != _clientThreads.end(); ) {
                if (it->done && it->done->load() && it->th.joinable()) {
                    it->th.join();
                    it = _clientThreads.erase(it);
                } else {
                    ++it;
                }
            }
            continue;
        }

        struct sockaddr_in peerSin;
        socklen_t peerLen = sizeof(peerSin);
        int clientFd = accept(_listenFd, (struct sockaddr *)&peerSin, &peerLen);
        if (clientFd < 0) {
            continue;
        }

        char peerIp[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, &peerSin.sin_addr, peerIp, sizeof(peerIp));
        uint16_t peerPort = ntohs(peerSin.sin_port);

        uint64_t connId = 0;
        {
            std::lock_guard<std::recursive_mutex> lock(_mutex);
            connId = _nextConnId++;
            ClearanceConnection conn;
            conn.connectionId = connId;
            conn.socketFd = clientFd;
            conn.peerAddress = peerIp;
            conn.peerPort = peerPort;
            conn.clearanceState = ClientClearanceState::NONE;
            conn.activeTier = ClearanceTier::NONE;
            conn.hasBeenGranted = false;
            _connections[connId] = std::move(conn);
        }

        {
            std::lock_guard<std::mutex> tLock(_clientThreadsMutex);
            for (auto it = _clientThreads.begin(); it != _clientThreads.end(); ) {
                if (it->done && it->done->load() && it->th.joinable()) {
                    it->th.join();
                    it = _clientThreads.erase(it);
                } else {
                    ++it;
                }
            }
            auto doneFlag = std::make_shared<std::atomic<bool>>(false);
            _clientThreads.push_back({
                std::thread([this, clientFd, connId, ip = std::string(peerIp), peerPort, doneFlag]() {
                    this->clientWorker(clientFd, connId, ip, peerPort);
                    doneFlag->store(true);
                }),
                doneFlag
            });
        }
    }
}

static bool sendAll(int fd, const void *buf, size_t count) {
    if (fd < 0) {
        return false;
    }
    const uint8_t *ptr = reinterpret_cast<const uint8_t *>(buf);
    size_t remaining = count;
    while (remaining > 0) {
        ssize_t n = send(fd, ptr, remaining, MSG_NOSIGNAL);
        if (n < 0) {
            if (errno == EINTR) {
                continue;
            }
            return false;
        }
        if (n == 0) {
            return false;
        }
        ptr += n;
        remaining -= static_cast<size_t>(n);
    }
    return true;
}

static bool readAll(int fd, void *buf, size_t count) {
    if (fd < 0) {
        return false;
    }
    uint8_t *ptr = reinterpret_cast<uint8_t *>(buf);
    size_t remaining = count;
    while (remaining > 0) {
        ssize_t n = recv(fd, ptr, remaining, 0);
        if (n < 0) {
            if (errno == EINTR) {
                continue;
            }
            return false;
        }
        if (n == 0) {
            return false; // EOF
        }
        ptr += n;
        remaining -= static_cast<size_t>(n);
    }
    return true;
}

static bool sendFramedMessage(int fd, const std::string &msg) {
    if (fd < 0 || msg.empty() || msg.length() > 1048576) {
        return false;
    }
    uint32_t lenBe = htonl(static_cast<uint32_t>(msg.length()));
    if (!sendAll(fd, &lenBe, sizeof(lenBe))) {
        return false;
    }
    return sendAll(fd, msg.data(), msg.length());
}

void AiSecurityClearanceManager::routerWorker() {
    while (_running.load()) {
        std::shared_ptr<RouterWorkItem> item;
        {
            std::unique_lock<std::mutex> lock(_routerQueueMutex);
            _routerQueueCv.wait(lock, [this]() {
                return !_running.load() || !_routerQueue.empty();
            });
            if (!_running.load() && _routerQueue.empty()) {
                break;
            }
            if (_routerQueue.empty()) {
                continue;
            }
            item = _routerQueue.front();
            _routerQueue.pop();
        }

        if (!item) {
            continue;
        }

        RouterWorkResult res;
        SshResult sres = ZyxelDriver::getInstance().executeClearanceCommand(
            item->command, res.output, res.matchedPrompt, item->timeoutMs);
        res.sshResult = static_cast<int>(sres);

        item->promise.set_value(res);
    }
}

void AiSecurityClearanceManager::clientWorker(int clientFd, uint64_t connId,
                                              const std::string &/* peerIp */,
                                              uint16_t /* peerPort */) {
    auto lastActivity = std::chrono::steady_clock::now();
    while (_running.load()) {
        struct pollfd pfd;
        pfd.fd = clientFd;
        pfd.events = POLLIN;
        pfd.revents = 0;

        auto now = std::chrono::steady_clock::now();
        auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(now - lastActivity).count();
        if (elapsed >= 300) {
            break;
        }
        int timeoutMs = static_cast<int>((300 - elapsed) * 1000);
        if (timeoutMs > 1000) timeoutMs = 1000;

        int pr = poll(&pfd, 1, timeoutMs);
        if (pr < 0) {
            if (errno == EINTR) continue;
            break;
        }
        if (pr == 0) {
            continue;
        }
        if (pfd.revents & (POLLERR | POLLHUP | POLLNVAL)) {
            break;
        }

        uint32_t lenBe = 0;
        if (!readAll(clientFd, &lenBe, sizeof(lenBe))) {
            break;
        }
        uint32_t len = ntohl(lenBe);
        if (len == 0 || len > 4096) {
            break;
        }

        std::vector<char> buf(len);
        if (!readAll(clientFd, buf.data(), len)) {
            break;
        }

        std::string reqMsg(buf.data(), len);
        std::string respMsg = handleClientMessage(connId, reqMsg);
        if (!respMsg.empty()) {
            if (!sendFramedMessage(clientFd, respMsg)) {
                break;
            }
        }

        lastActivity = std::chrono::steady_clock::now();

        if (reqMsg == "CLOSE") {
            break;
        }
    }

    handleSocketClosed(connId);
    close(clientFd);
}

std::string AiSecurityClearanceManager::handleClientMessage(uint64_t connId, const std::string &msg) {
    std::unique_lock<std::recursive_mutex> lock(_mutex);
    auto it = _connections.find(connId);
    if (it == _connections.end()) {
        return "FAILED - Connection not found";
    }

    ClearanceConnection &conn = it->second;

    if (msg == "HELLO 1") {
        conn.protocolVersion = 1;
        return "HELLO 1";
    }

    if (msg.rfind("REQUEST", 0) == 0) {
        std::string meta;
        if (msg.size() > 7 && msg[7] == ' ') {
            meta = msg.substr(8);
            while (!meta.empty() && isspace((unsigned char)meta.front())) meta.erase(0, 1);
            while (!meta.empty() && isspace((unsigned char)meta.back())) meta.pop_back();
        }

        auto extractAttr = [](const std::string &metaStr, const std::string &key) -> std::string {
            std::string qKey = key + "=\"";
            size_t pos = metaStr.find(qKey);
            if (pos != std::string::npos) {
                size_t start = pos + qKey.length();
                size_t end = metaStr.find('"', start);
                if (end != std::string::npos) {
                    return metaStr.substr(start, end - start);
                }
                return metaStr.substr(start);
            }
            std::string uKey = key + "=";
            pos = metaStr.find(uKey);
            if (pos != std::string::npos) {
                size_t start = pos + uKey.length();
                size_t end = metaStr.find(' ', start);
                if (end != std::string::npos) {
                    return metaStr.substr(start, end - start);
                }
                return metaStr.substr(start);
            }
            return "";
        };

        // Validate security level: must explicitly request R, RW, or RWP
        ClearanceTier parsedTier = ClearanceTier::NONE;
        std::string tierAttr = extractAttr(meta, "tier");
        if (tierAttr.empty()) {
            if (meta == "read" || meta == "READ" || meta == "R" || meta == "r") {
                parsedTier = ClearanceTier::READ;
            } else if (meta == "write" || meta == "WRITE" || meta == "RW" || meta == "rw" ||
                       meta == "readwrite" || meta == "read-write" || meta == "READ_WRITE") {
                parsedTier = ClearanceTier::READ_WRITE;
            } else if (meta == "password" || meta == "PASSWORD" || meta == "RWP" || meta == "rwp" ||
                       meta == "elevated" || meta == "ELEVATED") {
                parsedTier = ClearanceTier::READ_WRITE_PASSWORD;
            }
        } else {
            if (tierAttr == "read" || tierAttr == "READ" || tierAttr == "R" || tierAttr == "r") {
                parsedTier = ClearanceTier::READ;
            } else if (tierAttr == "write" || tierAttr == "WRITE" || tierAttr == "RW" || tierAttr == "rw" ||
                       tierAttr == "readwrite" || tierAttr == "read-write" || tierAttr == "READ_WRITE") {
                parsedTier = ClearanceTier::READ_WRITE;
            } else if (tierAttr == "password" || tierAttr == "PASSWORD" || tierAttr == "RWP" || tierAttr == "rwp" ||
                       tierAttr == "elevated" || tierAttr == "ELEVATED") {
                parsedTier = ClearanceTier::READ_WRITE_PASSWORD;
            }
        }

        if (parsedTier == ClearanceTier::NONE) {
            return "RSP SYNTAX - Missing or invalid security tier (must be R, RW, or RWP)";
        }

        conn.requestedTier = parsedTier;
        if (!meta.empty()) {
            conn.metadata = meta;
        }

        if (conn.clearanceState == ClientClearanceState::DENIED) {
            auto now = std::chrono::steady_clock::now();
            if (conn.deadline > now) {
                return "DENIED " + std::to_string(connId);
            }
            conn.clearanceState = ClientClearanceState::NONE;
            conn.activeTier = ClearanceTier::NONE;
            conn.hasBeenGranted = false;
        }

        if (conn.clearanceState == ClientClearanceState::LEVEL2) {
            auto now = std::chrono::steady_clock::now();
            if (conn.deadline <= now) {
                conn.clearanceState = ClientClearanceState::LEVEL3;
                conn.activeTier = ClearanceTier::READ;
                conn.hasBeenGranted = true;
                conn.deadline = std::chrono::steady_clock::time_point::max();
                if (_activeLevel2ConnId == conn.connectionId) {
                    _activeLevel2ConnId = 0;
                }
            } else {
                uint32_t rem = (uint32_t)std::chrono::duration_cast<std::chrono::seconds>(conn.deadline - now).count();
                return "GRANTED " + std::to_string(connId) + " " + std::to_string(rem);
            }
        }
        if (conn.hasBeenGranted && conn.clearanceState == ClientClearanceState::LEVEL3 && parsedTier == ClearanceTier::READ) {
            auto now = std::chrono::steady_clock::now();
            if (conn.deadline == std::chrono::steady_clock::time_point::max()) {
                return "GRANTED " + std::to_string(connId) + " 0";
            } else if (conn.deadline > now) {
                uint32_t rem = (uint32_t)std::chrono::duration_cast<std::chrono::seconds>(conn.deadline - now).count();
                return "GRANTED " + std::to_string(connId) + " " + std::to_string(rem);
            } else {
                conn.clearanceState = ClientClearanceState::NONE;
                conn.activeTier = ClearanceTier::NONE;
                conn.hasBeenGranted = false;
            }
        }
        if (conn.clearanceState == ClientClearanceState::PENDING) {
            return "WAITING " + std::to_string(connId);
        }
        if (_activeLevel2ConnId != 0 && _activeLevel2ConnId != connId) {
            return "BUSY";
        }
        if (_pendingApprovalConnId != 0 && _pendingApprovalConnId != connId) {
            return "BUSY";
        }

        _pendingApprovalConnId = connId;
        conn.clearanceState = ClientClearanceState::PENDING;
        std::string peerAddr = conn.peerAddress;
        uint16_t peerPort = conn.peerPort;
        std::string connMeta = conn.metadata;
        lock.unlock();

        std::string fconn = formatConnId(connId);
        std::string sourceHost = peerAddr + ":" + std::to_string(peerPort);

        std::string platformVal = "Unknown";
        std::string modelVal = "Unknown";
        std::string sessionVal = "Unknown";

        if (!connMeta.empty()) {
            std::string p = extractAttr(connMeta, "platform");
            if (!p.empty()) platformVal = p;
            std::string m = extractAttr(connMeta, "model");
            if (!m.empty()) modelVal = m;
            std::string s = extractAttr(connMeta, "session");
            if (!s.empty()) sessionVal = s;
        }

        // Build 76-column wide structured provenance box (pure 7-bit ASCII for universal 80x24 terminal fidelity)
        std::string titlePrefix = "+-- AI Router Clearance Request: Connection " + fconn + " ";
        std::string boxTop = titlePrefix;
        if (boxTop.length() < 75) {
            boxTop.append(75 - boxTop.length(), '-');
        }
        boxTop += "+";

        auto formatBoxRow = [](const std::string &label, const std::string &value) -> std::string {
            const size_t maxValLen = 54;
            std::string val = value;
            if (val.length() > maxValLen) {
                val = val.substr(0, maxValLen - 3) + "...";
            }
            std::string row = "| " + label + ": " + val;
            if (row.length() < 75) {
                row.append(75 - row.length(), ' ');
            }
            row += "|";
            return row;
        };

        std::string reqTierStr = "READ/WRITE";
        int tierColor = NcursesConsole::PAIR_TIER_WRITE;
        bool isBlink = true;
        bool isBold = false;
        std::string ansiEsc = "\033[5;31m";
        std::string ansiReset = "\033[0m";

        if (parsedTier == ClearanceTier::READ) {
            reqTierStr = "READ";
            tierColor = NcursesConsole::PAIR_TIER_READ;
            isBlink = false;
            isBold = false;
            ansiEsc = "\033[32m";
        } else if (parsedTier == ClearanceTier::READ_WRITE_PASSWORD) {
            reqTierStr = "READ/WRITE + PASSWORD ACCESS!!!";
            tierColor = NcursesConsole::PAIR_TIER_ELEVATED;
            isBlink = true;
            isBold = true;
            ansiEsc = "\033[1;5;31m";
        } else {
            reqTierStr = "READ/WRITE";
            tierColor = NcursesConsole::PAIR_TIER_WRITE;
            isBlink = true;
            isBold = false;
            ansiEsc = "\033[5;31m";
        }

        std::string rowHost  = formatBoxRow("Source Host ", sourceHost);
        std::string rowPlat  = formatBoxRow("Platform    ", platformVal);
        std::string rowModel = formatBoxRow("AI Model    ", modelVal + " (Session: " + sessionVal + ")");
        std::string rowTier  = formatBoxRow("Requested   ", reqTierStr);
        std::string rowScope = formatBoxRow("Scope       ", reqTierStr == "READ" ? "Diagnostic and read-only inspection" : "Read diagnostics & reversible mutations");
        std::string rowRestr = formatBoxRow("Restrictions", "Commit ('write') and reboot are BLOCKED");
        std::string rowDur   = formatBoxRow("Duration    ", reqTierStr == "READ" ? "Default indefinite (0s)" : "300s (downgrades to READ upon expiry)");

        std::string boxBottom = "+" + std::string(74, '-') + "+";

        std::string lineGrant = "Run 'grant " + fconn + " [seconds] " + (reqTierStr == "READ" ? "read" : "write") + "' (" + (reqTierStr == "READ" ? "default indefinite" : "default 300s, downgrades to READ") + ") to authorize.";
        std::string lineDeny  = "Run 'deny  " + fconn + " [seconds]' (optional lockout) to reject.";

        if (NcursesConsole::getInstance().isRunning()) {
            NcursesConsole::getInstance().logOutput(boxTop, NcursesConsole::PAIR_WARN, true);
            NcursesConsole::getInstance().logOutput(rowHost, NcursesConsole::PAIR_INFO, false);
            NcursesConsole::getInstance().logOutput(rowPlat, NcursesConsole::PAIR_INFO, false);
            NcursesConsole::getInstance().logOutput(rowModel, NcursesConsole::PAIR_INFO, false);
            NcursesConsole::getInstance().logOutput(rowTier, tierColor, isBold, isBlink);
            NcursesConsole::getInstance().logOutput(rowScope, NcursesConsole::PAIR_INFO, false);
            NcursesConsole::getInstance().logOutput(rowRestr, NcursesConsole::PAIR_WARN, true);
            NcursesConsole::getInstance().logOutput(rowDur, NcursesConsole::PAIR_INFO, false);
            NcursesConsole::getInstance().logOutput(boxBottom, NcursesConsole::PAIR_WARN, true);
            NcursesConsole::getInstance().logOutput(lineGrant, tierColor, isBold, isBlink);
            NcursesConsole::getInstance().logOutput(lineDeny, NcursesConsole::PAIR_WARN, true);
        } else {
            std::cout << "\n" << boxTop << "\n"
                      << rowHost << "\n"
                      << rowPlat << "\n"
                      << rowModel << "\n"
                      << ansiEsc << rowTier << ansiReset << "\n"
                      << rowScope << "\n"
                      << rowRestr << "\n"
                      << rowDur << "\n"
                      << boxBottom << "\n"
                      << ansiEsc << lineGrant << ansiReset << "\n"
                      << lineDeny << std::endl;
        }
        return "WAITING " + std::to_string(connId);
    }

    if (msg == "CANCEL") {
        if (conn.hasInFlight) {
            ZyxelDriver::getInstance().cancelActiveCommand();
            return "RSP CANCELED - Command cancelled";
        }
        return "RSP OK - No active command";
    }

    if (msg == "CLOSE") {
        lock.unlock();
        handleSocketClosed(connId);
        return "OK";
    }

    if (msg.rfind("DO ", 0) == 0) {
        std::string cmdLine = msg.substr(3);
        return executeDo(lock, conn, cmdLine);
    }

    return "RSP UNCLASSIFIED - Unknown wire verb";
}

std::string AiSecurityClearanceManager::executeDo(std::unique_lock<std::recursive_mutex> &lock, ClearanceConnection &conn, const std::string &cmdLine) {
    int timeoutMs = 5000;
    std::string matchedMethod;
    std::string curPrompt = ZyxelDriver::getInstance().getLastMatchedPrompt();
    std::string cleanCurPrompt = curPrompt.empty() ? "-" : curPrompt;
    std::replace(cleanCurPrompt.begin(), cleanCurPrompt.end(), ' ', '_');

    LineClassification cls = AiSecurityClassifier::classify(cmdLine, curPrompt, timeoutMs, matchedMethod);

    ClearanceAuditEntry audit;
    audit.timestamp = time(nullptr);
    audit.connectionId = conn.connectionId;
    audit.peerAddress = conn.peerAddress;
    audit.peerPort = conn.peerPort;
    audit.command = cmdLine;
    audit.matchedMethod = matchedMethod;
    audit.requiredLevel = (cls == LineClassification::LEVEL3 ? 3 : (cls == LineClassification::LEVEL1 ? 1 : 2));
    audit.effectiveLevel = (conn.clearanceState == ClientClearanceState::LEVEL2 ? 2 : 3);
    audit.sshResult = -1;
    audit.durationMs = 0;

    // Check Prompt Owner (Section 3.1)
    if (_sessionOwnerConnId != 0 && _sessionOwnerConnId != conn.connectionId) {
        audit.decision = "BUSY";
        audit.reason = "Session prompt owned by another connection";
        logAudit(audit);
        return "BUSY";
    }

    // Check if router is currently executing a command for another connection (Section 5.2)
    if (_routerBusyConnId != 0 && _routerBusyConnId != conn.connectionId) {
        audit.decision = "BUSY";
        audit.reason = "Router executing command for another connection";
        logAudit(audit);
        return "BUSY";
    }

    if (cls == LineClassification::UNCLASSIFIED) {
        audit.decision = "UNCLASSIFIED";
        audit.reason = "Line shape not in accepted driver catalog";
        logAudit(audit);
        return "RSP UNCLASSIFIED " + cleanCurPrompt + " Command not classified";
    }

    if (cls == LineClassification::LEVEL1) {
        audit.decision = "FORBIDDEN";
        audit.reason = "Level 1 device management rejected on agent socket";
        logAudit(audit);
        return "RSP FORBIDDEN " + cleanCurPrompt + " Device management forbidden";
    }

    if (conn.clearanceState == ClientClearanceState::DENIED) {
        auto now = std::chrono::steady_clock::now();
        if (conn.deadline > now) {
            audit.decision = "NOT_AUTHORIZED";
            audit.reason = "Connection explicitly denied by operator";
            logAudit(audit);
            return "RSP NOT_AUTHORIZED " + cleanCurPrompt + " Connection denied by operator";
        }
        conn.clearanceState = ClientClearanceState::NONE;
        conn.activeTier = ClearanceTier::NONE;
        conn.hasBeenGranted = false;
    }

    if (conn.clearanceState == ClientClearanceState::LEVEL2 && conn.deadline <= std::chrono::steady_clock::now()) {
        conn.clearanceState = ClientClearanceState::LEVEL3;
        conn.activeTier = ClearanceTier::READ;
        conn.hasBeenGranted = true;
        conn.deadline = std::chrono::steady_clock::time_point::max();
        if (_activeLevel2ConnId == conn.connectionId) {
            _activeLevel2ConnId = 0;
        }
        if (!conn.currentSubmode.empty()) {
            logGrantRevoke(conn.connectionId, "LEVEL2", "grant expired");
            ZyxelDriver::getInstance().abandonPolicySubmode();
            conn.currentSubmode.clear();
        }
    }

    if (conn.clearanceState == ClientClearanceState::LEVEL3 && conn.deadline != std::chrono::steady_clock::time_point::max() && conn.deadline <= std::chrono::steady_clock::now()) {
        conn.clearanceState = ClientClearanceState::NONE;
        conn.activeTier = ClearanceTier::NONE;
        conn.hasBeenGranted = false;
    }

    bool authorized = false;
    if (conn.hasBeenGranted) {
        if (conn.clearanceState == ClientClearanceState::LEVEL2) {
            authorized = true;
        } else if (conn.clearanceState == ClientClearanceState::LEVEL3 && cls == LineClassification::LEVEL3) {
            authorized = true;
        }
    }

    if (!authorized) {
        audit.decision = "NOT_AUTHORIZED";
        audit.reason = conn.hasBeenGranted ? "Clearance grant required for mutation" : "Clearance grant required for read";
        logAudit(audit);
        return "RSP NOT_AUTHORIZED " + cleanCurPrompt + (conn.hasBeenGranted ? " Clearance grant required for mutation" : " Clearance grant required for read");
    }

    // Execute via single router worker (Section 5.2)
    conn.hasInFlight = true;
    conn.inFlightCommand = cmdLine;
    _routerBusyConnId = conn.connectionId;
    uint64_t connId = conn.connectionId;
    int clientFd = conn.socketFd;

    auto tStart = std::chrono::steady_clock::now();
    RouterWorkResult wRes;
    bool completed = false;

    if (!_running.load()) {
        lock.unlock();
        std::string out;
        std::string matchedPrompt;
        SshResult sres = ZyxelDriver::getInstance().executeClearanceCommand(cmdLine, out, matchedPrompt, timeoutMs);
        wRes.sshResult = static_cast<int>(sres);
        wRes.output = out;
        wRes.matchedPrompt = matchedPrompt;
        completed = true;
    } else {
        auto workItem = std::make_shared<RouterWorkItem>();
        workItem->connId = connId;
        workItem->command = cmdLine;
        workItem->timeoutMs = timeoutMs;
        std::future<RouterWorkResult> fut = workItem->promise.get_future();

        {
            std::lock_guard<std::mutex> qLock(_routerQueueMutex);
            _routerQueue.push(workItem);
            _routerQueueCv.notify_one();
        }

        lock.unlock(); // Release lock while router worker executes on router!

        while (_running.load()) {
            if (fut.wait_for(std::chrono::milliseconds(20)) == std::future_status::ready) {
                wRes = fut.get();
                completed = true;
                break;
            }

            if (clientFd >= 0) {
                struct pollfd pfd;
                pfd.fd = clientFd;
                pfd.events = POLLIN;
                pfd.revents = 0;
                int pret = poll(&pfd, 1, 0);
                if (pret > 0 && (pfd.revents & POLLIN)) {
                    uint32_t cLenBe = 0;
                    if (readAll(clientFd, &cLenBe, sizeof(cLenBe))) {
                        uint32_t cLen = ntohl(cLenBe);
                        if (cLen > 0 && cLen <= 4096) {
                            std::vector<char> cBuf(cLen);
                            if (readAll(clientFd, cBuf.data(), cLen)) {
                                std::string cancelMsg(cBuf.data(), cLen);
                                if (cancelMsg == "CANCEL") {
                                    ZyxelDriver::getInstance().cancelActiveCommand();
                                }
                            }
                        }
                    }
                }
                if (pret > 0 && (pfd.revents & (POLLERR | POLLHUP | POLLNVAL))) {
                    ZyxelDriver::getInstance().cancelActiveCommand();
                    break;
                }
            }
        }

        if (!completed) {
            wRes.sshResult = static_cast<int>(SshResult::ERR_INTERRUPTED);
            wRes.output = "Command cancelled or disconnected";
            wRes.matchedPrompt = "-";
        }
    }

    auto tEnd = std::chrono::steady_clock::now();

    lock.lock(); // Re-acquire lock

    auto it = _connections.find(connId);
    if (it != _connections.end()) {
        it->second.hasInFlight = false;
        it->second.inFlightCommand.clear();
    }
    if (_routerBusyConnId == connId) {
        _routerBusyConnId = 0;
    }

    audit.sshResult = wRes.sshResult;
    audit.durationMs = static_cast<int>(std::chrono::duration_cast<std::chrono::milliseconds>(tEnd - tStart).count());
    audit.outputSummary = wRes.output.substr(0, 100);

    // Update session owner state (Section 3.1)
    if ((cmdLine.rfind("secure-policy insert", 0) == 0 ||
         cmdLine.rfind("secure-policy append", 0) == 0) &&
        wRes.sshResult == static_cast<int>(SshResult::SUCCESS)) {
        _sessionOwnerConnId = connId;
    }
    if (it != _connections.end() &&
        AiSecurityClassifier::isSecurePolicySubmode(wRes.matchedPrompt)) {
        it->second.currentSubmode = wRes.matchedPrompt;
        _sessionOwnerConnId = connId;
    }
    if (it != _connections.end() &&
        AiSecurityClassifier::isRootPrompt(wRes.matchedPrompt)) {
        if (_sessionOwnerConnId == connId) {
            _sessionOwnerConnId = 0;
        }
        it->second.currentSubmode.clear();
    }

    std::string codeStr = "OK";
    if (wRes.sshResult == static_cast<int>(SshResult::ERR_TIMEOUT)) codeStr = "TIMEOUT";
    else if (wRes.sshResult == static_cast<int>(SshResult::ERR_LOCKED)) codeStr = "LOCKED";
    else if (wRes.sshResult == static_cast<int>(SshResult::ERR_SYNTAX)) codeStr = "SYNTAX";
    else if (wRes.sshResult == static_cast<int>(SshResult::ERR_DISCONNECTED)) codeStr = "DISCONNECTED";
    else if (wRes.sshResult == static_cast<int>(SshResult::ERR_INTERRUPTED)) codeStr = "CANCELED";
    else if (wRes.sshResult == static_cast<int>(SshResult::ERR_BUSY)) codeStr = "BUSY";
    else if (wRes.sshResult != static_cast<int>(SshResult::SUCCESS)) codeStr = "FAILED";

    audit.decision = codeStr;
    audit.reason = "Command executed";
    logAudit(audit);

    // Format: RSP <code> <prompt> <output>
    std::string cleanPrompt = wRes.matchedPrompt.empty() ? "-" : wRes.matchedPrompt;
    std::replace(cleanPrompt.begin(), cleanPrompt.end(), ' ', '_');

    return "RSP " + codeStr + " " + cleanPrompt + " " + wRes.output;
}

void AiSecurityClearanceManager::handleSocketClosed(uint64_t connId) {
    bool wasPending = false;
    {
        std::lock_guard<std::recursive_mutex> lock(_mutex);
        auto it = _connections.find(connId);
        if (it == _connections.end()) {
            return;
        }

        wasPending = (_pendingApprovalConnId == connId);
        if (_pendingApprovalConnId == connId) {
            _pendingApprovalConnId = 0;
        }
        if (_activeLevel2ConnId == connId) {
            _activeLevel2ConnId = 0;
        }
        if (_routerBusyConnId == connId) {
            _routerBusyConnId = 0;
            ZyxelDriver::getInstance().cancelActiveCommand();
        }
        if (_sessionOwnerConnId == connId || !it->second.currentSubmode.empty()) {
            _sessionOwnerConnId = 0;
            logGrantRevoke(connId, "LEVEL2", "socket closed");
            ZyxelDriver::getInstance().abandonPolicySubmode();
        }

        _connections.erase(it);
    }

    if (wasPending) {
        std::string fconn = formatConnId(connId);
        if (NcursesConsole::getInstance().isRunning()) {
            NcursesConsole::getInstance().logOutput(
                "Router clearance request from connection " + fconn + " closed.",
                NcursesConsole::PAIR_MUTED, false);
        } else {
            std::cout << "\nRouter clearance request from connection " << fconn << " closed." << std::endl;
        }
    }
}

bool AiSecurityClearanceManager::hasPendingApproval(uint64_t &outConnId, std::string &outPeerIp, uint16_t &outPeerPort, ClearanceTier *outRequestedTier) const {
    std::lock_guard<std::recursive_mutex> lock(_mutex);
    if (_pendingApprovalConnId == 0) {
        return false;
    }
    auto it = _connections.find(_pendingApprovalConnId);
    if (it == _connections.end()) {
        return false;
    }
    outConnId = it->second.connectionId;
    outPeerIp = it->second.peerAddress;
    outPeerPort = it->second.peerPort;
    if (outRequestedTier) {
        *outRequestedTier = it->second.requestedTier;
    }
    return true;
}

bool AiSecurityClearanceManager::consoleApprove(uint64_t connId, uint32_t seconds, ClearanceTier tier) {
    std::lock_guard<std::recursive_mutex> lock(_mutex);
    if (_pendingApprovalConnId != connId) {
        return false;
    }
    auto it = _connections.find(connId);
    if (it == _connections.end()) {
        _pendingApprovalConnId = 0;
        return false;
    }

    it->second.activeTier = tier;
    it->second.hasBeenGranted = true;
    if (tier == ClearanceTier::READ) {
        it->second.clearanceState = ClientClearanceState::READ;
        if (seconds == 0) {
            it->second.deadline = std::chrono::steady_clock::time_point::max();
        } else {
            it->second.deadline = std::chrono::steady_clock::now() + std::chrono::seconds(seconds);
        }
    } else {
        if (seconds == 0) {
            seconds = 300;
        }
        it->second.clearanceState = ClientClearanceState::READ_WRITE;
        it->second.deadline = std::chrono::steady_clock::now() + std::chrono::seconds(seconds);
        _activeLevel2ConnId = connId;
    }
    _pendingApprovalConnId = 0;

    if (it->second.socketFd >= 0) {
        std::string grantedMsg = "GRANTED " + std::to_string(connId) + " " + std::to_string(seconds);
        sendFramedMessage(it->second.socketFd, grantedMsg);
    }
    return true;
}

bool AiSecurityClearanceManager::consoleDeny(uint64_t connId, uint32_t seconds) {
    std::lock_guard<std::recursive_mutex> lock(_mutex);
    if (_pendingApprovalConnId != connId) {
        return false;
    }
    auto it = _connections.find(connId);
    if (it != _connections.end()) {
        uint32_t lockout = (seconds > 0) ? seconds : 300;
        it->second.clearanceState = ClientClearanceState::DENIED;
        it->second.deadline = std::chrono::steady_clock::now() + std::chrono::seconds(lockout);
        if (it->second.socketFd >= 0) {
            std::string deniedMsg = "DENIED " + std::to_string(connId);
            sendFramedMessage(it->second.socketFd, deniedMsg);
        }
    }
    _pendingApprovalConnId = 0;
    return true;
}

std::vector<ClearanceStatus> AiSecurityClearanceManager::getActiveClearances() const {
    std::lock_guard<std::recursive_mutex> lock(_mutex);
    std::vector<ClearanceStatus> result;
    auto now = std::chrono::steady_clock::now();

    for (const auto &kv : _connections) {
        const auto &conn = kv.second;
        ClearanceStatus st;
        st.connectionId = conn.connectionId;
        st.formattedConnId = formatConnId(conn.connectionId);
        st.peerAddress = conn.peerAddress;
        st.peerPort = conn.peerPort;
        st.metadata = conn.metadata;

        st.activeTier = conn.activeTier;
        if (conn.clearanceState == ClientClearanceState::LEVEL2) {
            if (conn.deadline > now) {
                st.clearanceState = ClientClearanceState::LEVEL2;
                st.activeTier = ClearanceTier::READ_WRITE;
                if (conn.deadline == std::chrono::steady_clock::time_point::max()) {
                    st.remainingSeconds = 0;
                } else {
                    st.remainingSeconds = (uint32_t)std::chrono::duration_cast<std::chrono::seconds>(conn.deadline - now).count();
                }
            } else {
                st.clearanceState = ClientClearanceState::LEVEL3;
                st.activeTier = ClearanceTier::READ;
                st.remainingSeconds = 0;
            }
        } else if (conn.clearanceState == ClientClearanceState::DENIED) {
            if (conn.deadline > now) {
                st.clearanceState = ClientClearanceState::DENIED;
                st.remainingSeconds = (uint32_t)std::chrono::duration_cast<std::chrono::seconds>(conn.deadline - now).count();
            } else {
                st.clearanceState = ClientClearanceState::NONE;
                st.activeTier = ClearanceTier::NONE;
                st.remainingSeconds = 0;
            }
        } else if (conn.clearanceState == ClientClearanceState::LEVEL3) {
            if (conn.hasBeenGranted) {
                st.clearanceState = ClientClearanceState::LEVEL3;
                st.activeTier = ClearanceTier::READ;
                if (conn.deadline == std::chrono::steady_clock::time_point::max()) {
                    st.remainingSeconds = 0;
                } else if (conn.deadline > now) {
                    st.remainingSeconds = (uint32_t)std::chrono::duration_cast<std::chrono::seconds>(conn.deadline - now).count();
                } else {
                    st.clearanceState = ClientClearanceState::NONE;
                    st.activeTier = ClearanceTier::NONE;
                    st.remainingSeconds = 0;
                }
            } else {
                st.clearanceState = ClientClearanceState::NONE;
                st.activeTier = ClearanceTier::NONE;
                st.remainingSeconds = 0;
            }
        } else {
            st.clearanceState = conn.clearanceState;
            st.activeTier = conn.activeTier;
            st.remainingSeconds = 0;
        }
        result.push_back(st);
    }
    return result;
}

uint64_t AiSecurityClearanceManager::getSessionOwner() const {
    std::lock_guard<std::recursive_mutex> lock(_mutex);
    return _sessionOwnerConnId;
}

void AiSecurityClearanceManager::clearSessionOwner() {
    std::lock_guard<std::recursive_mutex> lock(_mutex);
    uint64_t connId = _sessionOwnerConnId;
    _sessionOwnerConnId = 0;
    logGrantRevoke(connId, "LEVEL2", "owner cleared");
    ZyxelDriver::getInstance().abandonPolicySubmode();
}

void AiSecurityClearanceManager::logGrantRevoke(uint64_t connId,
                                               const std::string &grantType,
                                               const std::string &reason) {
    ClearanceAuditEntry audit;
    audit.timestamp = time(nullptr);
    audit.connectionId = connId;
    audit.command = grantType;
    audit.matchedMethod = grantType;
    audit.decision = "REVOKED";
    audit.reason = reason;
    audit.outputSummary = ZyxelDriver::getInstance().getLastMatchedPrompt();
    auto it = _connections.find(connId);
    if (it != _connections.end()) {
        audit.peerAddress = it->second.peerAddress;
        audit.peerPort = it->second.peerPort;
    }
    logAudit(audit);
}

void AiSecurityClearanceManager::logAudit(const ClearanceAuditEntry &entry) {
    _auditLog.push_back(entry);
    if (_auditLog.size() > 1000) {
        _auditLog.erase(_auditLog.begin(), _auditLog.begin() + 100);
    }

    std::string auditPath = Config::resolveHomePath(Config::getInstance().getAuditFile());
    if (auditPath.empty()) {
        return;
    }

    int fd = ::open(auditPath.c_str(), O_WRONLY | O_CREAT | O_APPEND, 0600);
    if (fd < 0) {
        return;
    }

    nlohmann::json obj = {
        {"timestamp", entry.timestamp},
        {"connection_id", entry.connectionId},
        {"peer_address", entry.peerAddress},
        {"peer_port", entry.peerPort},
        {"command", entry.command},
        {"matched_method", entry.matchedMethod},
        {"required_level", entry.requiredLevel},
        {"effective_level", entry.effectiveLevel},
        {"decision", entry.decision},
        {"reason", entry.reason},
        {"ssh_result", entry.sshResult},
        {"duration_ms", entry.durationMs},
        {"output_summary", entry.outputSummary}
    };

    std::string line = obj.dump() + "\n";
    ssize_t written = ::write(fd, line.data(), line.size());
    (void)written;
    ::fsync(fd);
    ::close(fd);
}

std::vector<ClearanceAuditEntry> AiSecurityClearanceManager::getAuditLog(size_t limit) const {
    std::lock_guard<std::recursive_mutex> lock(_mutex);
    if (limit == 0 || limit >= _auditLog.size()) {
        return _auditLog;
    }
    return std::vector<ClearanceAuditEntry>(_auditLog.end() - limit, _auditLog.end());
}

void AiSecurityClearanceManager::clearAuditLog() {
    std::lock_guard<std::recursive_mutex> lock(_mutex);
    _auditLog.clear();
}

void AiSecurityClearanceManager::resetForTesting() {
    {
        std::lock_guard<std::mutex> qLock(_routerQueueMutex);
        while (!_routerQueue.empty()) {
            _routerQueue.pop();
        }
    }
    {
        std::lock_guard<std::mutex> tLock(_clientThreadsMutex);
        for (auto &rec : _clientThreads) {
            if (rec.th.joinable()) {
                rec.th.join();
            }
        }
        std::vector<ClientThreadRecord>().swap(_clientThreads);
    }
    std::lock_guard<std::recursive_mutex> lock(_mutex);
    std::map<uint64_t, ClearanceConnection>().swap(_connections);
    _pendingApprovalConnId = 0;
    _activeLevel2ConnId = 0;
    _sessionOwnerConnId = 0;
    _routerBusyConnId = 0;
    std::vector<ClearanceAuditEntry>().swap(_auditLog);
    _nextConnId = 1;
}

uint64_t AiSecurityClearanceManager::registerConnectionForTesting(int fakeFd,
                                                                 const std::string &peerIp,
                                                                 uint16_t peerPort) {
    std::lock_guard<std::recursive_mutex> lock(_mutex);
    uint64_t cid = _nextConnId++;
    ClearanceConnection conn;
    conn.connectionId = cid;
    conn.socketFd = fakeFd;
    conn.peerAddress = peerIp;
    conn.peerPort = peerPort;
    conn.clearanceState = ClientClearanceState::NONE;
    conn.activeTier = ClearanceTier::NONE;
    conn.hasBeenGranted = false;
    _connections[cid] = std::move(conn);
    return cid;
}

void AiSecurityClearanceManager::setConnectionLevelForTesting(uint64_t connId,
                                                             ClientClearanceState state,
                                                             int remainingSeconds) {
    std::lock_guard<std::recursive_mutex> lock(_mutex);
    auto it = _connections.find(connId);
    if (it != _connections.end()) {
        it->second.clearanceState = state;
        if (state == ClientClearanceState::LEVEL2 || state == ClientClearanceState::LEVEL3) {
            it->second.hasBeenGranted = true;
            it->second.activeTier = (state == ClientClearanceState::LEVEL2) ? ClearanceTier::READ_WRITE : ClearanceTier::READ;
            if (state == ClientClearanceState::LEVEL2) {
                it->second.deadline = std::chrono::steady_clock::now() + std::chrono::seconds(remainingSeconds);
                _activeLevel2ConnId = connId;
            } else {
                if (remainingSeconds == 0) {
                    it->second.deadline = std::chrono::steady_clock::time_point::max();
                } else {
                    it->second.deadline = std::chrono::steady_clock::now() + std::chrono::seconds(remainingSeconds);
                }
                if (_activeLevel2ConnId == connId) {
                    _activeLevel2ConnId = 0;
                }
            }
        } else if (state == ClientClearanceState::DENIED) {
            it->second.hasBeenGranted = false;
            it->second.activeTier = ClearanceTier::NONE;
            it->second.deadline = std::chrono::steady_clock::now() + std::chrono::seconds(remainingSeconds);
            if (_activeLevel2ConnId == connId) {
                _activeLevel2ConnId = 0;
            }
        } else {
            it->second.hasBeenGranted = false;
            it->second.activeTier = ClearanceTier::NONE;
            if (_activeLevel2ConnId == connId) {
                _activeLevel2ConnId = 0;
            }
        }
    }
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
