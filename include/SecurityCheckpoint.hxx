/*
 * SecurityCheckpoint.hxx
 *
 * Copyright (C) 2026, Charles Chiou
 */

#ifndef NETMON_SECURITYCHECKPOINT_HXX
#define NETMON_SECURITYCHECKPOINT_HXX

#include <string>
#include <vector>
#include <unordered_set>
#include <mutex>
#include <ctime>
#include <nlohmann/json.hpp>
#include "SnmpDatabase.hxx"

enum class PolicyMode {
    Disabled,
    DryRun,
    RequireApproval,
    Live
};

struct AuditEntry {
    time_t      timestamp;
    std::string tool;
    std::string requester;
    std::string details;
    bool        permitted;
    std::string reason;
};

class SecurityCheckpoint {
public:
    static SecurityCheckpoint &getInstance();

    void setPolicyMode(PolicyMode mode);
    PolicyMode getPolicyMode() const;
    static std::string policyModeToString(PolicyMode mode);
    static PolicyMode stringToPolicyMode(const std::string &s);

    bool isIpProtected(const std::string &ip) const;
    void addProtectedIp(const std::string &ip);

    bool validateBlockRequest(const std::string &ip, std::string &outReason);
    void logAudit(const std::string &tool, const std::string &requester,
                  const std::string &details, bool permitted, const std::string &reason);
    nlohmann::json getAuditLog(size_t limit = 50) const;

    // AI agent mutation entry point
    nlohmann::json handleAgentMutation(const std::string &tool,
                                      const std::string &requester,
                                      const nlohmann::json &payload);

    // Operator ticket queue
    int64_t enqueuePendingAction(const std::string &tool,
                                const std::string &requester,
                                const nlohmann::json &payload,
                                int ttlSeconds = 3600);
    bool approve(int64_t ticketId, std::string &outError);
    bool deny(int64_t ticketId, const std::string &reason);
    bool reconcile(int64_t ticketId, const std::string &action, std::string &outError);

    std::vector<PendingAction> getPendingTickets(const std::string &status = "pending") const;
    bool getTicket(int64_t ticketId, PendingAction &outAction) const;

    void recoverOnStartup();

    // Audit projection file
    void setAuditFilePath(const std::string &path);
    std::string getAuditFilePath() const;
    void projectAuditFile();
    void rebuildAuditFileFromSqlite();

    static std::string computeRecordHash(const AuditOutboxRecord &rec);

    void resetForTesting();

private:
    SecurityCheckpoint();
    ~SecurityCheckpoint() = default;
    SecurityCheckpoint(const SecurityCheckpoint &) = delete;
    SecurityCheckpoint &operator=(const SecurityCheckpoint &) = delete;

    std::string getNextPrevHash() const;

    std::unordered_set<std::string> _protectedIps;
    mutable std::mutex              _mutex;
    std::vector<AuditEntry>         _auditLog;

    PolicyMode                      _policyMode;
    std::string                     _auditFilePath;
    mutable std::mutex              _auditFileMutex;
};

#endif /* NETMON_SECURITYCHECKPOINT_HXX */

/*
 * Local variables:
 * mode: C++
 * c-file-style: "BSD"
 * c-basic-offset: 4
 * tab-width: 4
 * indent-tabs-mode: nil
 * End:
 */
