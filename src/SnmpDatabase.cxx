/*
 * SnmpDatabase.cxx
 *
 * Copyright (C) 2026, Charles Chiou
 */

#include "SnmpDatabase.hxx"
#include "Config.hxx"
#include <iostream>
#include <sstream>
#include <filesystem>
#include <cmath>
#include <algorithm>

namespace fs = std::filesystem;
using json = nlohmann::json;

SnmpDatabase &SnmpDatabase::getInstance() {
    static SnmpDatabase instance;
    return instance;
}

SnmpDatabase::SnmpDatabase()
    : _dbPath("")
    , _db(nullptr)
    , _stmtInsertSample(nullptr) {
}

SnmpDatabase::~SnmpDatabase() {
    close();
}

bool SnmpDatabase::open(const std::string &dbPath) {
    std::lock_guard<std::mutex> lock(_mutex);
    if (_db != nullptr) {
        return true;
    }

    if (!dbPath.empty()) {
        _dbPath = Config::resolveHomePath(dbPath);
    } else {
        _dbPath = Config::resolveHomePath(Config::getInstance().getDatabaseFile());
    }

    try {
        fs::path p(_dbPath);
        if (p.has_parent_path()) {
            fs::create_directories(p.parent_path());
        }
    } catch (const std::exception &ex) {
        std::cerr << "SnmpDatabase: Failed to create database directory: "
                  << ex.what() << std::endl;
    }

    int rc = sqlite3_open(_dbPath.c_str(), &_db);
    if (rc != SQLITE_OK) {
        std::cerr << "SnmpDatabase: Failed to open SQLite database "
                  << _dbPath << ": " << sqlite3_errmsg(_db) << std::endl;
        if (_db) {
            sqlite3_close(_db);
            _db = nullptr;
        }
        return false;
    }

    sqlite3_busy_timeout(_db, 5000);

    if (!initSchema()) {
        std::cerr << "SnmpDatabase: Failed to initialize schema" << std::endl;
        close();
        return false;
    }

    prepareStatements();
    std::cout << "SnmpDatabase: Opened " << _dbPath << " successfully" << std::endl;
    return true;
}

void SnmpDatabase::close() {
    std::lock_guard<std::mutex> lock(_mutex);
    finalizeStatements();
    if (_db != nullptr) {
        sqlite3_close(_db);
        _db = nullptr;
    }
    std::string().swap(_dbPath);
}

bool SnmpDatabase::isOpen() const {
    std::lock_guard<std::mutex> lock(_mutex);
    return _db != nullptr;
}

bool SnmpDatabase::initSchema() {
    const char *ddlSamples =
        "CREATE TABLE IF NOT EXISTS snmp_samples ("
        "  timestamp     INTEGER NOT NULL,"
        "  target_ip     TEXT NOT NULL,"
        "  if_name       TEXT NOT NULL,"
        "  in_bytes_sec  REAL NOT NULL,"
        "  out_bytes_sec REAL NOT NULL,"
        "  in_hc_octets  INTEGER NOT NULL,"
        "  out_hc_octets INTEGER NOT NULL,"
        "  oper_status   INTEGER NOT NULL,"
        "  in_errors     INTEGER DEFAULT 0,"
        "  out_errors    INTEGER DEFAULT 0"
        ");"
        "CREATE INDEX IF NOT EXISTS idx_snmp_samples_lookup "
        "ON snmp_samples(target_ip, if_name, timestamp);";

    const char *ddlRollups =
        "CREATE TABLE IF NOT EXISTS snmp_hourly_rollups ("
        "  timestamp     INTEGER NOT NULL,"
        "  target_ip     TEXT NOT NULL,"
        "  if_name       TEXT NOT NULL,"
        "  avg_in_bytes  REAL NOT NULL,"
        "  avg_out_bytes REAL NOT NULL,"
        "  max_in_bytes  REAL NOT NULL,"
        "  max_out_bytes REAL NOT NULL,"
        "  total_gb_in   REAL NOT NULL,"
        "  total_gb_out  REAL NOT NULL,"
        "  PRIMARY KEY (target_ip, if_name, timestamp)"
        ");";

    char *err = nullptr;
    if (sqlite3_exec(_db, ddlSamples, nullptr, nullptr, &err) != SQLITE_OK) {
        std::cerr << "SnmpDatabase: DDL error (samples): " << (err ? err : "") << std::endl;
        if (err) sqlite3_free(err);
        return false;
    }

    if (sqlite3_exec(_db, ddlRollups, nullptr, nullptr, &err) != SQLITE_OK) {
        std::cerr << "SnmpDatabase: DDL error (rollups): " << (err ? err : "") << std::endl;
        if (err) sqlite3_free(err);
        return false;
    }

    const char *ddlSecurity =
        "CREATE TABLE IF NOT EXISTS pending_actions ("
        "  id          INTEGER PRIMARY KEY AUTOINCREMENT,"
        "  created_at  INTEGER NOT NULL,"
        "  expires_at  INTEGER NOT NULL,"
        "  requester   TEXT NOT NULL,"
        "  tool        TEXT NOT NULL,"
        "  payload     TEXT NOT NULL,"
        "  status      TEXT NOT NULL"
        ");"
        "CREATE INDEX IF NOT EXISTS idx_pending_actions_status ON pending_actions(status);"
        "CREATE TABLE IF NOT EXISTS audit_outbox ("
        "  sequence    INTEGER PRIMARY KEY AUTOINCREMENT,"
        "  timestamp   INTEGER NOT NULL,"
        "  prev_hash   TEXT NOT NULL,"
        "  action_id   INTEGER,"
        "  tool        TEXT NOT NULL,"
        "  requester   TEXT NOT NULL,"
        "  payload     TEXT NOT NULL,"
        "  decision    TEXT NOT NULL,"
        "  reason      TEXT NOT NULL,"
        "  exported    INTEGER DEFAULT 0"
        ");"
        "CREATE INDEX IF NOT EXISTS idx_audit_outbox_exported ON audit_outbox(exported);"
        "CREATE TABLE IF NOT EXISTS security_policy ("
        "  key   TEXT PRIMARY KEY,"
        "  value TEXT NOT NULL"
        ");"
        "INSERT OR IGNORE INTO security_policy(key, value) VALUES ('mode', 'require_approval');"
        "CREATE TABLE IF NOT EXISTS syslog_events ("
        "  id                 INTEGER PRIMARY KEY AUTOINCREMENT,"
        "  timestamp          INTEGER NOT NULL,"
        "  source_ip          TEXT NOT NULL,"
        "  facility           INTEGER NOT NULL,"
        "  severity           INTEGER NOT NULL,"
        "  tag                TEXT,"
        "  message            TEXT NOT NULL,"
        "  time_adjacent      INTEGER DEFAULT 0,"
        "  adjacent_audit_seq INTEGER DEFAULT 0,"
        "  raw                TEXT"
        ");"
        "CREATE INDEX IF NOT EXISTS idx_syslog_timestamp ON syslog_events(timestamp);"
        "CREATE INDEX IF NOT EXISTS idx_syslog_time_adj ON syslog_events(time_adjacent);";

    if (sqlite3_exec(_db, ddlSecurity, nullptr, nullptr, &err) != SQLITE_OK) {
        std::cerr << "SnmpDatabase: DDL error (security): " << (err ? err : "") << std::endl;
        if (err) sqlite3_free(err);
        return false;
    }

    recoverExecutingActionsOnStartup();

    return true;
}

void SnmpDatabase::prepareStatements() {
    const char *sqlInsert =
        "INSERT INTO snmp_samples ("
        "  timestamp, target_ip, if_name, in_bytes_sec, out_bytes_sec,"
        "  in_hc_octets, out_hc_octets, oper_status, in_errors, out_errors"
        ") VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?);";

    sqlite3_prepare_v2(_db, sqlInsert, -1, &_stmtInsertSample, nullptr);
}

void SnmpDatabase::finalizeStatements() {
    if (_stmtInsertSample) {
        sqlite3_finalize(_stmtInsertSample);
        _stmtInsertSample = nullptr;
    }
}

bool SnmpDatabase::insertSample(const SnmpSampleRecord &sample) {
    std::lock_guard<std::mutex> lock(_mutex);
    if (!_db || !_stmtInsertSample) {
        return false;
    }

    sqlite3_reset(_stmtInsertSample);
    sqlite3_bind_int64(_stmtInsertSample, 1, sample.timestamp);
    sqlite3_bind_text(_stmtInsertSample, 2, sample.targetIp.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(_stmtInsertSample, 3, sample.ifName.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_double(_stmtInsertSample, 4, sample.inBytesSec);
    sqlite3_bind_double(_stmtInsertSample, 5, sample.outBytesSec);
    sqlite3_bind_int64(_stmtInsertSample, 6, static_cast<sqlite3_int64>(sample.inHcOctets));
    sqlite3_bind_int64(_stmtInsertSample, 7, static_cast<sqlite3_int64>(sample.outHcOctets));
    sqlite3_bind_int(_stmtInsertSample, 8, sample.operStatus);
    sqlite3_bind_int64(_stmtInsertSample, 9, sample.inErrors);
    sqlite3_bind_int64(_stmtInsertSample, 10, sample.outErrors);

    return sqlite3_step(_stmtInsertSample) == SQLITE_DONE;
}

bool SnmpDatabase::insertSamplesBatch(const std::vector<SnmpSampleRecord> &samples) {
    if (samples.empty()) return true;

    std::lock_guard<std::mutex> lock(_mutex);
    if (!_db || !_stmtInsertSample) {
        return false;
    }

    for (const auto &sample : samples) {
        sqlite3_reset(_stmtInsertSample);
        sqlite3_bind_int64(_stmtInsertSample, 1, sample.timestamp);
        sqlite3_bind_text(_stmtInsertSample, 2, sample.targetIp.c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_bind_text(_stmtInsertSample, 3, sample.ifName.c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_bind_double(_stmtInsertSample, 4, sample.inBytesSec);
        sqlite3_bind_double(_stmtInsertSample, 5, sample.outBytesSec);
        sqlite3_bind_int64(_stmtInsertSample, 6, static_cast<sqlite3_int64>(sample.inHcOctets));
        sqlite3_bind_int64(_stmtInsertSample, 7, static_cast<sqlite3_int64>(sample.outHcOctets));
        sqlite3_bind_int(_stmtInsertSample, 8, sample.operStatus);
        sqlite3_bind_int64(_stmtInsertSample, 9, sample.inErrors);
        sqlite3_bind_int64(_stmtInsertSample, 10, sample.outErrors);
        sqlite3_step(_stmtInsertSample);
    }
    return true;
}

json SnmpDatabase::queryHistory(const std::string &targetIp,
                               const std::string &ifName,
                               int64_t startEpoch,
                               int64_t endEpoch,
                               int maxPoints) {
    json result = json::array();
    std::lock_guard<std::mutex> lock(_mutex);
    if (!_db) return result;

    if (maxPoints <= 0) maxPoints = 300;
    int64_t duration = endEpoch - startEpoch;
    if (duration <= 0) duration = 86400;

    int64_t bucketSec = duration / maxPoints;
    if (bucketSec < 30) bucketSec = 30;

    bool includeRollups = (bucketSec > 30) && (startEpoch < (static_cast<int64_t>(time(nullptr)) - (60 * 86400)));
    std::string sql;
    if (bucketSec <= 30) {
        sql = "SELECT timestamp, in_bytes_sec, out_bytes_sec, oper_status, in_errors, out_errors "
              "FROM snmp_samples "
              "WHERE target_ip = ? AND if_name = ? AND timestamp >= ? AND timestamp <= ? "
              "ORDER BY timestamp ASC;";
    } else if (includeRollups) {
        std::ostringstream oss;
        oss << "SELECT (timestamp / " << bucketSec << ") * " << bucketSec << " AS bucket, "
            << "AVG(in_bytes_sec), AVG(out_bytes_sec), MIN(oper_status), "
            << "MAX(in_errors), MAX(out_errors) "
            << "FROM ("
            << "  SELECT timestamp, in_bytes_sec, out_bytes_sec, oper_status, in_errors, out_errors "
            << "  FROM snmp_samples "
            << "  WHERE target_ip = ? AND if_name = ? AND timestamp >= ? AND timestamp <= ? "
            << "  UNION ALL "
            << "  SELECT timestamp, avg_in_bytes AS in_bytes_sec, avg_out_bytes AS out_bytes_sec, 1 AS oper_status, 0 AS in_errors, 0 AS out_errors "
            << "  FROM snmp_hourly_rollups "
            << "  WHERE target_ip = ? AND if_name = ? AND timestamp >= ? AND timestamp <= ? "
            << "    AND timestamp < (SELECT COALESCE(MIN(timestamp), 2147483647) FROM snmp_samples WHERE target_ip = ? AND if_name = ?)"
            << ") "
            << "GROUP BY bucket ORDER BY bucket ASC;";
        sql = oss.str();
    } else {
        std::ostringstream oss;
        oss << "SELECT (timestamp / " << bucketSec << ") * " << bucketSec << " AS bucket, "
            << "AVG(in_bytes_sec), AVG(out_bytes_sec), MIN(oper_status), "
            << "MAX(in_errors), MAX(out_errors) "
            << "FROM snmp_samples "
            << "WHERE target_ip = ? AND if_name = ? AND timestamp >= ? AND timestamp <= ? "
            << "GROUP BY bucket ORDER BY bucket ASC;";
        sql = oss.str();
    }

    sqlite3_stmt *stmt = nullptr;
    if (sqlite3_prepare_v2(_db, sql.c_str(), -1, &stmt, nullptr) == SQLITE_OK) {
        sqlite3_bind_text(stmt, 1, targetIp.c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_bind_text(stmt, 2, ifName.c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_bind_int64(stmt, 3, startEpoch);
        sqlite3_bind_int64(stmt, 4, endEpoch);

        if (includeRollups) {
            sqlite3_bind_text(stmt, 5, targetIp.c_str(), -1, SQLITE_TRANSIENT);
            sqlite3_bind_text(stmt, 6, ifName.c_str(), -1, SQLITE_TRANSIENT);
            sqlite3_bind_int64(stmt, 7, startEpoch);
            sqlite3_bind_int64(stmt, 8, endEpoch);
            sqlite3_bind_text(stmt, 9, targetIp.c_str(), -1, SQLITE_TRANSIENT);
            sqlite3_bind_text(stmt, 10, ifName.c_str(), -1, SQLITE_TRANSIENT);
        }

        while (sqlite3_step(stmt) == SQLITE_ROW) {
            int64_t ts = sqlite3_column_int64(stmt, 0);
            double inBps = sqlite3_column_double(stmt, 1);
            double outBps = sqlite3_column_double(stmt, 2);
            int status = sqlite3_column_int(stmt, 3);
            int64_t inErr = sqlite3_column_int64(stmt, 4);
            int64_t outErr = sqlite3_column_int64(stmt, 5);

            json pt;
            pt["timestamp"] = ts;
            pt["in_bytes_sec"] = inBps;
            pt["out_bytes_sec"] = outBps;
            pt["in_mbps"] = (inBps * 8.0) / 1000000.0;
            pt["out_mbps"] = (outBps * 8.0) / 1000000.0;
            pt["oper_status"] = status;
            pt["in_errors"] = inErr;
            pt["out_errors"] = outErr;
            result.push_back(pt);
        }
        sqlite3_finalize(stmt);
    }

    return result;
}

json SnmpDatabase::queryWanStats(const std::string &targetIp,
                                const std::string &ifName,
                                int windowHours) {
    json stats = {
        {"target_ip", targetIp},
        {"if_name", ifName},
        {"window_hours", windowHours},
        {"avg_in_mbps", 0.0},
        {"avg_out_mbps", 0.0},
        {"peak_in_mbps", 0.0},
        {"peak_in_timestamp", 0},
        {"peak_out_mbps", 0.0},
        {"peak_out_timestamp", 0},
        {"total_gb_in", 0.0},
        {"total_gb_out", 0.0},
        {"sample_count", 0}
    };

    std::lock_guard<std::mutex> lock(_mutex);
    if (!_db) return stats;

    int64_t now = static_cast<int64_t>(time(nullptr));
    int64_t startTs = now - (windowHours * 3600);

    const char *sql =
        "SELECT AVG(in_bytes_sec), AVG(out_bytes_sec), COUNT(*), "
        "       SUM(in_bytes_sec * 30) / 1000000000.0, "
        "       SUM(out_bytes_sec * 30) / 1000000000.0 "
        "FROM snmp_samples "
        "WHERE target_ip = ? AND if_name = ? AND timestamp >= ?;";

    sqlite3_stmt *stmt = nullptr;
    if (sqlite3_prepare_v2(_db, sql, -1, &stmt, nullptr) == SQLITE_OK) {
        sqlite3_bind_text(stmt, 1, targetIp.c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_bind_text(stmt, 2, ifName.c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_bind_int64(stmt, 3, startTs);

        if (sqlite3_step(stmt) == SQLITE_ROW) {
            double avgIn = sqlite3_column_double(stmt, 0);
            double avgOut = sqlite3_column_double(stmt, 1);
            int count = sqlite3_column_int(stmt, 2);
            double gbIn = sqlite3_column_double(stmt, 3);
            double gbOut = sqlite3_column_double(stmt, 4);

            stats["avg_in_mbps"] = (avgIn * 8.0) / 1000000.0;
            stats["avg_out_mbps"] = (avgOut * 8.0) / 1000000.0;
            stats["sample_count"] = count;
            stats["total_gb_in"] = gbIn;
            stats["total_gb_out"] = gbOut;
        }
        sqlite3_finalize(stmt);
    }

    // Peak In
    const char *sqlPeakIn =
        "SELECT timestamp, in_bytes_sec FROM snmp_samples "
        "WHERE target_ip = ? AND if_name = ? AND timestamp >= ? "
        "ORDER BY in_bytes_sec DESC LIMIT 1;";
    if (sqlite3_prepare_v2(_db, sqlPeakIn, -1, &stmt, nullptr) == SQLITE_OK) {
        sqlite3_bind_text(stmt, 1, targetIp.c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_bind_text(stmt, 2, ifName.c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_bind_int64(stmt, 3, startTs);
        if (sqlite3_step(stmt) == SQLITE_ROW) {
            stats["peak_in_timestamp"] = sqlite3_column_int64(stmt, 0);
            stats["peak_in_mbps"] = (sqlite3_column_double(stmt, 1) * 8.0) / 1000000.0;
        }
        sqlite3_finalize(stmt);
    }

    // Peak Out
    const char *sqlPeakOut =
        "SELECT timestamp, out_bytes_sec FROM snmp_samples "
        "WHERE target_ip = ? AND if_name = ? AND timestamp >= ? "
        "ORDER BY out_bytes_sec DESC LIMIT 1;";
    if (sqlite3_prepare_v2(_db, sqlPeakOut, -1, &stmt, nullptr) == SQLITE_OK) {
        sqlite3_bind_text(stmt, 1, targetIp.c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_bind_text(stmt, 2, ifName.c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_bind_int64(stmt, 3, startTs);
        if (sqlite3_step(stmt) == SQLITE_ROW) {
            stats["peak_out_timestamp"] = sqlite3_column_int64(stmt, 0);
            stats["peak_out_mbps"] = (sqlite3_column_double(stmt, 1) * 8.0) / 1000000.0;
        }
        sqlite3_finalize(stmt);
    }

    return stats;
}

double SnmpDatabase::query95thPercentile(const std::string &targetIp,
                                        const std::string &ifName,
                                        int64_t startEpoch,
                                        int64_t endEpoch,
                                        bool inbound) {
    std::lock_guard<std::mutex> lock(_mutex);
    if (!_db) return 0.0;

    std::string col = inbound ? "in_bytes_sec" : "out_bytes_sec";
    std::string sql = "SELECT " + col + " FROM snmp_samples "
                      "WHERE target_ip = ? AND if_name = ? AND timestamp >= ? AND timestamp <= ? "
                      "ORDER BY " + col + " ASC;";

    std::vector<double> rates;
    sqlite3_stmt *stmt = nullptr;
    if (sqlite3_prepare_v2(_db, sql.c_str(), -1, &stmt, nullptr) == SQLITE_OK) {
        sqlite3_bind_text(stmt, 1, targetIp.c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_bind_text(stmt, 2, ifName.c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_bind_int64(stmt, 3, startEpoch);
        sqlite3_bind_int64(stmt, 4, endEpoch);

        while (sqlite3_step(stmt) == SQLITE_ROW) {
            rates.push_back(sqlite3_column_double(stmt, 0));
        }
        sqlite3_finalize(stmt);
    }

    if (rates.empty()) return 0.0;
    size_t idx = static_cast<size_t>(std::floor(rates.size() * 0.95));
    if (idx >= rates.size()) idx = rates.size() - 1;

    return (rates[idx] * 8.0) / 1000000.0; // Return Mbps
}

size_t SnmpDatabase::rollupAndPrune(int rawRetentionDays) {
    std::lock_guard<std::mutex> lock(_mutex);
    if (!_db) return 0;

    if (rawRetentionDays <= 0) rawRetentionDays = 90;
    int64_t cutoff = static_cast<int64_t>(time(nullptr)) - (rawRetentionDays * 86400);

    // Rollup hourly aggregates before pruning
    const char *sqlRollup =
        "INSERT OR REPLACE INTO snmp_hourly_rollups "
        "SELECT (timestamp / 3600) * 3600 AS hour_ts, target_ip, if_name, "
        "       AVG(in_bytes_sec), AVG(out_bytes_sec), "
        "       MAX(in_bytes_sec), MAX(out_bytes_sec), "
        "       SUM(in_bytes_sec * 30) / 1000000000.0, "
        "       SUM(out_bytes_sec * 30) / 1000000000.0 "
        "FROM snmp_samples "
        "WHERE timestamp < ? "
        "GROUP BY hour_ts, target_ip, if_name;";

    sqlite3_stmt *stmt = nullptr;
    if (sqlite3_prepare_v2(_db, sqlRollup, -1, &stmt, nullptr) == SQLITE_OK) {
        sqlite3_bind_int64(stmt, 1, cutoff);
        sqlite3_step(stmt);
        sqlite3_finalize(stmt);
    }

    // Delete pruned raw samples
    const char *sqlPrune = "DELETE FROM snmp_samples WHERE timestamp < ?;";
    size_t deletedCount = 0;
    if (sqlite3_prepare_v2(_db, sqlPrune, -1, &stmt, nullptr) == SQLITE_OK) {
        sqlite3_bind_int64(stmt, 1, cutoff);
        if (sqlite3_step(stmt) == SQLITE_DONE) {
            deletedCount = static_cast<size_t>(sqlite3_changes(_db));
        }
        sqlite3_finalize(stmt);
    }

    return deletedCount;
}

int64_t SnmpDatabase::getDatabaseSizeBytes() const {
    try {
        if (!_dbPath.empty() && fs::exists(_dbPath)) {
            return static_cast<int64_t>(fs::file_size(_dbPath));
        }
    } catch (...) {
    }
    return 0;
}

bool SnmpDatabase::setPolicyMode(const std::string &mode) {
    std::lock_guard<std::mutex> lock(_mutex);
    if (!_db) return false;

    const char *sql = "INSERT OR REPLACE INTO security_policy(key, value) VALUES ('mode', ?);";
    sqlite3_stmt *stmt = nullptr;
    if (sqlite3_prepare_v2(_db, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        return false;
    }
    sqlite3_bind_text(stmt, 1, mode.c_str(), -1, SQLITE_TRANSIENT);
    bool ok = (sqlite3_step(stmt) == SQLITE_DONE);
    sqlite3_finalize(stmt);
    return ok;
}

std::string SnmpDatabase::getPolicyMode() {
    std::lock_guard<std::mutex> lock(_mutex);
    if (!_db) return "require_approval";

    const char *sql = "SELECT value FROM security_policy WHERE key = 'mode';";
    sqlite3_stmt *stmt = nullptr;
    if (sqlite3_prepare_v2(_db, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        return "require_approval";
    }

    std::string mode = "require_approval";
    if (sqlite3_step(stmt) == SQLITE_ROW) {
        const unsigned char *val = sqlite3_column_text(stmt, 0);
        if (val) mode = reinterpret_cast<const char *>(val);
    }
    sqlite3_finalize(stmt);
    return mode;
}

int64_t SnmpDatabase::insertPendingAction(const PendingAction &action) {
    std::lock_guard<std::mutex> lock(_mutex);
    if (!_db) return 0;

    const char *sql = "INSERT INTO pending_actions (created_at, expires_at, requester, tool, payload, status) "
                      "VALUES (?, ?, ?, ?, ?, ?);";
    sqlite3_stmt *stmt = nullptr;
    if (sqlite3_prepare_v2(_db, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        return 0;
    }

    sqlite3_bind_int64(stmt, 1, action.createdAt);
    sqlite3_bind_int64(stmt, 2, action.expiresAt);
    sqlite3_bind_text(stmt, 3, action.requester.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 4, action.tool.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 5, action.payload.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 6, action.status.c_str(), -1, SQLITE_TRANSIENT);

    int64_t id = 0;
    if (sqlite3_step(stmt) == SQLITE_DONE) {
        id = sqlite3_last_insert_rowid(_db);
    }
    sqlite3_finalize(stmt);
    return id;
}

bool SnmpDatabase::getPendingAction(int64_t id, PendingAction &outAction) {
    std::lock_guard<std::mutex> lock(_mutex);
    if (!_db) return false;

    const char *sql = "SELECT id, created_at, expires_at, requester, tool, payload, status "
                      "FROM pending_actions WHERE id = ?;";
    sqlite3_stmt *stmt = nullptr;
    if (sqlite3_prepare_v2(_db, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        return false;
    }

    sqlite3_bind_int64(stmt, 1, id);
    bool found = false;
    if (sqlite3_step(stmt) == SQLITE_ROW) {
        outAction.id = sqlite3_column_int64(stmt, 0);
        outAction.createdAt = sqlite3_column_int64(stmt, 1);
        outAction.expiresAt = sqlite3_column_int64(stmt, 2);
        outAction.requester = reinterpret_cast<const char *>(sqlite3_column_text(stmt, 3));
        outAction.tool = reinterpret_cast<const char *>(sqlite3_column_text(stmt, 4));
        outAction.payload = reinterpret_cast<const char *>(sqlite3_column_text(stmt, 5));
        outAction.status = reinterpret_cast<const char *>(sqlite3_column_text(stmt, 6));
        found = true;
    }
    sqlite3_finalize(stmt);
    return found;
}

std::vector<PendingAction> SnmpDatabase::listPendingActions(const std::string &statusFilter) {
    std::lock_guard<std::mutex> lock(_mutex);
    std::vector<PendingAction> list;
    if (!_db) return list;

    std::string sql;
    if (statusFilter.empty()) {
        sql = "SELECT id, created_at, expires_at, requester, tool, payload, status FROM pending_actions ORDER BY id ASC;";
    } else {
        sql = "SELECT id, created_at, expires_at, requester, tool, payload, status FROM pending_actions WHERE status = ? ORDER BY id ASC;";
    }

    sqlite3_stmt *stmt = nullptr;
    if (sqlite3_prepare_v2(_db, sql.c_str(), -1, &stmt, nullptr) != SQLITE_OK) {
        return list;
    }

    if (!statusFilter.empty()) {
        sqlite3_bind_text(stmt, 1, statusFilter.c_str(), -1, SQLITE_TRANSIENT);
    }

    while (sqlite3_step(stmt) == SQLITE_ROW) {
        PendingAction a;
        a.id = sqlite3_column_int64(stmt, 0);
        a.createdAt = sqlite3_column_int64(stmt, 1);
        a.expiresAt = sqlite3_column_int64(stmt, 2);
        a.requester = reinterpret_cast<const char *>(sqlite3_column_text(stmt, 3));
        a.tool = reinterpret_cast<const char *>(sqlite3_column_text(stmt, 4));
        a.payload = reinterpret_cast<const char *>(sqlite3_column_text(stmt, 5));
        a.status = reinterpret_cast<const char *>(sqlite3_column_text(stmt, 6));
        list.push_back(a);
    }
    sqlite3_finalize(stmt);
    return list;
}

bool SnmpDatabase::claimPendingAction(int64_t id, int64_t nowTime, PendingAction &outAction) {
    std::lock_guard<std::mutex> lock(_mutex);
    if (!_db) return false;

    const char *sqlUpdate = "UPDATE pending_actions SET status = 'executing' "
                            "WHERE id = ? AND status = 'pending' AND expires_at > ?;";
    sqlite3_stmt *stmt = nullptr;
    if (sqlite3_prepare_v2(_db, sqlUpdate, -1, &stmt, nullptr) != SQLITE_OK) {
        return false;
    }

    sqlite3_bind_int64(stmt, 1, id);
    sqlite3_bind_int64(stmt, 2, nowTime);
    int stepRet = sqlite3_step(stmt);
    sqlite3_finalize(stmt);

    if (stepRet != SQLITE_DONE || sqlite3_changes(_db) != 1) {
        return false;
    }

    const char *sqlSelect = "SELECT id, created_at, expires_at, requester, tool, payload, status "
                            "FROM pending_actions WHERE id = ?;";
    if (sqlite3_prepare_v2(_db, sqlSelect, -1, &stmt, nullptr) != SQLITE_OK) {
        return false;
    }
    sqlite3_bind_int64(stmt, 1, id);
    bool found = false;
    if (sqlite3_step(stmt) == SQLITE_ROW) {
        outAction.id = sqlite3_column_int64(stmt, 0);
        outAction.createdAt = sqlite3_column_int64(stmt, 1);
        outAction.expiresAt = sqlite3_column_int64(stmt, 2);
        const char *t3 = reinterpret_cast<const char *>(sqlite3_column_text(stmt, 3));
        const char *t4 = reinterpret_cast<const char *>(sqlite3_column_text(stmt, 4));
        const char *t5 = reinterpret_cast<const char *>(sqlite3_column_text(stmt, 5));
        const char *t6 = reinterpret_cast<const char *>(sqlite3_column_text(stmt, 6));
        outAction.requester = t3 ? t3 : "";
        outAction.tool = t4 ? t4 : "";
        outAction.payload = t5 ? t5 : "";
        outAction.status = t6 ? t6 : "";
        found = true;
    }
    sqlite3_finalize(stmt);
    return found;
}

bool SnmpDatabase::commitActionApproved(int64_t id, const std::string &prevHash, const std::string &reason, AuditOutboxRecord &outAudit) {
    std::lock_guard<std::mutex> lock(_mutex);
    if (!_db) return false;

    PendingAction act;
    const char *sqlSel = "SELECT tool, requester, payload FROM pending_actions WHERE id = ?;";
    sqlite3_stmt *stmt = nullptr;
    if (sqlite3_prepare_v2(_db, sqlSel, -1, &stmt, nullptr) != SQLITE_OK) {
        return false;
    }
    sqlite3_bind_int64(stmt, 1, id);
    if (sqlite3_step(stmt) != SQLITE_ROW) {
        sqlite3_finalize(stmt);
        return false;
    }
    const char *at0 = reinterpret_cast<const char *>(sqlite3_column_text(stmt, 0));
    const char *at1 = reinterpret_cast<const char *>(sqlite3_column_text(stmt, 1));
    const char *at2 = reinterpret_cast<const char *>(sqlite3_column_text(stmt, 2));
    act.tool = at0 ? at0 : "";
    act.requester = at1 ? at1 : "";
    act.payload = at2 ? at2 : "";
    sqlite3_finalize(stmt);

    const char *sqlUp = "UPDATE pending_actions SET status = 'approved' WHERE id = ? AND status = 'executing';";
    if (sqlite3_prepare_v2(_db, sqlUp, -1, &stmt, nullptr) != SQLITE_OK) {
        return false;
    }
    sqlite3_bind_int64(stmt, 1, id);
    sqlite3_step(stmt);
    sqlite3_finalize(stmt);

    if (sqlite3_changes(_db) == 0) {
        return false;
    }

    int64_t now = time(nullptr);
    const char *sqlAudit = "INSERT INTO audit_outbox (timestamp, prev_hash, action_id, tool, requester, payload, decision, reason, exported) "
                           "VALUES (?, ?, ?, ?, ?, ?, 'approved', ?, 0);";
    if (sqlite3_prepare_v2(_db, sqlAudit, -1, &stmt, nullptr) != SQLITE_OK) {
        return false;
    }
    sqlite3_bind_int64(stmt, 1, now);
    sqlite3_bind_text(stmt, 2, prevHash.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_int64(stmt, 3, id);
    sqlite3_bind_text(stmt, 4, act.tool.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 5, act.requester.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 6, act.payload.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 7, reason.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_step(stmt);
    sqlite3_finalize(stmt);

    int64_t seq = sqlite3_last_insert_rowid(_db);
    outAudit.sequence = seq;
    outAudit.timestamp = now;
    outAudit.prevHash = prevHash;
    outAudit.actionId = id;
    outAudit.tool = act.tool;
    outAudit.requester = act.requester;
    outAudit.payload = act.payload;
    outAudit.decision = "approved";
    outAudit.reason = reason;
    outAudit.exported = 0;
    return true;
}

bool SnmpDatabase::commitActionFailed(int64_t id, const std::string &prevHash, const std::string &reason, AuditOutboxRecord &outAudit) {
    std::lock_guard<std::mutex> lock(_mutex);
    if (!_db) return false;

    PendingAction act;
    const char *sqlSel = "SELECT tool, requester, payload FROM pending_actions WHERE id = ?;";
    sqlite3_stmt *stmt = nullptr;
    if (sqlite3_prepare_v2(_db, sqlSel, -1, &stmt, nullptr) != SQLITE_OK) {
        return false;
    }
    sqlite3_bind_int64(stmt, 1, id);
    if (sqlite3_step(stmt) != SQLITE_ROW) {
        sqlite3_finalize(stmt);
        return false;
    }
    const char *ft0 = reinterpret_cast<const char *>(sqlite3_column_text(stmt, 0));
    const char *ft1 = reinterpret_cast<const char *>(sqlite3_column_text(stmt, 1));
    const char *ft2 = reinterpret_cast<const char *>(sqlite3_column_text(stmt, 2));
    act.tool = ft0 ? ft0 : "";
    act.requester = ft1 ? ft1 : "";
    act.payload = ft2 ? ft2 : "";
    sqlite3_finalize(stmt);

    const char *sqlUp = "UPDATE pending_actions SET status = 'pending' WHERE id = ? AND status = 'executing';";
    if (sqlite3_prepare_v2(_db, sqlUp, -1, &stmt, nullptr) != SQLITE_OK) {
        return false;
    }
    sqlite3_bind_int64(stmt, 1, id);
    sqlite3_step(stmt);
    sqlite3_finalize(stmt);

    if (sqlite3_changes(_db) == 0) {
        return false;
    }

    int64_t now = time(nullptr);
    const char *sqlAudit = "INSERT INTO audit_outbox (timestamp, prev_hash, action_id, tool, requester, payload, decision, reason, exported) "
                           "VALUES (?, ?, ?, ?, ?, ?, 'failed', ?, 0);";
    if (sqlite3_prepare_v2(_db, sqlAudit, -1, &stmt, nullptr) != SQLITE_OK) {
        return false;
    }
    sqlite3_bind_int64(stmt, 1, now);
    sqlite3_bind_text(stmt, 2, prevHash.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_int64(stmt, 3, id);
    sqlite3_bind_text(stmt, 4, act.tool.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 5, act.requester.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 6, act.payload.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 7, reason.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_step(stmt);
    sqlite3_finalize(stmt);

    int64_t seq = sqlite3_last_insert_rowid(_db);
    outAudit.sequence = seq;
    outAudit.timestamp = now;
    outAudit.prevHash = prevHash;
    outAudit.actionId = id;
    outAudit.tool = act.tool;
    outAudit.requester = act.requester;
    outAudit.payload = act.payload;
    outAudit.decision = "failed";
    outAudit.reason = reason;
    outAudit.exported = 0;
    return true;
}

bool SnmpDatabase::commitActionDenied(int64_t id, const std::string &prevHash, const std::string &reason, AuditOutboxRecord &outAudit) {
    std::lock_guard<std::mutex> lock(_mutex);
    if (!_db) return false;

    const char *sqlUp = "UPDATE pending_actions SET status = 'denied' WHERE id = ? AND status = 'pending';";
    sqlite3_stmt *stmt = nullptr;
    if (sqlite3_prepare_v2(_db, sqlUp, -1, &stmt, nullptr) != SQLITE_OK) {
        return false;
    }
    sqlite3_bind_int64(stmt, 1, id);
    sqlite3_step(stmt);
    sqlite3_finalize(stmt);

    if (sqlite3_changes(_db) == 0) {
        return false;
    }

    PendingAction act;
    const char *sqlSel = "SELECT tool, requester, payload FROM pending_actions WHERE id = ?;";
    if (sqlite3_prepare_v2(_db, sqlSel, -1, &stmt, nullptr) != SQLITE_OK) {
        return false;
    }
    sqlite3_bind_int64(stmt, 1, id);
    if (sqlite3_step(stmt) != SQLITE_ROW) {
        sqlite3_finalize(stmt);
        return false;
    }
    act.tool = reinterpret_cast<const char *>(sqlite3_column_text(stmt, 0));
    act.requester = reinterpret_cast<const char *>(sqlite3_column_text(stmt, 1));
    act.payload = reinterpret_cast<const char *>(sqlite3_column_text(stmt, 2));
    sqlite3_finalize(stmt);

    int64_t now = time(nullptr);
    const char *sqlAudit = "INSERT INTO audit_outbox (timestamp, prev_hash, action_id, tool, requester, payload, decision, reason, exported) "
                           "VALUES (?, ?, ?, ?, ?, ?, 'denied', ?, 0);";
    if (sqlite3_prepare_v2(_db, sqlAudit, -1, &stmt, nullptr) != SQLITE_OK) {
        return false;
    }
    sqlite3_bind_int64(stmt, 1, now);
    sqlite3_bind_text(stmt, 2, prevHash.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_int64(stmt, 3, id);
    sqlite3_bind_text(stmt, 4, act.tool.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 5, act.requester.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 6, act.payload.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 7, reason.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_step(stmt);
    sqlite3_finalize(stmt);

    int64_t seq = sqlite3_last_insert_rowid(_db);
    outAudit.sequence = seq;
    outAudit.timestamp = now;
    outAudit.prevHash = prevHash;
    outAudit.actionId = id;
    outAudit.tool = act.tool;
    outAudit.requester = act.requester;
    outAudit.payload = act.payload;
    outAudit.decision = "denied";
    outAudit.reason = reason;
    outAudit.exported = 0;
    return true;
}

bool SnmpDatabase::reconcileAction(int64_t id, const std::string &reconcileMode, const std::string &prevHash, AuditOutboxRecord &outAudit) {
    if (reconcileMode != "applied" && reconcileMode != "retry") {
        return false;
    }

    std::lock_guard<std::mutex> lock(_mutex);
    if (!_db) return false;

    std::string newStatus = (reconcileMode == "applied") ? "approved" : "pending";
    const char *sqlUp = "UPDATE pending_actions SET status = ? WHERE id = ? AND status = 'interrupted';";
    sqlite3_stmt *stmt = nullptr;
    if (sqlite3_prepare_v2(_db, sqlUp, -1, &stmt, nullptr) != SQLITE_OK) {
        return false;
    }
    sqlite3_bind_text(stmt, 1, newStatus.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_int64(stmt, 2, id);
    sqlite3_step(stmt);
    sqlite3_finalize(stmt);

    if (sqlite3_changes(_db) == 0) {
        return false;
    }

    PendingAction act;
    const char *sqlSel = "SELECT tool, requester, payload FROM pending_actions WHERE id = ?;";
    if (sqlite3_prepare_v2(_db, sqlSel, -1, &stmt, nullptr) != SQLITE_OK) {
        return false;
    }
    sqlite3_bind_int64(stmt, 1, id);
    if (sqlite3_step(stmt) != SQLITE_ROW) {
        sqlite3_finalize(stmt);
        return false;
    }
    act.tool = reinterpret_cast<const char *>(sqlite3_column_text(stmt, 0));
    act.requester = reinterpret_cast<const char *>(sqlite3_column_text(stmt, 1));
    act.payload = reinterpret_cast<const char *>(sqlite3_column_text(stmt, 2));
    sqlite3_finalize(stmt);

    int64_t now = time(nullptr);
    std::string dec = (reconcileMode == "applied") ? "approved" : "reconciled";
    std::string reason = (reconcileMode == "applied") ? "Operator reconciled: applied without router I/O"
                                                     : "Operator reconciled: returned to pending queue";

    const char *sqlAudit = "INSERT INTO audit_outbox (timestamp, prev_hash, action_id, tool, requester, payload, decision, reason, exported) "
                           "VALUES (?, ?, ?, ?, ?, ?, ?, ?, 0);";
    if (sqlite3_prepare_v2(_db, sqlAudit, -1, &stmt, nullptr) != SQLITE_OK) {
        return false;
    }
    sqlite3_bind_int64(stmt, 1, now);
    sqlite3_bind_text(stmt, 2, prevHash.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_int64(stmt, 3, id);
    sqlite3_bind_text(stmt, 4, act.tool.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 5, act.requester.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 6, act.payload.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 7, dec.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 8, reason.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_step(stmt);
    sqlite3_finalize(stmt);

    int64_t seq = sqlite3_last_insert_rowid(_db);
    outAudit.sequence = seq;
    outAudit.timestamp = now;
    outAudit.prevHash = prevHash;
    outAudit.actionId = id;
    outAudit.tool = act.tool;
    outAudit.requester = act.requester;
    outAudit.payload = act.payload;
    outAudit.decision = dec;
    outAudit.reason = reason;
    outAudit.exported = 0;
    return true;
}

void SnmpDatabase::recoverExecutingActionsOnStartup() {
    if (!_db) return;
    const char *sql = "UPDATE pending_actions SET status = 'interrupted' WHERE status = 'executing';";
    sqlite3_exec(_db, sql, nullptr, nullptr, nullptr);
}

int64_t SnmpDatabase::insertAuditOutbox(const AuditOutboxRecord &record) {
    std::lock_guard<std::mutex> lock(_mutex);
    if (!_db) return 0;

    const char *sql = "INSERT INTO audit_outbox (timestamp, prev_hash, action_id, tool, requester, payload, decision, reason, exported) "
                      "VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?);";
    sqlite3_stmt *stmt = nullptr;
    if (sqlite3_prepare_v2(_db, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        return 0;
    }

    sqlite3_bind_int64(stmt, 1, record.timestamp);
    sqlite3_bind_text(stmt, 2, record.prevHash.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_int64(stmt, 3, record.actionId);
    sqlite3_bind_text(stmt, 4, record.tool.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 5, record.requester.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 6, record.payload.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 7, record.decision.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 8, record.reason.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_int(stmt, 9, record.exported);

    int64_t seq = 0;
    if (sqlite3_step(stmt) == SQLITE_DONE) {
        seq = sqlite3_last_insert_rowid(_db);
    }
    sqlite3_finalize(stmt);
    return seq;
}

bool SnmpDatabase::getLastAuditOutbox(AuditOutboxRecord &outRecord) {
    std::lock_guard<std::mutex> lock(_mutex);
    if (!_db) return false;

    const char *sql = "SELECT sequence, timestamp, prev_hash, action_id, tool, requester, payload, decision, reason, exported "
                      "FROM audit_outbox ORDER BY sequence DESC LIMIT 1;";
    sqlite3_stmt *stmt = nullptr;
    if (sqlite3_prepare_v2(_db, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        return false;
    }

    bool found = false;
    if (sqlite3_step(stmt) == SQLITE_ROW) {
        outRecord.sequence = sqlite3_column_int64(stmt, 0);
        outRecord.timestamp = sqlite3_column_int64(stmt, 1);
        outRecord.prevHash = reinterpret_cast<const char *>(sqlite3_column_text(stmt, 2));
        outRecord.actionId = sqlite3_column_int64(stmt, 3);
        outRecord.tool = reinterpret_cast<const char *>(sqlite3_column_text(stmt, 4));
        outRecord.requester = reinterpret_cast<const char *>(sqlite3_column_text(stmt, 5));
        outRecord.payload = reinterpret_cast<const char *>(sqlite3_column_text(stmt, 6));
        outRecord.decision = reinterpret_cast<const char *>(sqlite3_column_text(stmt, 7));
        outRecord.reason = reinterpret_cast<const char *>(sqlite3_column_text(stmt, 8));
        outRecord.exported = sqlite3_column_int(stmt, 9);
        found = true;
    }
    sqlite3_finalize(stmt);
    return found;
}

std::vector<AuditOutboxRecord> SnmpDatabase::getUnexportedAuditOutbox() {
    std::lock_guard<std::mutex> lock(_mutex);
    std::vector<AuditOutboxRecord> list;
    if (!_db) return list;

    const char *sql = "SELECT sequence, timestamp, prev_hash, action_id, tool, requester, payload, decision, reason, exported "
                      "FROM audit_outbox WHERE exported = 0 ORDER BY sequence ASC;";
    sqlite3_stmt *stmt = nullptr;
    if (sqlite3_prepare_v2(_db, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        return list;
    }

    while (sqlite3_step(stmt) == SQLITE_ROW) {
        AuditOutboxRecord r;
        r.sequence = sqlite3_column_int64(stmt, 0);
        r.timestamp = sqlite3_column_int64(stmt, 1);
        r.prevHash = reinterpret_cast<const char *>(sqlite3_column_text(stmt, 2));
        r.actionId = sqlite3_column_int64(stmt, 3);
        r.tool = reinterpret_cast<const char *>(sqlite3_column_text(stmt, 4));
        r.requester = reinterpret_cast<const char *>(sqlite3_column_text(stmt, 5));
        r.payload = reinterpret_cast<const char *>(sqlite3_column_text(stmt, 6));
        r.decision = reinterpret_cast<const char *>(sqlite3_column_text(stmt, 7));
        r.reason = reinterpret_cast<const char *>(sqlite3_column_text(stmt, 8));
        r.exported = sqlite3_column_int(stmt, 9);
        list.push_back(r);
    }
    sqlite3_finalize(stmt);
    return list;
}

bool SnmpDatabase::markAuditOutboxExported(int64_t upToSequence) {
    std::lock_guard<std::mutex> lock(_mutex);
    if (!_db) return false;

    const char *sql = "UPDATE audit_outbox SET exported = 1 WHERE sequence <= ?;";
    sqlite3_stmt *stmt = nullptr;
    if (sqlite3_prepare_v2(_db, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        return false;
    }
    sqlite3_bind_int64(stmt, 1, upToSequence);
    bool ok = (sqlite3_step(stmt) == SQLITE_DONE);
    sqlite3_finalize(stmt);
    return ok;
}

std::vector<AuditOutboxRecord> SnmpDatabase::getAllAuditOutbox() {
    std::lock_guard<std::mutex> lock(_mutex);
    std::vector<AuditOutboxRecord> list;
    if (!_db) return list;

    const char *sql = "SELECT sequence, timestamp, prev_hash, action_id, tool, requester, payload, decision, reason, exported "
                      "FROM audit_outbox ORDER BY sequence ASC;";
    sqlite3_stmt *stmt = nullptr;
    if (sqlite3_prepare_v2(_db, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        return list;
    }

    while (sqlite3_step(stmt) == SQLITE_ROW) {
        AuditOutboxRecord r;
        r.sequence = sqlite3_column_int64(stmt, 0);
        r.timestamp = sqlite3_column_int64(stmt, 1);
        r.prevHash = reinterpret_cast<const char *>(sqlite3_column_text(stmt, 2));
        r.actionId = sqlite3_column_int64(stmt, 3);
        r.tool = reinterpret_cast<const char *>(sqlite3_column_text(stmt, 4));
        r.requester = reinterpret_cast<const char *>(sqlite3_column_text(stmt, 5));
        r.payload = reinterpret_cast<const char *>(sqlite3_column_text(stmt, 6));
        r.decision = reinterpret_cast<const char *>(sqlite3_column_text(stmt, 7));
        r.reason = reinterpret_cast<const char *>(sqlite3_column_text(stmt, 8));
        r.exported = sqlite3_column_int(stmt, 9);
        list.push_back(r);
    }
    sqlite3_finalize(stmt);
    return list;
}

int64_t SnmpDatabase::insertSyslogEvent(SyslogEvent &event) {
    std::lock_guard<std::mutex> lock(_mutex);
    if (!_db) return -1;

    // Check time-adjacency against audit_outbox within 10 seconds: abs(audit.timestamp - event.timestamp) <= 10
    int64_t adjSeq = 0;
    const char *sqlCheck =
        "SELECT sequence FROM audit_outbox "
        "WHERE timestamp BETWEEN (? - 10) AND (? + 10) "
        "ORDER BY abs(timestamp - ?) ASC LIMIT 1;";
    sqlite3_stmt *stmtCheck = nullptr;
    if (sqlite3_prepare_v2(_db, sqlCheck, -1, &stmtCheck, nullptr) == SQLITE_OK) {
        sqlite3_bind_int64(stmtCheck, 1, event.timestamp);
        sqlite3_bind_int64(stmtCheck, 2, event.timestamp);
        sqlite3_bind_int64(stmtCheck, 3, event.timestamp);
        if (sqlite3_step(stmtCheck) == SQLITE_ROW) {
            adjSeq = sqlite3_column_int64(stmtCheck, 0);
        }
        sqlite3_finalize(stmtCheck);
    }

    if (adjSeq > 0) {
        event.timeAdjacent = true;
        event.adjacentAuditSeq = adjSeq;
    } else {
        event.timeAdjacent = false;
        event.adjacentAuditSeq = 0;
    }

    const char *sqlInsert =
        "INSERT INTO syslog_events "
        "(timestamp, source_ip, facility, severity, tag, message, time_adjacent, adjacent_audit_seq, raw) "
        "VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?);";
    sqlite3_stmt *stmt = nullptr;
    if (sqlite3_prepare_v2(_db, sqlInsert, -1, &stmt, nullptr) != SQLITE_OK) {
        return -1;
    }

    sqlite3_bind_int64(stmt, 1, event.timestamp);
    sqlite3_bind_text(stmt, 2, event.sourceIp.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_int(stmt, 3, event.facility);
    sqlite3_bind_int(stmt, 4, event.severity);
    sqlite3_bind_text(stmt, 5, event.tag.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 6, event.message.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_int(stmt, 7, event.timeAdjacent ? 1 : 0);
    sqlite3_bind_int64(stmt, 8, event.adjacentAuditSeq);
    sqlite3_bind_text(stmt, 9, event.raw.c_str(), -1, SQLITE_TRANSIENT);

    int rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);

    if (rc == SQLITE_DONE) {
        event.id = sqlite3_last_insert_rowid(_db);
        return event.id;
    }
    return -1;
}

std::vector<SyslogEvent> SnmpDatabase::getSyslogEvents(size_t limit, int64_t sinceTimestamp) {
    std::lock_guard<std::mutex> lock(_mutex);
    std::vector<SyslogEvent> events;
    if (!_db) return events;

    const char *sql =
        "SELECT id, timestamp, source_ip, facility, severity, tag, message, time_adjacent, adjacent_audit_seq, raw "
        "FROM syslog_events "
        "WHERE timestamp >= ? "
        "ORDER BY timestamp DESC, id DESC LIMIT ?;";
    sqlite3_stmt *stmt = nullptr;
    if (sqlite3_prepare_v2(_db, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        return events;
    }

    sqlite3_bind_int64(stmt, 1, sinceTimestamp);
    sqlite3_bind_int64(stmt, 2, limit > 0 ? limit : 50);

    while (sqlite3_step(stmt) == SQLITE_ROW) {
        SyslogEvent ev;
        ev.id = sqlite3_column_int64(stmt, 0);
        ev.timestamp = sqlite3_column_int64(stmt, 1);
        const unsigned char *s = sqlite3_column_text(stmt, 2);
        if (s) ev.sourceIp = reinterpret_cast<const char *>(s);
        ev.facility = sqlite3_column_int(stmt, 3);
        ev.severity = sqlite3_column_int(stmt, 4);
        s = sqlite3_column_text(stmt, 5);
        if (s) ev.tag = reinterpret_cast<const char *>(s);
        s = sqlite3_column_text(stmt, 6);
        if (s) ev.message = reinterpret_cast<const char *>(s);
        ev.timeAdjacent = sqlite3_column_int(stmt, 7) != 0;
        ev.adjacentAuditSeq = sqlite3_column_int64(stmt, 8);
        s = sqlite3_column_text(stmt, 9);
        if (s) ev.raw = reinterpret_cast<const char *>(s);

        events.push_back(std::move(ev));
    }
    sqlite3_finalize(stmt);
    return events;
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
