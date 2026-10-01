/*
 * ZyxelSystemCmd.cxx
 *
 * Copyright (C) 2026, Charles Chiou
 */

#include "zyxel/ZyxelSystemCmd.hxx"
#include "zyxel/ZyxelScanner.hxx"
#include "ZyxelSshClient.hxx"

#include <algorithm>
#include <cctype>
#include <sstream>

std::string ZyxelSystemCmd::cmdShowVersion() {
    return "show version";
}

std::string ZyxelSystemCmd::cmdShowCpuStatus() {
    return "show cpu status";
}

std::string ZyxelSystemCmd::cmdShowMemStatus() {
    return "show mem status";
}

std::string ZyxelSystemCmd::cmdShowConnStatus() {
    return "show conn status";
}

std::string ZyxelSystemCmd::cmdPing(const std::string &ip, int count) {
    return "ping " + ip + " count " + std::to_string(count);
}

std::string ZyxelSystemCmd::cmdTraceroute(const std::string &ip) {
    return "traceroute " + ip;
}

std::string ZyxelSystemCmd::cmdWrite() {
    return "write";
}

std::string ZyxelSystemCmd::cmdReboot() {
    return "reboot";
}

bool ZyxelSystemCmd::parseVersion(const std::string &raw, ZyxelVersionInfo &out) {
    out = ZyxelVersionInfo();
    auto lines = ZyxelScanner::splitLines(raw);

    for (size_t i = 0; i < lines.size(); ++i) {
        const auto &line = lines[i];
        std::string key, val;
        if (ZyxelScanner::parseKeyValue(line, key, val, ':')) {
            std::string lowerKey = key;
            std::transform(lowerKey.begin(), lowerKey.end(), lowerKey.begin(), ::tolower);
            if (lowerKey == "model") {
                out.model = val;
            } else if (lowerKey == "firmware version") {
                out.firmwareVersion = val;
            } else if (lowerKey == "build date") {
                out.buildDate = val;
            } else if (lowerKey == "serial number") {
                out.serialNumber = val;
            }
        }
    }

    // Check if dual firmware table format (e.g. page 547)
    if (out.model.empty() || out.firmwareVersion.empty()) {
        int divIdx = ZyxelScanner::findDividerLine(lines);
        if (divIdx >= 0) {
            for (size_t i = divIdx + 1; i < lines.size(); ++i) {
                if (lines[i].find("Running") != std::string::npos) {
                    auto tokens = ZyxelScanner::splitTokens(lines[i]);
                    // Check if row is on this line or split across previous line
                    if (tokens.size() >= 6) {
                        // All on one line:
                        // tail tokens: [size-1]=status, [size-2]=time, [size-3]=date, [size-4]=firmwareVersion
                        // model tokens: 1 .. size-5
                        out.model.clear();
                        for (size_t m = 1; m + 4 < tokens.size(); ++m) {
                            if (!out.model.empty()) out.model += " ";
                            out.model += tokens[m];
                        }
                        out.firmwareVersion = tokens[tokens.size() - 4];
                        out.buildDate = tokens[tokens.size() - 3] + " " + tokens[tokens.size() - 2];
                    } else if (i > static_cast<size_t>(divIdx + 1)) {
                        // Split across lines: previous line has index, model, version
                        auto prevTokens = ZyxelScanner::splitTokens(lines[i - 1]);
                        if (prevTokens.size() >= 3) {
                            out.model.clear();
                            for (size_t m = 1; m + 1 < prevTokens.size(); ++m) {
                                if (!out.model.empty()) out.model += " ";
                                out.model += prevTokens[m];
                            }
                            out.firmwareVersion = prevTokens.back();
                        }
                        if (tokens.size() >= 2) {
                            out.buildDate = tokens[0] + " " + tokens[1];
                        }
                    }
                    break;
                }
            }
        }
    }

    if (out.model.empty() && out.firmwareVersion.empty()) {
        ZyxelScanner::logParseError("ZyxelSystemCmd", "show version", 0,
                                   "Missing model and firmware version",
                                   lines.empty() ? "" : lines[0]);
        return false;
    }
    return true;
}

bool ZyxelSystemCmd::parseCpuStatus(const std::string &raw, double &cpuPercentOut) {
    auto lines = ZyxelScanner::splitLines(raw);

    for (size_t i = 0; i < lines.size(); ++i) {
        std::string lower = lines[i];
        std::transform(lower.begin(), lower.end(), lower.begin(), ::tolower);
        if (lower.find("utilization") != std::string::npos || lower.find("cpu") != std::string::npos) {
            if (ZyxelScanner::extractPercentage(lines[i], cpuPercentOut)) {
                return true;
            }
        }
    }

    // Fallback: check any line with a percentage
    for (size_t i = 0; i < lines.size(); ++i) {
        if (ZyxelScanner::extractPercentage(lines[i], cpuPercentOut)) {
            return true;
        }
    }

    ZyxelScanner::logParseError("ZyxelSystemCmd", "show cpu status", 0,
                               "No CPU utilization percentage pattern found",
                               lines.empty() ? "" : lines[0]);
    return false;
}

bool ZyxelSystemCmd::parseMemStatus(const std::string &raw, double &memPercentOut) {
    auto lines = ZyxelScanner::splitLines(raw);

    for (size_t i = 0; i < lines.size(); ++i) {
        std::string lower = lines[i];
        std::transform(lower.begin(), lower.end(), lower.begin(), ::tolower);
        if (lower.find("memory") != std::string::npos || lower.find("mem") != std::string::npos) {
            if (ZyxelScanner::extractPercentage(lines[i], memPercentOut)) {
                return true;
            }
        }
    }

    // Fallback: check any line with a percentage
    for (size_t i = 0; i < lines.size(); ++i) {
        if (ZyxelScanner::extractPercentage(lines[i], memPercentOut)) {
            return true;
        }
    }

    ZyxelScanner::logParseError("ZyxelSystemCmd", "show mem status", 0,
                               "No memory usage percentage pattern found",
                               lines.empty() ? "" : lines[0]);
    return false;
}

bool ZyxelSystemCmd::parseConnStatus(const std::string &raw, ZyxelSessionSummary &out) {
    out = ZyxelSessionSummary();
    auto lines = ZyxelScanner::splitLines(raw);
    bool foundActive = false;

    for (size_t i = 0; i < lines.size(); ++i) {
        const std::string &line = lines[i];
        std::string lower = line;
        std::transform(lower.begin(), lower.end(), lower.begin(), ::tolower);

        // Check format with slash: "Current sessions: 250 / 50000" or "Sessions: 250/50000"
        size_t slashPos = lower.find('/');
        if (slashPos != std::string::npos && lower.find("session") != std::string::npos) {
            int act = 0, maxS = 0;
            std::string beforeSlash = line.substr(0, slashPos);
            if (ZyxelScanner::extractIntegerAfter(beforeSlash, "session", act) ||
                ZyxelScanner::extractIntegerAfter(beforeSlash, "current", act) ||
                ZyxelScanner::extractIntegerAfter(beforeSlash, "active", act)) {
                out.activeSessions = act;
                foundActive = true;
            }
            if (ZyxelScanner::extractIntegerAfter(line, "/", maxS)) {
                out.maxSessions = maxS;
            }
            if (foundActive) {
                break;
            }
        }

        // Check "Active sessions: 142" or "Current sessions: 142" or "Sessions: 142"
        if (lower.find("session") != std::string::npos && lower.find("max") == std::string::npos) {
            int act = 0;
            if (ZyxelScanner::extractIntegerAfter(line, "session", act) ||
                ZyxelScanner::extractIntegerAfter(line, "active", act) ||
                ZyxelScanner::extractIntegerAfter(line, "current", act)) {
                out.activeSessions = act;
                foundActive = true;
            }
        }

        // Check "Max sessions: 1000000" or "Maximum: 1000000"
        if (lower.find("max") != std::string::npos) {
            int maxS = 0;
            if (ZyxelScanner::extractIntegerAfter(line, "max session", maxS) ||
                ZyxelScanner::extractIntegerAfter(line, "maximum session", maxS) ||
                ZyxelScanner::extractIntegerAfter(line, "max", maxS)) {
                out.maxSessions = maxS;
            }
        }
    }

    if (!foundActive) {
        // Plain integer fallback if single line containing only a number
        for (const auto &line : lines) {
            std::string t = ZyxelScanner::trim(line);
            if (!t.empty() && std::all_of(t.begin(), t.end(), ::isdigit)) {
                out.activeSessions = std::stoi(t);
                foundActive = true;
                break;
            }
        }
    }

    if (foundActive) {
        if (out.maxSessions <= 0) {
            out.maxSessions = 1000000; // USG FLEX default limit
        }
        out.sessionUsagePercent = (out.maxSessions > 0) ?
            (out.activeSessions * 100.0 / out.maxSessions) : 0.0;
        return true;
    }

    ZyxelScanner::logParseError("ZyxelSystemCmd", "show conn status", 0,
                               "No active session count found",
                               lines.empty() ? "" : lines[0]);
    return false;
}

bool ZyxelSystemCmd::parsePing(const std::string &raw, ZyxelDiagnosticResult &out) {
    out = ZyxelDiagnosticResult();
    out.type = "ping";
    out.rawOutput = raw;

    auto lines = ZyxelScanner::splitLines(raw);
    bool foundPackets = false;

    for (size_t i = 0; i < lines.size(); ++i) {
        std::string lower = lines[i];
        std::transform(lower.begin(), lower.end(), lower.begin(), ::tolower);

        // e.g. "4 packets transmitted, 4 received, 0% packet loss"
        // or "4 packets transmitted, 0 packets received, 100% packet loss"
        if (lower.find("packets transmitted") != std::string::npos ||
            (lower.find("transmitted") != std::string::npos && lower.find("received") != std::string::npos)) {
            auto tokens = ZyxelScanner::splitTokens(lines[i]);
            for (size_t j = 0; j < tokens.size(); ++j) {
                std::string t = tokens[j];
                while (!t.empty() && (t.back() == ',' || t.back() == ';')) {
                    t.pop_back();
                }
                std::transform(t.begin(), t.end(), t.begin(), ::tolower);

                if (t == "transmitted" && j > 0) {
                    if (j >= 2 && (tokens[j-1] == "packets" || tokens[j-1] == "packet")) {
                        try { out.packetsTransmitted = std::stoi(tokens[j - 2]); } catch (...) {}
                    } else {
                        try { out.packetsTransmitted = std::stoi(tokens[j - 1]); } catch (...) {}
                    }
                } else if (t == "received" && j > 0) {
                    if (j >= 2 && (tokens[j-1] == "packets" || tokens[j-1] == "packet")) {
                        try { out.packetsReceived = std::stoi(tokens[j - 2]); } catch (...) {}
                    } else {
                        try { out.packetsReceived = std::stoi(tokens[j - 1]); } catch (...) {}
                    }
                }
            }
            double loss = 0.0;
            if (ZyxelScanner::extractPercentage(lines[i], loss)) {
                out.packetLossPercent = loss;
            } else if (out.packetsTransmitted > 0) {
                out.packetLossPercent = (out.packetsTransmitted - out.packetsReceived) * 100.0 / out.packetsTransmitted;
            }
            foundPackets = true;
        }

        // e.g. "rtt min/avg/max/mdev = 11.821/12.145/12.482/0.250 ms"
        // or "round-trip min/avg/max = 0.521/0.645/0.812 ms"
        if (lower.find("min/avg/max") != std::string::npos) {
            size_t eqPos = lower.find('=');
            if (eqPos != std::string::npos) {
                std::string rttPart = ZyxelScanner::trim(lines[i].substr(eqPos + 1));
                size_t spacePos = rttPart.find(' ');
                if (spacePos != std::string::npos) {
                    rttPart = rttPart.substr(0, spacePos);
                }
                std::stringstream ss(rttPart);
                std::string item;
                std::vector<double> vals;
                while (std::getline(ss, item, '/')) {
                    try { vals.push_back(std::stod(item)); } catch (...) {}
                }
                if (vals.size() >= 3) {
                    out.minLatencyMs = vals[0];
                    out.avgLatencyMs = vals[1];
                    out.maxLatencyMs = vals[2];
                }
            }
        }
    }

    // Diagnostic fallback for valid ping sessions that dropped 100% of packets
    if (!foundPackets) {
        std::string lowerRaw = raw;
        std::transform(lowerRaw.begin(), lowerRaw.end(), lowerRaw.begin(), ::tolower);
        bool hasPingHeader = (lowerRaw.find("ping ") != std::string::npos ||
                              lowerRaw.find("ping statistics") != std::string::npos);
        if (hasPingHeader && (lowerRaw.find("destination host unreachable") != std::string::npos ||
                              lowerRaw.find("destination net unreachable") != std::string::npos ||
                              lowerRaw.find("100% packet loss") != std::string::npos)) {
            out.packetsTransmitted = 1;
            out.packetsReceived = 0;
            out.packetLossPercent = 100.0;
            foundPackets = true;
        }
    }

    if (!foundPackets) {
        ZyxelScanner::logParseError("ZyxelSystemCmd", "ping", 0,
                                   "Ping statistics line missing",
                                   lines.empty() ? "" : lines[0]);
        return false;
    }
    return true;
}

bool ZyxelSystemCmd::parseTraceroute(const std::string &raw, ZyxelDiagnosticResult &out) {
    out = ZyxelDiagnosticResult();
    out.type = "traceroute";
    out.rawOutput = ZyxelSshClient::stripTrailingPrompt(raw);

    auto lines = ZyxelScanner::splitLines(out.rawOutput);

    for (const auto &line : lines) {
        std::string trimmed = ZyxelScanner::trim(line);
        if (trimmed.empty() || !std::isdigit(static_cast<unsigned char>(trimmed[0]))) {
            continue;
        }

        auto tokens = ZyxelScanner::splitTokens(trimmed);
        if (tokens.size() < 2) {
            continue;
        }

        bool isHopIndex = std::all_of(tokens[0].begin(), tokens[0].end(), ::isdigit);
        if (!isHopIndex) {
            continue;
        }

        int hopIdx = 0;
        try {
            hopIdx = std::stoi(tokens[0]);
        } catch (...) {
            continue;
        }
        if (hopIdx <= 0 || hopIdx > 255) {
            continue;
        }

        ZyxelHop hop;
        hop.hopIndex = hopIdx;
        std::string currentIp;
        bool allTimeout = true;

        for (size_t t = 1; t < tokens.size(); ++t) {
            const std::string &tok = tokens[t];
            if (tok == "*") {
                ZyxelHopProbe probe;
                probe.timeout = true;
                probe.ip = currentIp;
                hop.probes.push_back(probe);
            } else if (!tok.empty() && tok[0] == '!') {
                if (!hop.probes.empty()) {
                    hop.probes.back().icmpFlag = tok;
                }
            } else if (tok == "ms" || tok == "MS") {
                continue;
            } else {
                // Check if numeric latency (digits with at most 1 decimal point)
                size_t dots = 0;
                bool isNumeric = true;
                for (char c : tok) {
                    if (c == '.') {
                        dots++;
                    } else if (!std::isdigit(static_cast<unsigned char>(c))) {
                        isNumeric = false;
                        break;
                    }
                }

                if (isNumeric && dots <= 1 && !tok.empty()) {
                    try {
                        double rtt = std::stod(tok);
                        ZyxelHopProbe probe;
                        probe.timeout = false;
                        probe.rttMs = rtt;
                        probe.ip = currentIp;
                        allTimeout = false;
                        if (t + 1 < tokens.size() && (tokens[t + 1] == "ms" || tokens[t + 1] == "MS")) {
                            t++;
                        }
                        if (t + 1 < tokens.size() && !tokens[t + 1].empty() && tokens[t + 1][0] == '!') {
                            probe.icmpFlag = tokens[t + 1];
                            t++;
                        }
                        hop.probes.push_back(probe);
                    } catch (...) {}
                } else {
                    // Hostname or IP token
                    std::string cleanTok = tok;
                    if (cleanTok.length() >= 2 && cleanTok.front() == '(' && cleanTok.back() == ')') {
                        cleanTok = cleanTok.substr(1, cleanTok.length() - 2);
                    }
                    currentIp = cleanTok;
                    if (hop.primaryIp.empty()) {
                        hop.primaryIp = currentIp;
                    }
                }
            }
        }

        hop.isCompleteTimeout = allTimeout && !hop.probes.empty();
        if (hop.primaryIp.empty()) {
            for (const auto &p : hop.probes) {
                if (!p.ip.empty()) {
                    hop.primaryIp = p.ip;
                    break;
                }
            }
        }

        out.hops.push_back(hop);
    }

    if (!out.hops.empty()) {
        int totalProbes = 0;
        int successfulProbes = 0;
        double minLat = 999999.0;
        double maxLat = 0.0;
        double sumLat = 0.0;

        for (const auto &hop : out.hops) {
            for (const auto &probe : hop.probes) {
                totalProbes++;
                if (!probe.timeout) {
                    successfulProbes++;
                    sumLat += probe.rttMs;
                    if (probe.rttMs < minLat) minLat = probe.rttMs;
                    if (probe.rttMs > maxLat) maxLat = probe.rttMs;
                }
            }
        }

        out.packetsTransmitted = totalProbes;
        out.packetsReceived = static_cast<int>(out.hops.size());
        out.minLatencyMs = (successfulProbes > 0) ? minLat : 0.0;
        out.maxLatencyMs = (successfulProbes > 0) ? maxLat : 0.0;
        out.avgLatencyMs = (successfulProbes > 0) ? (sumLat / successfulProbes) : 0.0;
        out.packetLossPercent = (totalProbes > 0) ?
            ((totalProbes - successfulProbes) * 100.0 / totalProbes) : 0.0;
        return true;
    }

    if (raw.find("traceroute to") != std::string::npos) {
        out.packetsReceived = 0;
        return true;
    }

    ZyxelScanner::logParseError("ZyxelSystemCmd", "traceroute", 0,
                               "No traceroute hops or header detected",
                               lines.empty() ? "" : lines[0]);
    return false;
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
