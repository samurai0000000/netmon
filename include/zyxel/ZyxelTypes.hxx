/*
 * ZyxelTypes.hxx
 *
 * Copyright (C) 2026, Charles Chiou
 */

#ifndef NETMON_ZYXEL_TYPES_HXX
#define NETMON_ZYXEL_TYPES_HXX

#include <string>
#include <vector>
#include <nlohmann/json.hpp>

struct ZyxelVersionInfo {
    std::string model;
    std::string firmwareVersion;
    std::string buildDate;
    std::string serialNumber;

    nlohmann::json toJson() const {
        return {
            {"model", model},
            {"firmware_version", firmwareVersion},
            {"build_date", buildDate},
            {"serial_number", serialNumber}
        };
    }
};

struct ZyxelSystemLoad {
    double      cpuUsagePercent = 0.0;
    double      memUsagePercent = 0.0;
    std::string uptime;

    nlohmann::json toJson() const {
        return {
            {"cpu_usage_percent", cpuUsagePercent},
            {"mem_usage_percent", memUsagePercent},
            {"uptime", uptime}
        };
    }
};

struct ZyxelSessionSummary {
    int    activeSessions = 0;
    int    maxSessions = 0;
    double sessionUsagePercent = 0.0;

    nlohmann::json toJson() const {
        return {
            {"active_sessions", activeSessions},
            {"max_sessions", maxSessions},
            {"session_usage_percent", sessionUsagePercent}
        };
    }
};

struct ZyxelInterfaceInfo {
    std::string name;
    bool        linkStatus = false;
    std::string ip;
    std::string netmask;
    std::string mac;
    int         mtu = 1500;

    nlohmann::json toJson() const {
        return {
            {"name", name},
            {"link_status", linkStatus ? "up" : "down"},
            {"ip", ip},
            {"netmask", netmask},
            {"mac", mac},
            {"mtu", mtu}
        };
    }
};

struct ZyxelRouteEntry {
    std::string destination;
    std::string netmask;
    std::string gateway;
    std::string interface;
    int         metric = 0;

    nlohmann::json toJson() const {
        return {
            {"destination", destination},
            {"netmask", netmask},
            {"gateway", gateway},
            {"interface", interface},
            {"metric", metric}
        };
    }
};

struct ZyxelZoneInfo {
    std::string              name;
    std::vector<std::string> interfaces;

    nlohmann::json toJson() const {
        return {
            {"name", name},
            {"interfaces", interfaces}
        };
    }
};

struct ZyxelArpEntry {
    std::string ip;
    std::string mac;
    std::string interface;

    nlohmann::json toJson() const {
        return {
            {"ip", ip},
            {"mac", mac},
            {"interface", interface}
        };
    }
};

struct ZyxelAddressObject {
    std::string name;
    std::string type;               // "HOST", "RANGE", "SUBNET"
    std::string ip;
    std::string secondaryIpOrMask;  // For range end or subnet mask
    int         refCount = 0;

    nlohmann::json toJson() const {
        return {
            {"name", name},
            {"type", type},
            {"ip", ip},
            {"secondary_ip_or_mask", secondaryIpOrMask},
            {"ref_count", refCount}
        };
    }
};

struct ZyxelAddressGroup {
    std::string              name;
    std::vector<std::string> members;

    nlohmann::json toJson() const {
        return {
            {"name", name},
            {"members", members}
        };
    }
};

struct ZyxelServiceObject {
    std::string name;
    std::string protocol;           // "tcp", "udp", "icmp"
    int         portStart = 0;
    int         portEnd = 0;
    int         refCount = 0;

    nlohmann::json toJson() const {
        return {
            {"name", name},
            {"protocol", protocol},
            {"port_start", portStart},
            {"port_end", portEnd},
            {"ref_count", refCount}
        };
    }
};

struct ZyxelServiceGroup {
    std::string              name;
    std::vector<std::string> members;

    nlohmann::json toJson() const {
        return {
            {"name", name},
            {"members", members}
        };
    }
};

struct ZyxelFirewallRule {
    int         index = 0;
    std::string name;
    std::string fromZone;
    std::string toZone;
    std::string sourceIp;
    std::string destinationIp;
    std::string service;
    std::string action;             // "allow", "deny", "reject"
    bool        active = true;
    std::string description;

    nlohmann::json toJson() const {
        return {
            {"index", index},
            {"name", name},
            {"from_zone", fromZone},
            {"to_zone", toZone},
            {"source_ip", sourceIp},
            {"destination_ip", destinationIp},
            {"service", service},
            {"action", action},
            {"active", active},
            {"description", description}
        };
    }
};

struct ZyxelVirtualServerRule {
    int         index = 0;
    std::string name;
    std::string interface;
    std::string originalIp;
    std::string mapToIp;
    std::string originalService;
    std::string mappedService;
    bool        active = true;

    nlohmann::json toJson() const {
        return {
            {"index", index},
            {"name", name},
            {"interface", interface},
            {"original_ip", originalIp},
            {"map_to_ip", mapToIp},
            {"original_service", originalService},
            {"mapped_service", mappedService},
            {"active", active}
        };
    }
};

struct ZyxelHopProbe {
    std::string ip;
    double      rttMs = 0.0;
    bool        timeout = false;
    std::string icmpFlag;           // e.g. "!H", "!N", "!P", "!X"

    nlohmann::json toJson() const {
        nlohmann::json j = {
            {"ip", ip},
            {"rtt_ms", rttMs},
            {"timeout", timeout}
        };
        if (!icmpFlag.empty()) {
            j["icmp_flag"] = icmpFlag;
        }
        return j;
    }
};

struct ZyxelHop {
    int                        hopIndex = 0;
    std::vector<ZyxelHopProbe> probes;
    std::string                primaryIp;
    bool                       isCompleteTimeout = false;

    nlohmann::json toJson() const {
        nlohmann::json probeArr = nlohmann::json::array();
        for (const auto &p : probes) {
            probeArr.push_back(p.toJson());
        }
        return {
            {"hop_index", hopIndex},
            {"primary_ip", primaryIp},
            {"complete_timeout", isCompleteTimeout},
            {"probes", probeArr}
        };
    }
};

struct ZyxelDiagnosticResult {
    std::string           type;     // "ping", "traceroute"
    int                   packetsTransmitted = 0;
    int                   packetsReceived = 0;
    double                packetLossPercent = 0.0;
    double                minLatencyMs = 0.0;
    double                avgLatencyMs = 0.0;
    double                maxLatencyMs = 0.0;
    std::vector<ZyxelHop> hops;
    std::string           rawOutput;

    nlohmann::json toJson() const {
        nlohmann::json j = {
            {"type", type},
            {"packets_transmitted", packetsTransmitted},
            {"packets_received", packetsReceived},
            {"packet_loss_percent", packetLossPercent},
            {"min_latency_ms", minLatencyMs},
            {"avg_latency_ms", avgLatencyMs},
            {"max_latency_ms", maxLatencyMs},
            {"raw_output", rawOutput}
        };
        if (type == "traceroute" && !hops.empty()) {
            nlohmann::json hopsArr = nlohmann::json::array();
            for (const auto &h : hops) {
                hopsArr.push_back(h.toJson());
            }
            j["hops"] = hopsArr;
        }
        return j;
    }
};

#endif /* NETMON_ZYXEL_TYPES_HXX */

/*
 * Local variables:
 * mode: C++
 * c-file-style: "BSD"
 * c-basic-offset: 4
 * tab-width: 4
 * indent-tabs-mode: nil
 * End:
 */
