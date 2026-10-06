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
        int val = 0;
        if (ZyxelScanner::extractIntegerAfter(line, "forward", val)) {
            out.forwardedKb = static_cast<uint64_t>(val);
            foundAny = true;
        }
        if (ZyxelScanner::extractIntegerAfter(line, "drop", val)) {
            out.droppedKb = static_cast<uint64_t>(val);
            foundAny = true;
        }
        if (ZyxelScanner::extractIntegerAfter(line, "reject", val)) {
            out.rejectedKb = static_cast<uint64_t>(val);
            foundAny = true;
        }
        if (ZyxelScanner::extractIntegerAfter(line, "match", val) ||
            ZyxelScanner::extractIntegerAfter(line, "connection", val)) {
            out.matchedConnections = static_cast<uint64_t>(val);
            foundAny = true;
        }

        // Single key:value fallback for 64-bit values
        std::string key, valStr;
        if (line.find(',') == std::string::npos && ZyxelScanner::parseKeyValue(line, key, valStr, ':')) {
            std::string lowerKey = key;
            std::transform(lowerKey.begin(), lowerKey.end(), lowerKey.begin(), ::tolower);
            try {
                if (lowerKey.find("forward") != std::string::npos) {
                    out.forwardedKb = std::stoull(valStr);
                    foundAny = true;
                } else if (lowerKey.find("drop") != std::string::npos) {
                    out.droppedKb = std::stoull(valStr);
                    foundAny = true;
                } else if (lowerKey.find("reject") != std::string::npos) {
                    out.rejectedKb = std::stoull(valStr);
                    foundAny = true;
                } else if (lowerKey.find("match") != std::string::npos || lowerKey.find("connection") != std::string::npos) {
                    out.matchedConnections = std::stoull(valStr);
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

/*
 * Local variables:
 * mode: C++
 * c-file-style: "BSD"
 * c-basic-offset: 4
 * tab-width: 4
 * indent-tabs-mode: nil
 * End:
 */
