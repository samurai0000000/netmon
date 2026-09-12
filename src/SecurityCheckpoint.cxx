/*
 * SecurityCheckpoint.cxx
 *
 * Copyright (C) 2026, Charles Chiou
 */

#include "SecurityCheckpoint.hxx"
#include "Config.hxx"
#include <iostream>

SecurityCheckpoint &SecurityCheckpoint::getInstance() {
    static SecurityCheckpoint instance;
    return instance;
}

SecurityCheckpoint::SecurityCheckpoint() {
    // Invariant core infrastructure IPs that may NEVER be blocked by any AI agent
    _protectedIps.insert("127.0.0.1");
    _protectedIps.insert("::1");
    _protectedIps.insert("192.168.8.1");   // Zyxel USG Default Gateway
    _protectedIps.insert("192.168.8.39");  // builder (AI Hub / MCP gateway)
    _protectedIps.insert("192.168.8.245"); // fox (meshmon gateway)
    _protectedIps.insert("192.168.8.30");  // rhino (netmon sniffer host)
    _protectedIps.insert("255.255.255.255");
    _protectedIps.insert("192.168.11.255");
    _protectedIps.insert("192.168.8.255");
}

bool SecurityCheckpoint::isIpProtected(const std::string &ip) const {
    return _protectedIps.find(ip) != _protectedIps.end();
}

bool SecurityCheckpoint::validateBlockRequest(const std::string &ip, std::string &outReason) {
    if (ip.empty()) {
        outReason = "Target IP cannot be empty";
        return false;
    }

    if (isIpProtected(ip)) {
        outReason = "Target IP " + ip + " is a protected critical infrastructure invariant and cannot be blocked";
        return false;
    }

    if (!Config::getInstance().getAllowAiBlockIp()) {
        outReason = "Automated AI firewall rule modification is disabled in netmon.cfg (allow_ai_block_ip = false)";
        return false;
    }

    return true;
}

void SecurityCheckpoint::logAudit(const std::string &tool, const std::string &requester,
                                  const std::string &details, bool permitted, const std::string &reason) {
    std::lock_guard<std::mutex> lock(_mutex);
    AuditEntry entry;
    entry.timestamp = time(nullptr);
    entry.tool = tool;
    entry.requester = requester;
    entry.details = details;
    entry.permitted = permitted;
    entry.reason = reason;

    _auditLog.push_back(entry);
    if (_auditLog.size() > 1000) {
        _auditLog.erase(_auditLog.begin(), _auditLog.begin() + 100);
    }
}

nlohmann::json SecurityCheckpoint::getAuditLog(size_t limit) const {
    std::lock_guard<std::mutex> lock(_mutex);
    nlohmann::json arr = nlohmann::json::array();

    size_t total = _auditLog.size();
    size_t count = (limit < total) ? limit : total;
    size_t start = total - count;

    for (size_t i = start; i < total; ++i) {
        const auto &e = _auditLog[i];
        nlohmann::json item;
        item["timestamp"] = e.timestamp;
        item["tool"] = e.tool;
        item["requester"] = e.requester;
        item["details"] = e.details;
        item["permitted"] = e.permitted;
        item["reason"] = e.reason;
        arr.push_back(item);
    }

    nlohmann::json res;
    res["total_events"] = total;
    res["entries"] = arr;
    return res;
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
