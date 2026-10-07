/*
 * ZyxelNetworkCmd.cxx
 *
 * Copyright (C) 2026, Charles Chiou
 */

#include "zyxel/ZyxelNetworkCmd.hxx"
#include "zyxel/ZyxelScanner.hxx"

#include <algorithm>
#include <cctype>
#include <regex>

std::string ZyxelNetworkCmd::cmdShowInterfaces() {
    return "show interface summary all";
}

std::string ZyxelNetworkCmd::cmdShowInterface(const std::string &name) {
    return "show interface " + name;
}

std::string ZyxelNetworkCmd::cmdShowInterfaceBase() {
    return "show interface base";
}

std::string ZyxelNetworkCmd::cmdShowIpRoute() {
    return "show ip route-settings";
}

std::string ZyxelNetworkCmd::cmdAddRoute(const std::string &dest, const std::string &mask,
                                        const std::string &gw, int metric) {
    return "ip route " + dest + " " + mask + " " + gw + " " + std::to_string(metric);
}

std::string ZyxelNetworkCmd::cmdDeleteRoute(const std::string &dest, const std::string &mask,
                                           const std::string &gw) {
    return "no ip route " + dest + " " + mask + " " + gw;
}

std::string ZyxelNetworkCmd::cmdShowZones() {
    return "show zone";
}

std::string ZyxelNetworkCmd::cmdShowZone(const std::string &name) {
    return "show zone " + name;
}

std::string ZyxelNetworkCmd::cmdShowZoneDefaultBinding() {
    return "show zone default-binding";
}

std::string ZyxelNetworkCmd::cmdShowZoneBindingIface() {
    return "show zone binding-iface";
}

std::string ZyxelNetworkCmd::cmdShowArp() {
    return "show arp-table";
}

std::string ZyxelNetworkCmd::cmdShowArpGratuitous() {
    return "show arp gratuitous";
}

std::string ZyxelNetworkCmd::cmdShowIpDhcpBinding() {
    return "show ip dhcp binding";
}

std::string ZyxelNetworkCmd::cmdShowL2Isolation() {
    return "show l2-isolation";
}

std::string ZyxelNetworkCmd::cmdShowL2IsolationActivation() {
    return "show l2-isolation activation";
}

std::string ZyxelNetworkCmd::cmdShowL2IsolationWhitelist() {
    return "show l2-isolation white-list";
}

std::string ZyxelNetworkCmd::cmdInvalidInterfaceDryFire() {
    return "interface invalid_test_iface_999";
}

std::string ZyxelNetworkCmd::cmdTrunk(const std::string &name) {
    return "trunk " + name;
}

std::string ZyxelNetworkCmd::cmdNoTrunk(const std::string &name) {
    return "no trunk " + name;
}

std::string ZyxelNetworkCmd::cmdZone(const std::string &name) {
    return "zone " + name;
}

std::string ZyxelNetworkCmd::cmdNoZone(const std::string &name) {
    return "no zone " + name;
}

std::string ZyxelNetworkCmd::cmdIpMacBinding(bool activate) {
    return activate ? "ip-mac-binding activate" : "no ip-mac-binding activate";
}

std::string ZyxelNetworkCmd::cmdL2IsolationActivation(bool activate) {
    return activate ? "l2-isolation activation" : "no l2-isolation activation";
}

std::string ZyxelNetworkCmd::cmdIpException(const std::string &name) {
    return "ip-exception " + name;
}

std::string ZyxelNetworkCmd::cmdNoIpException(const std::string &name) {
    return "no ip-exception " + name;
}

std::string ZyxelNetworkCmd::cmdDeviceInsightActivation(bool activate) {
    return activate ? "device-insight activate" : "no device-insight activate";
}

bool ZyxelNetworkCmd::parseInterfaces(const std::string &raw,
                                     std::vector<ZyxelInterfaceInfo> &out) {
    out.clear();
    auto lines = ZyxelScanner::splitLines(raw);
    int divIdx = ZyxelScanner::findDividerLine(lines);

    if (divIdx > 0) {
        auto cols = ZyxelScanner::parseTableColumns(lines[divIdx - 1], lines[divIdx]);
        int nameCol = ZyxelScanner::findColumn(cols, "interface");
        int statCol = ZyxelScanner::findColumn(cols, "status");
        int ipCol = ZyxelScanner::findColumn(cols, "ip");
        int maskCol = ZyxelScanner::findColumn(cols, "netmask");
        if (maskCol < 0) maskCol = ZyxelScanner::findColumn(cols, "mask");
        int macCol = ZyxelScanner::findColumn(cols, "mac");
        int mtuCol = ZyxelScanner::findColumn(cols, "mtu");

        for (size_t i = divIdx + 1; i < lines.size(); ++i) {
            const std::string &line = lines[i];
            std::string trimmed = ZyxelScanner::trim(line);
            if (trimmed.empty() || trimmed.find("====") == 0 || trimmed.find("----") == 0) {
                continue;
            }

            if (!cols.empty() && nameCol >= 0 && statCol >= 0) {
                ZyxelInterfaceInfo info;
                info.name = ZyxelScanner::extractCell(line, cols[nameCol]);
                std::string statusLower = ZyxelScanner::extractCell(line, cols[statCol]);
                std::transform(statusLower.begin(), statusLower.end(), statusLower.begin(), ::tolower);
                info.linkStatus = (statusLower == "up" || statusLower == "yes" || statusLower == "active");
                if (ipCol >= 0) info.ip = ZyxelScanner::extractCell(line, cols[ipCol]);
                if (maskCol >= 0) info.netmask = ZyxelScanner::extractCell(line, cols[maskCol]);
                if (macCol >= 0) info.mac = ZyxelScanner::extractCell(line, cols[macCol]);
                if (mtuCol >= 0) {
                    try {
                        info.mtu = std::stoi(ZyxelScanner::extractCell(line, cols[mtuCol]));
                    } catch (...) {
                        info.mtu = 1500;
                    }
                }
                if (!info.name.empty()) {
                    out.push_back(info);
                }
            } else {
                auto tokens = ZyxelScanner::splitTokens(line);
                if (tokens.size() >= 2) {
                    ZyxelInterfaceInfo info;
                    info.name = tokens[0];
                    std::string statusLower = tokens[1];
                    std::transform(statusLower.begin(), statusLower.end(), statusLower.begin(), ::tolower);
                    info.linkStatus = (statusLower == "up" || statusLower == "yes" || statusLower == "active");
                    if (tokens.size() >= 3) info.ip = tokens[2];
                    if (tokens.size() >= 4) info.netmask = tokens[3];
                    if (tokens.size() >= 5) info.mac = tokens[4];
                    if (tokens.size() >= 6) {
                        try { info.mtu = std::stoi(tokens[5]); } catch (...) { info.mtu = 1500; }
                    }
                    out.push_back(info);
                }
            }
        }
    } else {
        // Try single interface block parse
        ZyxelInterfaceInfo info;
        for (const auto &line : lines) {
            std::string key, val;
            if (ZyxelScanner::parseKeyValue(line, key, val, ':')) {
                std::string k = key;
                std::transform(k.begin(), k.end(), k.begin(), ::tolower);
                if (k.find("interface") != std::string::npos) {
                    info.name = val;
                } else if (k == "status") {
                    std::string v = val;
                    std::transform(v.begin(), v.end(), v.begin(), ::tolower);
                    info.linkStatus = (v == "up" || v == "yes" || v == "active");
                } else if (k == "ip address" || k == "ip") {
                    info.ip = val;
                } else if (k == "subnet mask" || k == "netmask") {
                    info.netmask = val;
                } else if (k == "mac address" || k == "mac") {
                    info.mac = val;
                } else if (k == "mtu") {
                    try { info.mtu = std::stoi(val); } catch (...) {}
                }
            }
        }
        if (!info.name.empty()) {
            out.push_back(info);
        }
    }

    if (out.empty()) {
        ZyxelScanner::logParseError("ZyxelNetworkCmd", "show interface", 0,
                                   "No interface entries parsed",
                                   lines.empty() ? "" : lines[0]);
        return false;
    }
    return true;
}

bool ZyxelNetworkCmd::parseIpRoutes(const std::string &raw,
                                    std::vector<ZyxelRouteEntry> &out) {
    out.clear();
    auto lines = ZyxelScanner::splitLines(raw);
    int divIdx = ZyxelScanner::findDividerLine(lines);
    if (divIdx < 0) {
        ZyxelScanner::logParseError("ZyxelNetworkCmd", "show ip route", 0,
                                   "No route table divider found",
                                   lines.empty() ? "" : lines[0]);
        return false;
    }

    auto cols = ZyxelScanner::parseTableColumns(lines[divIdx - 1], lines[divIdx]);
    int dstCol = ZyxelScanner::findColumn(cols, "route");
    if (dstCol < 0) dstCol = ZyxelScanner::findColumn(cols, "destination");
    if (dstCol < 0) dstCol = ZyxelScanner::findColumn(cols, "ip address");
    int maskCol = ZyxelScanner::findColumn(cols, "netmask");
    if (maskCol < 0) maskCol = ZyxelScanner::findColumn(cols, "mask");
    int gwCol = ZyxelScanner::findColumn(cols, "nexthop");
    if (gwCol < 0) gwCol = ZyxelScanner::findColumn(cols, "gateway");
    int ifCol = ZyxelScanner::findColumn(cols, "interface");
    if (ifCol < 0) ifCol = ZyxelScanner::findColumn(cols, "iface");
    int metCol = ZyxelScanner::findColumn(cols, "metric");

    for (size_t i = divIdx + 1; i < lines.size(); ++i) {
        const std::string &line = lines[i];
        std::string trimmed = ZyxelScanner::trim(line);
        if (trimmed.empty() || trimmed.find("====") == 0 || trimmed.find("----") == 0) {
            continue;
        }

        if (!cols.empty() && dstCol >= 0 && gwCol >= 0) {
            ZyxelRouteEntry entry;
            entry.destination = ZyxelScanner::extractCell(line, cols[dstCol]);
            if (maskCol >= 0) entry.netmask = ZyxelScanner::extractCell(line, cols[maskCol]);
            entry.gateway = ZyxelScanner::extractCell(line, cols[gwCol]);
            if (ifCol >= 0) entry.interface = ZyxelScanner::extractCell(line, cols[ifCol]);
            if (metCol >= 0) {
                try {
                    entry.metric = std::stoi(ZyxelScanner::extractCell(line, cols[metCol]));
                } catch (...) {
                    entry.metric = 0;
                }
            }
            if (!entry.destination.empty()) {
                out.push_back(entry);
            }
        } else {
            auto tokens = ZyxelScanner::splitTokens(line);
            if (tokens.size() >= 3) {
                ZyxelRouteEntry entry;
                entry.destination = tokens[0];
                entry.netmask = tokens[1];
                entry.gateway = tokens[2];
                if (tokens.size() >= 4) {
                    try {
                        entry.metric = std::stoi(tokens[3]);
                    } catch (...) {
                        entry.metric = 0;
                    }
                }
                out.push_back(entry);
            }
        }
    }

    return true;
}

bool ZyxelNetworkCmd::parseZones(const std::string &raw,
                                std::vector<ZyxelZoneInfo> &out) {
    out.clear();
    auto lines = ZyxelScanner::splitLines(raw);
    int divIdx = ZyxelScanner::findDividerLine(lines);

    if (divIdx >= 0) {
        for (size_t i = divIdx + 1; i < lines.size(); ++i) {
            std::string line = ZyxelScanner::trim(lines[i]);
            if (line.empty() || line.find("====") == 0 || line.find("----") == 0) {
                continue;
            }
            auto tokens = ZyxelScanner::splitTokens(line);
            if (!tokens.empty()) {
                bool isNumeric = true;
                for (char c : tokens[0]) {
                    if (!std::isdigit(static_cast<unsigned char>(c))) {
                        isNumeric = false;
                        break;
                    }
                }
                if (isNumeric && tokens.size() == 1) {
                    continue;
                }

                ZyxelZoneInfo zone;
                size_t startIdx = 0;
                if (isNumeric && tokens.size() >= 2) {
                    zone.name = tokens[1];
                    startIdx = 2;
                } else {
                    zone.name = tokens[0];
                    startIdx = 1;
                }
                for (size_t j = startIdx; j < tokens.size(); ++j) {
                    std::string iface = tokens[j];
                    if (!iface.empty() && iface.back() == ',') {
                        iface.pop_back();
                    }
                    bool tokNumeric = true;
                    for (char c : iface) {
                        if (!std::isdigit(static_cast<unsigned char>(c))) {
                            tokNumeric = false;
                            break;
                        }
                    }
                    if (tokNumeric && j == tokens.size() - 1) {
                        continue;
                    }
                    if (!iface.empty()) {
                        zone.interfaces.push_back(iface);
                    }
                }
                out.push_back(zone);
            }
        }
    } else {
        // Block format: "zone: WAN", "interface: wan1"
        ZyxelZoneInfo currentZone;
        for (const auto &line : lines) {
            std::string key, val;
            if (ZyxelScanner::parseKeyValue(line, key, val, ':')) {
                std::string k = key;
                std::transform(k.begin(), k.end(), k.begin(), ::tolower);
                if (k == "zone") {
                    if (!currentZone.name.empty()) {
                        out.push_back(currentZone);
                        currentZone = ZyxelZoneInfo();
                    }
                    currentZone.name = val;
                } else if (k == "interface" || k == "member") {
                    currentZone.interfaces.push_back(val);
                }
            }
        }
        if (!currentZone.name.empty()) {
            out.push_back(currentZone);
        }
    }

    if (out.empty()) {
        ZyxelScanner::logParseError("ZyxelNetworkCmd", "show zone", 0,
                                   "No zone entries found",
                                   lines.empty() ? "" : lines[0]);
        return false;
    }
    return true;
}

bool ZyxelNetworkCmd::parseArp(const std::string &raw,
                              std::vector<ZyxelArpEntry> &out) {
    out.clear();
    auto lines = ZyxelScanner::splitLines(raw);
    int divIdx = ZyxelScanner::findDividerLine(lines);

    if (divIdx > 0) {
        auto cols = ZyxelScanner::parseTableColumns(lines[divIdx - 1], lines[divIdx]);
        int ipCol = ZyxelScanner::findColumn(cols, "ip");
        int macCol = ZyxelScanner::findColumn(cols, "mac");
        int ifCol = ZyxelScanner::findColumn(cols, "interface");

        for (size_t i = divIdx + 1; i < lines.size(); ++i) {
            const std::string &line = lines[i];
            std::string trimmed = ZyxelScanner::trim(line);
            if (trimmed.empty() || trimmed.find("====") == 0 || trimmed.find("----") == 0) {
                continue;
            }

            if (!cols.empty() && ipCol >= 0 && macCol >= 0) {
                ZyxelArpEntry entry;
                entry.ip = ZyxelScanner::extractCell(line, cols[ipCol]);
                entry.mac = ZyxelScanner::extractCell(line, cols[macCol]);
                if (ifCol >= 0) entry.interface = ZyxelScanner::extractCell(line, cols[ifCol]);
                if (!entry.ip.empty() && !entry.mac.empty()) {
                    out.push_back(entry);
                }
            } else {
                auto tokens = ZyxelScanner::splitTokens(line);
                if (tokens.size() >= 2) {
                    ZyxelArpEntry entry;
                    entry.ip = tokens[0];
                    entry.mac = tokens[1];
                    if (tokens.size() >= 3) {
                        entry.interface = tokens[2];
                    }
                    out.push_back(entry);
                }
            }
        }
    }

    if (out.empty()) {
        ZyxelScanner::logParseError("ZyxelNetworkCmd", "show arp-table", 0,
                                   "No ARP entries found in table",
                                   lines.empty() ? "" : lines[0]);
        return false;
    }
    return true;
}

bool ZyxelNetworkCmd::parseDhcpBindings(const std::string &raw,
                                       std::vector<ZyxelDhcpBindingEntry> &out) {
    out.clear();
    auto lines = ZyxelScanner::splitLines(raw);
    int divIdx = ZyxelScanner::findDividerLine(lines);
    if (divIdx < 0) {
        ZyxelScanner::logParseError("ZyxelNetworkCmd", "show ip dhcp binding", 0,
                                   "No table divider found",
                                   lines.empty() ? "" : lines[0]);
        return false;
    }

    for (size_t i = divIdx + 1; i < lines.size(); ++i) {
        std::string line = ZyxelScanner::trim(lines[i]);
        if (line.empty() || line.find("====") == 0 || line.find("----") == 0) {
            continue;
        }
        auto tokens = ZyxelScanner::splitTokens(line);
        if (tokens.size() >= 4) {
            ZyxelDhcpBindingEntry entry;
            try { entry.index = std::stoi(tokens[0]); } catch (...) {}
            entry.interfaceName = tokens[1];
            entry.ip = tokens[2];
            entry.mac = tokens[3];
            if (tokens.size() >= 5) entry.reserved = tokens[4];
            if (tokens.size() >= 6) entry.hostName = tokens[5];
            if (tokens.size() >= 7) entry.expirationTime = tokens[6];
            out.push_back(entry);
        }
    }
    return true;
}

bool ZyxelNetworkCmd::parseArpGratuitous(const std::string &raw,
                                        ZyxelArpGratuitousInfo &out) {
    auto lines = ZyxelScanner::splitLines(raw);
    bool foundStatus = false;
    out.active = false;
    out.intervalSeconds = 0;

    for (const auto &line : lines) {
        std::string key, val;
        if (ZyxelScanner::parseKeyValue(line, key, val, ':')) {
            std::string k = key;
            std::transform(k.begin(), k.end(), k.begin(), ::tolower);
            if (k.find("gratuitous arp status") != std::string::npos) {
                std::string v = val;
                std::transform(v.begin(), v.end(), v.begin(), ::tolower);
                out.active = (v == "yes" || v == "active" || v == "enable" || v == "on");
                foundStatus = true;
            } else if (k.find("periodical intervals") != std::string::npos) {
                int sec = 0;
                if (ZyxelScanner::extractIntegerAfter(line, "intervals", sec) ||
                    ZyxelScanner::extractIntegerAfter(val, "", sec)) {
                    out.intervalSeconds = sec;
                } else {
                    try { out.intervalSeconds = std::stoi(val); } catch (...) {}
                }
            }
        }
    }

    if (!foundStatus) {
        ZyxelScanner::logParseError("ZyxelNetworkCmd", "show arp gratuitous", 0,
                                   "Gratuitous ARP status field not found",
                                   lines.empty() ? "" : lines[0]);
        return false;
    }
    return true;
}

bool ZyxelNetworkCmd::parseZoneBindings(const std::string &raw,
                                       std::vector<ZyxelZoneBindingEntry> &out) {
    out.clear();
    auto lines = ZyxelScanner::splitLines(raw);
    int divIdx = ZyxelScanner::findDividerLine(lines);
    if (divIdx < 0) {
        ZyxelScanner::logParseError("ZyxelNetworkCmd", "show zone binding", 0,
                                   "No table divider found",
                                   lines.empty() ? "" : lines[0]);
        return false;
    }

    for (size_t i = divIdx + 1; i < lines.size(); ++i) {
        std::string line = ZyxelScanner::trim(lines[i]);
        if (line.empty() || line.find("====") == 0 || line.find("----") == 0) {
            continue;
        }
        auto tokens = ZyxelScanner::splitTokens(line);
        if (tokens.size() >= 3) {
            ZyxelZoneBindingEntry entry;
            try { entry.index = std::stoi(tokens[0]); } catch (...) {}
            entry.interfaceName = tokens[1];
            entry.zoneName = tokens[2];
            out.push_back(entry);
        } else if (tokens.size() == 2) {
            ZyxelZoneBindingEntry entry;
            entry.interfaceName = tokens[0];
            entry.zoneName = tokens[1];
            out.push_back(entry);
        }
    }

    if (out.empty()) {
        ZyxelScanner::logParseError("ZyxelNetworkCmd", "show zone binding", 0,
                                   "No zone binding rows parsed",
                                   lines.empty() ? "" : lines[0]);
        return false;
    }
    return true;
}

bool ZyxelNetworkCmd::parseL2IsolationActivation(const std::string &raw,
                                                bool &active) {
    auto lines = ZyxelScanner::splitLines(raw);
    for (const auto &line : lines) {
        std::string key, val;
        if (ZyxelScanner::parseKeyValue(line, key, val, ':')) {
            std::string k = key;
            std::transform(k.begin(), k.end(), k.begin(), ::tolower);
            if (k.find("layer 2 isolation status") != std::string::npos ||
                k.find("l2-isolation status") != std::string::npos) {
                std::string v = val;
                std::transform(v.begin(), v.end(), v.begin(), ::tolower);
                active = (v == "yes" || v == "active" || v == "enable" || v == "on");
                return true;
            }
        }
    }

    ZyxelScanner::logParseError("ZyxelNetworkCmd", "show l2-isolation activation", 0,
                               "Layer 2 Isolation Status not found",
                               lines.empty() ? "" : lines[0]);
    return false;
}

// Chapter 18 & 19 Routing & Routing Protocol Operational Generators (Envelope 3)
std::string ZyxelNetworkCmd::cmdShowIpRouteFilter(const std::string &proto) {
    return "show ip route " + proto;
}

std::string ZyxelNetworkCmd::cmdShowIpRouteSettings() {
    return "show ip route-settings";
}

std::string ZyxelNetworkCmd::cmdShowIpRouteControlVirtualServer() {
    return "show ip route control-virtual-server-rules";
}

std::string ZyxelNetworkCmd::cmdShowPolicyRoute() {
    return "show policy-route";
}

std::string ZyxelNetworkCmd::cmdShowPolicyRouteRuleCount() {
    return "show policy-route rule_count";
}

std::string ZyxelNetworkCmd::cmdShowPolicyRouteConnCheck() {
    return "show policy-route conn-check";
}

std::string ZyxelNetworkCmd::cmdShowPolicyRouteConnCheckStatus() {
    return "show policy-route conn-check status";
}

std::string ZyxelNetworkCmd::cmdShowPolicyRouteOverrideDirectRoute() {
    return "show policy-route override-direct-route";
}

std::string ZyxelNetworkCmd::cmdShowPolicyRouteUnderlayerRules() {
    return "show policy-route underlayer-rules";
}

std::string ZyxelNetworkCmd::cmdShowPolicyRouteControlVirtualServer() {
    return "show policy-route controll-virtual-server-rules";
}

std::string ZyxelNetworkCmd::cmdShowPolicyRouteControlIpsecDynamic() {
    return "show policy-route controll-ipsec-dynamic-rules";
}

std::string ZyxelNetworkCmd::cmdShowPolicyRouteBeginEnd(int begin, int end) {
    return "show policy-route begin " + std::to_string(begin) + " end " + std::to_string(end);
}

std::string ZyxelNetworkCmd::cmdShowPolicyRoute6() {
    return "show policy-route6";
}

std::string ZyxelNetworkCmd::cmdShowPolicyRoute6RuleCount() {
    return "show policy-route6 rule_count";
}

std::string ZyxelNetworkCmd::cmdShowPolicyRoute6OverrideDirectRoute() {
    return "show policy-route6 override-direct-route";
}

std::string ZyxelNetworkCmd::cmdShowPolicyRoute6ControlIpsecDynamic() {
    return "show policy-route6 controll-ipsec-dynamic-rules";
}

std::string ZyxelNetworkCmd::cmdShowBwmActivation() {
    return "show bwm activation";
}

std::string ZyxelNetworkCmd::cmdShowOspfGlobal() {
    return "show ospf global";
}

std::string ZyxelNetworkCmd::cmdShowOspfDatabase() {
    return "show ospf database";
}

std::string ZyxelNetworkCmd::cmdShowOspfNeighbor() {
    return "show ospf neighbor";
}

std::string ZyxelNetworkCmd::cmdShowOspfAreaVirtualLink(const std::string &areaIp) {
    return "show ospf area " + areaIp + " virtual-link";
}

std::string ZyxelNetworkCmd::cmdShowRipGlobal() {
    return "show rip global";
}

std::string ZyxelNetworkCmd::cmdShowBgpGlobal() {
    return "show bgp global";
}

std::string ZyxelNetworkCmd::cmdShowBgpSummary() {
    return "show bgp summary";
}

std::string ZyxelNetworkCmd::cmdShowBgpRoute() {
    return "show bgp route";
}

std::string ZyxelNetworkCmd::cmdShowBgpMem() {
    return "show bgp mem";
}

std::string ZyxelNetworkCmd::cmdShowBgpNeighbor() {
    return "show bgp neighbor";
}

// Chapter 18 & 19 Routing Configuration Generators (Hermetic / Tier 1)
std::string ZyxelNetworkCmd::cmdIpRouteControlVirtualServer(bool activate) {
    return activate ? "ip route control-virtual-server-rules activate"
                    : "no ip route control-virtual-server-rules activate";
}

std::string ZyxelNetworkCmd::cmdPolicyAppend() {
    return "policy append";
}

std::string ZyxelNetworkCmd::cmdPolicyDelete(int num) {
    return "policy delete " + std::to_string(num);
}

std::string ZyxelNetworkCmd::cmdPolicyDefaultRoute() {
    return "policy default-route";
}

std::string ZyxelNetworkCmd::cmdPolicyFlush() {
    return "policy flush";
}

std::string ZyxelNetworkCmd::cmdPolicyControlVirtualServer(bool activate) {
    return activate ? "policy controll-virtual-server-rules activate"
                    : "no policy controll-virtual-server-rules activate";
}

std::string ZyxelNetworkCmd::cmdPolicyControlIpsecDynamic(bool activate) {
    return activate ? "policy controll-ipsec-dynamic-rules activate"
                    : "no policy controll-ipsec-dynamic-rules activate";
}

std::string ZyxelNetworkCmd::cmdBwmActivation(bool activate) {
    return activate ? "bwm activate" : "no bwm activate";
}

std::string ZyxelNetworkCmd::cmdRouterOspf(bool enable) {
    return enable ? "router ospf" : "no router ospf";
}

std::string ZyxelNetworkCmd::cmdRouterBgp(int asNumber, bool enable) {
    return enable ? ("router bgp\n  as-number " + std::to_string(asNumber)) : "no router bgp";
}

std::string ZyxelNetworkCmd::cmdRouterRip(bool enable) {
    return enable ? "router rip" : "no router rip";
}

// Chapter 18 & 19 Dry-fire / Invalid Commands
std::string ZyxelNetworkCmd::cmdInvalidRouteDryFire() {
    return "show ip route 999.999.999.999";
}

std::string ZyxelNetworkCmd::cmdInvalidPolicyRouteDryFire() {
    return "show policy-route 99999";
}

std::string ZyxelNetworkCmd::cmdInvalidOspfAreaDryFire() {
    return "show ospf area 0.0.0.0 virtual-link";
}

// Envelope 3 Parsers
bool ZyxelNetworkCmd::parseRouteSettings(const std::string &raw,
                                        std::vector<ZyxelRouteSettingsEntry> &out) {
    out.clear();
    auto lines = ZyxelScanner::splitLines(raw);
    int divIdx = ZyxelScanner::findDividerLine(lines);
    if (divIdx < 0) {
        ZyxelScanner::logParseError("ZyxelNetworkCmd", "show ip route-settings", 0,
                                   "No table divider found",
                                   lines.empty() ? "" : lines[0]);
        return false;
    }

    for (size_t i = divIdx + 1; i < lines.size(); ++i) {
        std::string line = ZyxelScanner::trim(lines[i]);
        if (line.empty() || line.find("====") == 0 || line.find("----") == 0) {
            continue;
        }
        auto tokens = ZyxelScanner::splitTokens(line);
        if (tokens.size() >= 4) {
            ZyxelRouteSettingsEntry entry;
            entry.route = tokens[0];
            entry.netmask = tokens[1];
            entry.nexthop = tokens[2];
            try { entry.metric = std::stoi(tokens[3]); } catch (...) {}
            out.push_back(entry);
        }
    }
    return true;
}

bool ZyxelNetworkCmd::parsePolicyRouteRuleCount(const std::string &raw, int &count) {
    auto lines = ZyxelScanner::splitLines(raw);
    for (const auto &line : lines) {
        std::string key, val;
        if (ZyxelScanner::parseKeyValue(line, key, val, ':')) {
            std::string k = key;
            std::transform(k.begin(), k.end(), k.begin(), ::tolower);
            if (k.find("policy count") != std::string::npos) {
                try {
                    count = std::stoi(val);
                    return true;
                } catch (...) {
                    return false;
                }
            }
        }
    }
    ZyxelScanner::logParseError("ZyxelNetworkCmd", "show policy-route rule_count", 0,
                               "policy count not found",
                               lines.empty() ? "" : lines[0]);
    return false;
}

bool ZyxelNetworkCmd::parsePolicyRouteOverrideDirectRoute(const std::string &raw, bool &active) {
    auto lines = ZyxelScanner::splitLines(raw);
    for (const auto &line : lines) {
        std::string key, val;
        if (ZyxelScanner::parseKeyValue(line, key, val, ':')) {
            std::string k = key;
            std::transform(k.begin(), k.end(), k.begin(), ::tolower);
            if (k.find("override direct route status") != std::string::npos) {
                std::string v = val;
                std::transform(v.begin(), v.end(), v.begin(), ::tolower);
                active = (v == "on" || v == "yes" || v == "enable" || v == "active");
                return true;
            }
        }
    }
    ZyxelScanner::logParseError("ZyxelNetworkCmd", "override direct route status", 0,
                               "Status key not found",
                               lines.empty() ? "" : lines[0]);
    return false;
}

bool ZyxelNetworkCmd::parsePolicyRouteControlVirtualServer(const std::string &raw, bool &active) {
    auto lines = ZyxelScanner::splitLines(raw);
    for (const auto &line : lines) {
        std::string key, val;
        if (ZyxelScanner::parseKeyValue(line, key, val, ':')) {
            std::string k = key;
            std::transform(k.begin(), k.end(), k.begin(), ::tolower);
            if (k.find("status") != std::string::npos) {
                std::string v = val;
                std::transform(v.begin(), v.end(), v.begin(), ::tolower);
                active = (v == "on" || v == "yes" || v == "enable" || v == "active");
                return true;
            }
        }
    }
    ZyxelScanner::logParseError("ZyxelNetworkCmd", "control status", 0,
                               "Status key not found",
                               lines.empty() ? "" : lines[0]);
    return false;
}

bool ZyxelNetworkCmd::parseBwmActivation(const std::string &raw, bool &active) {
    auto lines = ZyxelScanner::splitLines(raw);
    for (const auto &line : lines) {
        std::string key, val;
        if (ZyxelScanner::parseKeyValue(line, key, val, ':')) {
            std::string k = key;
            std::transform(k.begin(), k.end(), k.begin(), ::tolower);
            if (k.find("bwm activation") != std::string::npos) {
                std::string v = val;
                std::transform(v.begin(), v.end(), v.begin(), ::tolower);
                active = (v == "yes" || v == "on" || v == "enable" || v == "active");
                return true;
            }
        }
    }
    ZyxelScanner::logParseError("ZyxelNetworkCmd", "show bwm activation", 0,
                               "bwm activation key not found",
                               lines.empty() ? "" : lines[0]);
    return false;
}

bool ZyxelNetworkCmd::parseOspfGlobal(const std::string &raw, ZyxelOspfGlobalInfo &out) {
    auto lines = ZyxelScanner::splitLines(raw);
    bool foundHeader = false;
    for (const auto &line : lines) {
        if (line.find("OSPF Global Information") != std::string::npos) {
            foundHeader = true;
            continue;
        }
        std::string key, val;
        if (ZyxelScanner::parseKeyValue(line, key, val, ':')) {
            std::string k = key;
            std::transform(k.begin(), k.end(), k.begin(), ::tolower);
            if (k.find("router id") != std::string::npos) {
                out.routerId = val;
            } else if (k == "redistribute rip") {
                out.redistributeRip = (val == "yes" || val == "on");
            } else if (k == "redistribute rip type") {
                out.redistributeRipType = val;
            } else if (k == "redistribute rip metric") {
                out.redistributeRipMetric = val;
            } else if (k == "redistribute static") {
                out.redistributeStatic = (val == "yes" || val == "on");
            } else if (k == "redistribute static type") {
                out.redistributeStaticType = val;
            } else if (k == "redistribute static metric") {
                out.redistributeStaticMetric = val;
            }
        }
    }
    if (!foundHeader && out.routerId.empty()) {
        ZyxelScanner::logParseError("ZyxelNetworkCmd", "show ospf global", 0,
                                   "OSPF Global Information header not found",
                                   lines.empty() ? "" : lines[0]);
        return false;
    }
    return true;
}

bool ZyxelNetworkCmd::parseRipGlobal(const std::string &raw, ZyxelRipGlobalInfo &out) {
    auto lines = ZyxelScanner::splitLines(raw);
    bool foundHeader = false;
    for (const auto &line : lines) {
        if (line.find("RIP Global Information") != std::string::npos) {
            foundHeader = true;
            continue;
        }
        std::string key, val;
        if (ZyxelScanner::parseKeyValue(line, key, val, ':')) {
            std::string k = key;
            std::transform(k.begin(), k.end(), k.begin(), ::tolower);
            if (k.find("authentication type") != std::string::npos) {
                out.authType = val;
            } else if (k.find("text string") != std::string::npos) {
                out.textString = val;
            } else if (k.find("md5 key") != std::string::npos) {
                out.md5Key = val;
            } else if (k.find("md5 string") != std::string::npos) {
                out.md5String = val;
            } else if (k == "redistribute ospf") {
                out.redistributeOspf = (val == "yes" || val == "on");
            } else if (k == "redistribute ospf metric") {
                try { out.redistributeOspfMetric = std::stoi(val); } catch (...) {}
            } else if (k == "redistribute static") {
                out.redistributeStatic = (val == "yes" || val == "on");
            } else if (k == "redistribute static metric") {
                try { out.redistributeStaticMetric = std::stoi(val); } catch (...) {}
            }
        }
    }
    if (!foundHeader && out.authType.empty()) {
        ZyxelScanner::logParseError("ZyxelNetworkCmd", "show rip global", 0,
                                   "RIP Global Information header not found",
                                   lines.empty() ? "" : lines[0]);
        return false;
    }
    return true;
}

bool ZyxelNetworkCmd::parseBgpGlobal(const std::string &raw, ZyxelBgpGlobalInfo &out) {
    auto lines = ZyxelScanner::splitLines(raw);
    bool foundHeader = false;
    for (const auto &line : lines) {
        if (line.find("BGP Global Information") != std::string::npos) {
            foundHeader = true;
            continue;
        }
        std::string key, val;
        if (ZyxelScanner::parseKeyValue(line, key, val, ':')) {
            std::string k = key;
            std::transform(k.begin(), k.end(), k.begin(), ::tolower);
            if (k.find("router id") != std::string::npos) {
                out.routerId = val;
            } else if (k.find("as number") != std::string::npos) {
                try { out.asNumber = std::stoi(val); } catch (...) {}
            } else if (k.find("redistribute connected") != std::string::npos) {
                out.redistributeConnected = (val == "yes" || val == "on");
            } else if (k.find("maximum paths") != std::string::npos) {
                out.maximumPaths = val;
            } else if (k.find("bgp network") != std::string::npos) {
                out.bgpNetwork = val;
            }
        }
    }
    if (!foundHeader && out.asNumber == 0) {
        ZyxelScanner::logParseError("ZyxelNetworkCmd", "show bgp global", 0,
                                   "BGP Global Information header not found",
                                   lines.empty() ? "" : lines[0]);
        return false;
    }
    return true;
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
