/*
 * SnmpDatabase.hxx
 *
 * Copyright (C) 2026, Charles Chiou
 */

#ifndef NETMON_SNMPDATABASE_HXX
#define NETMON_SNMPDATABASE_HXX

#include <string>
#include <vector>
#include <mutex>
#include <cstdint>
#include <nlohmann/json.hpp>
#include <sqlite3.h>

struct SnmpSampleRecord {
    int64_t     timestamp = 0;
    std::string targetIp;
    std::string ifName;
    double      inBytesSec = 0.0;
    double      outBytesSec = 0.0;
    uint64_t    inHcOctets = 0;
    uint64_t    outHcOctets = 0;
    int         operStatus = 1;
    uint32_t    inErrors = 0;
    uint32_t    outErrors = 0;
};

struct SnmpHourlyRollup {
    int64_t     timestamp = 0;
    std::string targetIp;
    std::string ifName;
    double      avgInBytes = 0.0;
    double      avgOutBytes = 0.0;
    double      maxInBytes = 0.0;
    double      maxOutBytes = 0.0;
    double      totalGbIn = 0.0;
    double      totalGbOut = 0.0;
};

struct PendingAction {
    int64_t     id = 0;
    int64_t     createdAt = 0;
    int64_t     expiresAt = 0;
    std::string requester;
    std::string tool;
    std::string payload;
    std::string status; // pending, executing, approved, denied, expired, interrupted
};

struct AuditOutboxRecord {
    int64_t     sequence = 0;
    int64_t     timestamp = 0;
    std::string prevHash;
    int64_t     actionId = 0;
    std::string tool;
    std::string requester;
    std::string payload;
    std::string decision;
    std::string reason;
    int         exported = 0;
};

struct SyslogEvent {
    int64_t     id = 0;
    int64_t     timestamp = 0;
    std::string sourceIp;
    int         facility = 0;
    int         severity = 0;
    std::string tag;
    std::string message;
    bool        timeAdjacent = false;
    int64_t     adjacentAuditSeq = 0;
    std::string raw;
};

class SnmpDatabase {
public:
    static SnmpDatabase &getInstance();

    bool open(const std::string &dbPath = "");
    void close();
    bool isOpen() const;

    // Ingestion
    bool insertSample(const SnmpSampleRecord &sample);
    bool insertSamplesBatch(const std::vector<SnmpSampleRecord> &samples);

    // Queries
    nlohmann::json queryHistory(const std::string &targetIp,
                                const std::string &ifName,
                                int64_t startEpoch,
                                int64_t endEpoch,
                                int maxPoints = 300);

    nlohmann::json queryWanStats(const std::string &targetIp,
                                 const std::string &ifName,
                                 int windowHours = 24);

    double query95thPercentile(const std::string &targetIp,
                               const std::string &ifName,
                               int64_t startEpoch,
                               int64_t endEpoch,
                               bool inbound = true);

    // Maintenance & Pruning
    size_t rollupAndPrune(int rawRetentionDays = 90);
    int64_t getDatabaseSizeBytes() const;

    // Security Policy
    bool setPolicyMode(const std::string &mode);
    std::string getPolicyMode();

    // Pending Actions Queue
    int64_t insertPendingAction(const PendingAction &action);
    bool getPendingAction(int64_t id, PendingAction &outAction);
    std::vector<PendingAction> listPendingActions(const std::string &statusFilter = "");

    // Atomic execution transitions
    bool claimPendingAction(int64_t id, int64_t nowTime, PendingAction &outAction);
    bool commitActionApproved(int64_t id, const std::string &prevHash, const std::string &reason, AuditOutboxRecord &outAudit);
    bool commitActionFailed(int64_t id, const std::string &prevHash, const std::string &reason, AuditOutboxRecord &outAudit);
    bool commitActionDenied(int64_t id, const std::string &prevHash, const std::string &reason, AuditOutboxRecord &outAudit);
    bool reconcileAction(int64_t id, const std::string &reconcileMode, const std::string &prevHash, AuditOutboxRecord &outAudit);
    void recoverExecutingActionsOnStartup();

    // Audit Outbox
    int64_t insertAuditOutbox(const AuditOutboxRecord &record);
    bool getLastAuditOutbox(AuditOutboxRecord &outRecord);
    std::vector<AuditOutboxRecord> getUnexportedAuditOutbox();
    bool markAuditOutboxExported(int64_t upToSequence);
    std::vector<AuditOutboxRecord> getAllAuditOutbox();

    // Syslog Ingestion & Query
    int64_t insertSyslogEvent(SyslogEvent &event);
    std::vector<SyslogEvent> getSyslogEvents(size_t limit = 50, int64_t sinceTimestamp = 0);

    sqlite3 *getHandle() const { return _db; }

private:
    SnmpDatabase();
    ~SnmpDatabase();
    SnmpDatabase(const SnmpDatabase &) = delete;
    SnmpDatabase &operator=(const SnmpDatabase &) = delete;

    bool initSchema();
    void prepareStatements();
    void finalizeStatements();

    std::string   _dbPath;
    sqlite3      *_db;
    mutable std::mutex _mutex;

    // Prepared statements
    sqlite3_stmt *_stmtInsertSample;
};

#endif /* NETMON_SNMPDATABASE_HXX */

/*
 * Local variables:
 * mode: C++
 * c-file-style: "BSD"
 * c-basic-offset: 4
 * tab-width: 4
 * indent-tabs-mode: nil
 * End:
 */
