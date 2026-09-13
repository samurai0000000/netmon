/*
 * LanSniffer.cxx
 *
 * Copyright (C) 2026, Charles Chiou
 */

#include "LanSniffer.hxx"
#include "Config.hxx"
#include "DeviceRegistry.hxx"
#include "OuiDatabase.hxx"
#include <algorithm>
#include <arpa/inet.h>
#include <chrono>
#include <cstdio>
#include <cstring>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <net/ethernet.h>
#include <netinet/if_ether.h>
#include <netinet/ip.h>
#include <netinet/tcp.h>
#include <netinet/udp.h>
#include <sstream>
#include <unistd.h>

LanSniffer &LanSniffer::getInstance() {
    static LanSniffer instance;
    return instance;
}

LanSniffer::LanSniffer()
    : _running(false)
    , _pcapActive(false)
    , _pcapHandle(nullptr)
    , _interface("")
    , _pcapStatus("inactive")
    , _pcapError("")
    , _pcapRemediationHint("")
    , _currentRingIdx(0)
    , _lastRingAdvance(0)
    , _totalBytes(0)
    , _totalPackets(0)
    , _dnsPackets(0)
    , _httpsPackets(0)
    , _sshPackets(0)
    , _httpPackets(0)
    , _arpPackets(0)
    , _bcastPackets(0)
    , _mcastPackets(0)
    , _icmpPackets(0)
    , _otherPackets(0)
    , _devRxBytes(0)
    , _devTxBytes(0)
    , _devRxPackets(0)
    , _devTxPackets(0)
    , _devLastTime(0)
    , _devCurrentBps(0.0)
    , _devCurrentPps(0.0)
    , _startTime(0) {
    time_t now = time(nullptr);
    _lastRingAdvance = now;
    _startTime = now;
    for (size_t i = 0; i < 60; ++i) {
        _minuteRing[i].timestamp = now;
    }
}

LanSniffer::~LanSniffer() {
    stop();
}

bool LanSniffer::isRunning() const {
    return _running.load();
}

bool LanSniffer::isPcapActive() const {
    return _pcapActive.load();
}

std::string LanSniffer::getPcapStatus() const {
    std::lock_guard<std::mutex> lock(_mutex);
    return _pcapStatus;
}

std::string LanSniffer::getPcapError() const {
    std::lock_guard<std::mutex> lock(_mutex);
    return _pcapError;
}

std::string LanSniffer::getPcapRemediationHint() const {
    std::lock_guard<std::mutex> lock(_mutex);
    return _pcapRemediationHint;
}

bool LanSniffer::checkPcapPermissions(std::string &reason, std::string &remediation) const {
    std::string iface = Config::getInstance().getInterface();
    if (iface.empty()) {
        iface = "br0";
    }

    char errbuf[PCAP_ERRBUF_SIZE];
    memset(errbuf, 0, sizeof(errbuf));

    pcap_t *testHandle = pcap_open_live(iface.c_str(), 64, 0, 10, errbuf);
    if (!testHandle) {
        reason = std::string(errbuf);
        if (reason.find("permission") != std::string::npos ||
            reason.find("Operation not permitted") != std::string::npos ||
            reason.find("socket") != std::string::npos) {
            remediation = "Run 'make setcap' or 'sudo setcap cap_net_raw=eip <binary>' to enable packet capture.";
        } else {
            remediation = "Verify interface '" + iface + "' exists and is UP (e.g. 'ip link show " + iface + "').";
        }
        return false;
    }

    pcap_close(testHandle);
    reason = "Ready";
    remediation = "";
    return true;
}

static void pcapCallback(u_char *user, const struct pcap_pkthdr *h, const u_char *bytes) {
    auto *sniffer = reinterpret_cast<LanSniffer *>(user);
    if (sniffer) {
        sniffer->processPacket(bytes, h->caplen, h->len);
    }
}

bool LanSniffer::start() {
    if (_running.load()) {
        return true;
    }

    _interface = Config::getInstance().getInterface();
    if (_interface.empty()) {
        _interface = "br0";
    }

    _running.store(true);
    _startTime = time(nullptr);
    _lastRingAdvance = _startTime;

    // Open PCAP handle synchronously so permissions and active status are determined before threads launch
    char errbuf[PCAP_ERRBUF_SIZE];
    memset(errbuf, 0, sizeof(errbuf));

    // snaplen = 96 bytes: capture only headers to eliminate memory bloat and disk writes
    _pcapHandle = pcap_open_live(_interface.c_str(), 96, 1, 100, errbuf);
    if (!_pcapHandle) {
        std::string errStr(errbuf);
        std::string hint = "Run 'make setcap' or 'sudo setcap cap_net_raw=eip <binary>' to enable packet capture.";

        {
            std::lock_guard<std::mutex> lock(_mutex);
            _pcapStatus = (errStr.find("permission") != std::string::npos ||
                           errStr.find("Operation not permitted") != std::string::npos ||
                           errStr.find("socket") != std::string::npos)
                          ? "permission_denied" : "open_failed";
            _pcapError = errStr;
            _pcapRemediationHint = hint;
        }

        std::cerr << "\n"
                  << "****************************************************************\n"
                  << " [WARNING] LIVE PACKET CAPTURE (PCAP) IS INACTIVE\n"
                  << " Interface:   " << _interface << "\n"
                  << " Error:       " << errStr << "\n"
                  << " Status:      Degraded to /proc/net/dev and ARP cache scraping\n"
                  << " Impact:      Protocol distribution (DNS, HTTPS, SSH) is zeroed\n"
                  << " Remediation: " << hint << "\n"
                  << "****************************************************************\n"
                  << std::endl;

        _pcapActive.store(false);
    } else {
        {
            std::lock_guard<std::mutex> lock(_mutex);
            _pcapStatus = "active";
            _pcapError = "";
            _pcapRemediationHint = "";
        }
        _pcapActive.store(true);
        std::cout << "LanSniffer: Live streaming capture active on " << _interface
                  << " (snaplen=96, Zero-DB mode)" << std::endl;
    }

    // Start scanner thread (ARP table and /proc/net/dev)
    _scannerThread = std::thread(&LanSniffer::scannerWorker, this);

    // Start pcap capture thread
    _pcapThread = std::thread(&LanSniffer::pcapWorker, this);

    return true;
}

void LanSniffer::stop() {
    if (!_running.load()) {
        return;
    }

    _running.store(false);

    if (_pcapHandle) {
        pcap_breakloop(_pcapHandle);
    }

    if (_pcapThread.joinable()) {
        _pcapThread.join();
    }

    if (_scannerThread.joinable()) {
        _scannerThread.join();
    }

    if (_pcapHandle) {
        pcap_close(_pcapHandle);
        _pcapHandle = nullptr;
    }

    _pcapActive.store(false);
}

void LanSniffer::pcapWorker() {
    if (!_pcapHandle) {
        // PCAP failed to open; sleep gently while running to avoid busy-spinning
        while (_running.load()) {
            std::this_thread::sleep_for(std::chrono::milliseconds(500));
        }
        return;
    }

    while (_running.load()) {
        int ret = pcap_dispatch(_pcapHandle, 100, pcapCallback, reinterpret_cast<u_char *>(this));
        if (ret < 0 && ret != -2) {
            // Error occurred
            if (_running.load()) {
                std::cerr << "LanSniffer: pcap_dispatch error: " << pcap_geterr(_pcapHandle) << std::endl;
            }
            break;
        } else if (ret == 0) {
            // Timeout elapsed, yield CPU
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }
    }

    _pcapActive.store(false);
}

void LanSniffer::scannerWorker() {
    // Initial immediate scan
    scanArpTable();
    scanProcDev();

    while (_running.load()) {
        for (int i = 0; i < 50 && _running.load(); ++i) {
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }

        if (!_running.load()) {
            break;
        }

        scanArpTable();
        scanProcDev();
        advanceMinuteRing();
    }
}

void LanSniffer::advanceMinuteRing() {
    time_t now = time(nullptr);
    std::lock_guard<std::mutex> lock(_mutex);

    if (now - _lastRingAdvance >= 60) {
        size_t minutesElapsed = static_cast<size_t>((now - _lastRingAdvance) / 60);
        for (size_t m = 0; m < minutesElapsed && m < 60; ++m) {
            _currentRingIdx = (_currentRingIdx + 1) % 60;
            _minuteRing[_currentRingIdx].timestamp = now;
            _minuteRing[_currentRingIdx].bytes = 0;
            _minuteRing[_currentRingIdx].packets = 0;
            _minuteRing[_currentRingIdx].hostBytes.clear();
        }
        _lastRingAdvance = now;
    }
}

void LanSniffer::scanArpTable() {
    std::ifstream file("/proc/net/arp");
    if (!file.is_open()) {
        return;
    }

    std::string line;
    // Skip header line: IP address HW type Flags HW address Mask Device
    std::getline(file, line);

    std::vector<std::pair<std::string, std::string>> arpEntries;

    while (std::getline(file, line)) {
        std::istringstream iss(line);
        std::string ip, hwType, flags, mac, mask, dev;
        if (iss >> ip >> hwType >> flags >> mac >> mask >> dev) {
            // Ignore incomplete / failed ARP resolutions (ATF_COM = 0x2)
            unsigned long flagVal = std::strtoul(flags.c_str(), nullptr, 0);
            if ((flagVal & 0x02) == 0) {
                continue;
            }

            // Exclude virtual/container interfaces (Docker, libvirt, veth pairs)
            if (dev == "docker0" || dev.rfind("docker", 0) == 0 ||
                dev.rfind("veth", 0) == 0 || dev.rfind("virbr", 0) == 0 ||
                dev.rfind("br-", 0) == 0) {
                continue;
            }

            // Exclude Docker default bridge subnets (172.17.x.x .. 172.31.x.x)
            if (ip.rfind("172.17.", 0) == 0 || ip.rfind("172.18.", 0) == 0 ||
                ip.rfind("172.19.", 0) == 0 || ip.rfind("172.20.", 0) == 0) {
                continue;
            }

            if (mac != "00:00:00:00:00:00" && !mac.empty()) {
                arpEntries.push_back({mac, ip});
            }
        }
    }
    file.close();

    time_t now = time(nullptr);
    for (const auto &entry : arpEntries) {
        const std::string &mac = entry.first;
        const std::string &ip = entry.second;

        DeviceRegistry::getInstance().upsertDevice(mac, ip);

        std::lock_guard<std::mutex> lock(_mutex);
        std::string normMac = OuiDatabase::normalizeMac(mac);
        auto &m = _hostMetrics[normMac];
        m.mac = normMac;
        m.ip = ip;
        m.lastSeen = now;
        if (m.firstSeen == 0) {
            m.firstSeen = now;
        }
        if (m.vendor.empty() || m.vendor == "Unknown Vendor") {
            m.vendor = OuiDatabase::getInstance().lookup(normMac);
        }
    }
}

void LanSniffer::scanProcDev() {
    std::ifstream file("/proc/net/dev");
    if (!file.is_open()) {
        return;
    }

    std::string line;
    // Skip first two header lines
    std::getline(file, line);
    std::getline(file, line);

    uint64_t rxBytes = 0, rxPackets = 0;
    uint64_t txBytes = 0, txPackets = 0;
    bool found = false;

    while (std::getline(file, line)) {
        size_t colonPos = line.find(':');
        if (colonPos == std::string::npos) {
            continue;
        }

        std::string ifName = line.substr(0, colonPos);
        // Trim whitespace
        ifName.erase(0, ifName.find_first_not_of(" \t"));
        ifName.erase(ifName.find_last_not_of(" \t") + 1);

        if (ifName == _interface) {
            std::istringstream iss(line.substr(colonPos + 1));
            uint64_t rxErrs, rxDrop, rxFifo, rxFrame, rxComp, rxMcast;
            uint64_t txErrs, txDrop, txFifo, txColls, txCarrier, txComp;

            if (iss >> rxBytes >> rxPackets >> rxErrs >> rxDrop >> rxFifo >> rxFrame >> rxComp >> rxMcast
                    >> txBytes >> txPackets >> txErrs >> txDrop >> txFifo >> txColls >> txCarrier >> txComp) {
                found = true;
            }
            break;
        }
    }
    file.close();

    if (!found) {
        return;
    }

    time_t now = time(nullptr);
    std::lock_guard<std::mutex> lock(_mutex);

    if (_devLastTime > 0 && now > _devLastTime) {
        double deltaSec = static_cast<double>(now - _devLastTime);
        if (rxBytes >= _devRxBytes && txBytes >= _devTxBytes) {
            uint64_t deltaBytes = (rxBytes - _devRxBytes) + (txBytes - _devTxBytes);
            uint64_t deltaPackets = (rxPackets - _devRxPackets) + (txPackets - _devTxPackets);
            _devCurrentBps = (deltaBytes * 8.0) / deltaSec;
            _devCurrentPps = deltaPackets / deltaSec;
        }
    }

    _devRxBytes = rxBytes;
    _devTxBytes = txBytes;
    _devRxPackets = rxPackets;
    _devTxPackets = txPackets;
    _devLastTime = now;
}

static std::string formatMac(const uint8_t *mac) {
    char buf[18];
    snprintf(buf, sizeof(buf), "%02x:%02x:%02x:%02x:%02x:%02x",
             mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
    return std::string(buf);
}

void LanSniffer::processPacket(const uint8_t *packet, size_t caplen, size_t origlen) {
    if (caplen < sizeof(struct ether_header)) {
        return;
    }

    time_t now = time(nullptr);
    const struct ether_header *eth = reinterpret_cast<const struct ether_header *>(packet);
    uint16_t ethType = ntohs(eth->ether_type);

    std::string srcMac = formatMac(eth->ether_shost);
    std::string dstMac = formatMac(eth->ether_dhost);

    std::lock_guard<std::mutex> lock(_mutex);

    _totalBytes += origlen;
    _totalPackets++;

    _minuteRing[_currentRingIdx].bytes += origlen;
    _minuteRing[_currentRingIdx].packets++;
    _minuteRing[_currentRingIdx].hostBytes[srcMac] += origlen;

    // Check broadcast and multicast
    if (eth->ether_dhost[0] == 0xff && eth->ether_dhost[1] == 0xff &&
        eth->ether_dhost[2] == 0xff && eth->ether_dhost[3] == 0xff &&
        eth->ether_dhost[4] == 0xff && eth->ether_dhost[5] == 0xff) {
        _bcastPackets++;
    } else if (eth->ether_dhost[0] & 0x01) {
        _mcastPackets++;
    }

    // Process IPv4
    if (ethType == ETHERTYPE_IP && caplen >= sizeof(struct ether_header) + sizeof(struct ip)) {
        const struct ip *iph = reinterpret_cast<const struct ip *>(packet + sizeof(struct ether_header));
        size_t ipHdrLen = iph->ip_hl * 4;

        char srcIpStr[INET_ADDRSTRLEN];
        char dstIpStr[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, &(iph->ip_src), srcIpStr, INET_ADDRSTRLEN);
        inet_ntop(AF_INET, &(iph->ip_dst), dstIpStr, INET_ADDRSTRLEN);

        auto &srcMetrics = _hostMetrics[srcMac];
        srcMetrics.mac = srcMac;
        srcMetrics.ip = srcIpStr;
        srcMetrics.lastSeen = now;
        if (srcMetrics.firstSeen == 0) {
            srcMetrics.firstSeen = now;
        }
        srcMetrics.bytesTx += origlen;
        srcMetrics.packetsTx++;
        if (srcMetrics.vendor.empty()) {
            srcMetrics.vendor = OuiDatabase::getInstance().lookup(srcMac);
        }
        srcMetrics.estimatedOs = (iph->ip_ttl <= 64) ? 64 : 128;

        auto &dstMetrics = _hostMetrics[dstMac];
        dstMetrics.mac = dstMac;
        dstMetrics.ip = dstIpStr;
        dstMetrics.lastSeen = now;
        if (dstMetrics.firstSeen == 0) {
            dstMetrics.firstSeen = now;
        }
        dstMetrics.bytesRx += origlen;
        dstMetrics.packetsRx++;
        if (dstMetrics.vendor.empty()) {
            dstMetrics.vendor = OuiDatabase::getInstance().lookup(dstMac);
        }

        // Track peer fan-out
        _hostPeers[srcMac].insert(dstIpStr);
        srcMetrics.uniquePeersCount = static_cast<uint32_t>(_hostPeers[srcMac].size());

        // Process TCP
        if (iph->ip_p == IPPROTO_TCP && caplen >= sizeof(struct ether_header) + ipHdrLen + sizeof(struct tcphdr)) {
            const struct tcphdr *tcph = reinterpret_cast<const struct tcphdr *>(
                packet + sizeof(struct ether_header) + ipHdrLen);

            uint16_t srcPort = ntohs(tcph->th_sport);
            uint16_t dstPort = ntohs(tcph->th_dport);

            _hostDstPorts[srcMac].insert(dstPort);
            srcMetrics.uniqueDstPorts = static_cast<uint32_t>(_hostDstPorts[srcMac].size());

            if (dstPort == 443 || srcPort == 443) {
                _httpsPackets++;
            } else if (dstPort == 22 || srcPort == 22) {
                _sshPackets++;
            } else if (dstPort == 80 || srcPort == 80) {
                _httpPackets++;
            } else {
                _otherPackets++;
            }

            // Connection lifecycle tracking
            if ((tcph->th_flags & TH_SYN) && !(tcph->th_flags & TH_ACK)) {
                srcMetrics.tcpSynSent++;
            } else if ((tcph->th_flags & TH_SYN) && (tcph->th_flags & TH_ACK)) {
                srcMetrics.tcpSynAckRecv++;
            } else if (tcph->th_flags & TH_RST) {
                srcMetrics.tcpRstRecv++;
            }
        } else if (iph->ip_p == IPPROTO_UDP && caplen >= sizeof(struct ether_header) + ipHdrLen + sizeof(struct udphdr)) {
            const struct udphdr *udph = reinterpret_cast<const struct udphdr *>(
                packet + sizeof(struct ether_header) + ipHdrLen);

            uint16_t srcPort = ntohs(udph->uh_sport);
            uint16_t dstPort = ntohs(udph->uh_dport);

            if (dstPort == 53 || srcPort == 53) {
                _dnsPackets++;
                srcMetrics.dnsQueryCount++;
            } else {
                _otherPackets++;
            }
        } else if (iph->ip_p == IPPROTO_ICMP) {
            _icmpPackets++;
        } else {
            _otherPackets++;
        }
    } else if (ethType == ETHERTYPE_ARP && caplen >= sizeof(struct ether_header) + sizeof(struct ether_arp)) {
        _arpPackets++;
        const struct ether_arp *arph = reinterpret_cast<const struct ether_arp *>(
            packet + sizeof(struct ether_header));

        char senderIpStr[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, arph->arp_spa, senderIpStr, INET_ADDRSTRLEN);
        std::string senderMac = formatMac(arph->arp_sha);

        auto &m = _hostMetrics[senderMac];
        m.mac = senderMac;
        m.ip = senderIpStr;
        m.arpRequestsSent++;
        m.lastSeen = now;
        if (m.firstSeen == 0) {
            m.firstSeen = now;
        }

        DeviceRegistry::getInstance().upsertDevice(senderMac, senderIpStr);
    }
}

nlohmann::json LanSniffer::getTopTalkers(size_t limit, int windowMinutes) const {
    std::lock_guard<std::mutex> lock(_mutex);

    struct HostAgg {
        std::string mac;
        uint64_t bytes = 0;
    };

    std::unordered_map<std::string, uint64_t> aggMap;

    if (windowMinutes <= 0 || windowMinutes > 60) {
        // Full in-memory cumulative
        for (const auto &pair : _hostMetrics) {
            aggMap[pair.first] = pair.second.bytesTx + pair.second.bytesRx;
        }
    } else {
        // Rolling window from ring buffer
        for (int i = 0; i < windowMinutes; ++i) {
            size_t idx = (_currentRingIdx + 60 - i) % 60;
            for (const auto &pair : _minuteRing[idx].hostBytes) {
                aggMap[pair.first] += pair.second;
            }
        }
    }

    std::vector<HostAgg> hosts;
    hosts.reserve(aggMap.size());
    for (const auto &pair : aggMap) {
        hosts.push_back({pair.first, pair.second});
    }

    std::sort(hosts.begin(), hosts.end(), [](const HostAgg &a, const HostAgg &b) {
        return a.bytes > b.bytes;
    });

    nlohmann::json talkersJson = nlohmann::json::array();
    size_t count = std::min(limit, hosts.size());

    for (size_t i = 0; i < count; ++i) {
        const std::string &mac = hosts[i].mac;
        DeviceInfo dev;
        bool hasDev = DeviceRegistry::getInstance().getDevice(mac, dev);

        nlohmann::json item;
        item["mac"] = mac;
        item["ip"] = hasDev ? dev.ip : "";
        item["name"] = hasDev ? dev.name : "";
        item["vendor"] = hasDev ? dev.vendor : OuiDatabase::getInstance().lookup(mac);
        item["category"] = hasDev ? dev.category : "unregistered";
        item["window_bytes"] = hosts[i].bytes;

        auto it = _hostMetrics.find(mac);
        if (it != _hostMetrics.end()) {
            item["bytes_tx"] = it->second.bytesTx;
            item["bytes_rx"] = it->second.bytesRx;
            item["packets_tx"] = it->second.packetsTx;
            item["packets_rx"] = it->second.packetsRx;
            item["active_flows"] = it->second.activeFlows;
            item["last_seen"] = it->second.lastSeen;
        }

        talkersJson.push_back(item);
    }

    nlohmann::json result;
    result["window_minutes"] = windowMinutes;
    result["top_talkers"] = talkersJson;
    result["total_hosts_observed"] = _hostMetrics.size();
    return result;
}

nlohmann::json LanSniffer::getTrafficSummary() const {
    std::lock_guard<std::mutex> lock(_mutex);

    time_t now = time(nullptr);
    time_t uptime = now - _startTime;

    nlohmann::json result;
    result["interface"] = _interface;
    result["pcap_active"] = _pcapActive.load();
    result["pcap_status"] = _pcapActive.load() ? "active" : (_pcapStatus.empty() ? "inactive" : _pcapStatus);
    if (!_pcapActive.load() && !_pcapError.empty()) {
        result["pcap_warning"] = _pcapError;
        result["pcap_remediation"] = _pcapRemediationHint;
    }
    result["uptime_seconds"] = uptime;

    if (_pcapActive.load()) {
        result["total_bytes"] = _totalBytes;
        result["total_packets"] = _totalPackets;

        // Compute current rate from the most recent 1-minute bucket
        uint64_t curMinBytes = _minuteRing[_currentRingIdx].bytes;
        uint64_t curMinPackets = _minuteRing[_currentRingIdx].packets;
        double elapsedInMinute = static_cast<double>(now - _lastRingAdvance);
        if (elapsedInMinute < 1.0) elapsedInMinute = 1.0;

        result["current_bytes_per_sec"] = curMinBytes / elapsedInMinute;
        result["current_packets_per_sec"] = curMinPackets / elapsedInMinute;
    } else {
        // Fallback to /proc/net/dev counters
        result["total_bytes"] = _devRxBytes + _devTxBytes;
        result["total_packets"] = _devRxPackets + _devTxPackets;
        result["current_bytes_per_sec"] = _devCurrentBps / 8.0;
        result["current_packets_per_sec"] = _devCurrentPps;
        result["interface_rx_bytes"] = _devRxBytes;
        result["interface_tx_bytes"] = _devTxBytes;
    }

    nlohmann::json proto;
    proto["dns"] = _dnsPackets;
    proto["https"] = _httpsPackets;
    proto["ssh"] = _sshPackets;
    proto["http"] = _httpPackets;
    proto["arp"] = _arpPackets;
    proto["broadcast"] = _bcastPackets;
    proto["multicast"] = _mcastPackets;
    proto["icmp"] = _icmpPackets;
    proto["other"] = _otherPackets;
    result["protocols"] = proto;

    result["unique_hosts_count"] = _hostMetrics.size();
    result["registered_devices_count"] = DeviceRegistry::getInstance().getDeviceCount();

    return result;
}

nlohmann::json LanSniffer::getDevicesJson() const {
    auto devices = DeviceRegistry::getInstance().getAllDevices();

    size_t infraCount = 0;
    size_t knownCount = 0;
    size_t visitorCount = 0;
    size_t unregCount = 0;

    nlohmann::json devList = nlohmann::json::array();
    for (const auto &dev : devices) {
        if (dev.category == "infrastructure") infraCount++;
        else if (dev.category == "known") knownCount++;
        else if (dev.category == "visitor") visitorCount++;
        else unregCount++;

        nlohmann::json d;
        d["mac"] = dev.mac;
        d["ip"] = dev.ip;
        d["name"] = dev.name;
        d["vendor"] = dev.vendor;
        d["category"] = dev.category;
        d["first_seen"] = dev.firstSeen;
        d["last_seen"] = dev.lastSeen;
        devList.push_back(d);
    }

    nlohmann::json result;
    result["total_devices"] = devices.size();
    result["infrastructure_count"] = infraCount;
    result["known_count"] = knownCount;
    result["visitor_count"] = visitorCount;
    result["unregistered_count"] = unregCount;
    result["devices"] = devList;

    return result;
}

nlohmann::json LanSniffer::getUnregisteredDevicesJson() const {
    auto unreg = DeviceRegistry::getInstance().getUnregisteredDevices();
    auto visitors = DeviceRegistry::getInstance().getVisitorDevices();

    nlohmann::json unregList = nlohmann::json::array();
    for (const auto &dev : unreg) {
        nlohmann::json d;
        d["mac"] = dev.mac;
        d["ip"] = dev.ip;
        d["name"] = dev.name;
        d["vendor"] = dev.vendor;
        d["category"] = "unregistered";
        d["first_seen"] = dev.firstSeen;
        d["last_seen"] = dev.lastSeen;
        unregList.push_back(d);
    }

    nlohmann::json visitorList = nlohmann::json::array();
    for (const auto &dev : visitors) {
        nlohmann::json d;
        d["mac"] = dev.mac;
        d["ip"] = dev.ip;
        d["name"] = dev.name;
        d["vendor"] = dev.vendor;
        d["category"] = "visitor";
        d["first_seen"] = dev.firstSeen;
        d["last_seen"] = dev.lastSeen;
        visitorList.push_back(d);
    }

    nlohmann::json result;
    result["unregistered_devices"] = unregList;
    result["unregistered_count"] = unregList.size();
    result["visitor_devices"] = visitorList;
    result["visitor_count"] = visitorList.size();

    return result;
}

nlohmann::json LanSniffer::getHostMetricsJson(const std::string &mac) const {
    std::string normMac = OuiDatabase::normalizeMac(mac);
    std::lock_guard<std::mutex> lock(_mutex);

    auto it = _hostMetrics.find(normMac);
    if (it == _hostMetrics.end()) {
        nlohmann::json err;
        err["error"] = "Host MAC not observed in active telemetry";
        return err;
    }

    const auto &m = it->second;
    nlohmann::json res;
    res["mac"] = m.mac;
    res["ip"] = m.ip;
    res["vendor"] = m.vendor;
    res["estimated_os_ttl"] = m.estimatedOs;

    nlohmann::json ipfix;
    ipfix["tcp_syn_sent"] = m.tcpSynSent;
    ipfix["tcp_syn_ack_recv"] = m.tcpSynAckRecv;
    ipfix["tcp_rst_recv"] = m.tcpRstRecv;
    ipfix["tcp_half_open"] = m.tcpHalfOpen;
    ipfix["active_flows"] = m.activeFlows;
    res["connection_state"] = ipfix;

    nlohmann::json rmon;
    rmon["bytes_tx"] = m.bytesTx;
    rmon["bytes_rx"] = m.bytesRx;
    rmon["packets_tx"] = m.packetsTx;
    rmon["packets_rx"] = m.packetsRx;
    res["volume_profiles"] = rmon;

    nlohmann::json diversity;
    diversity["unique_peers_count"] = m.uniquePeersCount;
    diversity["unique_dst_ports"] = m.uniqueDstPorts;
    res["transport_diversity"] = diversity;

    nlohmann::json app;
    app["dns_query_count"] = m.dnsQueryCount;
    app["arp_requests_sent"] = m.arpRequestsSent;
    app["bcast_packets_tx"] = m.bcastPacketsTx;
    res["application_behavior"] = app;

    res["first_seen"] = m.firstSeen;
    res["last_seen"] = m.lastSeen;

    return res;
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
