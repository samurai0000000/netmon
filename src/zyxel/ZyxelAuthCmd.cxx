/*
 * ZyxelAuthCmd.cxx
 *
 * Copyright (C) 2026, Charles Chiou
 */

#include "zyxel/ZyxelAuthCmd.hxx"
#include "zyxel/ZyxelScanner.hxx"
#include <sstream>
#include <algorithm>

// Ch 30: Cloud CNM

std::string ZyxelAuthCmd::cmdShowCnmAgentConfiguration() {
    return "show cnm-agent configuration";
}

std::string ZyxelAuthCmd::cmdShowMonitorMode() {
    return "show monitor-mode";
}

std::string ZyxelAuthCmd::cmdShowSecuReporterStatus() {
    return "show secu-reporter status";
}

std::string ZyxelAuthCmd::cmdShowSecumanagerStatus() {
    return "show secumanager status";
}

// Ch 31: Web Authentication

std::string ZyxelAuthCmd::cmdShowWebAuthActivation() {
    return "show web-auth activation";
}

std::string ZyxelAuthCmd::cmdShowWebAuthDefaultRule() {
    return "show web-auth default-rule";
}

std::string ZyxelAuthCmd::cmdShowWebAuthExceptionalService() {
    return "show web-auth exceptional-service";
}

std::string ZyxelAuthCmd::cmdShowWebAuthMethod() {
    return "show web-auth method";
}

std::string ZyxelAuthCmd::cmdShowWebAuthPolicy(const std::string &policy) {
    if (policy.empty()) {
        return "show web-auth policy all";
    }
    return "show web-auth policy " + policy;
}

std::string ZyxelAuthCmd::cmdShowWebAuthPortalStatus() {
    return "show web-auth portal status";
}

std::string ZyxelAuthCmd::cmdShowWebAuthRedirectFqdn() {
    return "show web-auth redirect-fqdn";
}

std::string ZyxelAuthCmd::cmdShowWebAuthStatus() {
    return "show web-auth status";
}

std::string ZyxelAuthCmd::cmdShowSsoAgent() {
    return "show sso agent";
}

std::string ZyxelAuthCmd::cmdShowSsoPort() {
    return "show sso port";
}

// Ch 32: Hotspot

std::string ZyxelAuthCmd::cmdShowAdvertisementActivation() {
    return "show advertisement activation";
}

std::string ZyxelAuthCmd::cmdShowBillingStatus() {
    return "show billing status";
}

std::string ZyxelAuthCmd::cmdShowFreeTimeStatus() {
    return "show free-time status";
}

std::string ZyxelAuthCmd::cmdShowIpIpnpActivation() {
    return "show ip ipnp activation";
}

std::string ZyxelAuthCmd::cmdShowPaymentServiceActivation() {
    return "show payment-service activation";
}

// Ch 50: User/Group

std::string ZyxelAuthCmd::cmdShowUsername(const std::string &name) {
    if (name.empty()) {
        return "show username";
    }
    return "show username " + name;
}

std::string ZyxelAuthCmd::cmdShowGroupname(const std::string &name) {
    if (name.empty()) {
        return "show groupname";
    }
    return "show groupname " + name;
}

std::string ZyxelAuthCmd::cmdShowLockoutUsers() {
    return "show lockout-users";
}

std::string ZyxelAuthCmd::cmdShowPasswordComplexityVerifyStatus() {
    return "show password complexity-verify status";
}

std::string ZyxelAuthCmd::cmdShowPwdExpiry(const std::string &type) {
    if (type.empty()) {
        return "show pwd-expiry all";
    }
    return "show pwd-expiry " + type;
}

std::string ZyxelAuthCmd::cmdShowUsersDefaultSetting(const std::string &type) {
    if (type.empty()) {
        return "show users default-setting all";
    }
    return "show users default-setting " + type;
}

std::string ZyxelAuthCmd::cmdShowUsersIdleDetectionSettings() {
    return "show users idle-detection-settings";
}

std::string ZyxelAuthCmd::cmdShowUsersRetrySettings() {
    return "show users retry-settings";
}

std::string ZyxelAuthCmd::cmdShowUsersSimultaneousLogonSettings() {
    return "show users simultaneous-logon-settings";
}

std::string ZyxelAuthCmd::cmdShowUsersUpdateLeaseSettings() {
    return "show users update-lease-settings";
}

std::string ZyxelAuthCmd::cmdShowUsers(const std::string &target) {
    if (target.empty()) {
        return "show users all";
    }
    return "show users " + target;
}

// Ch 55: AAA Server

std::string ZyxelAuthCmd::cmdShowAaaGroupServerRadius(const std::string &name) {
    return "show aaa group server radius " + name;
}

std::string ZyxelAuthCmd::cmdShowAaaGroupServerLdap(const std::string &name) {
    return "show aaa group server ldap " + name;
}

std::string ZyxelAuthCmd::cmdShowAaaGroupServerAd(const std::string &name) {
    return "show aaa group server ad " + name;
}

// Ch 56: Authentication Objects

std::string ZyxelAuthCmd::cmdShowAaaAuthentication(const std::string &name) {
    if (name.empty()) {
        return "show aaa authentication default";
    }
    return "show aaa authentication " + name;
}

// Ch 58: Certificates

std::string ZyxelAuthCmd::cmdShowCaCategoryLocal() {
    return "show ca category local";
}

std::string ZyxelAuthCmd::cmdShowCaCategoryRemote() {
    return "show ca category remote";
}

std::string ZyxelAuthCmd::cmdShowCaSpaceusage() {
    return "show ca spaceusage";
}

// Mutations

std::string ZyxelAuthCmd::cmdGroupname(const std::string &name) {
    return "groupname " + name;
}

std::string ZyxelAuthCmd::cmdNoGroupname(const std::string &name) {
    return "no groupname " + name;
}

std::string ZyxelAuthCmd::cmdAaaGroupServerRadius(const std::string &name) {
    return "aaa group server radius " + name;
}

std::string ZyxelAuthCmd::cmdNoAaaGroupServerRadius(const std::string &name) {
    return "no aaa group server radius " + name;
}

// Dry-fire and negative testing helpers

std::string ZyxelAuthCmd::cmdInvalidDynamicGuestDryFire() {
    return "show dynamic-guest status";
}

std::string ZyxelAuthCmd::cmdInvalidAaaServerDryFire() {
    return "show aaa-server";
}

std::string ZyxelAuthCmd::cmdInvalidAuthMethodDryFire() {
    return "show auth-method";
}

// Parsers

bool ZyxelAuthCmd::parseCnmConfiguration(const std::string &raw, bool &active) {
    std::istringstream stream(raw);
    std::string line;
    while (std::getline(stream, line)) {
        std::string trimmed = ZyxelScanner::trim(line);
        if (trimmed.find("Activate:") != std::string::npos) {
            auto colon = trimmed.find(':');
            std::string val = ZyxelScanner::trim(trimmed.substr(colon + 1));
            std::string lower = val;
            std::transform(lower.begin(), lower.end(), lower.begin(), ::tolower);
            active = (lower == "yes" || lower == "activate" || lower == "enable");
            return true;
        }
    }
    return false;
}

bool ZyxelAuthCmd::parseMonitorMode(const std::string &raw, ZyxelCnmStatus &out) {
    std::istringstream stream(raw);
    std::string line;
    bool found = false;
    while (std::getline(stream, line)) {
        std::string trimmed = ZyxelScanner::trim(line);
        auto colon = trimmed.find(':');
        if (colon == std::string::npos) continue;
        std::string key = ZyxelScanner::trim(trimmed.substr(0, colon));
        std::string val = ZyxelScanner::trim(trimmed.substr(colon + 1));
        if (key == "active") {
            out.active = (val == "yes" || val == "activate");
            found = true;
        } else if (key == "status") {
            out.status = val;
            found = true;
        } else if (key == "is_connected") {
            out.isConnected = (val == "yes" || val == "true");
        }
    }
    return found;
}

bool ZyxelAuthCmd::parseSecuReporterStatus(const std::string &raw, ZyxelSecuReporterStatus &out) {
    std::istringstream stream(raw);
    std::string line;
    bool found = false;
    while (std::getline(stream, line)) {
        std::string trimmed = ZyxelScanner::trim(line);
        auto colon = trimmed.find(':');
        if (colon == std::string::npos) continue;
        std::string key = ZyxelScanner::trim(trimmed.substr(0, colon));
        std::string val = ZyxelScanner::trim(trimmed.substr(colon + 1));
        if (key == "activate") {
            out.active = (val == "yes" || val == "activate");
            found = true;
        } else if (key == "send-reporter") {
            out.sendReporter = (val == "yes");
        } else if (key == "upload-interval") {
            try { out.uploadInterval = std::stoi(val); found = true; } catch (...) {}
        } else if (key == "upload-filesize") {
            try { out.uploadFilesize = std::stoi(val); } catch (...) {}
        } else if (key == "banner") {
            out.banner = (val == "yes");
        }
    }
    return found;
}

bool ZyxelAuthCmd::parseWebAuthActivation(const std::string &raw, bool &active) {
    std::istringstream stream(raw);
    std::string line;
    while (std::getline(stream, line)) {
        std::string trimmed = ZyxelScanner::trim(line);
        if (trimmed.find("Auth Policy Activation:") != std::string::npos) {
            auto colon = trimmed.find(':');
            std::string val = ZyxelScanner::trim(trimmed.substr(colon + 1));
            std::string lower = val;
            std::transform(lower.begin(), lower.end(), lower.begin(), ::tolower);
            active = (lower == "yes" || lower == "activate" || lower == "enable");
            return true;
        }
    }
    return false;
}

bool ZyxelAuthCmd::parseWebAuthPortalStatus(const std::string &raw, std::string &logoutIp, bool &sessionPage) {
    std::istringstream stream(raw);
    std::string line;
    bool found = false;
    while (std::getline(stream, line)) {
        std::string trimmed = ZyxelScanner::trim(line);
        auto colon = trimmed.find(':');
        if (colon == std::string::npos) continue;
        std::string key = ZyxelScanner::trim(trimmed.substr(0, colon));
        std::string val = ZyxelScanner::trim(trimmed.substr(colon + 1));
        if (key == "Logout IP") {
            logoutIp = val;
            found = true;
        } else if (key == "Session Page") {
            sessionPage = (val == "yes" || val == "activate");
            found = true;
        }
    }
    return found;
}

bool ZyxelAuthCmd::parseSsoPort(const std::string &raw, int &port) {
    std::istringstream stream(raw);
    std::string line;
    while (std::getline(stream, line)) {
        std::string trimmed = ZyxelScanner::trim(line);
        if (trimmed.find("ZySSO port:") != std::string::npos) {
            auto colon = trimmed.find(':');
            std::string val = ZyxelScanner::trim(trimmed.substr(colon + 1));
            try {
                port = std::stoi(val);
                return true;
            } catch (...) {
                return false;
            }
        }
    }
    return false;
}

bool ZyxelAuthCmd::parseBillingStatus(const std::string &raw, std::string &acctMethod, int &accumTimeout) {
    std::istringstream stream(raw);
    std::string line;
    bool found = false;
    while (std::getline(stream, line)) {
        std::string trimmed = ZyxelScanner::trim(line);
        auto colon = trimmed.find(':');
        if (colon == std::string::npos) continue;
        std::string key = ZyxelScanner::trim(trimmed.substr(0, colon));
        std::string val = ZyxelScanner::trim(trimmed.substr(colon + 1));
        if (key == "Billing accounting method") {
            acctMethod = val;
            found = true;
        } else if (key == "Accumulation idle timeout") {
            try { accumTimeout = std::stoi(val); found = true; } catch (...) {}
        }
    }
    return found;
}

bool ZyxelAuthCmd::parseFreeTimeStatus(const std::string &raw, bool &active, std::string &period) {
    std::istringstream stream(raw);
    std::string line;
    bool found = false;
    while (std::getline(stream, line)) {
        std::string trimmed = ZyxelScanner::trim(line);
        auto colon = trimmed.find(':');
        if (colon == std::string::npos) continue;
        std::string key = ZyxelScanner::trim(trimmed.substr(0, colon));
        std::string val = ZyxelScanner::trim(trimmed.substr(colon + 1));
        if (key == "Activate") {
            active = (val == "yes" || val == "activate");
            found = true;
        } else if (key == "Time Period") {
            period = val;
            found = true;
        }
    }
    return found;
}

bool ZyxelAuthCmd::parseUsers(const std::string &raw, std::vector<ZyxelUserEntry> &out) {
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
                ZyxelUserEntry u;
                try {
                    u.no = std::stoi(ZyxelScanner::extractCell(line, cols[noCol]));
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

bool ZyxelAuthCmd::parseUsersRetrySettings(const std::string &raw, ZyxelUserRetrySettings &out) {
    std::istringstream stream(raw);
    std::string line;
    bool found = false;
    while (std::getline(stream, line)) {
        std::string trimmed = ZyxelScanner::trim(line);
        auto colon = trimmed.find(':');
        if (colon == std::string::npos) continue;
        std::string key = ZyxelScanner::trim(trimmed.substr(0, colon));
        std::string val = ZyxelScanner::trim(trimmed.substr(colon + 1));
        if (key == "enable logon retry limit") {
            out.retryLimitActive = (val == "yes" || val == "activate");
            found = true;
        } else if (key == "maximum retry count") {
            try { out.maxRetryCount = std::stoi(val); found = true; } catch (...) {}
        } else if (key == "lockout period") {
            try { out.lockoutPeriod = std::stoi(val); found = true; } catch (...) {}
        }
    }
    return found;
}

bool ZyxelAuthCmd::parseCaSpaceUsage(const std::string &raw, ZyxelCaSpaceUsage &out) {
    std::istringstream stream(raw);
    std::string line;
    bool found = false;
    while (std::getline(stream, line)) {
        std::string trimmed = ZyxelScanner::trim(line);
        auto colon = trimmed.find(':');
        if (colon == std::string::npos) continue;
        std::string key = ZyxelScanner::trim(trimmed.substr(0, colon));
        std::string val = ZyxelScanner::trim(trimmed.substr(colon + 1));
        std::istringstream valStream(val);
        if (key == "total") {
            valStream >> out.total;
            found = true;
        } else if (key == "available") {
            valStream >> out.available;
            found = true;
        } else if (key == "in use") {
            valStream >> out.inUse;
            found = true;
        } else if (key == "my certificate") {
            valStream >> out.myCert;
            found = true;
        }
    }
    return found;
}

bool ZyxelAuthCmd::parseCaCertInfo(const std::string &raw, std::vector<ZyxelCaCertInfo> &out) {
    out.clear();
    std::istringstream stream(raw);
    std::string line;
    ZyxelCaCertInfo current;
    bool inCert = false;
    while (std::getline(stream, line)) {
        std::string trimmed = ZyxelScanner::trim(line);
        auto colon = trimmed.find(':');
        if (colon == std::string::npos) continue;
        std::string key = ZyxelScanner::trim(trimmed.substr(0, colon));
        std::string val = ZyxelScanner::trim(trimmed.substr(colon + 1));
        if (key == "certificate") {
            if (inCert && !current.certificate.empty()) {
                out.push_back(current);
                current = ZyxelCaCertInfo();
            }
            current.certificate = val;
            inCert = true;
        } else if (key == "type") {
            current.type = val;
        } else if (key == "subject") {
            current.subject = val;
        } else if (key == "issuer") {
            current.issuer = val;
        } else if (key == "status") {
            current.status = val;
        }
    }
    if (inCert && !current.certificate.empty()) {
        out.push_back(current);
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
