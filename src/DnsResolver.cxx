/*
 * DnsResolver.cxx
 *
 * Copyright (C) 2026, Charles Chiou
 */

#include "DnsResolver.hxx"
#include <iostream>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <cstring>
#include <algorithm>
#include <cctype>

DnsResolver &DnsResolver::getInstance() {
    static DnsResolver instance;
    return instance;
}

DnsResolver::DnsResolver()
    : _running(false)
    , _stripDomain("selfso.com")
    , _dnsCallback(nullptr) {
}

DnsResolver::~DnsResolver() {
    stop();
}

bool DnsResolver::start(const std::string &stripDomain) {
    if (_running.load()) {
        return true;
    }

    if (!stripDomain.empty()) {
        _stripDomain = stripDomain;
    }

    _running = true;
    _workerThread = std::thread(&DnsResolver::workerLoop, this);
    return true;
}

void DnsResolver::stop() {
    if (!_running.load()) {
        return;
    }

    _running = false;
    _cv.notify_all();

    if (_workerThread.joinable()) {
        _workerThread.join();
    }
}

void DnsResolver::setDnsCallback(DnsCallback cb) {
    _dnsCallback = cb;
}

void DnsResolver::enqueueLookup(const std::string &mac, const std::string &ip) {
    if (ip.empty() || mac.empty()) {
        return;
    }

    {
        std::lock_guard<std::mutex> lock(_cacheMutex);
        auto it = _negativeCache.find(ip);
        if (it != _negativeCache.end()) {
            auto now = std::chrono::steady_clock::now();
            if (std::chrono::duration_cast<std::chrono::seconds>(now - it->second).count() < 300) {
                return; // Suppress recently failed lookup
            }
        }
    }

    std::lock_guard<std::mutex> lock(_queueMutex);
    if (_pendingIps.find(ip) != _pendingIps.end()) {
        return; // Already pending
    }

    _pendingIps[ip] = true;
    _taskQueue.push({mac, ip});
    _cv.notify_one();
}

std::string DnsResolver::stripDomainSuffix(const std::string &fqdn,
                                          const std::string &stripDomain) {
    if (fqdn.empty()) {
        return "";
    }

    std::string s = fqdn;
    // Remove trailing dot if present
    if (!s.empty() && s.back() == '.') {
        s.pop_back();
    }

    if (!stripDomain.empty()) {
        std::string suffix = stripDomain;
        if (suffix.front() != '.') {
            suffix = "." + suffix;
        }
        if (s.length() > suffix.length() &&
            s.rfind(suffix) == (s.length() - suffix.length())) {
            return s.substr(0, s.length() - suffix.length());
        }
    }

    // Fallback: strip from first dot if multiple labels
    size_t firstDot = s.find('.');
    if (firstDot != std::string::npos && firstDot > 0) {
        return s.substr(0, firstDot);
    }

    return s;
}

bool DnsResolver::forwardConfirm(const std::string &fqdn, const std::string &expectedIp) {
    if (fqdn.empty() || expectedIp.empty()) {
        return false;
    }

    struct addrinfo hints;
    struct addrinfo *res = nullptr;
    std::memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;

    if (getaddrinfo(fqdn.c_str(), nullptr, &hints, &res) != 0) {
        return false;
    }

    bool matched = false;
    for (struct addrinfo *p = res; p != nullptr; p = p->ai_next) {
        struct sockaddr_in *sin = reinterpret_cast<struct sockaddr_in *>(p->ai_addr);
        char ipBuf[INET_ADDRSTRLEN];
        if (inet_ntop(AF_INET, &(sin->sin_addr), ipBuf, sizeof(ipBuf))) {
            if (expectedIp == ipBuf) {
                matched = true;
                break;
            }
        }
    }

    freeaddrinfo(res);
    return matched;
}

void DnsResolver::workerLoop() {
    while (_running.load()) {
        Task task;
        {
            std::unique_lock<std::mutex> lock(_queueMutex);
            _cv.wait(lock, [this]() {
                return !_running.load() || !_taskQueue.empty();
            });

            if (!_running.load() && _taskQueue.empty()) {
                break;
            }

            task = _taskQueue.front();
            _taskQueue.pop();
        }

        struct sockaddr_in sa;
        std::memset(&sa, 0, sizeof(sa));
        sa.sin_family = AF_INET;

        bool success = false;
        std::string resolvedName;
        bool confirmed = false;

        if (inet_pton(AF_INET, task.ip.c_str(), &(sa.sin_addr)) == 1) {
            char hostBuf[NI_MAXHOST];
            int rc = getnameinfo(reinterpret_cast<struct sockaddr *>(&sa), sizeof(sa),
                                 hostBuf, sizeof(hostBuf), nullptr, 0, NI_NAMEREQD);
            if (rc == 0) {
                std::string fqdn = hostBuf;
                confirmed = forwardConfirm(fqdn, task.ip);
                resolvedName = stripDomainSuffix(fqdn, _stripDomain);
                success = true;
            }
        }

        {
            std::lock_guard<std::mutex> lock(_cacheMutex);
            if (success) {
                _negativeCache.erase(task.ip);
            } else {
                _negativeCache[task.ip] = std::chrono::steady_clock::now();
            }
        }

        {
            std::lock_guard<std::mutex> lock(_queueMutex);
            _pendingIps.erase(task.ip);
        }

        if (success && !resolvedName.empty() && _dnsCallback) {
            _dnsCallback(task.mac, task.ip, resolvedName, confirmed);
        }
    }
}

void DnsResolver::waitUntilDone(int timeoutMs) {
    auto deadline = std::chrono::steady_clock::now() + std::chrono::milliseconds(timeoutMs);
    while (_running.load() && std::chrono::steady_clock::now() < deadline) {
        {
            std::lock_guard<std::mutex> lock(_queueMutex);
            if (_taskQueue.empty() && _pendingIps.empty()) {
                break;
            }
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
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
