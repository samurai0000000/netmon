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
#include <thread>
#include <atomic>
#include <chrono>

class ZyxelDriver : public RouterDriver {
public:
    static ZyxelDriver &getInstance();

    ZyxelDriver();
    virtual ~ZyxelDriver() override;

    void configure(const std::string &host, int port,
                   const std::string &user,
                   const std::string &pinPath = "");

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

    std::vector<std::string> getDryRunLog() const;
    void clearDryRunLog();

    void clearAuthFailure();
    virtual void cancelActiveCommand();

    virtual SshResult executeClearanceCommand(const std::string &command,
                                            std::string &outputOut,
                                            std::string &matchedPromptOut,
                                            int timeoutMs = 5000,
                                            bool isDiagnostic = false);
    virtual SshResult unwindToRootPrompt();
    SshResult abandonPolicySubmode();
    virtual std::string getLastMatchedPrompt() const;
    std::string startupAlert() const;

    // Diagnostic lock and channel isolation
    bool acquireDiagnosticLock();
    void releaseDiagnosticLock();
    bool isDiagnosticActive() const;
    ZyxelSshClient &getSshClient();
    SshResult sendDiagnosticLine(const std::string &line,
                                 std::string &outputOut,
                                 int timeoutMs = 5000);

    // Testing and isolation helpers
    void resetForTesting();

private:
    void keepaliveWorker();

    bool ensureConnectedUnlocked();
    void logDryRunCommand(const std::string &cmd);
    SshResult transmitLine(const std::string &line, std::string &outputOut, int timeoutMs = 5000);

    SshResult executeBlockSequence(const std::string &ip, const std::string &objName,
                                   const std::string &reason, std::string &stepOut,
                                   std::string &outputOut, std::string &showOut);
    SshResult executeUnblockSequence(const std::string &objName, std::string &stepOut,
                                     std::string &outputOut, std::string &showOut);
    void scanAllowAnyUnlocked();

    mutable std::mutex              _driverMutex;
    mutable std::mutex              _dryRunMutex;
    mutable std::mutex              _telemetryMutex;

    std::atomic<bool>               _running;
    std::thread                     _keepaliveThread;

    ZyxelSshClient                  _sshClient;

    bool                            _configured;
    bool                            _authFailed;
    bool                            _dryRun;
    bool                            _liveEnabled;
    std::vector<std::string>        _dryRunLog;
    std::string                     _startupAlert;
    std::atomic<bool>               _diagnosticActive;
    std::chrono::steady_clock::time_point _lastAuthFailTime;
    nlohmann::json                  _cachedStatus;
    ZyxelSecurityTelemetry          _securityTelemetry;
};

class ZyxelDiagnosticGuard {
public:
    explicit ZyxelDiagnosticGuard(ZyxelDriver &driver = ZyxelDriver::getInstance())
        : _driver(driver), _acquired(driver.acquireDiagnosticLock()) {}
    ~ZyxelDiagnosticGuard() {
        if (_acquired) {
            _driver.releaseDiagnosticLock();
        }
    }
    bool isAcquired() const { return _acquired; }
private:
    ZyxelDriver &_driver;
    bool _acquired;
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
