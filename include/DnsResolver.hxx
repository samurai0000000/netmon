/*
 * DnsResolver.hxx
 *
 * Copyright (C) 2026, Charles Chiou
 */

#ifndef NETMON_DNSRESOLVER_HXX
#define NETMON_DNSRESOLVER_HXX

#include <string>
#include <queue>
#include <mutex>
#include <thread>
#include <condition_variable>
#include <atomic>
#include <functional>
#include <unordered_map>
#include <chrono>

class DnsResolver {
public:
    using DnsCallback = std::function<void(const std::string &mac,
                                           const std::string &ip,
                                           const std::string &hostname,
                                           bool forwardConfirmed)>;

    static DnsResolver &getInstance();

    bool start(const std::string &stripDomain = "selfso.com");
    void stop();

    void enqueueLookup(const std::string &mac, const std::string &ip);
    void setDnsCallback(DnsCallback cb);
    void waitUntilDone(int timeoutMs = 1000);

    static std::string stripDomainSuffix(const std::string &fqdn,
                                        const std::string &stripDomain = "");

    static bool forwardConfirm(const std::string &fqdn, const std::string &expectedIp);

private:
    DnsResolver();
    ~DnsResolver();
    DnsResolver(const DnsResolver &) = delete;
    DnsResolver &operator=(const DnsResolver &) = delete;

    void workerLoop();

    struct Task {
        std::string mac;
        std::string ip;
    };

    std::atomic<bool> _running;
    std::thread _workerThread;
    std::mutex _queueMutex;
    std::condition_variable _cv;
    std::queue<Task> _taskQueue;
    std::unordered_map<std::string, bool> _pendingIps;

    std::mutex _cacheMutex;
    std::unordered_map<std::string, std::chrono::steady_clock::time_point> _negativeCache;

    std::string _stripDomain;
    DnsCallback _dnsCallback;
};

#endif /* NETMON_DNSRESOLVER_HXX */

/*
 * Local variables:
 * mode: C++
 * c-file-style: "BSD"
 * c-basic-offset: 4
 * tab-width: 4
 * indent-tabs-mode: nil
 * End:
 */
