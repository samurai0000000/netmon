/*
 * SecurityCheckpoint.cxx
 *
 * Copyright (C) 2026, Charles Chiou
 */

#include "SecurityCheckpoint.hxx"
#include "Config.hxx"
#include "ZyxelDriver.hxx"
#include <iostream>
#include <sstream>
#include <iomanip>
#include <fstream>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#include <openssl/sha.h>

SecurityCheckpoint &SecurityCheckpoint::getInstance() {
    static SecurityCheckpoint instance;
    return instance;
}

SecurityCheckpoint::SecurityCheckpoint()
    : _policyMode(PolicyMode::RequireApproval) {
    // Invariant core infrastructure IPs that may NEVER be blocked by any AI agent
    _protectedIps.insert("127.0.0.1");
    _protectedIps.insert("::1");
    _protectedIps.insert("255.255.255.255");
}

void SecurityCheckpoint::setPolicyMode(PolicyMode mode) {
    std::lock_guard<std::mutex> lock(_mutex);
    _policyMode = mode;
    SnmpDatabase::getInstance().setPolicyMode(policyModeToString(mode));
}

PolicyMode SecurityCheckpoint::getPolicyMode() const {
    std::lock_guard<std::mutex> lock(_mutex);
    return _policyMode;
}

std::string SecurityCheckpoint::policyModeToString(PolicyMode mode) {
    switch (mode) {
    case PolicyMode::Disabled:
        return "disabled";
    case PolicyMode::DryRun:
        return "dry_run";
    case PolicyMode::RequireApproval:
        return "require_approval";
    case PolicyMode::Live:
        return "live";
    default:
        return "require_approval";
    }
}

PolicyMode SecurityCheckpoint::stringToPolicyMode(const std::string &s) {
    if (s == "disabled") {
        return PolicyMode::Disabled;
    } else if (s == "dry_run") {
        return PolicyMode::DryRun;
    } else if (s == "require_approval") {
        return PolicyMode::RequireApproval;
    } else if (s == "live") {
        return PolicyMode::Live;
    }
    return PolicyMode::RequireApproval;
}

bool SecurityCheckpoint::isIpProtected(const std::string &ip) const {
    std::lock_guard<std::mutex> lock(_mutex);
    if (_protectedIps.find(ip) != _protectedIps.end()) {
        return true;
    }
    if (!Config::getInstance().getGatewayHost().empty() &&
        ip == Config::getInstance().getGatewayHost()) {
        return true;
    }
    for (const auto &target : Config::getInstance().getSnmpTargets()) {
        if (!target.ip.empty() && ip == target.ip) {
            return true;
        }
    }
    return false;
}

void SecurityCheckpoint::addProtectedIp(const std::string &ip) {
    std::lock_guard<std::mutex> lock(_mutex);
    _protectedIps.insert(ip);
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

nlohmann::json SecurityCheckpoint::handleAgentMutation(const std::string &tool,
                                                      const std::string &requester,
                                                      const nlohmann::json &payload) {
    std::string ip = payload.value("ip", "");
    if (ip.empty()) {
        return {{"status", "error"}, {"error", "Missing target IP"}};
    }

    if (isIpProtected(ip)) {
        logAudit(tool, requester, "ip=" + ip, false, "Target IP is protected");
        return {
            {"status", "denied"},
            {"target_ip", ip},
            {"reason", "Target IP " + ip + " is a protected critical infrastructure invariant and cannot be blocked"}
        };
    }

    PolicyMode mode = getPolicyMode();
    if (mode == PolicyMode::Disabled) {
        logAudit(tool, requester, "ip=" + ip, false, "Policy mode disabled");
        return {
            {"status", "disabled"},
            {"target_ip", ip},
            {"reason", "Automated AI firewall rule modification is disabled in netmon"}
        };
    }

    if (mode == PolicyMode::DryRun) {
        logAudit(tool, requester, "ip=" + ip, true, "Dry-run simulation");
        AuditOutboxRecord auditRec;
        auditRec.timestamp = time(nullptr);
        auditRec.prevHash = getNextPrevHash();
        auditRec.actionId = 0;
        auditRec.tool = tool;
        auditRec.requester = requester;
        auditRec.payload = payload.dump();
        auditRec.decision = "dry_run";
        auditRec.reason = "Dry-run simulation";
        auditRec.exported = 0;
        SnmpDatabase::getInstance().insertAuditOutbox(auditRec);
        projectAuditFile();
        return {
            {"status", "dry_run"},
            {"simulated", true},
            {"target_ip", ip},
            {"tool", tool},
            {"payload", payload}
        };
    }

    if (mode == PolicyMode::RequireApproval) {
        int ttl = payload.value("ttl", 3600);
        int64_t ticketId = enqueuePendingAction(tool, requester, payload, ttl);
        if (ticketId <= 0) {
            return {{"status", "error"}, {"error", "Failed to enqueue pending action"}};
        }
        return {
            {"status", "pending"},
            {"ticket_id", ticketId},
            {"message", "Mutation enqueued for human approval"}
        };
    }

    if (mode == PolicyMode::Live) {
        int ttl = payload.value("ttl", 3600);
        int64_t ticketId = enqueuePendingAction(tool, requester, payload, ttl);
        if (ticketId <= 0) {
            return {{"status", "error"}, {"error", "Failed to enqueue pending action"}};
        }
        return {
            {"status", "pending"},
            {"ticket_id", ticketId},
            {"message", "Mutation enqueued for human approval"}
        };
    }

    return {{"status", "error"}, {"error", "Invalid policy mode"}};
}

int64_t SecurityCheckpoint::enqueuePendingAction(const std::string &tool,
                                                const std::string &requester,
                                                const nlohmann::json &payload,
                                                int ttlSeconds) {
    PendingAction act;
    act.createdAt = time(nullptr);
    act.expiresAt = act.createdAt + ttlSeconds;
    act.requester = requester;
    act.tool = tool;
    act.payload = payload.dump();
    act.status = "pending";

    int64_t id = SnmpDatabase::getInstance().insertPendingAction(act);
    if (id > 0) {
        logAudit(tool, requester, "ticket=" + std::to_string(id), true, "Enqueued pending approval");
    }
    return id;
}

bool SecurityCheckpoint::approve(int64_t ticketId, std::string &outError) {
    PendingAction claimed;
    int64_t now = time(nullptr);
    if (!SnmpDatabase::getInstance().claimPendingAction(ticketId, now, claimed)) {
        outError = "Ticket expired, already executing, or does not exist";
        return false;
    }

    nlohmann::json payloadJson;
    try {
        payloadJson = nlohmann::json::parse(claimed.payload);
    } catch (...) {
        outError = "Malformed stored payload in ticket";
        AuditOutboxRecord auditRec;
        std::string prevHash = getNextPrevHash();
        SnmpDatabase::getInstance().commitActionFailed(ticketId, prevHash, outError, auditRec);
        projectAuditFile();
        return false;
    }

    std::string ip = payloadJson.value("ip", "");
    if (isIpProtected(ip)) {
        outError = "Target IP " + ip + " is a protected critical infrastructure invariant and cannot be blocked";
        AuditOutboxRecord auditRec;
        std::string prevHash = getNextPrevHash();
        SnmpDatabase::getInstance().commitActionFailed(ticketId, prevHash, outError, auditRec);
        projectAuditFile();
        return false;
    }

    std::string tool = claimed.tool;
    std::string method = payloadJson.value("method", "");
    std::string reason = payloadJson.value("reason", "");
    nlohmann::json routerRes;

    if (tool == "firewall_block_ip" || method == "blockIp") {
        routerRes = ZyxelDriver::getInstance().blockIp(ip, reason);
    } else if (tool == "firewall_unblock_ip" || method == "unblockIp") {
        routerRes = ZyxelDriver::getInstance().unblockIp(ip);
    } else {
        outError = "Unknown mutation tool: " + tool;
        AuditOutboxRecord auditRec;
        std::string prevHash = getNextPrevHash();
        SnmpDatabase::getInstance().commitActionFailed(ticketId, prevHash, outError, auditRec);
        projectAuditFile();
        return false;
    }

    bool success = (routerRes.value("status", "") == "success");
    AuditOutboxRecord auditRec;
    std::string prevHash = getNextPrevHash();
    std::string note = "Operator approved";
    if (routerRes.contains("show") && routerRes["show"].is_string() &&
        !routerRes["show"].get<std::string>().empty()) {
        note = routerRes["show"].get<std::string>();
    }
    if (success) {
        SnmpDatabase::getInstance().commitActionApproved(ticketId, prevHash, note, auditRec);
        projectAuditFile();
        return true;
    } else {
        outError = routerRes.value("error", "Router execution failed");
        SnmpDatabase::getInstance().commitActionFailed(ticketId, prevHash, outError, auditRec);
        projectAuditFile();
        return false;
    }
}

bool SecurityCheckpoint::deny(int64_t ticketId, const std::string &reason) {
    AuditOutboxRecord auditRec;
    std::string prevHash = getNextPrevHash();
    bool ok = SnmpDatabase::getInstance().commitActionDenied(ticketId, prevHash, reason, auditRec);
    if (ok) {
        projectAuditFile();
    }
    return ok;
}

bool SecurityCheckpoint::reconcile(int64_t ticketId, const std::string &action, std::string &outError) {
    if (action != "applied" && action != "retry") {
        outError = "Invalid reconcile action (must be 'applied' or 'retry')";
        return false;
    }

    AuditOutboxRecord auditRec;
    std::string prevHash = getNextPrevHash();
    bool ok = SnmpDatabase::getInstance().reconcileAction(ticketId, action, prevHash, auditRec);
    if (!ok) {
        outError = "Ticket is not in interrupted state or does not exist";
        return false;
    }

    projectAuditFile();
    return true;
}

std::vector<PendingAction> SecurityCheckpoint::getPendingTickets(const std::string &status) const {
    return SnmpDatabase::getInstance().listPendingActions(status);
}

bool SecurityCheckpoint::getTicket(int64_t ticketId, PendingAction &outAction) const {
    return SnmpDatabase::getInstance().getPendingAction(ticketId, outAction);
}

void SecurityCheckpoint::recoverOnStartup() {
    SnmpDatabase::getInstance().recoverExecutingActionsOnStartup();

    std::string path = getAuditFilePath();
    if (path.empty()) {
        return;
    }

    std::ifstream infile(path);
    if (!infile.good()) {
        rebuildAuditFileFromSqlite();
        return;
    }

    std::vector<AuditOutboxRecord> dbRecords = SnmpDatabase::getInstance().getAllAuditOutbox();
    std::unordered_map<int64_t, AuditOutboxRecord> dbMap;
    for (const auto &r : dbRecords) {
        dbMap[r.sequence] = r;
    }

    std::string line;
    bool mismatch = false;
    while (std::getline(infile, line)) {
        if (line.empty()) continue;
        try {
            nlohmann::json obj = nlohmann::json::parse(line);
            int64_t seq = obj.value("sequence", static_cast<int64_t>(0));
            auto it = dbMap.find(seq);
            if (it == dbMap.end()) {
                mismatch = true;
                break;
            }
            std::string fileRecordHash = obj.value("record_hash", "");
            std::string expectedRecordHash = computeRecordHash(it->second);
            if (fileRecordHash != expectedRecordHash) {
                mismatch = true;
                break;
            }
        } catch (...) {
            mismatch = true;
            break;
        }
    }
    infile.close();

    if (mismatch) {
        std::cerr << "SecurityCheckpoint: Audit file hash mismatch against SQLite authority. Rebuilding projection from SQLite." << std::endl;
        rebuildAuditFileFromSqlite();
    } else {
        projectAuditFile();
    }
}

std::string SecurityCheckpoint::getAuditFilePath() const {
    std::lock_guard<std::mutex> lock(_mutex);
    if (!_auditFilePath.empty()) {
        return _auditFilePath;
    }
    return Config::resolveHomePath(Config::getInstance().getAuditFile());
}

void SecurityCheckpoint::setAuditFilePath(const std::string &path) {
    std::lock_guard<std::mutex> lock(_mutex);
    _auditFilePath = path;
}

void SecurityCheckpoint::projectAuditFile() {
    std::lock_guard<std::mutex> lock(_auditFileMutex);
    std::string path = getAuditFilePath();
    if (path.empty()) return;

    auto unexported = SnmpDatabase::getInstance().getUnexportedAuditOutbox();
    if (unexported.empty()) return;

    int fd = ::open(path.c_str(), O_WRONLY | O_CREAT | O_APPEND, 0600);
    if (fd < 0) return;

    int64_t maxSeq = 0;
    for (const auto &rec : unexported) {
        nlohmann::json lineObj = {
            {"sequence", rec.sequence},
            {"timestamp", rec.timestamp},
            {"prev_hash", rec.prevHash},
            {"record_hash", computeRecordHash(rec)},
            {"action_id", rec.actionId},
            {"tool", rec.tool},
            {"requester", rec.requester},
            {"payload", rec.payload},
            {"decision", rec.decision},
            {"reason", rec.reason}
        };
        std::string line = lineObj.dump() + "\n";
        ssize_t written = ::write(fd, line.data(), line.size());
        (void)written;
        if (rec.sequence > maxSeq) {
            maxSeq = rec.sequence;
        }
    }
    ::fsync(fd);
    ::close(fd);

    if (maxSeq > 0) {
        SnmpDatabase::getInstance().markAuditOutboxExported(maxSeq);
    }
}

void SecurityCheckpoint::rebuildAuditFileFromSqlite() {
    std::lock_guard<std::mutex> lock(_auditFileMutex);
    std::string path = getAuditFilePath();
    if (path.empty()) return;

    std::string tmpPath = path + ".tmp";
    int fd = ::open(tmpPath.c_str(), O_WRONLY | O_CREAT | O_TRUNC, 0600);
    if (fd < 0) return;

    auto allRecords = SnmpDatabase::getInstance().getAllAuditOutbox();
    int64_t maxSeq = 0;
    for (const auto &rec : allRecords) {
        nlohmann::json lineObj = {
            {"sequence", rec.sequence},
            {"timestamp", rec.timestamp},
            {"prev_hash", rec.prevHash},
            {"record_hash", computeRecordHash(rec)},
            {"action_id", rec.actionId},
            {"tool", rec.tool},
            {"requester", rec.requester},
            {"payload", rec.payload},
            {"decision", rec.decision},
            {"reason", rec.reason}
        };
        std::string line = lineObj.dump() + "\n";
        ssize_t written = ::write(fd, line.data(), line.size());
        (void)written;
        if (rec.sequence > maxSeq) {
            maxSeq = rec.sequence;
        }
    }
    ::fsync(fd);
    ::close(fd);

    ::rename(tmpPath.c_str(), path.c_str());
    if (maxSeq > 0) {
        SnmpDatabase::getInstance().markAuditOutboxExported(maxSeq);
    }
}

std::string SecurityCheckpoint::computeRecordHash(const AuditOutboxRecord &rec) {
    std::string data = std::to_string(rec.sequence) + "|" +
                       std::to_string(rec.timestamp) + "|" +
                       rec.prevHash + "|" +
                       std::to_string(rec.actionId) + "|" +
                       rec.tool + "|" +
                       rec.requester + "|" +
                       rec.payload + "|" +
                       rec.decision + "|" +
                       rec.reason;
    unsigned char md[SHA256_DIGEST_LENGTH];
    SHA256(reinterpret_cast<const unsigned char *>(data.data()), data.size(), md);
    std::ostringstream oss;
    for (int i = 0; i < SHA256_DIGEST_LENGTH; ++i) {
        oss << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(md[i]);
    }
    return oss.str();
}

std::string SecurityCheckpoint::getNextPrevHash() const {
    AuditOutboxRecord lastRec;
    if (SnmpDatabase::getInstance().getLastAuditOutbox(lastRec)) {
        return computeRecordHash(lastRec);
    }
    return "";
}

void SecurityCheckpoint::resetForTesting() {
    std::lock_guard<std::mutex> lock(_mutex);
    std::vector<AuditEntry>().swap(_auditLog);
    _policyMode = PolicyMode::RequireApproval;
    std::unordered_set<std::string> invariants = {"127.0.0.1", "::1", "255.255.255.255"};
    for (auto it = _protectedIps.begin(); it != _protectedIps.end(); ) {
        if (invariants.find(*it) == invariants.end()) {
            it = _protectedIps.erase(it);
        } else {
            ++it;
        }
    }
    std::string().swap(_auditFilePath);
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
