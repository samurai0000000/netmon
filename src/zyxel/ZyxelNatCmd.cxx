/*
 * ZyxelNatCmd.cxx
 *
 * Copyright (C) 2026, Charles Chiou
 */

#include "zyxel/ZyxelNatCmd.hxx"
#include "zyxel/ZyxelScanner.hxx"

#include <algorithm>
#include <cctype>

std::string ZyxelNatCmd::cmdShowVirtualServers(const std::string &name) {
    if (name.empty()) {
        return "show ip virtual-server";
    }
    return "show ip virtual-server " + name;
}

std::string ZyxelNatCmd::cmdAddVirtualServer(const ZyxelVirtualServerRule &rule) {
    std::string cmd = "ip virtual-server " + rule.name;
    if (!rule.interface.empty()) {
        cmd += " interface " + rule.interface;
    }
    if (!rule.originalIp.empty()) {
        cmd += " original-ip " + rule.originalIp;
    }
    if (!rule.mapToIp.empty()) {
        cmd += " map-to " + rule.mapToIp;
    }
    if (!rule.originalService.empty()) {
        cmd += " map-type original-service " + rule.originalService;
        if (!rule.mappedService.empty()) {
            cmd += " mapped-service " + rule.mappedService;
        } else {
            cmd += " mapped-service " + rule.originalService;
        }
    } else {
        cmd += " map-type port";
    }
    if (!rule.active) {
        cmd += " deactivate";
    }
    return cmd;
}

std::string ZyxelNatCmd::cmdDeleteVirtualServer(const std::string &name) {
    return "no ip virtual-server " + name;
}

std::string ZyxelNatCmd::cmdShowVirtualServerStatus() {
    return "show ip virtual-server status";
}

std::string ZyxelNatCmd::cmdShowVirtualServerLoadBalancer(const std::string &name) {
    if (name.empty()) {
        return "show ip virtual-server load-balancer";
    }
    return "show ip virtual-server load-balancer " + name;
}

std::string ZyxelNatCmd::cmdInvalidVirtualServerDryFire() {
    return "show ip virtual-server load-balancer NETMON_NONEXISTENT_VS_999";
}

std::string ZyxelNatCmd::cmdShowDdns(const std::string &profileName) {
    if (profileName.empty()) {
        return "show ddns";
    }
    return "show ddns " + profileName;
}

std::string ZyxelNatCmd::cmdShowDdnsStatus() {
    return "show ddns-status";
}

std::string ZyxelNatCmd::cmdAddDdnsProfile(const std::string &name) {
    return "ip ddns profile " + name;
}

std::string ZyxelNatCmd::cmdDeleteDdnsProfile(const std::string &name) {
    return "no ip ddns profile " + name;
}

std::string ZyxelNatCmd::cmdInvalidDdnsDryFire() {
    return "show ddns NETMON_NONEXISTENT_DDNS_999";
}

std::string ZyxelNatCmd::cmdShowHttpRedirect(const std::string &desc) {
    if (desc.empty()) {
        return "show ip http-redirect";
    }
    return "show ip http-redirect " + desc;
}

std::string ZyxelNatCmd::cmdAddHttpRedirect(const std::string &desc) {
    return "ip http-redirect " + desc;
}

std::string ZyxelNatCmd::cmdDeleteHttpRedirect(const std::string &desc) {
    return "no ip http-redirect " + desc;
}

std::string ZyxelNatCmd::cmdInvalidHttpRedirectDryFire() {
    return "show ip http-redirect NETMON_NONEXISTENT_HTTP_999";
}

std::string ZyxelNatCmd::cmdShowRedirectService(int ruleIndex) {
    if (ruleIndex > 0) {
        return "show redirect-service " + std::to_string(ruleIndex);
    }
    return "show redirect-service";
}

std::string ZyxelNatCmd::cmdInvalidRedirectServiceDryFire() {
    return "show redirect-service 99";
}

std::string ZyxelNatCmd::cmdShowAlg(const std::string &proto) {
    return "show alg " + proto;
}

std::string ZyxelNatCmd::cmdShowAlgFtp() {
    return "show alg ftp";
}

std::string ZyxelNatCmd::cmdShowAlgSip() {
    return "show alg sip";
}

std::string ZyxelNatCmd::cmdShowAlgH323() {
    return "show alg h323";
}

std::string ZyxelNatCmd::cmdInvalidAlgDryFire() {
    return "show alg invalid_proto_999";
}

std::string ZyxelNatCmd::cmdShowUpnp() {
    return "show upnp";
}

std::string ZyxelNatCmd::cmdShowUpnpIgd() {
    return "show upnp-igd";
}

std::string ZyxelNatCmd::cmdShowNatPmp() {
    return "show nat-pmp";
}

std::string ZyxelNatCmd::cmdInvalidUpnpDryFire() {
    return "show upnp invalid_probe_999";
}

bool ZyxelNatCmd::parseVirtualServers(const std::string &raw,
                                     std::vector<ZyxelVirtualServerRule> &out) {
    out.clear();
    auto lines = ZyxelScanner::splitLines(raw);

    // Block format from manual page 192:
    // virtual server: WAN-LAN_H323
    //   Index: 1 
    //   active: yes
    //   interface: wan1
    //   original IP: 10.0.0.8
    //   mapped IP: 192.168.1.56
    //   original service: H323
    //   mapped service: H323

    ZyxelVirtualServerRule currentRule;
    bool inRule = false;

    for (const auto &line : lines) {
        std::string lower = line;
        std::transform(lower.begin(), lower.end(), lower.begin(), ::tolower);

        if (lower.find("virtual server:") != std::string::npos) {
            if (inRule) {
                out.push_back(currentRule);
                currentRule = ZyxelVirtualServerRule();
            }
            inRule = true;
            size_t cPos = line.find(':');
            currentRule.name = ZyxelScanner::trim(line.substr(cPos + 1));
            continue;
        }

        if (!inRule) {
            continue;
        }

        std::string key, val;
        if (ZyxelScanner::parseKeyValue(line, key, val, ':')) {
            std::string k = key;
            std::transform(k.begin(), k.end(), k.begin(), ::tolower);
            if (k == "index") {
                try {
                    currentRule.index = std::stoi(val);
                } catch (...) {
                    currentRule.index = 0;
                }
            } else if (k == "active") {
                std::string v = val;
                std::transform(v.begin(), v.end(), v.begin(), ::tolower);
                currentRule.active = (v == "yes" || v == "active" || v == "true");
            } else if (k == "interface") {
                currentRule.interface = val;
            } else if (k == "original ip") {
                currentRule.originalIp = val;
            } else if (k == "mapped ip") {
                currentRule.mapToIp = val;
            } else if (k == "original service") {
                currentRule.originalService = val;
            } else if (k == "mapped service") {
                currentRule.mappedService = val;
            }
        }
    }

    if (inRule) {
        out.push_back(currentRule);
    }

    // Check table format fallback
    if (out.empty()) {
        int divIdx = ZyxelScanner::findDividerLine(lines);
        if (divIdx > 0) {
            auto cols = ZyxelScanner::parseTableColumns(lines[divIdx - 1], lines[divIdx]);
            int nameCol = ZyxelScanner::findColumn(cols, "name");
            int ifCol = ZyxelScanner::findColumn(cols, "interface");
            int origCol = ZyxelScanner::findColumn(cols, "original");
            int mapCol = ZyxelScanner::findColumn(cols, "mapped");
            if (mapCol < 0) mapCol = ZyxelScanner::findColumn(cols, "mapp");
            int statCol = ZyxelScanner::findColumn(cols, "status");

            for (size_t i = divIdx + 1; i < lines.size(); ++i) {
                const std::string &line = lines[i];
                std::string trimmed = ZyxelScanner::trim(line);
                if (trimmed.empty() || trimmed.find("====") == 0 || trimmed.find("----") == 0) {
                    continue;
                }

                if (!cols.empty() && nameCol >= 0 && ifCol >= 0 && origCol >= 0 && mapCol >= 0) {
                    ZyxelVirtualServerRule rule;
                    rule.name = ZyxelScanner::extractCell(line, cols[nameCol]);
                    rule.interface = ZyxelScanner::extractCell(line, cols[ifCol]);
                    rule.originalIp = ZyxelScanner::extractCell(line, cols[origCol]);
                    rule.mapToIp = ZyxelScanner::extractCell(line, cols[mapCol]);
                    if (statCol >= 0) {
                        std::string st = ZyxelScanner::extractCell(line, cols[statCol]);
                        std::transform(st.begin(), st.end(), st.begin(), ::tolower);
                        rule.active = (st == "yes" || st == "active");
                    }
                    if (!rule.name.empty()) {
                        out.push_back(rule);
                    }
                } else {
                    auto tokens = ZyxelScanner::splitTokens(line);
                    if (tokens.size() >= 5) {
                        ZyxelVirtualServerRule rule;
                        rule.name = tokens[0];
                        rule.interface = tokens[1];
                        rule.originalIp = tokens[2];
                        rule.mapToIp = tokens[3];
                        std::string st = tokens[4];
                        std::transform(st.begin(), st.end(), st.begin(), ::tolower);
                        rule.active = (st == "yes" || st == "active");
                        out.push_back(rule);
                    }
                }
            }
        }
    }

    if (out.empty()) {
        ZyxelScanner::logParseError("ZyxelNatCmd", "show ip virtual-server", 0,
                                   "No virtual server entries found",
                                   lines.empty() ? "" : lines[0]);
        return false;
    }
    return true;
}

bool ZyxelNatCmd::parseDdnsStatus(const std::string &raw,
                                  std::vector<ZyxelDdnsStatusEntry> &out) {
    out.clear();
    auto lines = ZyxelScanner::splitLines(raw);
    int divIdx = ZyxelScanner::findDividerLine(lines);
    if (divIdx < 0) {
        ZyxelScanner::logParseError("ZyxelNatCmd", "show ddns-status", 0,
                                   "Divider line not found",
                                   lines.empty() ? "" : lines[0]);
        return false;
    }

    ZyxelDdnsStatusEntry current;
    bool hasEntry = false;

    for (size_t i = static_cast<size_t>(divIdx + 1); i < lines.size(); ++i) {
        const std::string &rawLine = lines[i];
        std::string line = ZyxelScanner::trim(rawLine);
        if (line.empty() || line.find("====") == 0 || line.find("----") == 0) {
            continue;
        }

        auto tokens = ZyxelScanner::splitTokens(line);
        bool isIndexStart = (!rawLine.empty() &&
                             !std::isspace(static_cast<unsigned char>(rawLine[0])) &&
                             std::isdigit(static_cast<unsigned char>(line[0])) &&
                             !tokens.empty() &&
                             tokens[0].find('.') == std::string::npos);

        if (isIndexStart) {
            if (hasEntry) {
                out.push_back(current);
                current = ZyxelDdnsStatusEntry();
                hasEntry = false;
            }
            if (tokens.size() >= 3) {
                try {
                    current.index = std::stoi(tokens[0]);
                } catch (...) {
                    current.index = 0;
                }
                current.profileName = tokens[1];
                current.domainName = tokens[2];
                hasEntry = true;
                if (tokens.size() >= 5) {
                    current.effectiveIp = tokens[3];
                    current.status = tokens[4];
                    if (tokens.size() >= 6) {
                        current.updateTime = tokens[5];
                        for (size_t k = 6; k < tokens.size(); ++k) {
                            current.updateTime += " " + tokens[k];
                        }
                    }
                }
            }
        } else if (hasEntry && !line.empty()) {
            if (tokens.size() >= 2) {
                current.effectiveIp = tokens[0];
                current.status = tokens[1];
                if (tokens.size() >= 3) {
                    current.updateTime = tokens[2];
                    for (size_t k = 3; k < tokens.size(); ++k) {
                        current.updateTime += " " + tokens[k];
                    }
                }
            }
        }
    }

    if (hasEntry) {
        out.push_back(current);
    }

    if (out.empty()) {
        ZyxelScanner::logParseError("ZyxelNatCmd", "show ddns-status", 0,
                                   "No DDNS status entries found",
                                   lines.empty() ? "" : lines[0]);
        return false;
    }
    return true;
}

bool ZyxelNatCmd::parseAlgStatus(const std::string &raw,
                                 const std::string &proto,
                                 ZyxelAlgStatus &out) {
    out = ZyxelAlgStatus();
    out.protocol = proto;
    auto lines = ZyxelScanner::splitLines(raw);
    if (lines.empty()) {
        return false;
    }

    bool foundAny = false;
    for (size_t i = 0; i < lines.size(); ++i) {
        const std::string &line = lines[i];
        std::string key, val;
        if (ZyxelScanner::parseKeyValue(line, key, val, ':')) {
            std::string k = key;
            std::transform(k.begin(), k.end(), k.begin(), ::tolower);
            std::string v = val;
            std::transform(v.begin(), v.end(), v.begin(), ::tolower);

            if (k == "active") {
                out.active = (v == "yes" || v == "active" || v == "true");
                foundAny = true;
            } else if (k == "transformation") {
                out.transformation = (v == "yes" || v == "active" || v == "true");
                foundAny = true;
            } else if (k == "signaling port") {
                try {
                    out.signalingPort = std::stoi(val);
                    foundAny = true;
                } catch (...) {
                }
            }
        }

        if (out.signalingPort == 0 && line.find("====") != std::string::npos && i + 1 < lines.size()) {
            auto tokens = ZyxelScanner::splitTokens(lines[i + 1]);
            if (tokens.size() >= 2) {
                try {
                    out.signalingPort = std::stoi(tokens[1]);
                    foundAny = true;
                } catch (...) {
                }
            }
        }
    }

    if (!foundAny) {
        ZyxelScanner::logParseError("ZyxelNatCmd", "show alg " + proto, 0,
                                   "No ALG fields found",
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
