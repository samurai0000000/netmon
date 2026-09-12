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

    bool isIpProtected(const std::string &ip) const;
    bool validateBlockRequest(const std::string &ip, std::string &outReason);
    void logAudit(const std::string &tool, const std::string &requester,
                  const std::string &details, bool permitted, const std::string &reason);

    nlohmann::json getAuditLog(size_t limit = 50) const;

private:
    SecurityCheckpoint();
    ~SecurityCheckpoint() = default;
    SecurityCheckpoint(const SecurityCheckpoint &) = delete;
    SecurityCheckpoint &operator=(const SecurityCheckpoint &) = delete;

    std::unordered_set<std::string> _protectedIps;
    mutable std::mutex _mutex;
    std::vector<AuditEntry> _auditLog;
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
