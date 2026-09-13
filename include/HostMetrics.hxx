/*
 * HostMetrics.hxx
 *
 * Copyright (C) 2026, Charles Chiou
 */

#ifndef NETMON_HOSTMETRICS_HXX
#define NETMON_HOSTMETRICS_HXX

#include <string>
#include <cstdint>
#include <ctime>

struct HostMetrics {
    // Identity & Profiling
    std::string mac;                 // Hardware MAC address (e.g. "02:00:00:00:00:01")
    std::string ip;                  // IPv4 address string
    std::string hostname;            // DHCP Option 12 or mDNS announced name
    std::string vendor;              // IEEE OUI vendor string (e.g. "Vendor Name")
    uint8_t     estimatedOs = 0;     // IP TTL signature (64=Linux/iOS/Android, 128=Windows)

    // 1. Connection Lifecycle & State (IPFIX / Zeek)
    uint32_t    tcpSynSent = 0;      // Outbound connection attempts
    uint32_t    tcpSynAckRecv = 0;   // Successfully established handshakes
    uint32_t    tcpRstRecv = 0;      // Connections rejected / refused (RST received)
    uint32_t    tcpHalfOpen = 0;     // SYN sent with no response (unreachable / scan)
    uint32_t    activeFlows = 0;     // Concurrent active transport sessions

    // 2. Traffic Symmetry & Volume Profiles (RMON-2 / IPFIX)
    uint64_t    bytesTx = 0;         // Total outbound payload bytes
    uint64_t    bytesRx = 0;         // Total inbound payload bytes
    uint64_t    packetsTx = 0;       // Total outbound packets
    uint64_t    packetsRx = 0;       // Total inbound packets
    double      txRxRatio = 1.0;     // Upload/download asymmetry (producer vs consumer)

    // 3. Transport & Diversity / Fan-Out Metrics (Scanning & Botnet Detection)
    uint32_t    uniquePeersCount = 0;   // Distinct remote IPs contacted in window
    uint32_t    uniqueDstPorts = 0;     // Distinct destination ports targeted
    double      fanOutRatio = 0.0;      // Peers per byte (high ratio = scanner / worm)

    // 4. Application Layer Behavior (DNS, DHCP, ARP)
    uint32_t    dnsQueryCount = 0;   // Total DNS queries made
    uint32_t    dnsNxdomainCount = 0;// Failed domain lookups (beaconing indicator)
    uint32_t    arpRequestsSent = 0; // Local L2 discovery rate
    uint32_t    bcastPacketsTx = 0;  // Broadcast/multicast chatter emitted

    time_t      firstSeen = 0;
    time_t      lastSeen = 0;
};

#endif /* NETMON_HOSTMETRICS_HXX */

/*
 * Local variables:
 * mode: C++
 * c-file-style: "BSD"
 * c-basic-offset: 4
 * tab-width: 4
 * indent-tabs-mode: nil
 * End:
 */
