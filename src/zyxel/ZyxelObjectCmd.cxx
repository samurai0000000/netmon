/*
 * ZyxelObjectCmd.cxx
 *
 * Copyright (C) 2026, Charles Chiou
 */

#include "zyxel/ZyxelObjectCmd.hxx"
#include "zyxel/ZyxelScanner.hxx"

#include <algorithm>
#include <cctype>

std::string ZyxelObjectCmd::cmdAddAddressHost(const std::string &name, const std::string &ip) {
    return "address-object " + name + " " + ip;
}

std::string ZyxelObjectCmd::cmdAddAddressRange(const std::string &name, const std::string &ipStart,
                                              const std::string &ipEnd) {
    return "address-object " + name + " " + ipStart + "-" + ipEnd;
}

std::string ZyxelObjectCmd::cmdAddAddressSubnet(const std::string &name, const std::string &ip,
                                               const std::string &mask) {
    if (mask.find('/') == 0) {
        return "address-object " + name + " " + ip + mask;
    }
    return "address-object " + name + " " + ip + "/" + mask;
}

std::string ZyxelObjectCmd::cmdDeleteAddress(const std::string &name) {
    return "no address-object " + name;
}

std::string ZyxelObjectCmd::cmdShowAddressObjects(const std::string &name) {
    if (name.empty()) {
        return "show address-object";
    }
    return "show address-object " + name;
}

std::string ZyxelObjectCmd::cmdAddAddressGroupMember(const std::string &group, const std::string &member) {
    return "address-group " + group + " " + member;
}

std::string ZyxelObjectCmd::cmdDeleteAddressGroupMember(const std::string &group, const std::string &member) {
    return "no address-group " + group + " " + member;
}

std::string ZyxelObjectCmd::cmdShowAddressGroup(const std::string &name) {
    if (name.empty()) {
        return "show object-group address";
    }
    return "show object-group address " + name;
}

std::string ZyxelObjectCmd::cmdAddService(const std::string &name, const std::string &proto, int port) {
    return "service-object " + name + " " + proto + " eq " + std::to_string(port);
}

std::string ZyxelObjectCmd::cmdDeleteService(const std::string &name) {
    return "no service-object " + name;
}

std::string ZyxelObjectCmd::cmdShowServiceObjects(const std::string &name) {
    if (name.empty()) {
        return "show service-object";
    }
    return "show service-object " + name;
}

std::string ZyxelObjectCmd::cmdShowServiceGroup(const std::string &name) {
    if (name.empty()) {
        return "show object-group service";
    }
    return "show object-group service " + name;
}

bool ZyxelObjectCmd::parseAddressObjects(const std::string &raw,
                                        std::vector<ZyxelAddressObject> &out) {
    out.clear();
    auto lines = ZyxelScanner::splitLines(raw);
    int divIdx = ZyxelScanner::findDividerLine(lines);

    if (divIdx > 0) {
        auto cols = ZyxelScanner::parseTableColumns(lines[divIdx - 1], lines[divIdx]);
        int nameCol = ZyxelScanner::findColumn(cols, "name");
        int typeCol = ZyxelScanner::findColumn(cols, "type");
        int addrCol = ZyxelScanner::findColumn(cols, "address");
        int refCol = ZyxelScanner::findColumn(cols, "ref");

        for (size_t i = divIdx + 1; i < lines.size(); ++i) {
            const std::string &line = lines[i];
            std::string trimmed = ZyxelScanner::trim(line);
            if (trimmed.empty() || trimmed.find("====") == 0 || trimmed.find("----") == 0) {
                continue;
            }

            if (!cols.empty() && nameCol >= 0 && typeCol >= 0 && addrCol >= 0) {
                ZyxelAddressObject obj;
                obj.name = ZyxelScanner::extractCell(line, cols[nameCol]);
                obj.type = ZyxelScanner::extractCell(line, cols[typeCol]);
                std::string addr = ZyxelScanner::extractCell(line, cols[addrCol]);
                size_t dashPos = addr.find('-');
                size_t slashPos = addr.find('/');
                if (dashPos != std::string::npos) {
                    obj.ip = addr.substr(0, dashPos);
                    obj.secondaryIpOrMask = addr.substr(dashPos + 1);
                } else if (slashPos != std::string::npos) {
                    obj.ip = addr.substr(0, slashPos);
                    obj.secondaryIpOrMask = addr.substr(slashPos + 1);
                } else {
                    obj.ip = addr;
                }
                if (refCol >= 0) {
                    try {
                        obj.refCount = std::stoi(ZyxelScanner::extractCell(line, cols[refCol]));
                    } catch (...) {
                        obj.refCount = 0;
                    }
                }
                if (!obj.name.empty()) {
                    out.push_back(obj);
                }
            } else {
                auto tokens = ZyxelScanner::splitTokens(line);
                if (tokens.size() >= 3) {
                    ZyxelAddressObject obj;
                    obj.name = tokens[0];
                    obj.type = tokens[1];
                    std::string addr = tokens[2];
                    size_t dashPos = addr.find('-');
                    size_t slashPos = addr.find('/');
                    if (dashPos != std::string::npos) {
                        obj.ip = addr.substr(0, dashPos);
                        obj.secondaryIpOrMask = addr.substr(dashPos + 1);
                    } else if (slashPos != std::string::npos) {
                        obj.ip = addr.substr(0, slashPos);
                        obj.secondaryIpOrMask = addr.substr(slashPos + 1);
                    } else {
                        obj.ip = addr;
                    }
                    if (tokens.size() >= 4) {
                        try {
                            obj.refCount = std::stoi(tokens[3]);
                        } catch (...) {
                            obj.refCount = 0;
                        }
                    }
                    out.push_back(obj);
                }
            }
        }
    }

    // Token-based fallback across all lines (robust against missing/different divider lines)
    if (out.empty()) {
        for (const auto &line : lines) {
            std::string trimmed = ZyxelScanner::trim(line);
            if (trimmed.empty() || trimmed.find("====") == 0 || trimmed.find("----") == 0) {
                continue;
            }
            auto tokens = ZyxelScanner::splitTokens(trimmed);
            if (tokens.size() >= 3) {
                std::string typeUpper = tokens[1];
                std::transform(typeUpper.begin(), typeUpper.end(), typeUpper.begin(), ::toupper);
                if (typeUpper == "HOST" || typeUpper == "RANGE" || typeUpper == "SUBNET" ||
                    typeUpper == "INTERFACE" || typeUpper == "FQDN" || typeUpper == "MAC" ||
                    typeUpper == "IPV6") {
                    ZyxelAddressObject obj;
                    obj.name = tokens[0];
                    obj.type = typeUpper;
                    std::string addr = tokens[2];
                    size_t dashPos = addr.find('-');
                    size_t slashPos = addr.find('/');
                    if (dashPos != std::string::npos) {
                        obj.ip = addr.substr(0, dashPos);
                        obj.secondaryIpOrMask = addr.substr(dashPos + 1);
                    } else if (slashPos != std::string::npos) {
                        obj.ip = addr.substr(0, slashPos);
                        obj.secondaryIpOrMask = addr.substr(slashPos + 1);
                    } else {
                        obj.ip = addr;
                    }
                    if (tokens.size() >= 4) {
                        try { obj.refCount = std::stoi(tokens[3]); } catch (...) { obj.refCount = 0; }
                    }
                    out.push_back(obj);
                }
            }
        }
    }

    if (out.empty()) {
        ZyxelScanner::logParseError("ZyxelObjectCmd", "show address-object", 0,
                                   "No address objects found in table",
                                   lines.empty() ? "" : lines[0]);
        return false;
    }
    return true;
}

bool ZyxelObjectCmd::parseAddressGroups(const std::string &raw,
                                       std::vector<ZyxelAddressGroup> &out) {
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
                ZyxelAddressGroup group;
                group.name = tokens[0];
                for (size_t j = 1; j < tokens.size(); ++j) {
                    std::string m = tokens[j];
                    if (!m.empty() && m.back() == ',') {
                        m.pop_back();
                    }
                    if (!m.empty()) {
                        group.members.push_back(m);
                    }
                }
                out.push_back(group);
            }
        }
    } else {
        // Block format: "address-group: Blocked_Hosts", "member: host1"
        ZyxelAddressGroup currentGroup;
        for (const auto &line : lines) {
            std::string key, val;
            if (ZyxelScanner::parseKeyValue(line, key, val, ':')) {
                std::string k = key;
                std::transform(k.begin(), k.end(), k.begin(), ::tolower);
                if (k.find("group") != std::string::npos) {
                    if (!currentGroup.name.empty()) {
                        out.push_back(currentGroup);
                        currentGroup = ZyxelAddressGroup();
                    }
                    currentGroup.name = val;
                } else if (k == "member" || k == "members") {
                    currentGroup.members.push_back(val);
                }
            }
        }
        if (!currentGroup.name.empty()) {
            out.push_back(currentGroup);
        }
    }

    if (out.empty()) {
        ZyxelScanner::logParseError("ZyxelObjectCmd", "show address-group", 0,
                                   "No address groups found",
                                   lines.empty() ? "" : lines[0]);
        return false;
    }
    return true;
}

bool ZyxelObjectCmd::parseServiceObjects(const std::string &raw,
                                        std::vector<ZyxelServiceObject> &out) {
    out.clear();
    auto lines = ZyxelScanner::splitLines(raw);
    int divIdx = ZyxelScanner::findDividerLine(lines);

    if (divIdx > 0) {
        auto cols = ZyxelScanner::parseTableColumns(lines[divIdx - 1], lines[divIdx]);
        int nameCol = ZyxelScanner::findColumn(cols, "name");
        int protoCol = ZyxelScanner::findColumn(cols, "protocol");
        int portCol = ZyxelScanner::findColumn(cols, "port");
        int refCol = ZyxelScanner::findColumn(cols, "ref");

        for (size_t i = divIdx + 1; i < lines.size(); ++i) {
            const std::string &line = lines[i];
            std::string trimmed = ZyxelScanner::trim(line);
            if (trimmed.empty() || trimmed.find("====") == 0 || trimmed.find("----") == 0) {
                continue;
            }

            if (!cols.empty() && nameCol >= 0 && protoCol >= 0 && portCol >= 0) {
                ZyxelServiceObject obj;
                obj.name = ZyxelScanner::extractCell(line, cols[nameCol]);
                obj.protocol = ZyxelScanner::extractCell(line, cols[protoCol]);
                std::string portStr = ZyxelScanner::extractCell(line, cols[portCol]);
                size_t dashPos = portStr.find('-');
                if (dashPos != std::string::npos) {
                    try {
                        obj.portStart = std::stoi(portStr.substr(0, dashPos));
                        obj.portEnd = std::stoi(portStr.substr(dashPos + 1));
                    } catch (...) {
                        obj.portStart = obj.portEnd = 0;
                    }
                } else {
                    try {
                        obj.portStart = obj.portEnd = std::stoi(portStr);
                    } catch (...) {
                        obj.portStart = obj.portEnd = 0;
                    }
                }
                if (refCol >= 0) {
                    try {
                        obj.refCount = std::stoi(ZyxelScanner::extractCell(line, cols[refCol]));
                    } catch (...) {
                        obj.refCount = 0;
                    }
                }
                if (!obj.name.empty()) {
                    out.push_back(obj);
                }
            } else {
                auto tokens = ZyxelScanner::splitTokens(line);
                if (tokens.size() >= 3) {
                    ZyxelServiceObject obj;
                    obj.name = tokens[0];
                    obj.protocol = tokens[1];
                    std::string portStr = tokens[2];
                    size_t dashPos = portStr.find('-');
                    if (dashPos != std::string::npos) {
                        try {
                            obj.portStart = std::stoi(portStr.substr(0, dashPos));
                            obj.portEnd = std::stoi(portStr.substr(dashPos + 1));
                        } catch (...) {
                            obj.portStart = obj.portEnd = 0;
                        }
                    } else {
                        try {
                            obj.portStart = obj.portEnd = std::stoi(portStr);
                        } catch (...) {
                            obj.portStart = obj.portEnd = 0;
                        }
                    }

                    if (tokens.size() >= 4) {
                        try {
                            obj.refCount = std::stoi(tokens[3]);
                        } catch (...) {
                            obj.refCount = 0;
                        }
                    }
                    out.push_back(obj);
                }
            }
        }
    }

    if (out.empty()) {
        ZyxelScanner::logParseError("ZyxelObjectCmd", "show service-object", 0,
                                   "No service objects found in table",
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
