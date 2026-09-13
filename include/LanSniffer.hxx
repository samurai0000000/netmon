/*
 * LanSniffer.hxx
 *
 * Copyright (C) 2026, Charles Chiou
 */

#ifndef NETMON_LANSNIFFER_HXX
#define NETMON_LANSNIFFER_HXX

#include <string>
#include <vector>
#include <unordered_map>
#include <unordered_set>
#include <mutex>
#include <thread>
#include <atomic>
#include <pcap.h>
#include <nlohmann/json.hpp>
#include "HostMetrics.hxx"

struct MinuteBucket {
    time_t timestamp = 0;
    uint64_t bytes = 0;
    uint64_t packets = 0;
    std::unordered_map<std::string, uint64_t> hostBytes;
};

class LanSniffer {
public:
    static LanSniffer &getInstance();

    bool start();
    void stop();
    bool isRunning() const;
    bool isPcapActive() const;
    std::string getPcapStatus() const;
    std::string getPcapError() const;
    std::string getPcapRemediationHint() const;
    bool checkPcapPermissions(std::string &reason, std::string &remediation) const;

    nlohmann::json getTopTalkers(size_t limit = 10, int windowMinutes = 15) const;
    nlohmann::json getTrafficSummary() const;
    nlohmann::json getDevicesJson() const;
    nlohmann::json getUnregisteredDevicesJson() const;
    nlohmann::json getHostMetricsJson(const std::string &mac) const;

    void processPacket(const uint8_t *packet, size_t caplen, size_t origlen);

private:
    LanSniffer();
    ~LanSniffer();
    LanSniffer(const LanSniffer &) = delete;
    LanSniffer &operator=(const LanSniffer &) = delete;

    void pcapWorker();
    void scannerWorker();
    void scanArpTable();
    void scanProcDev();
    void advanceMinuteRing();

    std::atomic<bool> _running;
    std::atomic<bool> _pcapActive;
    pcap_t *_pcapHandle;
    std::string _interface;
    std::string _pcapStatus;
    std::string _pcapError;
    std::string _pcapRemediationHint;

    std::thread _pcapThread;
    std::thread _scannerThread;

    mutable std::mutex _mutex;

    // Sliding window ring buffer (60 buckets = 1 hour)
    MinuteBucket _minuteRing[60];
    size_t _currentRingIdx;
    time_t _lastRingAdvance;

    // In-memory host metrics map (keyed by MAC address)
    std::unordered_map<std::string, HostMetrics> _hostMetrics;
    std::unordered_map<std::string, std::unordered_set<std::string>> _hostPeers;
    std::unordered_map<std::string, std::unordered_set<uint16_t>> _hostDstPorts;

    // Protocol & traffic counters
    uint64_t _totalBytes;
    uint64_t _totalPackets;
    uint64_t _dnsPackets;
    uint64_t _httpsPackets;
    uint64_t _sshPackets;
    uint64_t _httpPackets;
    uint64_t _arpPackets;
    uint64_t _bcastPackets;
    uint64_t _mcastPackets;
    uint64_t _icmpPackets;
    uint64_t _otherPackets;

    // Proc/net/dev baseline counters for fallback rate tracking
    uint64_t _devRxBytes;
    uint64_t _devTxBytes;
    uint64_t _devRxPackets;
    uint64_t _devTxPackets;
    time_t   _devLastTime;
    double   _devCurrentBps;
    double   _devCurrentPps;

    time_t   _startTime;
};

#endif /* NETMON_LANSNIFFER_HXX */

/*
 * Local variables:
 * mode: C++
 * c-file-style: "BSD"
 * c-basic-offset: 4
 * tab-width: 4
 * indent-tabs-mode: nil
 * End:
 */
