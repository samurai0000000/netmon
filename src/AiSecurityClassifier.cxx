/*
 * AiSecurityClassifier.cxx
 *
 * Copyright (C) 2026, Charles Chiou
 */

#include "AiSecurityClearance.hxx"

#include <string>
#include <vector>
#include <cctype>
#include <arpa/inet.h>

std::string clearanceLevelToString(ClearanceLevel level) {
    switch (level) {
    case ClearanceLevel::LEVEL3: return "LEVEL3";
    case ClearanceLevel::LEVEL2: return "LEVEL2";
    case ClearanceLevel::LEVEL1: return "LEVEL1";
    default:                     return "UNKNOWN";
    }
}

std::string clearanceTierToString(ClearanceTier tier) {
    switch (tier) {
    case ClearanceTier::NONE:                return "NONE";
    case ClearanceTier::READ:                return "READ";
    case ClearanceTier::READ_WRITE:          return "READ/WRITE";
    case ClearanceTier::READ_WRITE_PASSWORD: return "READ/WRITE + PASSWORD ACCESS!!!";
    default:                                 return "UNKNOWN";
    }
}

std::string clearanceResultToString(ClearanceResult result) {
    switch (result) {
    case ClearanceResult::OK:             return "OK";
    case ClearanceResult::NOT_AUTHORIZED: return "NOT_AUTHORIZED";
    case ClearanceResult::FORBIDDEN:      return "FORBIDDEN";
    case ClearanceResult::UNCLASSIFIED:   return "UNCLASSIFIED";
    case ClearanceResult::BUSY:           return "BUSY";
    case ClearanceResult::CANCELED:       return "CANCELED";
    case ClearanceResult::TIMEOUT:        return "TIMEOUT";
    case ClearanceResult::LOCKED:         return "LOCKED";
    case ClearanceResult::SYNTAX:         return "SYNTAX";
    case ClearanceResult::DISCONNECTED:   return "DISCONNECTED";
    case ClearanceResult::FAILED:         return "FAILED";
    default:                              return "FAILED";
    }
}

ClearanceResult stringToClearanceResult(const std::string &s) {
    if (s == "OK")             return ClearanceResult::OK;
    if (s == "NOT_AUTHORIZED") return ClearanceResult::NOT_AUTHORIZED;
    if (s == "FORBIDDEN")      return ClearanceResult::FORBIDDEN;
    if (s == "UNCLASSIFIED")   return ClearanceResult::UNCLASSIFIED;
    if (s == "BUSY")           return ClearanceResult::BUSY;
    if (s == "CANCELED")       return ClearanceResult::CANCELED;
    if (s == "TIMEOUT")        return ClearanceResult::TIMEOUT;
    if (s == "LOCKED")         return ClearanceResult::LOCKED;
    if (s == "SYNTAX")         return ClearanceResult::SYNTAX;
    if (s == "DISCONNECTED")   return ClearanceResult::DISCONNECTED;
    return ClearanceResult::FAILED;
}

// Tokenizes a line by space while preserving consecutive non-space characters
std::vector<std::string> AiSecurityClassifier::tokenize(const std::string &line) {
    std::vector<std::string> tokens;
    std::string cur;
    for (char c : line) {
        if (std::isspace(static_cast<unsigned char>(c))) {
            if (!cur.empty()) {
                tokens.push_back(cur);
                cur.clear();
            }
        } else {
            cur += c;
        }
    }
    if (!cur.empty()) {
        tokens.push_back(cur);
    }
    return tokens;
}

bool AiSecurityClassifier::isControlOrChained(const std::string &line) {
    for (char c : line) {
        // Reject control characters (0-31 except space, tab), DEL (127), and shell metacharacters
        unsigned char uc = static_cast<unsigned char>(c);
        if (uc < 32 || uc == 127) {
            return true;
        }
        if (c == ';' || c == '&' || c == '|' || c == '`' || c == '$' || c == '<' || c == '>') {
            return true;
        }
    }
    return false;
}

bool AiSecurityClassifier::isNameValid(const std::string &name) {
    if (name.empty() || name.length() > 31) {
        return false;
    }
    for (char c : name) {
        if (!std::isalnum(static_cast<unsigned char>(c)) && c != '_') {
            return false;
        }
    }
    return true;
}

bool AiSecurityClassifier::isIpv4Valid(const std::string &ip) {
    if (ip.empty() || ip.length() > 15) {
        return false;
    }
    struct in_addr addr;
    if (inet_pton(AF_INET, ip.c_str(), &addr) != 1) {
        return false;
    }
    // Verify no leading zeros or weird formatting by re-formatting
    char buf[INET_ADDRSTRLEN];
    if (inet_ntop(AF_INET, &addr, buf, sizeof(buf)) == nullptr) {
        return false;
    }
    return (ip == std::string(buf));
}

bool AiSecurityClassifier::isPortValid(const std::string &portStr) {
    if (portStr.empty() || portStr.length() > 5) {
        return false;
    }
    for (char c : portStr) {
        if (!std::isdigit(static_cast<unsigned char>(c))) {
            return false;
        }
    }
    try {
        unsigned long v = std::stoul(portStr);
        return (v >= 1 && v <= 65535);
    } catch (...) {
        return false;
    }
}

bool AiSecurityClassifier::isPositionValid(const std::string &posStr) {
    if (posStr.empty() || posStr.length() > 4) {
        return false;
    }
    for (char c : posStr) {
        if (!std::isdigit(static_cast<unsigned char>(c))) {
            return false;
        }
    }
    try {
        unsigned long v = std::stoul(posStr);
        return (v >= 1 && v <= 9999);
    } catch (...) {
        return false;
    }
}

bool AiSecurityClassifier::isMetricValid(const std::string &metricStr) {
    if (metricStr.empty() || metricStr.length() > 3) {
        return false;
    }
    for (char c : metricStr) {
        if (!std::isdigit(static_cast<unsigned char>(c))) {
            return false;
        }
    }
    try {
        unsigned long v = std::stoul(metricStr);
        return (v <= 127);
    } catch (...) {
        return false;
    }
}

bool AiSecurityClassifier::isProtocolValid(const std::string &protoStr) {
    return (protoStr == "tcp" || protoStr == "udp" || protoStr == "icmp");
}

bool AiSecurityClassifier::isMaskValid(const std::string &maskStr) {
    if (maskStr.empty()) {
        return false;
    }
    // Prefix length: 0 to 32
    bool allDigits = true;
    for (char c : maskStr) {
        if (!std::isdigit(static_cast<unsigned char>(c))) {
            allDigits = false;
            break;
        }
    }
    if (allDigits && maskStr.length() <= 2) {
        try {
            unsigned long p = std::stoul(maskStr);
            if (p <= 32) {
                return true;
            }
        } catch (...) {
        }
    }
    // Dotted quad netmask
    if (isIpv4Valid(maskStr)) {
        return true;
    }
    return false;
}

bool AiSecurityClassifier::isRootPrompt(const std::string &prompt) {
    if (prompt.empty()) {
        return true;
    }
    if (prompt.find("(config") != std::string::npos) {
        return false;
    }
    char last = prompt.back();
    return (last == '>' || last == '#');
}

bool AiSecurityClassifier::isSecurePolicySubmode(const std::string &prompt) {
    return (prompt.find("(config-secure-policy") != std::string::npos ||
            prompt.find("(config-submode") != std::string::npos);
}

LineClassification AiSecurityClassifier::classify(const std::string &line,
                                                 const std::string &currentPrompt,
                                                 int &timeoutMsOut,
                                                 std::string &matchedMethodOut) {
    timeoutMsOut = 5000;
    matchedMethodOut = "UNCLASSIFIED";

    if (line.empty() || isControlOrChained(line)) {
        return LineClassification::UNCLASSIFIED;
    }

    auto tokens = tokenize(line);
    if (tokens.empty()) {
        return LineClassification::UNCLASSIFIED;
    }

    // -------------------------------------------------------------
    // Level 1 Refused Commands (Section 3.3)
    // -------------------------------------------------------------
    if (tokens.size() == 1) {
        if (tokens[0] == "write") {
            matchedMethodOut = "cmdWrite";
            return LineClassification::LEVEL1;
        }
        if (tokens[0] == "reboot") {
            matchedMethodOut = "cmdReboot";
            return LineClassification::LEVEL1;
        }
    }

    bool inSubmode = isSecurePolicySubmode(currentPrompt);

    // -------------------------------------------------------------
    // Level 2 Submode Fragments (Section 3.2 & 3.3)
    // Only accepted while in secure-policy submode
    // -------------------------------------------------------------
    if (inSubmode) {
        if (tokens.size() == 1 && tokens[0] == "exit") {
            matchedMethodOut = "cmdInsertRule";
            return LineClassification::LEVEL2_SUBMODE;
        }
        if (tokens.size() == 1 && (tokens[0] == "activate" || tokens[0] == "deactivate")) {
            matchedMethodOut = "cmdInsertRule";
            return LineClassification::LEVEL2_SUBMODE;
        }
        if (tokens.size() == 2) {
            const std::string &verb = tokens[0];
            const std::string &arg = tokens[1];
            if (verb == "name" && isNameValid(arg)) {
                matchedMethodOut = "cmdInsertRule";
                return LineClassification::LEVEL2_SUBMODE;
            }
            if (verb == "from" && isNameValid(arg)) {
                matchedMethodOut = "cmdInsertRule";
                return LineClassification::LEVEL2_SUBMODE;
            }
            if (verb == "to" && isNameValid(arg)) {
                matchedMethodOut = "cmdInsertRule";
                return LineClassification::LEVEL2_SUBMODE;
            }
            if (verb == "sourceip" && isNameValid(arg)) {
                matchedMethodOut = "cmdInsertRule";
                return LineClassification::LEVEL2_SUBMODE;
            }
            if (verb == "destinationip" && isNameValid(arg)) {
                matchedMethodOut = "cmdInsertRule";
                return LineClassification::LEVEL2_SUBMODE;
            }
            if (verb == "service" && isNameValid(arg)) {
                matchedMethodOut = "cmdInsertRule";
                return LineClassification::LEVEL2_SUBMODE;
            }
            if (verb == "action" && (arg == "allow" || arg == "deny")) {
                matchedMethodOut = "cmdInsertRule";
                return LineClassification::LEVEL2_SUBMODE;
            }
        }
        if (tokens[0] == "description" && tokens.size() >= 2) {
            // Description is up to 60 characters total, no control characters
            std::string desc = line.substr(line.find("description") + 11);
            while (!desc.empty() && std::isspace(static_cast<unsigned char>(desc.front()))) {
                desc.erase(0, 1);
            }
            if (!desc.empty() && desc.length() <= 60 && !isControlOrChained(desc)) {
                matchedMethodOut = "cmdInsertRule";
                return LineClassification::LEVEL2_SUBMODE;
            }
        }
        // When in submode, root commands or other lines are rejected
        return LineClassification::UNCLASSIFIED;
    }

    // -------------------------------------------------------------
    // Level 3 Read and Diagnostics (Section 3.3)
    // -------------------------------------------------------------
    // -------------------------------------------------------------
    // Level 3 Read and Diagnostics (Section 3.3 & Expanded Catalog)
    // -------------------------------------------------------------
    if (tokens[0] == "show") {
        if (tokens.size() < 2) {
            return LineClassification::UNCLASSIFIED;
        }

        // Specific syntax rejections per router grammar
        if (tokens[1] == "address-group") {
            return LineClassification::UNCLASSIFIED;
        }

        // Check for specific legacy shapes and exact method names first
        if (tokens.size() == 2) {
            if (tokens[1] == "version") {
                matchedMethodOut = "cmdShowVersion";
                return LineClassification::LEVEL3;
            }
            if (tokens[1] == "zone") {
                matchedMethodOut = "cmdShowZones";
                return LineClassification::LEVEL3;
            }
            if (tokens[1] == "arp-table") {
                matchedMethodOut = "cmdShowArp";
                return LineClassification::LEVEL3;
            }
            if (tokens[1] == "address-object") {
                matchedMethodOut = "cmdShowAddressObjects";
                return LineClassification::LEVEL3;
            }
            if (tokens[1] == "service-object") {
                timeoutMsOut = 15000;
                matchedMethodOut = "cmdShowServiceObjects";
                return LineClassification::LEVEL3;
            }
            if (tokens[1] == "secure-policy") {
                timeoutMsOut = 15000;
                matchedMethodOut = "cmdShowSecurePolicy";
                return LineClassification::LEVEL3;
            }
        } else if (tokens.size() == 3) {
            if (tokens[1] == "cpu" && tokens[2] == "status") {
                matchedMethodOut = "cmdShowCpuStatus";
                return LineClassification::LEVEL3;
            }
            if (tokens[1] == "mem" && tokens[2] == "status") {
                matchedMethodOut = "cmdShowMemStatus";
                return LineClassification::LEVEL3;
            }
            if (tokens[1] == "conn" && tokens[2] == "status") {
                matchedMethodOut = "cmdShowConnStatus";
                return LineClassification::LEVEL3;
            }
            if (tokens[1] == "interface") {
                if (isNameValid(tokens[2])) {
                    matchedMethodOut = "cmdShowInterface";
                    return LineClassification::LEVEL3;
                }
                return LineClassification::UNCLASSIFIED;
            }
            if (tokens[1] == "interfaces" && tokens[2] == "status") {
                matchedMethodOut = "cmdShowInterfacesStatus";
                return LineClassification::LEVEL3;
            }
            if (tokens[1] == "interfaces" && tokens[2] == "detail") {
                matchedMethodOut = "cmdShowInterfacesDetail";
                return LineClassification::LEVEL3;
            }
            if (tokens[1] == "ip" && tokens[2] == "route") {
                matchedMethodOut = "cmdShowIpRoutes";
                return LineClassification::LEVEL3;
            }
            if (tokens[1] == "address-object") {
                if (isNameValid(tokens[2])) {
                    matchedMethodOut = "cmdShowAddressObjects";
                    return LineClassification::LEVEL3;
                }
                return LineClassification::UNCLASSIFIED;
            }
            if (tokens[1] == "service-object") {
                if (isNameValid(tokens[2])) {
                    timeoutMsOut = 15000;
                    matchedMethodOut = "cmdShowServiceObjects";
                    return LineClassification::LEVEL3;
                }
                return LineClassification::UNCLASSIFIED;
            }
            if (tokens[1] == "object-group" && tokens[2] == "address") {
                matchedMethodOut = "cmdShowAddressGroup";
                return LineClassification::LEVEL3;
            }
            if (tokens[1] == "object-group" && tokens[2] == "service") {
                matchedMethodOut = "cmdShowServiceGroup";
                return LineClassification::LEVEL3;
            }
            if (tokens[1] == "secure-policy") {
                if (isPositionValid(tokens[2]) || isNameValid(tokens[2])) {
                    matchedMethodOut = "cmdShowSecurePolicy";
                    return LineClassification::LEVEL3;
                }
                return LineClassification::UNCLASSIFIED;
            }
            if (tokens[1] == "ip" && tokens[2] == "route-settings") {
                matchedMethodOut = "cmdShowIpRouteSettings";
                return LineClassification::LEVEL3;
            }
            if (tokens[1] == "ip" && tokens[2] == "virtual-server") {
                timeoutMsOut = 15000;
                matchedMethodOut = "cmdShowVirtualServers";
                return LineClassification::LEVEL3;
            }
        } else if (tokens.size() == 4) {
            if (tokens[1] == "interface" && tokens[2] == "summary" && tokens[3] == "all") {
                matchedMethodOut = "cmdShowInterfaces";
                return LineClassification::LEVEL3;
            }
            if (tokens[1] == "app" && tokens[2] == "statistics" && tokens[3] == "summary") {
                timeoutMsOut = 15000;
                matchedMethodOut = "cmdShowAppStatisticsSummary";
                return LineClassification::LEVEL3;
            }
            if (tokens[1] == "idp" && tokens[2] == "statistics" && tokens[3] == "summary") {
                timeoutMsOut = 15000;
                matchedMethodOut = "cmdShowIdpStatisticsSummary";
                return LineClassification::LEVEL3;
            }
            if (tokens[1] == "object-group" && tokens[2] == "address") {
                if (isNameValid(tokens[3])) {
                    matchedMethodOut = "cmdShowAddressGroup";
                    return LineClassification::LEVEL3;
                }
                return LineClassification::UNCLASSIFIED;
            }
            if (tokens[1] == "object-group" && tokens[2] == "service") {
                if (isNameValid(tokens[3])) {
                    matchedMethodOut = "cmdShowServiceGroup";
                    return LineClassification::LEVEL3;
                }
                return LineClassification::UNCLASSIFIED;
            }
            if (tokens[1] == "ip" && tokens[2] == "virtual-server") {
                if (isNameValid(tokens[3])) {
                    timeoutMsOut = 15000;
                    matchedMethodOut = "cmdShowVirtualServers";
                    return LineClassification::LEVEL3;
                }
                return LineClassification::UNCLASSIFIED;
            }
        }

        // Generalized validation for remaining read-only show commands across all 10 envelopes
        for (size_t i = 1; i < tokens.size(); ++i) {
            const std::string &tok = tokens[i];
            if (tok.empty() || tok.length() > 64) {
                return LineClassification::UNCLASSIFIED;
            }
            if (tok.find("..") != std::string::npos || tok.front() == '/') {
                return LineClassification::UNCLASSIFIED;
            }
            for (char c : tok) {
                if (!std::isalnum(static_cast<unsigned char>(c)) &&
                    c != '_' && c != '-' && c != '.' && c != '/') {
                    return LineClassification::UNCLASSIFIED;
                }
            }
        }

        // Formulate method name and timeout
        std::string method = "cmdShow";
        bool cap = true;
        for (char c : tokens[1]) {
            if (c == '-' || c == '_') {
                cap = true;
            } else if (cap) {
                method += static_cast<char>(std::toupper(static_cast<unsigned char>(c)));
                cap = false;
            } else {
                method += static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
            }
        }
        matchedMethodOut = method;

        if (tokens[1] == "running-config" || tokens[1] == "startup-config" ||
            tokens[1] == "logging" || tokens[1] == "crypto" || tokens[1] == "app" ||
            tokens[1] == "idp" || tokens[1] == "anti-spam" || tokens[1] == "anti-virus") {
            timeoutMsOut = 15000;
        } else {
            timeoutMsOut = 5000;
        }
        return LineClassification::LEVEL3;
    }

    if (tokens[0] == "ping") {
        if (tokens.size() == 4 && isIpv4Valid(tokens[1]) && tokens[2] == "count") {
            try {
                int count = std::stoi(tokens[3]);
                if (count >= 1 && count <= 20) {
                    timeoutMsOut = 10000 + (count * 2000);
                    matchedMethodOut = "cmdPing";
                    return LineClassification::LEVEL3;
                }
            } catch (...) {
            }
        }
        if (tokens.size() == 2 && isIpv4Valid(tokens[1])) {
            timeoutMsOut = 10000;
            matchedMethodOut = "cmdPing";
            return LineClassification::LEVEL3;
        }
    }

    if (tokens[0] == "traceroute") {
        if (tokens.size() == 2 && isIpv4Valid(tokens[1])) {
            timeoutMsOut = 60000;
            matchedMethodOut = "cmdTraceroute";
            return LineClassification::LEVEL3;
        }
    }

    // -------------------------------------------------------------
    // Level 2 Root Lines (Section 3.3)
    // -------------------------------------------------------------
    if (tokens[0] == "no") {
        if (tokens.size() == 3) {
            if (tokens[1] == "address-object" && isNameValid(tokens[2])) {
                matchedMethodOut = "cmdDeleteAddress";
                return LineClassification::LEVEL2_ROOT;
            }
            if (tokens[1] == "service-object" && isNameValid(tokens[2])) {
                matchedMethodOut = "cmdDeleteService";
                return LineClassification::LEVEL2_ROOT;
            }
            if (tokens[1] == "secure-policy" && (isPositionValid(tokens[2]) || isNameValid(tokens[2]))) {
                matchedMethodOut = "cmdDeleteRule";
                return LineClassification::LEVEL2_ROOT;
            }
            if (tokens[1] == "ip" && tokens[2] == "virtual-server") {
                // incomplete
            }
        } else if (tokens.size() == 4) {
            if (tokens[1] == "address-group" && isNameValid(tokens[2]) && isNameValid(tokens[3])) {
                matchedMethodOut = "cmdDeleteAddressGroupMember";
                return LineClassification::LEVEL2_ROOT;
            }
            if (tokens[1] == "secure-policy" && tokens[2] == "name" && isNameValid(tokens[3])) {
                matchedMethodOut = "cmdDeleteRule";
                return LineClassification::LEVEL2_ROOT;
            }
            if (tokens[1] == "ip" && tokens[2] == "virtual-server" && isNameValid(tokens[3])) {
                matchedMethodOut = "cmdDeleteVirtualServer";
                return LineClassification::LEVEL2_ROOT;
            }
        } else if (tokens.size() == 6) {
            if (tokens[1] == "ip" && tokens[2] == "route" &&
                isIpv4Valid(tokens[3]) && isMaskValid(tokens[4]) && isIpv4Valid(tokens[5])) {
                matchedMethodOut = "cmdDeleteRoute";
                return LineClassification::LEVEL2_ROOT;
            }
        }
    }

    if (tokens[0] == "ip" && tokens.size() >= 2) {
        if (tokens[1] == "route") {
            // ip route <ipv4> <mask> <ipv4> <metric>
            if (tokens.size() == 6 &&
                isIpv4Valid(tokens[2]) && isMaskValid(tokens[3]) &&
                isIpv4Valid(tokens[4]) && isMetricValid(tokens[5])) {
                matchedMethodOut = "cmdAddRoute";
                return LineClassification::LEVEL2_ROOT;
            }
        } else if (tokens[1] == "virtual-server") {
            // ip virtual-server <name> followed only by optional:
            //   interface <name>
            //   original-ip <ipv4>
            //   map-to <ipv4>
            // required:
            //   map-type port
            // required:
            //   original-service <name>
            //   mapped-service <name>
            // required:
            //   activate | deactivate
            if (tokens.size() >= 8 && isNameValid(tokens[2])) {
                size_t idx = 3;
                if (idx + 1 < tokens.size() && tokens[idx] == "interface" && isNameValid(tokens[idx + 1])) {
                    idx += 2;
                }
                if (idx + 1 < tokens.size() && tokens[idx] == "original-ip" && isIpv4Valid(tokens[idx + 1])) {
                    idx += 2;
                }
                if (idx + 1 < tokens.size() && tokens[idx] == "map-to" && isIpv4Valid(tokens[idx + 1])) {
                    idx += 2;
                }
                if (idx < tokens.size() && tokens[idx] == "map-type") {
                    idx++;
                    if (idx < tokens.size() && (tokens[idx] == "port" || tokens[idx] == "service")) {
                        idx++;
                    }
                    // original-service <name> is required
                    if (idx + 1 < tokens.size() && tokens[idx] == "original-service" && isNameValid(tokens[idx + 1])) {
                        idx += 2;
                        // mapped-service <name> is required
                        if (idx + 1 < tokens.size() && tokens[idx] == "mapped-service" && isNameValid(tokens[idx + 1])) {
                            idx += 2;
                            if (idx == tokens.size()) {
                                matchedMethodOut = "cmdAddVirtualServer";
                                return LineClassification::LEVEL2_ROOT;
                            }
                            if (idx < tokens.size() && (tokens[idx] == "activate" || tokens[idx] == "deactivate")) {
                                if (idx + 1 == tokens.size()) {
                                    matchedMethodOut = "cmdAddVirtualServer";
                                    return LineClassification::LEVEL2_ROOT;
                                }
                            }
                        }
                    }
                }
            }
        }
    }

    if (tokens[0] == "address-object" && tokens.size() == 3) {
        if (isNameValid(tokens[1])) {
            const std::string &val = tokens[2];
            // address-object <name> <ipv4>
            if (isIpv4Valid(val)) {
                matchedMethodOut = "cmdAddAddressHost";
                return LineClassification::LEVEL2_ROOT;
            }
            // address-object <name> <ipv4>-<ipv4>
            size_t dash = val.find('-');
            if (dash != std::string::npos && dash > 0 && dash + 1 < val.length()) {
                std::string ip1 = val.substr(0, dash);
                std::string ip2 = val.substr(dash + 1);
                if (isIpv4Valid(ip1) && isIpv4Valid(ip2)) {
                    matchedMethodOut = "cmdAddAddressRange";
                    return LineClassification::LEVEL2_ROOT;
                }
            }
            // address-object <name> <ipv4>/<prefix> or <ipv4>/<dotted-quad>
            size_t slash = val.find('/');
            if (slash != std::string::npos && slash > 0 && slash + 1 < val.length()) {
                std::string ip = val.substr(0, slash);
                std::string mask = val.substr(slash + 1);
                if (isIpv4Valid(ip) && isMaskValid(mask)) {
                    matchedMethodOut = "cmdAddAddressSubnet";
                    return LineClassification::LEVEL2_ROOT;
                }
            }
        }
    }

    if (tokens[0] == "address-group" && tokens.size() == 3) {
        if (isNameValid(tokens[1]) && isNameValid(tokens[2])) {
            matchedMethodOut = "cmdAddAddressGroupMember";
            return LineClassification::LEVEL2_ROOT;
        }
    }

    if (tokens[0] == "service-object" && tokens.size() == 5) {
        // service-object <name> <proto> eq <port>
        if (isNameValid(tokens[1]) && isProtocolValid(tokens[2]) && tokens[3] == "eq" && isPortValid(tokens[4])) {
            matchedMethodOut = "cmdAddService";
            return LineClassification::LEVEL2_ROOT;
        }
    }

    if (tokens[0] == "secure-policy" && tokens.size() == 3) {
        // secure-policy insert <position>
        if (tokens[1] == "insert" && isPositionValid(tokens[2])) {
            matchedMethodOut = "cmdInsertRule";
            return LineClassification::LEVEL2_ROOT;
        }
    }

    return LineClassification::UNCLASSIFIED;
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
