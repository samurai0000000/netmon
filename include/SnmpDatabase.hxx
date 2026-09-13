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
