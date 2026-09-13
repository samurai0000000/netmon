/*
 * Config.hxx
 *
 * Copyright (C) 2026, Charles Chiou
 */

#ifndef NETMON_CONFIG_HXX
#define NETMON_CONFIG_HXX

#include <string>
#include <memory>
#include <libconfig.h++>

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

    const std::string &getRouterPassword() const;
    void setRouterPassword(const std::string &pass);

    const std::string &getRouterKeyPath() const;
    void setRouterKeyPath(const std::string &path);

    const std::string &getConfigPath() const;
    static std::string resolveHomePath(const std::string &path);

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
    std::string _routerPassword;
    std::string _routerKeyPath;
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
