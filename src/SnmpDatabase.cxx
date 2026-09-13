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

    char *err = nullptr;
    sqlite3_exec(_db, "PRAGMA journal_mode=WAL;", nullptr, nullptr, &err);
    if (err) {
        sqlite3_free(err);
        err = nullptr;
    }
    sqlite3_exec(_db, "PRAGMA synchronous=NORMAL;", nullptr, nullptr, &err);
    if (err) {
        sqlite3_free(err);
        err = nullptr;
    }

    if (!initSchema()) {
        std::cerr << "SnmpDatabase: Failed to initialize schema" << std::endl;
        close();
        return false;
    }

    prepareStatements();
    std::cout << "SnmpDatabase: Opened " << _dbPath << " successfully (WAL mode)" << std::endl;
    return true;
}

void SnmpDatabase::close() {
    std::lock_guard<std::mutex> lock(_mutex);
    finalizeStatements();
    if (_db != nullptr) {
        sqlite3_close(_db);
        _db = nullptr;
    }
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

    sqlite3_exec(_db, "BEGIN TRANSACTION;", nullptr, nullptr, nullptr);
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
    sqlite3_exec(_db, "COMMIT;", nullptr, nullptr, nullptr);
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

    std::string sql;
    if (bucketSec <= 30) {
        sql = "SELECT timestamp, in_bytes_sec, out_bytes_sec, oper_status, in_errors, out_errors "
              "FROM snmp_samples "
              "WHERE target_ip = ? AND if_name = ? AND timestamp >= ? AND timestamp <= ? "
              "ORDER BY timestamp ASC;";
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

/*
 * Local variables:
 * mode: C++
 * c-file-style: "BSD"
 * c-basic-offset: 4
 * tab-width: 4
 * indent-tabs-mode: nil
 * End:
 */
