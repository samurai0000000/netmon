/*
 * ZyxelFirewallCmd.cxx
 *
 * Copyright (C) 2026, Charles Chiou
 */

#include "zyxel/ZyxelFirewallCmd.hxx"
#include "zyxel/ZyxelScanner.hxx"

#include <algorithm>
#include <cctype>

std::string ZyxelFirewallCmd::cmdShowSecurePolicy(const std::string &nameOrNum) {
    if (nameOrNum.empty()) {
        return "show secure-policy";
    }
    return "show secure-policy " + nameOrNum;
}

std::string ZyxelFirewallCmd::cmdShowSecurePolicyStatus() {
    return "show secure-policy status";
}

std::string ZyxelFirewallCmd::cmdShowSecurePolicyBlockRules() {
    return "show secure-policy block_rules";
}

std::string ZyxelFirewallCmd::cmdShowSecurePolicy6(const std::string &nameOrNum) {
    if (nameOrNum.empty()) {
        return "show secure-policy6";
    }
    return "show secure-policy6 " + nameOrNum;
}

std::vector<std::string> ZyxelFirewallCmd::cmdInsertRule(int position, const ZyxelFirewallRule &rule) {
    std::vector<std::string> cmds;
    cmds.push_back("secure-policy insert " + std::to_string(position));
    if (!rule.name.empty()) {
        cmds.push_back("name " + rule.name);
    }
    if (!rule.description.empty()) {
        cmds.push_back("description " + rule.description);
    }
    if (!rule.fromZone.empty()) {
        cmds.push_back("from " + rule.fromZone);
    }
    if (!rule.toZone.empty()) {
        cmds.push_back("to " + rule.toZone);
    }
    if (!rule.sourceIp.empty()) {
        cmds.push_back("sourceip " + rule.sourceIp);
    }
    if (!rule.destinationIp.empty()) {
        cmds.push_back("destinationip " + rule.destinationIp);
    }
    if (!rule.service.empty()) {
        cmds.push_back("service " + rule.service);
    }
    cmds.push_back("action " + (rule.action.empty() ? "allow" : rule.action));
    cmds.push_back(rule.active ? "activate" : "deactivate");
    cmds.push_back("exit");
    return cmds;
}

std::vector<std::string> ZyxelFirewallCmd::cmdInsertFastDeny(int position,
                                                            const std::string &ruleName,
                                                            const std::string &srcObjName,
                                                            const std::string &reason) {
    std::vector<std::string> cmds;
    cmds.push_back("secure-policy insert " + std::to_string(position));
    cmds.push_back("name " + ruleName);
    cmds.push_back("description " + (reason.empty() ? ruleName : reason));
    cmds.push_back("action deny");
    cmds.push_back("sourceip " + srcObjName);
    cmds.push_back("activate");
    cmds.push_back("exit");
    return cmds;
}

std::string ZyxelFirewallCmd::cmdDeleteRule(const std::string &nameOrNum) {
    bool isAllDigits = !nameOrNum.empty() &&
        std::all_of(nameOrNum.begin(), nameOrNum.end(), ::isdigit);
    if (isAllDigits) {
        return "no secure-policy " + nameOrNum;
    }
    return "no secure-policy name " + nameOrNum;
}

std::vector<std::string> ZyxelFirewallCmd::cmdAppendRule(const ZyxelFirewallRule &rule) {
    std::vector<std::string> cmds;
    cmds.push_back("secure-policy append");
    if (!rule.name.empty()) {
        cmds.push_back("name " + rule.name);
    }
    if (!rule.description.empty()) {
        cmds.push_back("description " + rule.description);
    }
    if (!rule.fromZone.empty()) {
        cmds.push_back("from " + rule.fromZone);
    }
    if (!rule.toZone.empty()) {
        cmds.push_back("to " + rule.toZone);
    }
    if (!rule.sourceIp.empty()) {
        cmds.push_back("sourceip " + rule.sourceIp);
    }
    if (!rule.destinationIp.empty()) {
        cmds.push_back("destinationip " + rule.destinationIp);
    }
    if (!rule.service.empty()) {
        cmds.push_back("service " + rule.service);
    }
    cmds.push_back("action " + (rule.action.empty() ? "allow" : rule.action));
    cmds.push_back(rule.active ? "activate" : "deactivate");
    cmds.push_back("exit");
    return cmds;
}

std::vector<std::string> ZyxelFirewallCmd::cmdAppendFastDeny(const std::string &ruleName,
                                                            const std::string &srcObjName,
                                                            const std::string &reason) {
    std::vector<std::string> cmds;
    cmds.push_back("secure-policy append");
    cmds.push_back("name " + ruleName);
    cmds.push_back("description " + (reason.empty() ? ruleName : reason));
    cmds.push_back("action deny");
    cmds.push_back("sourceip " + srcObjName);
    cmds.push_back("activate");
    cmds.push_back("exit");
    return cmds;
}

std::string ZyxelFirewallCmd::cmdDeleteRuleByName(const std::string &name) {
    return "no secure-policy name " + name;
}

std::string ZyxelFirewallCmd::cmdDeleteRuleByNumber(int num) {
    return "no secure-policy " + std::to_string(num);
}

std::string ZyxelFirewallCmd::cmdActivateRule(const std::string &nameOrNum, bool activate) {
    bool isAllDigits = !nameOrNum.empty() &&
        std::all_of(nameOrNum.begin(), nameOrNum.end(), ::isdigit);
    std::string target = isAllDigits ? (" " + nameOrNum) : (" name " + nameOrNum);
    if (activate) {
        return "secure-policy" + target + " activate";
    }
    return "no secure-policy" + target + " activate";
}

std::string ZyxelFirewallCmd::cmdActivateSecurePolicy(bool activate) {
    if (activate) {
        return "secure-policy activate";
    }
    return "no secure-policy activate";
}

std::string ZyxelFirewallCmd::cmdActivateSecurePolicy6(bool activate) {
    if (activate) {
        return "secure-policy6 activate";
    }
    return "no secure-policy6 activate";
}

std::string ZyxelFirewallCmd::cmdSetAsymmetricalRoute(bool enable) {
    if (enable) {
        return "secure-policy asymmetrical-route activate";
    }
    return "no secure-policy asymmetrical-route activate";
}

std::string ZyxelFirewallCmd::cmdShowDeviceHa() {
    return "show device-ha";
}

std::string ZyxelFirewallCmd::cmdShowDeviceHaStatus() {
    return "show device-ha status";
}

std::string ZyxelFirewallCmd::cmdShowDeviceHaMode() {
    return "show device-ha mode";
}

std::string ZyxelFirewallCmd::cmdShowDeviceHa2() {
    return "show device-ha2";
}

std::string ZyxelFirewallCmd::cmdShowDeviceHa2Interfaces() {
    return "show device-ha2 interfaces";
}

std::string ZyxelFirewallCmd::cmdShowDeviceHa2DeviceStatus() {
    return "show device-ha2 device-status";
}

std::string ZyxelFirewallCmd::cmdInvalidSecurePolicyDryFire() {
    return "show secure-policy 999";
}

std::string ZyxelFirewallCmd::cmdInvalidSecurePolicy6DryFire() {
    return "show secure-policy6 999";
}

std::string ZyxelFirewallCmd::cmdInvalidDeviceHaDryFire() {
    return "show device-ha invalid_probe_999";
}

std::string ZyxelFirewallCmd::cmdInvalidDeviceHa2DryFire() {
    return "show device-ha2 invalid_probe_999";
}

bool ZyxelFirewallCmd::parseSecurePolicy(const std::string &raw,
                                        std::vector<ZyxelFirewallRule> &out) {
    out.clear();
    auto lines = ZyxelScanner::splitLines(raw);

    // Check if table format first
    int divIdx = ZyxelScanner::findDividerLine(lines);
    if (divIdx > 0) {
        auto cols = ZyxelScanner::parseTableColumns(lines[divIdx - 1], lines[divIdx]);
        int idxCol = ZyxelScanner::findColumn(cols, "rule");
        if (idxCol < 0) idxCol = ZyxelScanner::findColumn(cols, "index");
        int nameCol = ZyxelScanner::findColumn(cols, "name");
        int fromCol = ZyxelScanner::findColumn(cols, "from");
        int toCol = ZyxelScanner::findColumn(cols, "to");
        int srcCol = ZyxelScanner::findColumn(cols, "source");
        int dstCol = ZyxelScanner::findColumn(cols, "dest");
        int srvCol = ZyxelScanner::findColumn(cols, "service");
        int actCol = ZyxelScanner::findColumn(cols, "action");
        int statCol = ZyxelScanner::findColumn(cols, "status");

        for (size_t i = divIdx + 1; i < lines.size(); ++i) {
            const std::string &line = lines[i];
            std::string trimmed = ZyxelScanner::trim(line);
            if (trimmed.empty() || trimmed.find("====") == 0 || trimmed.find("----") == 0) {
                continue;
            }

            if (!cols.empty() && nameCol >= 0 && fromCol >= 0 && toCol >= 0) {
                ZyxelFirewallRule rule;
                if (idxCol >= 0) {
                    try {
                        rule.index = std::stoi(ZyxelScanner::extractCell(line, cols[idxCol]));
                    } catch (...) {
                        rule.index = 0;
                    }
                }
                rule.name = ZyxelScanner::extractCell(line, cols[nameCol]);
                rule.fromZone = ZyxelScanner::extractCell(line, cols[fromCol]);
                rule.toZone = ZyxelScanner::extractCell(line, cols[toCol]);
                if (srcCol >= 0) rule.sourceIp = ZyxelScanner::extractCell(line, cols[srcCol]);
                if (dstCol >= 0) rule.destinationIp = ZyxelScanner::extractCell(line, cols[dstCol]);
                if (srvCol >= 0) rule.service = ZyxelScanner::extractCell(line, cols[srvCol]);
                if (actCol >= 0) rule.action = ZyxelScanner::extractCell(line, cols[actCol]);
                if (statCol >= 0) {
                    std::string st = ZyxelScanner::extractCell(line, cols[statCol]);
                    std::transform(st.begin(), st.end(), st.begin(), ::tolower);
                    rule.active = (st == "yes" || st == "active" || st == "true");
                }
                if (!rule.name.empty()) {
                    out.push_back(rule);
                }
            } else {
                auto tokens = ZyxelScanner::splitTokens(line);
                if (tokens.size() >= 8) {
                    ZyxelFirewallRule rule;
                    try {
                        rule.index = std::stoi(tokens[0]);
                    } catch (...) {
                        rule.index = 0;
                    }
                    rule.name = tokens[1];
                    rule.fromZone = tokens[2];
                    rule.toZone = tokens[3];
                    rule.sourceIp = tokens[4];
                    rule.destinationIp = tokens[5];
                    rule.service = tokens[6];
                    rule.action = tokens[7];
                    if (tokens.size() >= 9) {
                        std::string st = tokens[8];
                        std::transform(st.begin(), st.end(), st.begin(), ::tolower);
                        rule.active = (st == "yes" || st == "active" || st == "true");
                    }
                    out.push_back(rule);
                }
            }
        }
    }

    if (out.empty()) {
        // Detailed block format (from manual page 225)
        ZyxelFirewallRule currentRule;
        bool inRule = false;

        for (const auto &line : lines) {
            std::string lower = line;
            std::transform(lower.begin(), lower.end(), lower.begin(), ::tolower);

            if (lower.find("secure-policy rule:") != std::string::npos) {
                if (inRule) {
                    out.push_back(currentRule);
                    currentRule = ZyxelFirewallRule();
                }
                inRule = true;
                size_t cPos = lower.find(':');
                try {
                    currentRule.index = std::stoi(ZyxelScanner::trim(lower.substr(cPos + 1)));
                } catch (...) {
                    currentRule.index = 0;
                }
                continue;
            }

            if (!inRule) {
                continue;
            }

            // Parse key-value lines
            size_t namePos = lower.find("name:");
            if (namePos != std::string::npos) {
                currentRule.name = ZyxelScanner::trim(line.substr(namePos + 5));
            }

            size_t descPos = lower.find("description:");
            if (descPos != std::string::npos) {
                currentRule.description = ZyxelScanner::trim(line.substr(descPos + 12));
            }

            size_t fromPos = lower.find("from:");
            size_t toPos = lower.find("to:");
            if (fromPos != std::string::npos && toPos != std::string::npos && toPos > fromPos) {
                size_t comma = line.find(',', fromPos);
                if (comma != std::string::npos && comma < toPos) {
                    currentRule.fromZone = ZyxelScanner::trim(line.substr(fromPos + 5, comma - fromPos - 5));
                }
                currentRule.toZone = ZyxelScanner::trim(line.substr(toPos + 3));
            }

            size_t srcPos = lower.find("source ip:");
            if (srcPos != std::string::npos) {
                size_t comma = line.find(',', srcPos);
                if (comma != std::string::npos) {
                    currentRule.sourceIp = ZyxelScanner::trim(line.substr(srcPos + 10, comma - srcPos - 10));
                } else {
                    currentRule.sourceIp = ZyxelScanner::trim(line.substr(srcPos + 10));
                }
            }

            size_t dstPos = lower.find("destination ip:");
            size_t svcPos = lower.find("service:");
            if (dstPos != std::string::npos) {
                if (svcPos != std::string::npos && svcPos > dstPos) {
                    size_t comma = line.find(',', dstPos);
                    if (comma != std::string::npos && comma < svcPos) {
                        currentRule.destinationIp = ZyxelScanner::trim(line.substr(dstPos + 15, comma - dstPos - 15));
                    }
                    currentRule.service = ZyxelScanner::trim(line.substr(svcPos + 8));
                } else {
                    currentRule.destinationIp = ZyxelScanner::trim(line.substr(dstPos + 15));
                }
            }

            size_t actPos = lower.find("action:");
            size_t stPos = lower.find("status:");
            if (actPos != std::string::npos) {
                if (stPos != std::string::npos && stPos > actPos) {
                    size_t comma = line.find(',', actPos);
                    if (comma != std::string::npos && comma < stPos) {
                        currentRule.action = ZyxelScanner::trim(line.substr(actPos + 7, comma - actPos - 7));
                    }
                    std::string st = ZyxelScanner::trim(lower.substr(stPos + 7));
                    currentRule.active = (st == "yes" || st == "active" || st == "true");
                } else {
                    currentRule.action = ZyxelScanner::trim(line.substr(actPos + 7));
                }
            }
        }

        if (inRule) {
            out.push_back(currentRule);
        }
    }

    if (out.empty()) {
        ZyxelScanner::logParseError("ZyxelFirewallCmd", "show secure-policy", 0,
                                   "No firewall policies found",
                                   lines.empty() ? "" : lines[0]);
        return false;
    }
    return true;
}

bool ZyxelFirewallCmd::parseSecurePolicyStatus(const std::string &raw,
                                              ZyxelSecurePolicyStatus &out) {
    out = ZyxelSecurePolicyStatus();
    auto lines = ZyxelScanner::splitLines(raw);
    if (lines.empty()) {
        return false;
    }

    bool foundAny = false;
    for (const auto &line : lines) {
        std::string key, val;
        if (ZyxelScanner::parseKeyValue(line, key, val, ':')) {
            std::string k = key;
            std::transform(k.begin(), k.end(), k.begin(), ::tolower);
            std::string v = val;
            std::transform(v.begin(), v.end(), v.begin(), ::tolower);

            if (k.find("asymmetrical route status") != std::string::npos) {
                out.asymmetricalRoute = (v == "yes" || v == "active" || v == "true");
                foundAny = true;
            } else if (k.find("secure-policy status") != std::string::npos || k == "status") {
                out.active = (v == "yes" || v == "active" || v == "true");
                foundAny = true;
            } else if (k.find("default rule") != std::string::npos) {
                out.defaultRule = val;
                foundAny = true;
            } else if (k.find("tcp flag detect") != std::string::npos) {
                out.tcpFlagDetect = (v == "yes" || v == "active" || v == "true");
                foundAny = true;
            }
        }
    }

    if (!foundAny) {
        ZyxelScanner::logParseError("ZyxelFirewallCmd", "show secure-policy status", 0,
                                   "No secure-policy status fields found",
                                   lines[0]);
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
