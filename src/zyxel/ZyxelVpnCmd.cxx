/*
 * ZyxelVpnCmd.cxx
 *
 * Copyright (C) 2026, Charles Chiou
 */

#include "zyxel/ZyxelVpnCmd.hxx"
#include "zyxel/ZyxelScanner.hxx"
#include <sstream>
#include <algorithm>

// Envelope 8: IPSec VPN (Ch 33)

std::string ZyxelVpnCmd::cmdShowCryptoBoostTcp() {
    return "show crypto boost-tcp";
}

std::string ZyxelVpnCmd::cmdShowCryptoIgnoreDfBit() {
    return "show crypto ignore-df-bit";
}

std::string ZyxelVpnCmd::cmdShowCryptoMap(const std::string &mapName) {
    if (mapName.empty()) {
        return "show crypto map";
    }
    return "show crypto map " + mapName;
}

std::string ZyxelVpnCmd::cmdShowCryptoMap6(const std::string &mapName) {
    if (mapName.empty()) {
        return "show crypto map6";
    }
    return "show crypto map6 " + mapName;
}

std::string ZyxelVpnCmd::cmdShowCryptoMapConnCheck() {
    return "show crypto map conn-check";
}

std::string ZyxelVpnCmd::cmdShowIkev2Policy(const std::string &policyName) {
    if (policyName.empty()) {
        return "show ikev2 policy";
    }
    return "show ikev2 policy " + policyName;
}

std::string ZyxelVpnCmd::cmdShowIkev2Policy6(const std::string &policyName) {
    if (policyName.empty()) {
        return "show ikev2 policy6";
    }
    return "show ikev2 policy6 " + policyName;
}

std::string ZyxelVpnCmd::cmdShowIsakmpKeepalive() {
    return "show isakmp keepalive";
}

std::string ZyxelVpnCmd::cmdShowIsakmpPolicy(const std::string &policyName) {
    if (policyName.empty()) {
        return "show isakmp policy";
    }
    return "show isakmp policy " + policyName;
}

std::string ZyxelVpnCmd::cmdShowIsakmpSa() {
    return "show isakmp sa";
}

std::string ZyxelVpnCmd::cmdShowSaCounter() {
    return "show sa counter";
}

std::string ZyxelVpnCmd::cmdShowSaMonitor() {
    return "show sa monitor";
}

std::string ZyxelVpnCmd::cmdShowVcpAllowedCryptoMap() {
    return "show vcp allowed crypto map";
}

std::string ZyxelVpnCmd::cmdShowVcpAllowedCryptoMap6() {
    return "show vcp allowed crypto map6";
}

std::string ZyxelVpnCmd::cmdShowVcpAllowedUsers() {
    return "show vcp allowed users";
}

std::string ZyxelVpnCmd::cmdShowVpnConcentrator(const std::string &profileName) {
    if (profileName.empty()) {
        return "show vpn-concentrator";
    }
    return "show vpn-concentrator " + profileName;
}

std::string ZyxelVpnCmd::cmdShowVpnConcentrator6(const std::string &profileName) {
    if (profileName.empty()) {
        return "show vpn-concentrator6";
    }
    return "show vpn-concentrator6 " + profileName;
}

std::string ZyxelVpnCmd::cmdShowVpnConfigurationProvisionActivation() {
    return "show vpn-configuration-provision activation";
}

std::string ZyxelVpnCmd::cmdShowVpnConfigurationProvisionAuthentication() {
    return "show vpn-configuration-provision authentication";
}

std::string ZyxelVpnCmd::cmdShowVpnConfigurationProvisionIosfilter() {
    return "show vpn-configuration-provision iosfilter";
}

std::string ZyxelVpnCmd::cmdShowVpnConfigurationProvisionPort() {
    return "show vpn-configuration-provision port";
}

std::string ZyxelVpnCmd::cmdShowVpnConfigurationProvisionRules() {
    return "show vpn-configuration-provision rules";
}

std::string ZyxelVpnCmd::cmdShowVpnCounters() {
    return "show vpn-counters";
}

std::string ZyxelVpnCmd::cmdShowVpnServiceStatus() {
    return "show vpn-service status";
}

std::string ZyxelVpnCmd::cmdIsakmpPolicy(const std::string &name) {
    return "isakmp policy " + name;
}

std::string ZyxelVpnCmd::cmdNoIsakmpPolicy(const std::string &name) {
    return "no isakmp policy " + name;
}

std::string ZyxelVpnCmd::cmdCryptoMap(const std::string &name) {
    return "crypto map " + name;
}

std::string ZyxelVpnCmd::cmdNoCryptoMap(const std::string &name) {
    return "no crypto map " + name;
}

// Envelope 8: SSL VPN (Ch 34)

std::string ZyxelVpnCmd::cmdShowSslvpnLoginPort() {
    return "show sslvpn login-port";
}

std::string ZyxelVpnCmd::cmdShowSslvpnPolicy(const std::string &profileName) {
    if (profileName.empty()) {
        return "show sslvpn policy";
    }
    return "show sslvpn policy " + profileName;
}

std::string ZyxelVpnCmd::cmdShowSslvpnApplication() {
    return "show sslvpn application";
}

std::string ZyxelVpnCmd::cmdShowSslvpnMonitor() {
    return "show sslvpn monitor";
}

std::string ZyxelVpnCmd::cmdShowWorkspaceApplication() {
    return "show workspace application";
}

std::string ZyxelVpnCmd::cmdShowWorkspaceCifs() {
    return "show workspace cifs";
}

std::string ZyxelVpnCmd::cmdShowSslVpnNetworkExtensionLocalIp() {
    return "show ssl-vpn network-extension local-ip";
}

std::string ZyxelVpnCmd::cmdSslvpnApplication(const std::string &name) {
    return "sslvpn application " + name;
}

std::string ZyxelVpnCmd::cmdNoSslvpnApplication(const std::string &name) {
    return "no sslvpn application " + name;
}

std::string ZyxelVpnCmd::cmdSslvpnPolicy(const std::string &name) {
    return "sslvpn policy " + name;
}

std::string ZyxelVpnCmd::cmdNoSslvpnPolicy(const std::string &name) {
    return "no sslvpn policy " + name;
}

// Envelope 8: L2TP VPN (Ch 35)

std::string ZyxelVpnCmd::cmdShowAccountL2tp(const std::string &profileName) {
    if (profileName.empty()) {
        return "show account l2tp";
    }
    return "show account l2tp " + profileName;
}

std::string ZyxelVpnCmd::cmdShowInterfacePpp() {
    return "show interface ppp";
}

std::string ZyxelVpnCmd::cmdShowL2tpOverIpsec() {
    return "show l2tp-over-ipsec";
}

std::string ZyxelVpnCmd::cmdShowL2tpOverIpsecSession() {
    return "show l2tp-over-ipsec session";
}

std::string ZyxelVpnCmd::cmdAccountL2tp(const std::string &name) {
    return "account l2tp " + name;
}

std::string ZyxelVpnCmd::cmdNoAccountL2tp(const std::string &name) {
    return "no account l2tp " + name;
}

// Dry-fire and negative testing helpers

std::string ZyxelVpnCmd::cmdInvalidIpsecDryFire() {
    return "show ikev2 policy6";
}

std::string ZyxelVpnCmd::cmdInvalidSslvpnDryFire() {
    return "show ssl-vpn network-extension local-ip";
}

std::string ZyxelVpnCmd::cmdInvalidL2tpDryFire() {
    return "no l2tp-over-ipsec session tunnel-id 99999";
}

// Envelope 8 Parsers

bool ZyxelVpnCmd::parseCryptoBoostTcp(const std::string &raw, bool &boostTcp) {
    std::istringstream stream(raw);
    std::string line;
    while (std::getline(stream, line)) {
        std::string trimmed = ZyxelScanner::trim(line);
        if (trimmed.find("ipsec boost tcp:") != std::string::npos ||
            trimmed.find("crypto boost-tcp") != std::string::npos) {
            std::string lower = trimmed;
            std::transform(lower.begin(), lower.end(), lower.begin(), ::tolower);
            boostTcp = (lower.find("activate") != std::string::npos &&
                        lower.find("deactivate") == std::string::npos) ||
                       (lower.find("yes") != std::string::npos);
            return true;
        }
    }
    return false;
}

bool ZyxelVpnCmd::parseVpnCounters(const std::string &raw, ZyxelVpnCounters &out) {
    std::istringstream stream(raw);
    std::string line;
    bool found = false;
    while (std::getline(stream, line)) {
        std::string trimmed = ZyxelScanner::trim(line);
        auto colon = trimmed.find(':');
        if (colon == std::string::npos) {
            continue;
        }
        std::string key = ZyxelScanner::trim(trimmed.substr(0, colon));
        std::string valStr = ZyxelScanner::trim(trimmed.substr(colon + 1));
        std::istringstream valStream(valStr);
        if (key == "inbpkt count") {
            valStream >> out.inbpktCount;
            found = true;
        } else if (key == "outbpkt count") {
            valStream >> out.outbpktCount;
            found = true;
        } else if (key == "inbpkt throughput") {
            valStream >> out.inbpktThroughput;
            found = true;
        } else if (key == "outbpkt throughput") {
            valStream >> out.outbpktThroughput;
            found = true;
        }
    }
    return found;
}

bool ZyxelVpnCmd::parseVpnServiceStatus(const std::string &raw, bool &active, bool &autoDisable) {
    std::istringstream stream(raw);
    std::string line;
    bool found = false;
    while (std::getline(stream, line)) {
        std::string trimmed = ZyxelScanner::trim(line);
        auto colon = trimmed.find(':');
        if (colon == std::string::npos) {
            continue;
        }
        std::string key = ZyxelScanner::trim(trimmed.substr(0, colon));
        std::string val = ZyxelScanner::trim(trimmed.substr(colon + 1));
        if (key == "status") {
            active = (val == "activate" || val == "yes" || val == "enable");
            found = true;
        } else if (key == "auto disable status") {
            autoDisable = (val == "activate" || val == "yes" || val == "enable");
            found = true;
        }
    }
    return found;
}

bool ZyxelVpnCmd::parseVpnConfigurationProvisionActivation(const std::string &raw, bool &active) {
    std::istringstream stream(raw);
    std::string line;
    while (std::getline(stream, line)) {
        std::string trimmed = ZyxelScanner::trim(line);
        if (trimmed.find("VPN configuration provision activation:") != std::string::npos) {
            auto colon = trimmed.find(':');
            if (colon != std::string::npos) {
                std::string val = ZyxelScanner::trim(trimmed.substr(colon + 1));
                active = (val == "yes" || val == "activate" || val == "enable");
                return true;
            }
        }
    }
    return false;
}

bool ZyxelVpnCmd::parseVpnConfigurationProvisionPort(const std::string &raw, int &port) {
    std::istringstream stream(raw);
    std::string line;
    while (std::getline(stream, line)) {
        std::string trimmed = ZyxelScanner::trim(line);
        if (trimmed.find("VPN configuration provision port:") != std::string::npos) {
            auto colon = trimmed.find(':');
            if (colon != std::string::npos) {
                std::string val = ZyxelScanner::trim(trimmed.substr(colon + 1));
                try {
                    port = std::stoi(val);
                    return true;
                } catch (...) {
                    return false;
                }
            }
        }
    }
    return false;
}

bool ZyxelVpnCmd::parseSaCounter(const std::string &raw, int &status) {
    std::istringstream stream(raw);
    std::string line;
    while (std::getline(stream, line)) {
        std::string trimmed = ZyxelScanner::trim(line);
        if (trimmed.find("vpn status:") != std::string::npos) {
            auto colon = trimmed.find(':');
            if (colon != std::string::npos) {
                std::string val = ZyxelScanner::trim(trimmed.substr(colon + 1));
                try {
                    status = std::stoi(val);
                    return true;
                } catch (...) {
                    return false;
                }
            }
        }
    }
    return false;
}

bool ZyxelVpnCmd::parseSslvpnLoginPort(const std::string &raw, int &port) {
    std::istringstream stream(raw);
    std::string line;
    while (std::getline(stream, line)) {
        std::string trimmed = ZyxelScanner::trim(line);
        if (trimmed.find("SSL VPN Login Port:") != std::string::npos) {
            auto colon = trimmed.find(':');
            if (colon != std::string::npos) {
                std::string val = ZyxelScanner::trim(trimmed.substr(colon + 1));
                try {
                    port = std::stoi(val);
                    return true;
                } catch (...) {
                    return false;
                }
            }
        }
    }
    return false;
}

bool ZyxelVpnCmd::parseL2tpOverIpsec(const std::string &raw, ZyxelL2tpStatus &out) {
    std::istringstream stream(raw);
    std::string line;
    bool found = false;
    while (std::getline(stream, line)) {
        std::string trimmed = ZyxelScanner::trim(line);
        auto colon = trimmed.find(':');
        if (colon == std::string::npos) {
            continue;
        }
        std::string key = ZyxelScanner::trim(trimmed.substr(0, colon));
        std::string val = ZyxelScanner::trim(trimmed.substr(colon + 1));
        if (key == "activate") {
            out.activate = (val == "yes" || val == "activate" || val == "enable");
            found = true;
        } else if (key == "crypto") {
            out.crypto = val;
            found = true;
        } else if (key == "address pool") {
            out.addressPool = val;
        } else if (key == "authentication") {
            out.authentication = val;
        } else if (key == "certificate") {
            out.certificate = val;
        } else if (key == "user") {
            out.user = val;
        } else if (key == "keepalive timer") {
            try {
                out.keepaliveTimer = std::stoi(val);
            } catch (...) {
            }
        } else if (key == "first dns server") {
            out.firstDnsServer = val;
        }
    }
    return found;
}

bool ZyxelVpnCmd::parseVcpAllowedUsers(const std::string &raw, std::vector<ZyxelVcpUser> &out) {
    out.clear();
    if (raw.empty()) return false;
    auto lines = ZyxelScanner::splitLines(raw);
    int divIdx = ZyxelScanner::findDividerLine(lines);
    if (divIdx > 0) {
        auto cols = ZyxelScanner::parseTableColumns(lines[divIdx - 1], lines[divIdx]);
        int noCol = ZyxelScanner::findColumn(cols, "no.");
        int userCol = ZyxelScanner::findColumn(cols, "username");
        int typeCol = ZyxelScanner::findColumn(cols, "user type");
        int descCol = ZyxelScanner::findColumn(cols, "description");

        for (size_t i = divIdx + 1; i < lines.size(); ++i) {
            const std::string &line = lines[i];
            std::string trimmed = ZyxelScanner::trim(line);
            if (trimmed.empty() || trimmed.find("====") == 0 || trimmed.find("----") == 0 ||
                trimmed.rfind("Router", 0) == 0) {
                continue;
            }

            if (noCol >= 0 && userCol >= 0 && typeCol >= 0) {
                ZyxelVcpUser u;
                try {
                    u.index = std::stoi(ZyxelScanner::extractCell(line, cols[noCol]));
                    u.username = ZyxelScanner::extractCell(line, cols[userCol]);
                    u.userType = ZyxelScanner::extractCell(line, cols[typeCol]);
                    if (descCol >= 0) {
                        u.description = ZyxelScanner::extractCell(line, cols[descCol]);
                    }
                    out.push_back(u);
                } catch (...) {}
            }
        }
    }
    return !out.empty();
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
