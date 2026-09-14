/*
 * MacVendorResolver.hxx
 *
 * Copyright (C) 2026, Charles Chiou
 */

#ifndef NETMON_MACVENDORRESOLVER_HXX
#define NETMON_MACVENDORRESOLVER_HXX

#include <string>
#include <unordered_map>
#include <queue>
#include <mutex>
#include <thread>
#include <condition_variable>
#include <atomic>
#include <functional>
#include <sqlite3.h>

class MacVendorResolver {
public:
    using VendorCallback = std::function<void(const std::string &mac, const std::string &vendor)>;

    static MacVendorResolver &getInstance();

    bool start(const std::string &dbPath = "");
    void stop();

    // Fast synchronous lookup. Returns cached vendor if available, or fallback vendor.
    // If not cached and not an LAA MAC, enqueues an asynchronous online resolution.
    std::string resolve(const std::string &mac, const std::string &ip = "");

    // Explicitly enqueue a MAC for background online resolution
    void enqueueLookup(const std::string &mac, const std::string &ip = "");
    void waitUntilDone(int timeoutMs = 1000);

    // Register callback invoked when an asynchronous lookup finishes
    void setVendorCallback(VendorCallback cb);

    static std::string extractOuiPrefix(const std::string &mac);

private:
    MacVendorResolver();
    ~MacVendorResolver();
    MacVendorResolver(const MacVendorResolver &) = delete;
    MacVendorResolver &operator=(const MacVendorResolver &) = delete;

    void workerLoop();
    bool initSqliteCache(const std::string &dbPath);
    bool lookupSqliteCache(const std::string &oui, std::string &vendor);
    void saveSqliteCache(const std::string &oui, const std::string &vendor);

    bool lookupLocalSystemFiles(const std::string &oui, std::string &vendor);
    bool lookupOnlineApi(const std::string &oui, std::string &vendor);

    struct LookupTask {
        std::string mac;
        std::string ip;
        std::string oui;
    };

    std::atomic<bool> _running;
    std::thread _workerThread;
    std::mutex _queueMutex;
    std::condition_variable _cv;
    std::queue<LookupTask> _taskQueue;
    std::unordered_map<std::string, bool> _pendingOuis;

    mutable std::mutex _cacheMutex;
    std::unordered_map<std::string, std::string> _memoryCache;

    sqlite3 *_db;
    std::mutex _dbMutex;

    VendorCallback _vendorCallback;
    std::string _localIeeeDataPath;
    std::string _localNmapDataPath;
};

#endif /* NETMON_MACVENDORRESOLVER_HXX */

/*
 * Local variables:
 * mode: C++
 * c-file-style: "BSD"
 * c-basic-offset: 4
 * tab-width: 4
 * indent-tabs-mode: nil
 * End:
 */
