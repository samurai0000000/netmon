/*
 * SyslogServer.cxx
 *
 * Copyright (C) 2026, Charles Chiou
 */

#include <unistd.h>
#include <poll.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <cstring>
#include <iostream>
#include <ctime>
#include <chrono>
#include <algorithm>

#include "SyslogServer.hxx"
#include "Config.hxx"
#include "SnmpDatabase.hxx"

int64_t SyslogParser::parseRfc3164Timestamp(const std::string &tsStr) {
    static const char *const months[] = {
        "Jan", "Feb", "Mar", "Apr", "May", "Jun",
        "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"
    };
    if (tsStr.size() < 15) return 0;
    std::string mStr = tsStr.substr(0, 3);
    int month = -1;
    for (int i = 0; i < 12; ++i) {
        if (mStr == months[i]) {
            month = i;
            break;
        }
    }
    if (month < 0) return 0;

    int day = 0, hour = 0, min = 0, sec = 0;
    if (sscanf(tsStr.c_str() + 3, "%d %d:%d:%d", &day, &hour, &min, &sec) != 4) {
        return 0;
    }

    time_t now = std::time(nullptr);
    struct tm tmNow;
    localtime_r(&now, &tmNow);

    struct tm tmParsed = {};
    tmParsed.tm_year = tmNow.tm_year;
    tmParsed.tm_mon = month;
    tmParsed.tm_mday = day;
    tmParsed.tm_hour = hour;
    tmParsed.tm_min = min;
    tmParsed.tm_sec = sec;
    tmParsed.tm_isdst = -1;

    time_t res = mktime(&tmParsed);
    return res > 0 ? static_cast<int64_t>(res) : static_cast<int64_t>(now);
}

int64_t SyslogParser::parseIsoTimestamp(const std::string &tsStr) {
    if (tsStr.empty() || tsStr == "-") return 0;
    int year = 0, month = 0, day = 0, hour = 0, min = 0, sec = 0;
    if (sscanf(tsStr.c_str(), "%4d-%2d-%2dT%2d:%2d:%2d", &year, &month, &day, &hour, &min, &sec) < 6) {
        return 0;
    }
    struct tm tmParsed = {};
    tmParsed.tm_year = year - 1900;
    tmParsed.tm_mon = month - 1;
    tmParsed.tm_mday = day;
    tmParsed.tm_hour = hour;
    tmParsed.tm_min = min;
    tmParsed.tm_sec = sec;
    tmParsed.tm_isdst = 0;

    time_t res = timegm(&tmParsed);
    return res > 0 ? static_cast<int64_t>(res) : static_cast<int64_t>(std::time(nullptr));
}

bool SyslogParser::parse(const std::string &raw, SyslogEvent &out) {
    return parse(raw.data(), raw.size(), out);
}

bool SyslogParser::parse(const char *data, size_t len, SyslogEvent &out) {
    if (data == nullptr || len == 0) {
        return false;
    }
    if (len > MAX_DATAGRAM_SIZE) {
        return false;
    }
    if (data[0] != '<') {
        return false;
    }

    size_t priEnd = 1;
    while (priEnd < len && data[priEnd] != '>') {
        priEnd++;
    }
    if (priEnd >= len || priEnd == 1 || priEnd > 5) {
        return false;
    }

    int priVal = 0;
    for (size_t i = 1; i < priEnd; ++i) {
        if (data[i] < '0' || data[i] > '9') {
            return false;
        }
        priVal = priVal * 10 + (data[i] - '0');
    }
    if (priVal < 0 || priVal > 191) {
        return false;
    }

    out.facility = priVal / 8;
    out.severity = priVal % 8;

    size_t cur = priEnd + 1;
    if (cur >= len) {
        return false;
    }

    out.raw.assign(data, len);

    // Detect RFC 5424 (version 1: starts with "1 ")
    if (len - cur >= 2 && data[cur] == '1' && data[cur + 1] == ' ') {
        cur += 2; // skip "1 "

        auto nextToken = [&](std::string &tok) -> bool {
            while (cur < len && data[cur] == ' ') cur++;
            if (cur >= len) return false;
            size_t start = cur;
            while (cur < len && data[cur] != ' ') cur++;
            tok.assign(data + start, cur - start);
            return true;
        };

        // 1. TIMESTAMP
        std::string tsStr;
        if (!nextToken(tsStr)) return false;
        out.timestamp = parseIsoTimestamp(tsStr);
        if (out.timestamp == 0) {
            out.timestamp = std::time(nullptr);
        }

        // 2. HOSTNAME
        std::string hostname;
        if (!nextToken(hostname)) return false;

        // 3. APP-NAME
        std::string appName;
        if (!nextToken(appName)) return false;

        // 4. PROCID
        std::string procId;
        if (!nextToken(procId)) return false;

        // 5. MSGID
        std::string msgId;
        if (!nextToken(msgId)) return false;

        if (appName != "-" && !appName.empty()) {
            out.tag = appName;
            if (procId != "-" && !procId.empty()) {
                out.tag += "[" + procId + "]";
            }
        } else if (procId != "-" && !procId.empty()) {
            out.tag = procId;
        } else {
            out.tag.clear();
        }

        // 6. STRUCTURED-DATA
        while (cur < len && data[cur] == ' ') cur++;
        if (cur < len) {
            if (data[cur] == '-') {
                cur++; // skip nil structured data
            } else if (data[cur] == '[') {
                while (cur < len && data[cur] == '[') {
                    size_t endBracket = cur;
                    while (endBracket < len && data[endBracket] != ']') {
                        endBracket++;
                    }
                    if (endBracket < len && data[endBracket] == ']') {
                        cur = endBracket + 1;
                        while (cur < len && data[cur] == ' ') cur++;
                    } else {
                        break;
                    }
                }
            }
        }

        // Remainder is MSG
        while (cur < len && data[cur] == ' ') cur++;
        if (cur < len) {
            out.message.assign(data + cur, len - cur);
        } else {
            out.message.clear();
        }
        return true;
    }

    // RFC 3164
    static const char *const months[] = {
        "Jan", "Feb", "Mar", "Apr", "May", "Jun",
        "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"
    };
    bool hasRfc3164Ts = false;
    if (len - cur >= 15) {
        for (int m = 0; m < 12; ++m) {
            if (std::strncmp(data + cur, months[m], 3) == 0 && data[cur + 3] == ' ') {
                hasRfc3164Ts = true;
                break;
            }
        }
    }

    if (hasRfc3164Ts) {
        std::string tsStr(data + cur, 15);
        out.timestamp = parseRfc3164Timestamp(tsStr);
        cur += 15;
        while (cur < len && data[cur] == ' ') cur++;

        // Hostname
        while (cur < len && data[cur] != ' ') cur++;
        while (cur < len && data[cur] == ' ') cur++;
    } else {
        out.timestamp = std::time(nullptr);
    }

    // Look for TAG: MSG
    size_t colonPos = cur;
    while (colonPos < len && data[colonPos] != ':') {
        if (data[colonPos] == ' ' && (colonPos - cur) > 32) break;
        colonPos++;
    }

    if (colonPos < len && data[colonPos] == ':') {
        out.tag.assign(data + cur, colonPos - cur);
        cur = colonPos + 1;
        while (cur < len && data[cur] == ' ') cur++;
        if (cur < len) {
            out.message.assign(data + cur, len - cur);
        } else {
            out.message.clear();
        }
    } else {
        if (cur < len) {
            out.message.assign(data + cur, len - cur);
        } else {
            out.message.clear();
        }
    }

    return true;
}

SyslogServer &SyslogServer::getInstance() {
    static SyslogServer instance;
    return instance;
}

SyslogServer::SyslogServer()
    : _running(false),
      _sockfd(-1),
      _port(1514),
      _bindAddress("0.0.0.0"),
      _routerAddress("") {
}

SyslogServer::~SyslogServer() {
    stop();
    join();
}

bool SyslogServer::start(const std::string &bindAddress, int port) {
    if (_running.load()) {
        return true;
    }

    _bindAddress = bindAddress;
    _port = port;

    _sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (_sockfd < 0) {
        std::cerr << "SyslogServer: Failed to create UDP socket" << std::endl;
        return false;
    }

    int opt = 1;
    setsockopt(_sockfd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    struct sockaddr_in servaddr;
    std::memset(&servaddr, 0, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_port = htons(_port);
    if (_bindAddress.empty() || _bindAddress == "0.0.0.0") {
        servaddr.sin_addr.s_addr = INADDR_ANY;
    } else {
        inet_pton(AF_INET, _bindAddress.c_str(), &servaddr.sin_addr);
    }

    if (bind(_sockfd, reinterpret_cast<const struct sockaddr *>(&servaddr), sizeof(servaddr)) < 0) {
        std::cerr << "SyslogServer: Failed to bind UDP " << _bindAddress << ":" << _port << std::endl;
        ::close(_sockfd);
        _sockfd = -1;
        return false;
    }

    socklen_t addrLen = sizeof(servaddr);
    if (getsockname(_sockfd, reinterpret_cast<struct sockaddr *>(&servaddr), &addrLen) == 0) {
        _port = ntohs(servaddr.sin_port);
    }

    _running.store(true);
    _thread = std::thread(&SyslogServer::runLoop, this);
    std::cout << "SyslogServer: Listening on UDP " << _bindAddress << ":" << _port << std::endl;
    return true;
}

void SyslogServer::stop() {
    _running.store(false);
}

void SyslogServer::join() {
    if (_thread.joinable()) {
        _thread.join();
    }
    if (_sockfd >= 0) {
        ::close(_sockfd);
        _sockfd = -1;
    }
}

bool SyslogServer::isRunning() const {
    return _running.load();
}

void SyslogServer::setRouterAddress(const std::string &ip) {
    std::lock_guard<std::mutex> lock(_mutex);
    _routerAddress = ip;
}

std::string SyslogServer::getRouterAddress() const {
    std::lock_guard<std::mutex> lock(_mutex);
    return _routerAddress;
}

int SyslogServer::getPort() const {
    return _port;
}

void SyslogServer::resetForTesting() {
    stop();
    join();
    std::lock_guard<std::mutex> lock(_mutex);
    std::string().swap(_routerAddress);
    std::string().swap(_bindAddress);
    _port = 1514;
}

void SyslogServer::runLoop() {
    struct pollfd pfd;
    pfd.fd = _sockfd;
    pfd.events = POLLIN;

    while (_running.load()) {
        int ret = poll(&pfd, 1, 100);
        if (ret < 0) {
            if (errno == EINTR) continue;
            break;
        }
        if (ret == 0) continue; // poll timeout

        if (pfd.revents & POLLIN) {
            char buf[SyslogParser::MAX_DATAGRAM_SIZE + 128];
            struct sockaddr_in clientAddr;
            socklen_t cLen = sizeof(clientAddr);
            ssize_t n = recvfrom(_sockfd, buf, sizeof(buf) - 1, 0,
                                 reinterpret_cast<struct sockaddr *>(&clientAddr), &cLen);
            if (n <= 0) continue;

            char clientIp[INET_ADDRSTRLEN] = {0};
            inet_ntop(AF_INET, &clientAddr.sin_addr, clientIp, sizeof(clientIp));

            std::string expected = getRouterAddress();
            if (expected.empty()) {
                expected = Config::getInstance().getGatewayHost();
            }
            if (expected.empty() && !Config::getInstance().getSnmpTargets().empty()) {
                expected = Config::getInstance().getSnmpTargets()[0].ip;
            }

            if (expected.empty() || std::string(clientIp) != expected) {
                // Drop datagram from non-router source
                continue;
            }

            buf[n] = '\0';
            SyslogEvent ev;
            ev.sourceIp = clientIp;
            if (SyslogParser::parse(buf, static_cast<size_t>(n), ev)) {
                SnmpDatabase::getInstance().insertSyslogEvent(ev);
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
