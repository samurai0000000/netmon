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

/*
 * Local variables:
 * mode: C++
 * c-file-style: "BSD"
 * c-basic-offset: 4
 * tab-width: 4
 * indent-tabs-mode: nil
 * End:
 */
