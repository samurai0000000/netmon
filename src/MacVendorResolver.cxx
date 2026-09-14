/*
 * MacVendorResolver.cxx
 *
 * Copyright (C) 2026, Charles Chiou
 */

#include "MacVendorResolver.hxx"
#include "OuiDatabase.hxx"
#include "Config.hxx"

#include <iostream>
#include <fstream>
#include <sstream>
#include <filesystem>
#include <algorithm>
#include <cctype>
#include <chrono>

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-parameter"
#pragma GCC diagnostic ignored "-Wshadow"
#include <httplib.h>
#pragma GCC diagnostic pop

#include <nlohmann/json.hpp>

namespace fs = std::filesystem;
using json = nlohmann::json;

MacVendorResolver &MacVendorResolver::getInstance() {
    static MacVendorResolver instance;
    return instance;
}

MacVendorResolver::MacVendorResolver()
    : _running(false)
    , _db(nullptr)
    , _vendorCallback(nullptr)
    , _localIeeeDataPath("/usr/share/ieee-data/oui.txt")
    , _localNmapDataPath("/usr/share/nmap/nmap-mac-prefixes") {
}

MacVendorResolver::~MacVendorResolver() {
    stop();
}

std::string MacVendorResolver::extractOuiPrefix(const std::string &mac) {
    std::string clean;
    clean.reserve(12);
    for (char c : mac) {
        if (std::isxdigit(static_cast<unsigned char>(c))) {
            clean.push_back(static_cast<char>(std::toupper(static_cast<unsigned char>(c))));
        }
    }
    if (clean.length() >= 6) {
        return clean.substr(0, 6);
    }
    return "";
}

bool MacVendorResolver::start(const std::string &dbPath) {
    if (_running.load()) {
        return true;
    }

    initSqliteCache(dbPath);

    _running = true;
    _workerThread = std::thread(&MacVendorResolver::workerLoop, this);
    return true;
}

void MacVendorResolver::stop() {
    if (!_running.load()) {
        return;
    }

    _running = false;
    _cv.notify_all();

    if (_workerThread.joinable()) {
        _workerThread.join();
    }

    std::lock_guard<std::mutex> lock(_dbMutex);
    if (_db) {
        sqlite3_close(_db);
        _db = nullptr;
    }
}

void MacVendorResolver::setVendorCallback(VendorCallback cb) {
    _vendorCallback = cb;
}

bool MacVendorResolver::initSqliteCache(const std::string &dbPath) {
    std::lock_guard<std::mutex> lock(_dbMutex);
    if (_db) {
        return true;
    }

    std::string path = dbPath;
    if (path.empty()) {
        path = Config::resolveHomePath("~/.config/netmon/netmon_telemetry.db");
    } else {
        path = Config::resolveHomePath(path);
    }

    try {
        fs::path p(path);
        if (p.has_parent_path()) {
            fs::create_directories(p.parent_path());
        }
    } catch (...) {
    }

    int rc = sqlite3_open(path.c_str(), &_db);
    if (rc != SQLITE_OK) {
        std::cerr << "MacVendorResolver: Failed to open SQLite cache " << path
                  << ": " << sqlite3_errmsg(_db) << std::endl;
        if (_db) {
            sqlite3_close(_db);
            _db = nullptr;
        }
        return false;
    }

    const char *createTableSql =
        "CREATE TABLE IF NOT EXISTS oui_cache ("
        "oui TEXT PRIMARY KEY, "
        "vendor TEXT NOT NULL, "
        "updated_at INTEGER NOT NULL"
        ");";

    char *errMsg = nullptr;
    rc = sqlite3_exec(_db, createTableSql, nullptr, nullptr, &errMsg);
    if (rc != SQLITE_OK) {
        std::cerr << "MacVendorResolver: Failed to create table oui_cache: "
                  << (errMsg ? errMsg : "") << std::endl;
        sqlite3_free(errMsg);
        return false;
    }

    // Preload SQLite cache into memory
    const char *selectSql = "SELECT oui, vendor FROM oui_cache;";
    sqlite3_stmt *stmt = nullptr;
    if (sqlite3_prepare_v2(_db, selectSql, -1, &stmt, nullptr) == SQLITE_OK) {
        std::lock_guard<std::mutex> cacheLock(_cacheMutex);
        while (sqlite3_step(stmt) == SQLITE_ROW) {
            const unsigned char *o = sqlite3_column_text(stmt, 0);
            const unsigned char *v = sqlite3_column_text(stmt, 1);
            if (o && v) {
                _memoryCache[reinterpret_cast<const char *>(o)] =
                    reinterpret_cast<const char *>(v);
            }
        }
        sqlite3_finalize(stmt);
    }

    return true;
}

bool MacVendorResolver::lookupSqliteCache(const std::string &oui, std::string &vendor) {
    std::lock_guard<std::mutex> lock(_dbMutex);
    if (!_db) {
        return false;
    }

    const char *sql = "SELECT vendor FROM oui_cache WHERE oui = ? LIMIT 1;";
    sqlite3_stmt *stmt = nullptr;
    if (sqlite3_prepare_v2(_db, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        return false;
    }

    sqlite3_bind_text(stmt, 1, oui.c_str(), -1, SQLITE_STATIC);
    bool found = false;
    if (sqlite3_step(stmt) == SQLITE_ROW) {
        const unsigned char *val = sqlite3_column_text(stmt, 0);
        if (val) {
            vendor = reinterpret_cast<const char *>(val);
            found = true;
        }
    }

    sqlite3_finalize(stmt);
    return found;
}

void MacVendorResolver::saveSqliteCache(const std::string &oui, const std::string &vendor) {
    std::lock_guard<std::mutex> lock(_dbMutex);
    if (!_db) {
        return;
    }

    const char *sql = "INSERT OR REPLACE INTO oui_cache (oui, vendor, updated_at) VALUES (?, ?, ?);";
    sqlite3_stmt *stmt = nullptr;
    if (sqlite3_prepare_v2(_db, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        return;
    }

    uint64_t now = std::chrono::duration_cast<std::chrono::seconds>(
        std::chrono::system_clock::now().time_since_epoch()).count();

    sqlite3_bind_text(stmt, 1, oui.c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 2, vendor.c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_int64(stmt, 3, static_cast<sqlite3_int64>(now));

    sqlite3_step(stmt);
    sqlite3_finalize(stmt);
}

std::string MacVendorResolver::resolve(const std::string &mac, const std::string &ip) {
    if (OuiDatabase::isRandomizedMac(mac)) {
        return "Randomized / Private MAC (Mobile OS)";
    }

    std::string oui = extractOuiPrefix(mac);
    if (oui.empty()) {
        return OuiDatabase::getInstance().lookup(mac);
    }

    {
        std::lock_guard<std::mutex> lock(_cacheMutex);
        auto it = _memoryCache.find(oui);
        if (it != _memoryCache.end() && !it->second.empty() && it->second != "Unknown Vendor") {
            return it->second;
        }
    }

    std::string vendor;
    if (lookupSqliteCache(oui, vendor) && !vendor.empty() && vendor != "Unknown Vendor") {
        std::lock_guard<std::mutex> lock(_cacheMutex);
        _memoryCache[oui] = vendor;
        return vendor;
    }

    std::string fallback = OuiDatabase::getInstance().lookup(mac);
    if (fallback != "Unknown Vendor" && fallback.find("Apple") == std::string::npos) {
        std::lock_guard<std::mutex> lock(_cacheMutex);
        _memoryCache[oui] = fallback;
        saveSqliteCache(oui, fallback);
        return fallback;
    }

    // Enqueue for asynchronous online lookup
    enqueueLookup(mac, ip);
    return fallback;
}

void MacVendorResolver::enqueueLookup(const std::string &mac, const std::string &ip) {
    if (OuiDatabase::isRandomizedMac(mac)) {
        return;
    }

    std::string oui = extractOuiPrefix(mac);
    if (oui.empty()) {
        return;
    }

    std::lock_guard<std::mutex> lock(_queueMutex);
    if (_pendingOuis.find(oui) != _pendingOuis.end()) {
        return; // Already pending
    }

    _pendingOuis[oui] = true;
    _taskQueue.push({mac, ip, oui});
    _cv.notify_one();
}

bool MacVendorResolver::lookupLocalSystemFiles(const std::string &oui, std::string &vendor) {
    // Check /usr/share/ieee-data/oui.txt (format: "00-00-00   (hex)   Xerox Corporation")
    if (fs::exists(_localIeeeDataPath)) {
        std::string dashOui = oui.substr(0, 2) + "-" + oui.substr(2, 2) + "-" + oui.substr(4, 2);
        std::ifstream f(_localIeeeDataPath);
        std::string line;
        while (std::getline(f, line)) {
            if (line.find(dashOui) == 0) {
                size_t pos = line.find("(hex)");
                if (pos != std::string::npos) {
                    std::string v = line.substr(pos + 5);
                    // Trim whitespace and tabs
                    size_t start = v.find_first_not_of(" \t\r\n");
                    if (start != std::string::npos) {
                        v = v.substr(start);
                    }
                    size_t end = v.find_last_not_of(" \t\r\n");
                    if (end != std::string::npos) {
                        v = v.substr(0, end + 1);
                    }
                    if (!v.empty()) {
                        vendor = v;
                        return true;
                    }
                }
            }
        }
    }

    // Check /usr/share/nmap/nmap-mac-prefixes (format: "000000 Xerox")
    if (fs::exists(_localNmapDataPath)) {
        std::ifstream f(_localNmapDataPath);
        std::string line;
        while (std::getline(f, line)) {
            if (line.find(oui) == 0 && line.length() > 7 && line[6] == ' ') {
                std::string v = line.substr(7);
                size_t end = v.find_last_not_of(" \t\r\n");
                if (end != std::string::npos) {
                    v = v.substr(0, end + 1);
                }
                if (!v.empty()) {
                    vendor = v;
                    return true;
                }
            }
        }
    }

    return false;
}

bool MacVendorResolver::lookupOnlineApi(const std::string &oui, std::string &vendor) {
    try {
        httplib::SSLClient cli("api.maclookup.app");
        cli.set_connection_timeout(3, 0);
        cli.set_read_timeout(3, 0);
        cli.set_default_headers({
            {"User-Agent", "NetMon/1.0"}
        });

        std::string path = "/v2/macs/" + oui;
        auto res = cli.Get(path.c_str());
        if (res && res->status == 200) {
            json root = json::parse(res->body);
            if (root.value("success", false) && root.value("found", false)) {
                std::string comp = root.value("company", "");
                if (!comp.empty()) {
                    vendor = comp;
                    return true;
                }
            }
        }
    } catch (...) {
    }

    return false;
}

void MacVendorResolver::workerLoop() {
    while (_running.load()) {
        LookupTask task;
        {
            std::unique_lock<std::mutex> lock(_queueMutex);
            _cv.wait(lock, [this]() {
                return !_running.load() || !_taskQueue.empty();
            });

            if (!_running.load() && _taskQueue.empty()) {
                break;
            }

            task = _taskQueue.front();
            _taskQueue.pop();
        }

        std::string vendor;
        bool found = false;

        // 1. Check SQLite
        if (lookupSqliteCache(task.oui, vendor) && !vendor.empty() && vendor != "Unknown Vendor") {
            found = true;
        }

        // 2. Check local system databases (ieee-data / nmap)
        if (!found && lookupLocalSystemFiles(task.oui, vendor)) {
            found = true;
        }

        // 3. Check Online API
        if (!found && lookupOnlineApi(task.oui, vendor)) {
            found = true;
            // Sleep politely to avoid hammering public API
            std::this_thread::sleep_for(std::chrono::milliseconds(120));
        }

        // 4. Fallback to OuiDatabase
        if (!found) {
            vendor = OuiDatabase::getInstance().lookup(task.mac);
            if (vendor == "Unknown Vendor" || vendor.empty()) {
                vendor = "Unidentified Vendor";
            }
        }

        // Cache result
        {
            std::lock_guard<std::mutex> lock(_cacheMutex);
            _memoryCache[task.oui] = vendor;
        }
        saveSqliteCache(task.oui, vendor);

        {
            std::lock_guard<std::mutex> lock(_queueMutex);
            _pendingOuis.erase(task.oui);
        }

        // Notify callback
        if (_vendorCallback) {
            _vendorCallback(task.mac, vendor);
        }
    }
}

void MacVendorResolver::waitUntilDone(int timeoutMs) {
    auto deadline = std::chrono::steady_clock::now() + std::chrono::milliseconds(timeoutMs);
    while (_running.load() && std::chrono::steady_clock::now() < deadline) {
        {
            std::lock_guard<std::mutex> lock(_queueMutex);
            if (_taskQueue.empty() && _pendingOuis.empty()) {
                break;
            }
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
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
