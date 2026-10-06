/*
 * NetMonShell.hxx
 *
 * Copyright (C) 2026, Charles Chiou
 */

#ifndef NETMON_NETMONSHELL_HXX
#define NETMON_NETMONSHELL_HXX

#include <string>
#include <vector>
#include <atomic>
#include <functional>

class NetMonShell {
public:
    static NetMonShell &getInstance();

    void runInteractive();
    void stop();

    int executeCommand(const std::string &cmdLine);
    void resetForTesting();

    using PasswordReaderFunc = std::function<std::string(const std::string &prompt)>;
    void setPasswordReaderForTesting(PasswordReaderFunc reader);

    bool isExecutingCommand() const;
    void cancelCurrentCommand();

private:
    NetMonShell();
    ~NetMonShell() = default;
    NetMonShell(const NetMonShell &) = delete;
    NetMonShell &operator=(const NetMonShell &) = delete;

    void printBanner() const;
    void printHelp() const;
    void cmdStatus();
    void cmdDevices(const std::string &catFilter = "");
    void cmdUnregistered();
    void cmdTopTalkers(int limit = 10, int windowMins = 15);
    void cmdTraffic();
    void cmdNameDevice(const std::string &mac, const std::string &name,
                       const std::string &category = "");
    void cmdReload();
    void cmdAuthLogin(const std::string &password);
    void cmdAuthSetPassword(const std::string &currentPass, const std::string &newPass);
    void cmdAuthList();
    void cmdRouterSetPassword();
    void cmdRouterClearPassword();
    void cmdRouterStatus();
    void cmdRouterPing(const std::string &target, int count = 4);
    void cmdRouterTraceroute(const std::string &target);
    bool isAuthenticated() const;

    std::atomic<bool> _running;
    std::atomic<bool> _executingCommand;
    std::string       _sessionToken;
};

#endif /* NETMON_NETMONSHELL_HXX */

/*
 * Local variables:
 * mode: C++
 * c-file-style: "BSD"
 * c-basic-offset: 4
 * tab-width: 4
 * indent-tabs-mode: nil
 * End:
 */
