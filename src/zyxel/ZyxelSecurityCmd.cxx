/*
 * ZyxelSecurityCmd.cxx
 *
 * Copyright (C) 2026, Charles Chiou
 */

#include "zyxel/ZyxelSecurityCmd.hxx"
#include "zyxel/ZyxelScanner.hxx"
#include "zyxel/ZyxelSystemCmd.hxx"
#include "ZyxelSshClient.hxx"

#include <algorithm>
#include <cctype>
#include <sstream>
#include <iostream>

std::string ZyxelSecurityCmd::cmdShowAppStatisticsSummary() {
    return "show app statistics summary";
}

std::string ZyxelSecurityCmd::cmdShowIdpStatisticsSummary() {
    return "show idp statistics summary";
}

std::string ZyxelSecurityCmd::cmdShowConnStatus() {
    return "show conn status";
}

bool ZyxelSecurityCmd::parseConnStatus(const std::string &raw, ZyxelSessionSummary &out) {
    return ZyxelSystemCmd::parseConnStatus(raw, out);
}

bool ZyxelSecurityCmd::parseAppStatisticsSummary(const std::string &raw, ZyxelAppPatrolSummary &out) {
    out = ZyxelAppPatrolSummary();
    if (raw.empty()) {
        return false;
    }

    auto lines = ZyxelScanner::splitLines(raw);
    bool foundAny = false;

    for (const auto &line : lines) {
        if (line.find(',') != std::string::npos) {
            int val = 0;
            if (ZyxelScanner::extractIntegerAfter(line, "forward", val)) {
                out.forwardedKb += static_cast<uint64_t>(val);
                foundAny = true;
            }
            if (ZyxelScanner::extractIntegerAfter(line, "drop", val)) {
                out.droppedKb += static_cast<uint64_t>(val);
                foundAny = true;
            }
            if (ZyxelScanner::extractIntegerAfter(line, "reject", val)) {
                out.rejectedKb += static_cast<uint64_t>(val);
                foundAny = true;
            }
            if (ZyxelScanner::extractIntegerAfter(line, "match", val) ||
                ZyxelScanner::extractIntegerAfter(line, "connection", val)) {
                out.matchedConnections += static_cast<uint64_t>(val);
                foundAny = true;
            }
        } else {
            std::string key, valStr;
            if (ZyxelScanner::parseKeyValue(line, key, valStr, ':')) {
                std::string lowerKey = key;
                std::transform(lowerKey.begin(), lowerKey.end(), lowerKey.begin(), ::tolower);
                try {
                    if (lowerKey.find("forward") != std::string::npos) {
                        out.forwardedKb += std::stoull(valStr);
                        foundAny = true;
                    } else if (lowerKey.find("drop") != std::string::npos) {
                        out.droppedKb += std::stoull(valStr);
                        foundAny = true;
                    } else if (lowerKey.find("reject") != std::string::npos) {
                        out.rejectedKb += std::stoull(valStr);
                        foundAny = true;
                    } else if (lowerKey.find("match") != std::string::npos || lowerKey.find("connection") != std::string::npos) {
                        out.matchedConnections += std::stoull(valStr);
                        foundAny = true;
                    }
                } catch (...) {
                }
            }
        }
    }

    if (!foundAny) {
        std::string snippet = raw.substr(0, std::min<size_t>(raw.length(), 60));
        std::replace(snippet.begin(), snippet.end(), '\n', ' ');
        std::replace(snippet.begin(), snippet.end(), '\r', ' ');
        std::cout << "[ZYXEL_PARSE_ERROR] class=ZyxelSecurityCmd cmd=\"show app statistics summary\" "
                  << "line=0 reason=\"No application patrol metrics found\" snippet=\""
                  << snippet << "\"" << std::endl;
        return false;
    }

    return true;
}

bool ZyxelSecurityCmd::parseIdpStatisticsSummary(const std::string &raw, ZyxelIdpSummary &out) {
    out = ZyxelIdpSummary();
    if (raw.empty()) {
        return false;
    }

    auto lines = ZyxelScanner::splitLines(raw);
    bool foundAny = false;

    for (const auto &line : lines) {
        std::string lowerLine = line;
        std::transform(lowerLine.begin(), lowerLine.end(), lowerLine.begin(), ::tolower);

        if (lowerLine.find("status") != std::string::npos || lowerLine.find("idp") != std::string::npos) {
            if (lowerLine.find("enable") != std::string::npos ||
                lowerLine.find("active") != std::string::npos ||
                lowerLine.find("on") != std::string::npos) {
                out.enabled = true;
                foundAny = true;
            } else if (lowerLine.find("disable") != std::string::npos ||
                       lowerLine.find("off") != std::string::npos ||
                       lowerLine.find("inactive") != std::string::npos) {
                out.enabled = false;
                foundAny = true;
            }
        }

        int val = 0;
        if (ZyxelScanner::extractIntegerAfter(line, "threat", val) ||
            ZyxelScanner::extractIntegerAfter(line, "detect", val)) {
            out.threatsDetected = static_cast<uint64_t>(val);
            foundAny = true;
        }
        if (ZyxelScanner::extractIntegerAfter(line, "drop", val)) {
            out.packetsDropped = static_cast<uint64_t>(val);
            foundAny = true;
        }
        if (ZyxelScanner::extractIntegerAfter(line, "reset", val)) {
            out.connectionsReset = static_cast<uint64_t>(val);
            foundAny = true;
        }

        // Single key:value fallback for 64-bit values
        std::string key, valStr;
        if (line.find(',') == std::string::npos && ZyxelScanner::parseKeyValue(line, key, valStr, ':')) {
            std::string lowerKey = key;
            std::transform(lowerKey.begin(), lowerKey.end(), lowerKey.begin(), ::tolower);
            try {
                if (lowerKey.find("threat") != std::string::npos || lowerKey.find("detect") != std::string::npos) {
                    out.threatsDetected = std::stoull(valStr);
                    foundAny = true;
                } else if (lowerKey.find("drop") != std::string::npos) {
                    out.packetsDropped = std::stoull(valStr);
                    foundAny = true;
                } else if (lowerKey.find("reset") != std::string::npos) {
                    out.connectionsReset = std::stoull(valStr);
                    foundAny = true;
                }
            } catch (...) {
            }
        }
    }

    if (!foundAny) {
        std::string snippet = raw.substr(0, std::min<size_t>(raw.length(), 60));
        std::replace(snippet.begin(), snippet.end(), '\n', ' ');
        std::replace(snippet.begin(), snippet.end(), '\r', ' ');
        std::cout << "[ZYXEL_PARSE_ERROR] class=ZyxelSecurityCmd cmd=\"show idp statistics summary\" "
                  << "line=0 reason=\"No IDP metrics found\" snippet=\""
                  << snippet << "\"" << std::endl;
        return false;
    }

    return true;
}

// Envelope 7: Bandwidth Management (Ch 36)
std::string ZyxelSecurityCmd::cmdShowBwmActivation() {
    return "show bwm activation";
}

std::string ZyxelSecurityCmd::cmdShowBwmAll() {
    return "show bwm all";
}

std::string ZyxelSecurityCmd::cmdShowBwm(const std::string &rule) {
    if (rule.empty()) {
        return "show bwm all";
    }
    return "show bwm " + rule;
}

std::string ZyxelSecurityCmd::cmdShowBwmControlTcpAck() {
    return "show bwm control-tcp-ack";
}

std::string ZyxelSecurityCmd::cmdShowBwmDefault() {
    return "show bwm default";
}

std::string ZyxelSecurityCmd::cmdShowBwmApplicationsList() {
    return "show bwm applications list";
}

std::string ZyxelSecurityCmd::cmdBwmAppend() {
    return "bwm append";
}

std::string ZyxelSecurityCmd::cmdBwmDelete(int index) {
    return "bwm delete " + std::to_string(index);
}

std::string ZyxelSecurityCmd::cmdBwmActivate(bool enable) {
    return enable ? "bwm activate" : "no bwm activate";
}

std::string ZyxelSecurityCmd::cmdBwmControlTcpAckActivate(bool enable) {
    return enable ? "bwm control-tcp-ack activate" : "no bwm control-tcp-ack activate";
}

std::string ZyxelSecurityCmd::cmdBwmDefaultInboundPriority(int prio) {
    return "bwm default inbound priority " + std::to_string(prio);
}

std::string ZyxelSecurityCmd::cmdBwmDefaultOutboundPriority(int prio) {
    return "bwm default outbound priority " + std::to_string(prio);
}

// Envelope 7: Application Patrol (Ch 37)
std::string ZyxelSecurityCmd::cmdShowAppProfiles() {
    return "show app profiles";
}

std::string ZyxelSecurityCmd::cmdAppProfile(const std::string &name) {
    return "app " + name;
}

std::string ZyxelSecurityCmd::cmdNoAppProfile(const std::string &name) {
    return "no app " + name;
}

// Envelope 7: Anti-Virus (Ch 38)
std::string ZyxelSecurityCmd::cmdShowAntiVirusProfile() {
    return "show anti-virus profile";
}

std::string ZyxelSecurityCmd::cmdShowAntiVirusUpdate() {
    return "show anti-virus update";
}

std::string ZyxelSecurityCmd::cmdShowAntiVirusStatistics() {
    return "show anti-virus statistics";
}

std::string ZyxelSecurityCmd::cmdAntiVirusProfile(const std::string &name) {
    return "anti-virus " + name;
}

std::string ZyxelSecurityCmd::cmdNoAntiVirusProfile(const std::string &name) {
    return "no anti-virus " + name;
}

// Envelope 7: RTLS (Ch 39)
std::string ZyxelSecurityCmd::cmdShowRtls() {
    return "show rtls";
}

// Envelope 7: Reputation Filter (Ch 40)
std::string ZyxelSecurityCmd::cmdShowReputationFilterStatus() {
    return "show reputation-filter status";
}

std::string ZyxelSecurityCmd::cmdShowReputationFilterIpReputationStatus() {
    return "show reputation-filter ip-reputation status";
}

std::string ZyxelSecurityCmd::cmdShowReputationFilterDnsThreatFilterStatus() {
    return "show reputation-filter dns-threat-filter status";
}

std::string ZyxelSecurityCmd::cmdShowReputationFilterUrlThreatFilterStatus() {
    return "show reputation-filter url-threat-filter status";
}

// Envelope 7: Sandboxing (Ch 41)
std::string ZyxelSecurityCmd::cmdShowSandboxingStatus() {
    return "show sandboxing status";
}

// Envelope 7: IDP (Ch 42)
std::string ZyxelSecurityCmd::cmdShowIdpEngine() {
    return "show idp engine";
}

std::string ZyxelSecurityCmd::cmdShowIdpSignature() {
    return "show idp signature";
}

// Envelope 7: Content Filtering (Ch 43)
std::string ZyxelSecurityCmd::cmdShowContentFilterProfile() {
    return "show content-filter profile";
}

std::string ZyxelSecurityCmd::cmdShowContentFilterSettings() {
    return "show content-filter settings";
}

std::string ZyxelSecurityCmd::cmdShowContentFilterStatistics() {
    return "show content-filter statistics";
}

std::string ZyxelSecurityCmd::cmdContentFilterCfQueueFlush() {
    return "content-filter cf-queue flush";
}

// Envelope 7: Anti-Spam (Ch 44)
std::string ZyxelSecurityCmd::cmdShowAntiSpamProfile() {
    return "show anti-spam profile";
}

std::string ZyxelSecurityCmd::cmdShowAntiSpamStatistics() {
    return "show anti-spam statistics";
}

std::string ZyxelSecurityCmd::cmdAntiSpamProfileAppend() {
    return "anti-spam profile append";
}

std::string ZyxelSecurityCmd::cmdAntiSpamProfileDelete(int index) {
    return "anti-spam profile delete " + std::to_string(index);
}

// Envelope 7: CDR (Ch 45)
std::string ZyxelSecurityCmd::cmdShowCdrStatus() {
    return "show cdr status";
}

std::string ZyxelSecurityCmd::cmdShowCdrBlockList() {
    return "show cdr block-list";
}

std::string ZyxelSecurityCmd::cmdShowCdrRules() {
    return "show cdr rules";
}

std::string ZyxelSecurityCmd::cmdCdrActivate(bool enable) {
    return enable ? "cdr activate" : "no cdr activate";
}

// Envelope 7: SSL Inspection (Ch 46)
std::string ZyxelSecurityCmd::cmdShowSslInspectionStatus() {
    return "show ssl-inspection status";
}

std::string ZyxelSecurityCmd::cmdShowSslInspectionProfile() {
    return "show ssl-inspection profile";
}

std::string ZyxelSecurityCmd::cmdShowSslInspectionStatistics() {
    return "show ssl-inspection statistics";
}

std::string ZyxelSecurityCmd::cmdSslInspectionProfile(const std::string &name) {
    return "ssl-inspection profile " + name;
}

std::string ZyxelSecurityCmd::cmdNoSslInspectionProfile(const std::string &name) {
    return "no ssl-inspection profile " + name;
}

// Dry-fire helpers
std::string ZyxelSecurityCmd::cmdInvalidBwmDryFire() {
    return "show bwm 9999";
}

std::string ZyxelSecurityCmd::cmdInvalidAppDryFire() {
    return "show app invalid_subcommand_dry_fire";
}

std::string ZyxelSecurityCmd::cmdInvalidAntiVirusDryFire() {
    return "show anti-virus invalid_subcommand_dry_fire";
}

std::string ZyxelSecurityCmd::cmdInvalidSslInspectionDryFire() {
    return "show ssl-inspection invalid_subcommand_dry_fire";
}

// Envelope 7 Parsers
bool ZyxelSecurityCmd::parseBwmActivation(const std::string &raw, bool &active) {
    active = false;
    if (raw.empty()) return false;
    auto lines = ZyxelScanner::splitLines(raw);
    for (const auto &line : lines) {
        std::string lower = line;
        std::transform(lower.begin(), lower.end(), lower.begin(), ::tolower);
        if (lower.find("bwm activation") != std::string::npos) {
            if (lower.find("yes") != std::string::npos || lower.find("enable") != std::string::npos) {
                active = true;
                return true;
            } else if (lower.find("no") != std::string::npos || lower.find("disable") != std::string::npos) {
                active = false;
                return true;
            }
        }
    }
    return false;
}

bool ZyxelSecurityCmd::parseBwmControlTcpAck(const std::string &raw, bool &active) {
    active = false;
    if (raw.empty()) return false;
    auto lines = ZyxelScanner::splitLines(raw);
    for (const auto &line : lines) {
        std::string lower = line;
        std::transform(lower.begin(), lower.end(), lower.begin(), ::tolower);
        if (lower.find("tcp ack") != std::string::npos) {
            if (lower.find("yes") != std::string::npos || lower.find("enable") != std::string::npos) {
                active = true;
                return true;
            } else if (lower.find("no") != std::string::npos || lower.find("disable") != std::string::npos) {
                active = false;
                return true;
            }
        }
    }
    return false;
}

bool ZyxelSecurityCmd::parseBwmAll(const std::string &raw, std::vector<ZyxelBwmRule> &rules) {
    rules.clear();
    if (raw.empty()) return false;
    auto lines = ZyxelScanner::splitLines(raw);
    ZyxelBwmRule current;
    bool inRule = false;
    for (const auto &line : lines) {
        std::string trimmed = ZyxelScanner::trim(line);
        if (trimmed.rfind("index:", 0) == 0) {
            if (inRule) {
                rules.push_back(current);
                current = ZyxelBwmRule();
            }
            inRule = true;
            current.index = ZyxelScanner::trim(trimmed.substr(6));
            continue;
        }
        if (!inRule) continue;

        std::string key, val;
        if (ZyxelScanner::parseKeyValue(trimmed, key, val, ':')) {
            std::string lowerKey = key;
            std::transform(lowerKey.begin(), lowerKey.end(), lowerKey.begin(), ::tolower);
            std::string lowerVal = val;
            std::transform(lowerVal.begin(), lowerVal.end(), lowerVal.begin(), ::tolower);

            if (lowerKey == "activate") {
                current.active = (lowerVal == "yes" || lowerVal == "enable");
            } else if (lowerKey == "description") {
                current.description = val;
            } else if (lowerKey == "bwm type") {
                current.bwmType = val;
            } else if (lowerKey == "src") {
                current.src = val;
            } else if (lowerKey == "dst") {
                current.dst = val;
            } else if (lowerKey == "inbound") {
                try { current.inboundKbps = std::stoul(val); } catch (...) {}
            } else if (lowerKey == "outbound") {
                try { current.outboundKbps = std::stoul(val); } catch (...) {}
            }
        }
    }
    if (inRule) {
        rules.push_back(current);
    }
    return !rules.empty();
}

bool ZyxelSecurityCmd::parseCdrStatus(const std::string &raw, ZyxelCdrStatus &status) {
    status = ZyxelCdrStatus();
    if (raw.empty()) return false;
    auto lines = ZyxelScanner::splitLines(raw);
    bool foundAny = false;
    for (const auto &line : lines) {
        std::string key, val;
        if (ZyxelScanner::parseKeyValue(line, key, val, ':')) {
            std::string lowerKey = key;
            std::transform(lowerKey.begin(), lowerKey.end(), lowerKey.begin(), ::tolower);
            std::string lowerVal = val;
            std::transform(lowerVal.begin(), lowerVal.end(), lowerVal.begin(), ::tolower);

            if (lowerKey == "activate") {
                status.active = (lowerVal == "yes" || lowerVal == "enable");
                foundAny = true;
            } else if (lowerKey == "blocked-by") {
                status.blockedBy = val;
                foundAny = true;
            } else if (lowerKey == "block period") {
                try { status.blockPeriod = std::stoi(val); foundAny = true; } catch (...) {}
            } else if (lowerKey == "email-alert") {
                status.emailAlert = val;
                foundAny = true;
            }
        }
    }
    return foundAny;
}

bool ZyxelSecurityCmd::parseCdrRules(const std::string &raw, std::vector<ZyxelCdrRule> &rules) {
    rules.clear();
    if (raw.empty()) return false;
    auto lines = ZyxelScanner::splitLines(raw);
    int divIdx = ZyxelScanner::findDividerLine(lines);
    if (divIdx > 0) {
        auto cols = ZyxelScanner::parseTableColumns(lines[divIdx - 1], lines[divIdx]);
        int noCol = ZyxelScanner::findColumn(cols, "no.");
        int catCol = ZyxelScanner::findColumn(cols, "category");
        int occCol = ZyxelScanner::findColumn(cols, "occurence");
        if (occCol < 0) occCol = ZyxelScanner::findColumn(cols, "occurrence");
        int durCol = ZyxelScanner::findColumn(cols, "duration");
        int conCol = ZyxelScanner::findColumn(cols, "containment");
        int evtCol = ZyxelScanner::findColumn(cols, "event");

        for (size_t i = divIdx + 1; i < lines.size(); ++i) {
            const std::string &line = lines[i];
            std::string trimmed = ZyxelScanner::trim(line);
            if (trimmed.empty() || trimmed.find("====") == 0 || trimmed.find("----") == 0 ||
                trimmed.rfind("Router", 0) == 0) {
                continue;
            }

            if (noCol >= 0 && catCol >= 0 && occCol >= 0 && durCol >= 0 && conCol >= 0) {
                ZyxelCdrRule rule;
                try {
                    rule.index = std::stoi(ZyxelScanner::extractCell(line, cols[noCol]));
                    rule.category = ZyxelScanner::extractCell(line, cols[catCol]);
                    rule.occurrence = std::stoi(ZyxelScanner::extractCell(line, cols[occCol]));
                    rule.duration = std::stoi(ZyxelScanner::extractCell(line, cols[durCol]));
                    rule.containment = ZyxelScanner::extractCell(line, cols[conCol]);
                    if (evtCol >= 0) {
                        rule.eventType = ZyxelScanner::extractCell(line, cols[evtCol]);
                    }
                    rules.push_back(rule);
                } catch (...) {}
            }
        }
    }
    return !rules.empty();
}

bool ZyxelSecurityCmd::parseSslInspectionStatus(const std::string &raw, ZyxelSslInspectionStatus &status) {
    status = ZyxelSslInspectionStatus();
    if (raw.empty()) return false;
    auto lines = ZyxelScanner::splitLines(raw);
    bool foundAny = false;
    for (const auto &line : lines) {
        std::string key, val;
        if (ZyxelScanner::parseKeyValue(line, key, val, ':')) {
            std::string lowerKey = key;
            std::transform(lowerKey.begin(), lowerKey.end(), lowerKey.begin(), ::tolower);
            std::string lowerVal = val;
            std::transform(lowerVal.begin(), lowerVal.end(), lowerVal.begin(), ::tolower);

            if (lowerKey == "server sign cert mode") {
                status.certMode = val;
                foundAny = true;
            } else if (lowerKey == "tls 1.3 activate") {
                status.tls13Active = (lowerVal == "yes" || lowerVal == "enable");
                foundAny = true;
            } else if (lowerKey == "server query timeout sec") {
                try { status.timeoutSec = std::stoi(val); foundAny = true; } catch (...) {}
            } else if (lowerKey == "server query timeout action") {
                status.timeoutAction = val;
                foundAny = true;
            }
        }
    }
    return foundAny;
}

bool ZyxelSecurityCmd::parseContentFilterSettings(const std::string &raw, ZyxelContentFilterSettings &settings) {
    settings = ZyxelContentFilterSettings();
    if (raw.empty()) return false;
    auto lines = ZyxelScanner::splitLines(raw);
    bool foundAny = false;
    for (const auto &line : lines) {
        std::string key, val;
        if (ZyxelScanner::parseKeyValue(line, key, val, ':')) {
            std::string lowerKey = key;
            std::transform(lowerKey.begin(), lowerKey.end(), lowerKey.begin(), ::tolower);
            std::string lowerVal = val;
            std::transform(lowerVal.begin(), lowerVal.end(), lowerVal.begin(), ::tolower);

            if (lowerKey == "default block") {
                settings.defaultBlock = (lowerVal == "yes" || lowerVal == "enable");
                foundAny = true;
            } else if (lowerKey == "license key") {
                settings.licenseKey = val;
                foundAny = true;
            } else if (lowerKey == "service timeout") {
                try { settings.serviceTimeout = std::stoi(val); foundAny = true; } catch (...) {}
            } else if (lowerKey == "block message") {
                settings.blockMessage = val;
                foundAny = true;
            }
        }
    }
    return foundAny;
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
