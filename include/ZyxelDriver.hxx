/*
 * ZyxelDriver.hxx
 *
 * Copyright (C) 2026, Charles Chiou
 */

#ifndef NETMON_ZYXELDRIVER_HXX
#define NETMON_ZYXELDRIVER_HXX

#include "RouterDriver.hxx"
#include "ZyxelSshClient.hxx"
#include "zyxel/ZyxelTypes.hxx"

#include <string>
#include <vector>
#include <mutex>
#include <condition_variable>
#include <thread>
#include <atomic>
#include <chrono>

struct RouterMutation {
    std::string id;
    std::string op;             // "block" or "unblock"
    std::string ip;
    std::string sanitizedName;  // e.g. "192_168_8_50"
    std::string reason;
    std::string state;          // "pending" or "applied_running"
    time_t      timestamp;
};

class ZyxelDriver : public RouterDriver {
public:
    static ZyxelDriver &getInstance();

    ZyxelDriver();
    virtual ~ZyxelDriver() override;

    void configure(const std::string &host, int port,
                   const std::string &user,
                   const std::string &pinPath = "");

    void setJournalPath(const std::string &path);
    std::string getJournalPath() const;

    void start();
    void stop();

    virtual nlohmann::json getStatus() override;
    virtual nlohmann::json getSessions() override;
    virtual nlohmann::json blockIp(const std::string &ip, const std::string &reason) override;
    virtual nlohmann::json unblockIp(const std::string &ip) override;
    virtual nlohmann::json getSecurityMetrics() override;

    ZyxelSecurityTelemetry getSecurityTelemetry() const;
    void setSecurityTelemetryForTesting(const ZyxelSecurityTelemetry &telem);

    nlohmann::json ping(const std::string &target, int count = 4);
    nlohmann::json traceroute(const std::string &target);

    bool isConfigured() const;
    bool isConnected() const;

    bool isDryRun() const;
    void setDryRun(bool enable);
    bool isLiveEnabled() const;
    void setLiveEnabled(bool enable);
    bool isFlashWriteEnabled() const;
    void setFlashWriteEnabled(bool enable);

    std::vector<std::string> getDryRunLog() const;
    void clearDryRunLog();

    bool replayJournal();
    void flushFlashWrite();
    void clearAuthFailure();
    virtual void cancelActiveCommand();

    virtual SshResult executeClearanceCommand(const std::string &command,
                                            std::string &outputOut,
                                            std::string &matchedPromptOut,
                                            int timeoutMs = 5000);
    virtual SshResult unwindToRootPrompt();
    virtual std::string getLastMatchedPrompt() const;

    // Testing and isolation helpers
    void resetForTesting();
    std::vector<RouterMutation> getPendingMutationsForTesting() const;
    bool removeMutationForTesting(const std::string &id);
    bool addMutationForTesting(const RouterMutation &m);
    bool saveJournalForTesting(const std::vector<RouterMutation> &mutations, bool dirty);
    bool loadJournalForTesting(std::vector<RouterMutation> &mutations, bool &dirty) const;

private:
    void debounceWorker();
    void keepaliveWorker();

    bool ensureConnectedUnlocked();
    bool replayJournalUnlocked();
    void logDryRunCommand(const std::string &cmd);

    bool loadJournal(std::vector<RouterMutation> &mutations, bool &dirty) const;
    bool saveJournal(const std::vector<RouterMutation> &mutations, bool dirty) const;
    bool appendMutationToJournal(const RouterMutation &m);
    bool removeMutationFromJournal(const std::string &id);
    bool clearJournal();

    SshResult executeBlockSequence(const std::string &ip, const std::string &sanitizedName, const std::string &reason);
    SshResult executeUnblockSequence(const std::string &sanitizedName);
    void executeRollback(const std::string &sanitizedName);

    mutable std::mutex              _driverMutex;
    mutable std::mutex              _journalMutex;
    mutable std::mutex              _debounceMutex;
    mutable std::mutex              _dryRunMutex;
    mutable std::mutex              _telemetryMutex;

    std::condition_variable         _debounceCv;
    std::atomic<bool>               _running;
    std::thread                     _debounceThread;
    std::thread                     _keepaliveThread;

    std::string                     _journalPath;
    ZyxelSshClient                  _sshClient;

    bool                            _configured;
    bool                            _pendingFlashWrite;
    bool                            _authFailed;
    bool                            _dryRun;
    bool                            _liveEnabled;
    bool                            _flashWriteEnabled;
    std::vector<std::string>        _dryRunLog;
    std::chrono::steady_clock::time_point _lastAuthFailTime;
    std::chrono::steady_clock::time_point _lastMutationTime;
    nlohmann::json                  _cachedStatus;
    ZyxelSecurityTelemetry          _securityTelemetry;
};

#endif /* NETMON_ZYXELDRIVER_HXX */

/*
 * Local variables:
 * mode: C++
 * c-file-style: "BSD"
 * c-basic-offset: 4
 * tab-width: 4
 * indent-tabs-mode: nil
 * End:
 */
