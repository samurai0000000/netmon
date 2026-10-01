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

std::string ZyxelNetworkCmd::cmdShowArp() {
    return "show arp-table";
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

    if (divIdx > 0) {
        auto cols = ZyxelScanner::parseTableColumns(lines[divIdx - 1], lines[divIdx]);
        int dstCol = ZyxelScanner::findColumn(cols, "route");
        if (dstCol < 0) dstCol = ZyxelScanner::findColumn(cols, "destination");
        int maskCol = ZyxelScanner::findColumn(cols, "netmask");
        if (maskCol < 0) maskCol = ZyxelScanner::findColumn(cols, "mask");
        int gwCol = ZyxelScanner::findColumn(cols, "nexthop");
        if (gwCol < 0) gwCol = ZyxelScanner::findColumn(cols, "gateway");
        int ifCol = ZyxelScanner::findColumn(cols, "interface");
        int metCol = ZyxelScanner::findColumn(cols, "metric");

        for (size_t i = divIdx + 1; i < lines.size(); ++i) {
            const std::string &line = lines[i];
            std::string trimmed = ZyxelScanner::trim(line);
            if (trimmed.empty() || trimmed.find("====") == 0 || trimmed.find("----") == 0) {
                continue;
            }

            if (!cols.empty() && dstCol >= 0 && maskCol >= 0 && gwCol >= 0) {
                ZyxelRouteEntry entry;
                entry.destination = ZyxelScanner::extractCell(line, cols[dstCol]);
                entry.netmask = ZyxelScanner::extractCell(line, cols[maskCol]);
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
    }

    if (out.empty()) {
        ZyxelScanner::logParseError("ZyxelNetworkCmd", "show ip route-settings", 0,
                                   "No route entries found in table",
                                   lines.empty() ? "" : lines[0]);
        return false;
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
                ZyxelZoneInfo zone;
                zone.name = tokens[0];
                for (size_t j = 1; j < tokens.size(); ++j) {
                    std::string iface = tokens[j];
                    if (!iface.empty() && iface.back() == ',') {
                        iface.pop_back();
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

/*
 * Local variables:
 * mode: C++
 * c-file-style: "BSD"
 * c-basic-offset: 4
 * tab-width: 4
 * indent-tabs-mode: nil
 * End:
 */
