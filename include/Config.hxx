/*
 * Config.hxx
 *
 * Copyright (C) 2026, Charles Chiou
 */

#ifndef NETMON_CONFIG_HXX
#define NETMON_CONFIG_HXX

#include <string>
#include <vector>
#include <memory>
#include <libconfig.h++>

struct SnmpTargetConfig {
    std::string name;
    std::string ip;
    std::string community = "public";
    std::string version = "2c";
    int port = 161;
    int pollIntervalSec = 30;
    std::vector<std::string> interfaces;
    std::vector<std::string> wanInterfaces;
};

struct WebConfig {
    bool enabled = true;
    int port = 3884;
    std::string bindAddress = "0.0.0.0";
    bool endpointsEnabled = false;
    int adminPort = 3886;
};

class Config {
public:
    static Config &getInstance();

    bool load(const std::string &customPath = "");
    bool save();

    const std::string &getInterface() const;
    void setInterface(const std::string &interface);

    const std::string &getGatewayHost() const;
    void setGatewayHost(const std::string &host);

    int getGatewayPort() const;
    void setGatewayPort(int port);

    int getReconnectIntervalSec() const;
    void setReconnectIntervalSec(int sec);

    const std::string &getDevicesFile() const;
    void setDevicesFile(const std::string &file);

    const std::string &getLogLevel() const;
    void setLogLevel(const std::string &level);

    bool getAllowAiBlockIp() const;
    void setAllowAiBlockIp(bool allow);

    bool getAllowAiRawExec() const;
    void setAllowAiRawExec(bool allow);

    const std::string &getRouterUser() const;
    void setRouterUser(const std::string &user);

    const std::string &getRouterKeyPath() const;
    void setRouterKeyPath(const std::string &path);

    const std::vector<SnmpTargetConfig> &getSnmpTargets() const;
    void setSnmpTargets(const std::vector<SnmpTargetConfig> &targets);
    void addSnmpTarget(const SnmpTargetConfig &target);

    int getSnmpPollIntervalSec() const;
    void setSnmpPollIntervalSec(int sec);

    const std::string &getDatabaseFile() const;
    void setDatabaseFile(const std::string &file);

    const std::string &getAuditFile() const;
    void setAuditFile(const std::string &file);

    int getRawRetentionDays() const;
    void setRawRetentionDays(int days);

    const WebConfig &getWebConfig() const;
    void setWebConfig(const WebConfig &web);

    int getWebPort() const;
    void setWebPort(int port);

    int getAdminPort() const;
    void setAdminPort(int port);

    bool isWebEnabled() const;
    void setWebEnabled(bool enabled);

    bool getRouterDryRun() const;
    void setRouterDryRun(bool enable);

    bool getRouterLiveEnabled() const;
    void setRouterLiveEnabled(bool enable);

    bool getRouterFlashWrite() const;
    void setRouterFlashWrite(bool enable);

    const std::string &getConfigPath() const;
    static std::string resolveHomePath(const std::string &path);
    void resetForTesting();

private:
    Config();
    ~Config() = default;
    Config(const Config &) = delete;
    Config &operator=(const Config &) = delete;

    std::string _configPath;
    std::string _interface;
    std::string _gatewayHost;
    int         _gatewayPort;
    int         _reconnectIntervalSec;
    std::string _devicesFile;
    std::string _logLevel;
    bool        _allowAiBlockIp;
    bool        _allowAiRawExec;
    std::string _routerUser;
    std::string _routerKeyPath;
    bool        _routerDryRun;
    bool        _routerLiveEnabled;
    bool        _routerFlashWrite;

    int         _snmpPollIntervalSec;
    std::vector<SnmpTargetConfig> _snmpTargets;
    std::string _databaseFile;
    std::string _auditFile;
    int         _rawRetentionDays;
    WebConfig   _webConfig;
};

#endif /* NETMON_CONFIG_HXX */

/*
 * Local variables:
 * mode: C++
 * c-file-style: "BSD"
 * c-basic-offset: 4
 * tab-width: 4
 * indent-tabs-mode: nil
 * End:
 */
