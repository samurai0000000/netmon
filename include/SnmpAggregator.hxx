/*
 * SnmpAggregator.hxx
 *
 * Copyright (C) 2026, Charles Chiou
 */

#ifndef NETMON_SNMPAGGREGATOR_HXX
#define NETMON_SNMPAGGREGATOR_HXX

#include <string>
#include <vector>
#include <map>
#include <mutex>
#include <thread>
#include <atomic>
#include <cstdint>
#include <nlohmann/json.hpp>

struct InterfaceState {
    int         ifIndex = 0;
    std::string ifName;
    std::string ifDescr;
    std::string ifAlias;
    std::string ipAddress;
    uint32_t    ifType = 0;
    uint64_t    ifSpeed = 0;
    int         operStatus = 1;

    // 64-bit HC counters
    uint64_t    hcInOctets = 0;
    uint64_t    hcOutOctets = 0;
    uint64_t    prevHcIn = 0;
    uint64_t    prevHcOut = 0;
    int64_t     lastSampleTime = 0;

    // Computed rates
    double      rateInBps = 0.0;
    double      rateOutBps = 0.0;
    double      avg5MinInBps = 0.0;
    double      avg5MinOutBps = 0.0;

    uint32_t    inErrors = 0;
    uint32_t    outErrors = 0;
};

struct SnmpDeviceState {
    std::string targetIp;
    std::string sysDescr;
    std::string sysName;
    int64_t     sysUpTime = 0;
    int64_t     prevSysUpTime = 0;
    int64_t     lastPollTime = 0;
    bool        reachable = false;
    std::vector<std::string> wanInterfaceNames;
    std::map<std::string, InterfaceState> interfaces; // Keyed by persistent ifName
};

class SnmpAggregator {
public:
    static SnmpAggregator &getInstance();

    bool start();
    void stop();
    void join();
    bool isRunning() const;

    // AI MCP and Web API queries (read pre-calculated data instantly, 0 ms latency)
    nlohmann::json getDeviceMetrics(const std::string &targetIp,
                                    const std::string &filter = "monitored");
    nlohmann::json getWanStatus(const std::string &targetIp = "");
    nlohmann::json getInterfaceCounters(const std::string &targetIp,
                                        const std::string &ifName);
    nlohmann::json queryOid(const std::string &targetIp,
                            const std::string &oidStr,
                            const std::string &community = "");

private:
    SnmpAggregator();
    ~SnmpAggregator();
    SnmpAggregator(const SnmpAggregator &) = delete;
    SnmpAggregator &operator=(const SnmpAggregator &) = delete;

    void pollLoop();
    void pollTarget(SnmpDeviceState &device,
                    const std::string &community,
                    const std::string &version,
                    int port);

    bool querySystemInfo(void *ss, SnmpDeviceState &device);
    bool walkInterfaces(void *ss, SnmpDeviceState &device);

    std::atomic<bool>                      _running;
    std::thread                            _pollThread;
    mutable std::mutex                     _mutex;
    mutable std::mutex                     _snmpMutex;
    std::map<std::string, SnmpDeviceState> _devices;
};

#endif /* NETMON_SNMPAGGREGATOR_HXX */

/*
 * Local variables:
 * mode: C++
 * c-file-style: "BSD"
 * c-basic-offset: 4
 * tab-width: 4
 * indent-tabs-mode: nil
 * End:
 */
