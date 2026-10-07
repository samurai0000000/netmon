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

std::string ZyxelObjectCmd::cmdShowScheduleObjects(const std::string &name) {
    if (name.empty()) {
        return "show schedule-object";
    }
    return "show schedule-object " + name;
}

std::string ZyxelObjectCmd::cmdShowScheduleRecurring() {
    return "show schedule-object recurring";
}

std::string ZyxelObjectCmd::cmdShowScheduleOneTime() {
    return "show schedule-object one-time";
}

std::string ZyxelObjectCmd::cmdAddScheduleOneTime(const std::string &name,
                                                const std::string &startDate,
                                                const std::string &startTime,
                                                const std::string &endDate,
                                                const std::string &endTime) {
    return "schedule-object " + name + " " + startDate + " " + startTime + " " +
           endDate + " " + endTime;
}

std::string ZyxelObjectCmd::cmdDeleteSchedule(const std::string &name) {
    return "no schedule-object " + name;
}

std::string ZyxelObjectCmd::cmdAddScheduleGroup(const std::string &name) {
    return "object-group schedule " + name;
}

std::string ZyxelObjectCmd::cmdDeleteScheduleGroup(const std::string &name) {
    return "no object-group schedule " + name;
}

std::string ZyxelObjectCmd::cmdShowAddress6Objects(const std::string &name) {
    if (name.empty()) {
        return "show address6-object";
    }
    return "show address6-object " + name;
}

std::string ZyxelObjectCmd::cmdAddAddress6Host(const std::string &name, const std::string &ipv6) {
    return "address6-object " + name + " " + ipv6;
}

std::string ZyxelObjectCmd::cmdAddAddress6Subnet(const std::string &name, const std::string &ipv6,
                                                int prefixLen) {
    return "address6-object " + name + " " + ipv6 + "/" + std::to_string(prefixLen);
}

std::string ZyxelObjectCmd::cmdAddAddress6Range(const std::string &name,
                                               const std::string &startIpv6,
                                               const std::string &endIpv6) {
    return "address6-object " + name + " " + startIpv6 + "-" + endIpv6;
}

std::string ZyxelObjectCmd::cmdDeleteAddress6(const std::string &name) {
    return "no address6-object " + name;
}

std::string ZyxelObjectCmd::cmdShowAddress6Group(const std::string &name) {
    if (name.empty()) {
        return "show object-group address6";
    }
    return "show object-group address6 " + name;
}

std::string ZyxelObjectCmd::cmdAddAddress6Group(const std::string &name) {
    return "object-group address6 " + name;
}

std::string ZyxelObjectCmd::cmdDeleteAddress6Group(const std::string &name) {
    return "no object-group address6 " + name;
}

std::string ZyxelObjectCmd::cmdShowFqdn() {
    return "show fqdn";
}

std::string ZyxelObjectCmd::cmdShowFqdnObjects() {
    return "show fqdn-object all";
}

std::string ZyxelObjectCmd::cmdShowFqdnQueryPeriod() {
    return "show fqdn-object query-period";
}

std::string ZyxelObjectCmd::cmdShowFqdnSyncPeriod() {
    return "show fqdn-object sync-period";
}

std::string ZyxelObjectCmd::cmdShowGeoIpCountryCode() {
    return "show geo-ip country-code";
}

std::string ZyxelObjectCmd::cmdShowGeoIpDatabaseUpdate() {
    return "show geo-ip database update";
}

std::string ZyxelObjectCmd::cmdShowGeoIpDatabaseVersion() {
    return "show geo-ip database version";
}

std::string ZyxelObjectCmd::cmdAddServiceGroup(const std::string &name) {
    return "object-group service " + name;
}

std::string ZyxelObjectCmd::cmdDeleteServiceGroup(const std::string &name) {
    return "no object-group service " + name;
}

std::string ZyxelObjectCmd::cmdShowAccountPppoe() {
    return "show account pppoe";
}

std::string ZyxelObjectCmd::cmdShowAccountPptp() {
    return "show account pptp";
}

std::string ZyxelObjectCmd::cmdShowAccountCellular() {
    return "show account cellular";
}

std::string ZyxelObjectCmd::cmdShowSslvpnApplication() {
    return "show sslvpn application";
}

std::string ZyxelObjectCmd::cmdShowDhcp6Interface() {
    return "show dhcp6 interface";
}

std::string ZyxelObjectCmd::cmdShowDhcp6LeaseObjects() {
    return "show dhcp6 lease-object";
}

std::string ZyxelObjectCmd::cmdShowDhcp6RequestObjects() {
    return "show dhcp6 request-object";
}

std::string ZyxelObjectCmd::cmdShowIpv6Dhcp6Bindings() {
    return "show ipv6 dhcp6 binding";
}

std::string ZyxelObjectCmd::cmdInvalidAddressObjectDryFire() {
    return "show address-object NETMON_NONEXISTENT_ADDR_999";
}

std::string ZyxelObjectCmd::cmdInvalidAddress6ObjectDryFire() {
    return "show address6-object NETMON_NONEXISTENT_ADDR6_999";
}

std::string ZyxelObjectCmd::cmdInvalidServiceObjectDryFire() {
    return "show service-object NETMON_NONEXISTENT_SVC_999";
}

std::string ZyxelObjectCmd::cmdInvalidScheduleObjectDryFire() {
    return "show schedule-object NETMON_NONEXISTENT_SCHED_999";
}

bool ZyxelObjectCmd::parseAddress6Objects(const std::string &raw,
                                         std::vector<ZyxelAddress6Object> &out) {
    out.clear();
    auto lines = ZyxelScanner::splitLines(raw);
    int divIdx = ZyxelScanner::findDividerLine(lines);
    if (divIdx < 0) {
        if (raw.find("ERROR:") != std::string::npos || raw.find("retval =") != std::string::npos) {
            return false;
        }
        return false;
    }

    ZyxelAddress6Object current;
    bool inObject = false;

    for (size_t i = divIdx + 1; i < lines.size(); ++i) {
        const std::string &line = lines[i];
        std::string trimmed = ZyxelScanner::trim(line);
        if (trimmed.empty() || trimmed.find("====") == 0 || trimmed.find("----") == 0) {
            continue;
        }
        if (trimmed.find("Router") == 0) {
            break;
        }

        auto tokens = ZyxelScanner::splitTokens(trimmed);
        if (tokens.empty()) {
            continue;
        }

        if (tokens[0].find(':') != std::string::npos) {
            if (inObject) {
                current.address = tokens[0];
            }
        } else if (tokens.size() >= 1 &&
                   (tokens.back().find_first_not_of("0123456789") == std::string::npos &&
                    (tokens.size() == 1 || tokens[0] == "lan1" || tokens[0] == "lan2" ||
                     tokens[0] == "dmz" || tokens[0] == "wan1" || tokens[0] == "wan2"))) {
            if (inObject) {
                try {
                    current.refCount = std::stoi(tokens.back());
                } catch (...) {
                    current.refCount = 0;
                }
            }
        } else {
            if (inObject && !current.name.empty()) {
                out.push_back(current);
                current = ZyxelAddress6Object();
            }
            inObject = true;
            current.name = tokens[0];
            if (tokens.size() >= 2) {
                current.type = tokens[1];
                if (tokens.size() >= 3 &&
                    (tokens[1] == "INTERFACE" || tokens[1] == "interface")) {
                    current.type += " " + tokens[2];
                }
            }
        }
    }
    if (inObject && !current.name.empty()) {
        out.push_back(current);
    }
    return true;
}

bool ZyxelObjectCmd::parseScheduleObjects(const std::string &raw,
                                         std::vector<ZyxelScheduleObject> &out) {
    out.clear();
    auto lines = ZyxelScanner::splitLines(raw);
    int divIdx = ZyxelScanner::findDividerLine(lines);
    if (divIdx < 0) {
        if (raw.find("ERROR:") != std::string::npos || raw.find("retval =") != std::string::npos) {
            return false;
        }
        return false;
    }

    auto cols = ZyxelScanner::parseTableColumns(lines[divIdx - 1], lines[divIdx]);
    int nameCol = ZyxelScanner::findColumn(cols, "name");
    int typeCol = ZyxelScanner::findColumn(cols, "type");
    int startEndCol = ZyxelScanner::findColumn(cols, "start/end");
    int refCol = ZyxelScanner::findColumn(cols, "ref");

    for (size_t i = divIdx + 1; i < lines.size(); ++i) {
        const std::string &line = lines[i];
        std::string trimmed = ZyxelScanner::trim(line);
        if (trimmed.empty() || trimmed.find("====") == 0 || trimmed.find("----") == 0) {
            continue;
        }
        if (trimmed.find("Router") == 0) {
            break;
        }

        if (!cols.empty() && nameCol >= 0) {
            ZyxelScheduleObject obj;
            obj.name = ZyxelScanner::extractCell(line, cols[nameCol]);
            if (typeCol >= 0) {
                obj.type = ZyxelScanner::extractCell(line, cols[typeCol]);
            }
            if (startEndCol >= 0) {
                obj.startEnd = ZyxelScanner::extractCell(line, cols[startEndCol]);
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
            auto tokens = ZyxelScanner::splitTokens(trimmed);
            if (tokens.size() >= 2) {
                ZyxelScheduleObject obj;
                obj.name = tokens[0];
                obj.type = tokens[1];
                if (tokens.size() >= 3) {
                    obj.startEnd = tokens[2];
                }
                if (tokens.size() >= 4) {
                    try { obj.refCount = std::stoi(tokens[3]); } catch (...) { obj.refCount = 0; }
                }
                out.push_back(obj);
            }
        }
    }
    return true;
}

bool ZyxelObjectCmd::parseServiceGroups(const std::string &raw,
                                       std::vector<ZyxelServiceGroup> &out) {
    out.clear();
    auto lines = ZyxelScanner::splitLines(raw);
    int divIdx = ZyxelScanner::findDividerLine(lines);
    if (divIdx < 0) {
        if (raw.find("ERROR:") != std::string::npos || raw.find("retval =") != std::string::npos) {
            return false;
        }
        return false;
    }

    std::string header = (divIdx > 0) ? lines[divIdx - 1] : "";
    if (header.find("Object/Group name") != std::string::npos) {
        ZyxelServiceGroup group;
        group.name = "group";
        for (size_t i = divIdx + 1; i < lines.size(); ++i) {
            std::string trimmed = ZyxelScanner::trim(lines[i]);
            if (trimmed.empty() || trimmed.find("====") == 0 || trimmed.find("----") == 0) {
                continue;
            }
            if (trimmed.find("Router") == 0) {
                break;
            }
            auto tokens = ZyxelScanner::splitTokens(trimmed);
            if (!tokens.empty()) {
                group.members.push_back(tokens[0]);
            }
        }
        out.push_back(group);
        return true;
    }

    for (size_t i = divIdx + 1; i < lines.size(); ++i) {
        std::string trimmed = ZyxelScanner::trim(lines[i]);
        if (trimmed.empty() || trimmed.find("====") == 0 || trimmed.find("----") == 0) {
            continue;
        }
        if (trimmed.find("Router") == 0) {
            break;
        }
        auto tokens = ZyxelScanner::splitTokens(trimmed);
        if (!tokens.empty()) {
            ZyxelServiceGroup group;
            group.name = tokens[0];
            out.push_back(group);
        }
    }
    return true;
}

bool ZyxelObjectCmd::parseAccountPppoe(const std::string &raw,
                                      std::vector<ZyxelAccountPppoeEntry> &out) {
    out.clear();
    if (raw.find("ERROR:") != std::string::npos || raw.find("retval =") != std::string::npos) {
        return false;
    }

    auto lines = ZyxelScanner::splitLines(raw);
    ZyxelAccountPppoeEntry current;
    bool inProfile = false;

    for (const auto &line : lines) {
        std::string key, val;
        if (ZyxelScanner::parseKeyValue(line, key, val, ':')) {
            std::string k = key;
            std::transform(k.begin(), k.end(), k.begin(), ::tolower);
            if (k == "profile name") {
                if (inProfile && !current.profileName.empty()) {
                    out.push_back(current);
                    current = ZyxelAccountPppoeEntry();
                }
                inProfile = true;
                current.profileName = val;
            } else if (inProfile) {
                if (k == "protocol") {
                    current.protocol = val;
                } else if (k == "username") {
                    current.username = val;
                } else if (k == "authentication type") {
                    current.authType = val;
                } else if (k == "service name") {
                    current.serviceName = val;
                } else if (k == "compression") {
                    std::string v = val;
                    std::transform(v.begin(), v.end(), v.begin(), ::tolower);
                    current.compression = (v == "yes" || v == "true");
                } else if (k == "idle timeout") {
                    try {
                        current.idleTimeout = std::stoi(val);
                    } catch (...) {
                        current.idleTimeout = 0;
                    }
                }
            }
        }
    }
    if (inProfile && !current.profileName.empty()) {
        out.push_back(current);
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
