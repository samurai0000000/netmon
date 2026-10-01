/*
 * FuzzZyxelParsers.cxx
 *
 * Copyright (C) 2026, Charles Chiou
 */

#include <cstdint>
#include <cstddef>
#include <string>
#include <vector>
#include <exception>

#include "zyxel/ZyxelTypes.hxx"
#include "zyxel/ZyxelScanner.hxx"
#include "zyxel/ZyxelSystemCmd.hxx"
#include "zyxel/ZyxelNetworkCmd.hxx"
#include "zyxel/ZyxelObjectCmd.hxx"
#include "zyxel/ZyxelFirewallCmd.hxx"
#include "zyxel/ZyxelNatCmd.hxx"

static inline void safeDumpJson(const nlohmann::json &j) {
    try {
        volatile auto s = j.dump(-1, ' ', false, nlohmann::json::error_handler_t::replace);
        (void)s;
    } catch (...) {}
}

extern "C" int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size) {
    static bool init = []() {
        ZyxelScanner::setQuietLogging(true);
        return true;
    }();
    (void)init;

    if (size == 0 || size > 65536) {
        return 0; // Cap input size to 64KB for fuzzer efficiency
    }

    std::string input(reinterpret_cast<const char*>(data), size);

    try {
        // 1. ZyxelSystemCmd
        ZyxelVersionInfo version;
        if (ZyxelSystemCmd::parseVersion(input, version)) {
            safeDumpJson(version.toJson());
        }

        double cpuPercent = 0.0;
        ZyxelSystemCmd::parseCpuStatus(input, cpuPercent);

        double memPercent = 0.0;
        ZyxelSystemCmd::parseMemStatus(input, memPercent);

        ZyxelSessionSummary sessionSummary;
        if (ZyxelSystemCmd::parseConnStatus(input, sessionSummary)) {
            safeDumpJson(sessionSummary.toJson());
        }

        ZyxelDiagnosticResult pingResult;
        if (ZyxelSystemCmd::parsePing(input, pingResult)) {
            safeDumpJson(pingResult.toJson());
        }

        ZyxelDiagnosticResult traceResult;
        if (ZyxelSystemCmd::parseTraceroute(input, traceResult)) {
            safeDumpJson(traceResult.toJson());
        }

        // 2. ZyxelNetworkCmd
        std::vector<ZyxelInterfaceInfo> ifaces;
        if (ZyxelNetworkCmd::parseInterfaces(input, ifaces)) {
            for (const auto &item : ifaces) {
                safeDumpJson(item.toJson());
            }
        }

        std::vector<ZyxelRouteEntry> routes;
        if (ZyxelNetworkCmd::parseIpRoutes(input, routes)) {
            for (const auto &item : routes) {
                safeDumpJson(item.toJson());
            }
        }

        std::vector<ZyxelZoneInfo> zones;
        if (ZyxelNetworkCmd::parseZones(input, zones)) {
            for (const auto &item : zones) {
                safeDumpJson(item.toJson());
            }
        }

        std::vector<ZyxelArpEntry> arp;
        if (ZyxelNetworkCmd::parseArp(input, arp)) {
            for (const auto &item : arp) {
                safeDumpJson(item.toJson());
            }
        }

        // 3. ZyxelObjectCmd
        std::vector<ZyxelAddressObject> addrs;
        if (ZyxelObjectCmd::parseAddressObjects(input, addrs)) {
            for (const auto &item : addrs) {
                safeDumpJson(item.toJson());
            }
        }

        std::vector<ZyxelAddressGroup> addrGroups;
        if (ZyxelObjectCmd::parseAddressGroups(input, addrGroups)) {
            for (const auto &item : addrGroups) {
                safeDumpJson(item.toJson());
            }
        }

        std::vector<ZyxelServiceObject> svcs;
        if (ZyxelObjectCmd::parseServiceObjects(input, svcs)) {
            for (const auto &item : svcs) {
                safeDumpJson(item.toJson());
            }
        }

        // 4. ZyxelFirewallCmd
        std::vector<ZyxelFirewallRule> rules;
        if (ZyxelFirewallCmd::parseSecurePolicy(input, rules)) {
            for (const auto &item : rules) {
                safeDumpJson(item.toJson());
            }
        }

        // 5. ZyxelNatCmd
        std::vector<ZyxelVirtualServerRule> vsRules;
        if (ZyxelNatCmd::parseVirtualServers(input, vsRules)) {
            for (const auto &item : vsRules) {
                safeDumpJson(item.toJson());
            }
        }
    } catch (...) {}

    return 0;
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
