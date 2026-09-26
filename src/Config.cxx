/*
 * Config.cxx
 *
 * Copyright (C) 2026, Charles Chiou
 */

#include "Config.hxx"
#include <cstdlib>
#include <iostream>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

Config &Config::getInstance() {
    static Config instance;
    return instance;
}

Config::Config()
    : _configPath("")
    , _interface("br0")
    , _gatewayHost("127.0.0.1")
    , _gatewayPort(3885)
    , _reconnectIntervalSec(5)
    , _devicesFile("~/.config/netmon/devices.cfg")
    , _logLevel("info")
    , _allowAiBlockIp(false)
    , _allowAiRawExec(false)
    , _snmpPollIntervalSec(30)
    , _databaseFile("~/.config/netmon/netmon_telemetry.db")
    , _rawRetentionDays(90) {
    _webConfig.enabled = true;
    _webConfig.port = 3884;
    _webConfig.bindAddress = "0.0.0.0";
    _webConfig.endpointsEnabled = false;
}

std::string Config::resolveHomePath(const std::string &path) {
    if (path.empty() || path[0] != '~') {
        return path;
    }

    const char *home = getenv("HOME");
    if (!home) {
        return path;
    }

    if (path == "~") {
        return std::string(home);
    }

    if (path[1] == '/') {
        return std::string(home) + path.substr(1);
    }

    return path;
}

bool Config::load(const std::string &customPath) {
    if (!customPath.empty()) {
        _configPath = resolveHomePath(customPath);
    } else {
        std::string homeDir = resolveHomePath("~/.config/netmon");
        mkdir(homeDir.c_str(), 0700);
        chmod(homeDir.c_str(), 0700);
        _configPath = homeDir + "/netmon.cfg";
    }

    libconfig::Config cfg;

    try {
        cfg.readFile(_configPath.c_str());
    } catch (const libconfig::FileIOException &) {
        // File does not exist yet, save default configuration
        return save();
    } catch (const libconfig::ParseException &pex) {
        std::cerr << "Config parse error at " << pex.getFile() << ":"
                  << pex.getLine() << " - " << pex.getError() << std::endl;
        return false;
    }

    try {
        cfg.lookupValue("interface", _interface);
        cfg.lookupValue("gateway_host", _gatewayHost);
        cfg.lookupValue("gateway_port", _gatewayPort);
        cfg.lookupValue("reconnect_interval_sec", _reconnectIntervalSec);
        cfg.lookupValue("devices_file", _devicesFile);
        cfg.lookupValue("log_level", _logLevel);
        cfg.lookupValue("allow_ai_block_ip", _allowAiBlockIp);
        cfg.lookupValue("allow_ai_raw_exec", _allowAiRawExec);
        cfg.lookupValue("router_user", _routerUser);
        cfg.lookupValue("router_key_path", _routerKeyPath);
        cfg.lookupValue("database_file", _databaseFile);
        cfg.lookupValue("raw_retention_days", _rawRetentionDays);
    } catch (const libconfig::SettingNotFoundException &) {
        // Some settings were missing, keep defaults
    }

    // Web section
    if (cfg.exists("web")) {
        try {
            const libconfig::Setting &webSetting = cfg.lookup("web");
            webSetting.lookupValue("enabled", _webConfig.enabled);
            webSetting.lookupValue("port", _webConfig.port);
            webSetting.lookupValue("bind_address", _webConfig.bindAddress);
            webSetting.lookupValue("endpoints_enabled", _webConfig.endpointsEnabled);
            webSetting.lookupValue("admin_port", _webConfig.adminPort);
        } catch (const libconfig::SettingNotFoundException &) {
        }
    }

    // SNMP section
    if (cfg.exists("snmp")) {
        try {
            const libconfig::Setting &snmpSetting = cfg.lookup("snmp");
            snmpSetting.lookupValue("poll_interval_sec", _snmpPollIntervalSec);

            if (snmpSetting.exists("targets")) {
                const libconfig::Setting &targetsSetting = snmpSetting["targets"];
                int count = targetsSetting.getLength();
                _snmpTargets.clear();
                for (int i = 0; i < count; ++i) {
                    const libconfig::Setting &t = targetsSetting[i];
                    SnmpTargetConfig target;
                    t.lookupValue("name", target.name);
                    t.lookupValue("ip", target.ip);
                    t.lookupValue("community", target.community);
                    t.lookupValue("version", target.version);
                    t.lookupValue("port", target.port);
                    target.pollIntervalSec = _snmpPollIntervalSec;
                    t.lookupValue("poll_interval_sec", target.pollIntervalSec);

                    if (t.exists("interfaces")) {
                        const libconfig::Setting &ifaces = t["interfaces"];
                        for (int j = 0; j < ifaces.getLength(); ++j) {
                            target.interfaces.push_back(std::string(ifaces[j].c_str()));
                        }
                    }

                    if (t.exists("wan_interfaces")) {
                        const libconfig::Setting &wanIfaces = t["wan_interfaces"];
                        for (int j = 0; j < wanIfaces.getLength(); ++j) {
                            target.wanInterfaces.push_back(std::string(wanIfaces[j].c_str()));
                        }
                    }

                    if (!target.ip.empty()) {
                        _snmpTargets.push_back(target);
                    }
                }
            }
        } catch (const libconfig::SettingNotFoundException &) {
        }
    }

    // Override with environment variables if present (12-factor secure credential handling)
    const char *envUser = getenv("NETMON_ROUTER_USER");
    if (envUser && *envUser) _routerUser = envUser;
    const char *envPass = getenv("NETMON_ROUTER_PASSWORD");
    if (envPass && *envPass) _routerPassword = envPass;
    const char *envKey = getenv("NETMON_ROUTER_KEY_PATH");
    if (envKey && *envKey) _routerKeyPath = envKey;

    const char *envDb = getenv("NETMON_DB_PATH");
    if (envDb && *envDb) _databaseFile = envDb;
    const char *envRetention = getenv("NETMON_DB_RETENTION_DAYS");
    if (envRetention && *envRetention) _rawRetentionDays = std::atoi(envRetention);

    const char *envWebPort = getenv("NETMON_WEB_PORT");
    if (envWebPort && *envWebPort) _webConfig.port = std::atoi(envWebPort);
    const char *envAdminPort = getenv("NETMON_ADMIN_PORT");
    if (envAdminPort && *envAdminPort) _webConfig.adminPort = std::atoi(envAdminPort);
    const char *envWebEnabled = getenv("NETMON_WEB_ENABLED");
    if (envWebEnabled && *envWebEnabled) {
        std::string s(envWebEnabled);
        _webConfig.enabled = (s == "1" || s == "true" || s == "TRUE" || s == "yes");
    }
    const char *envWebBind = getenv("NETMON_WEB_BIND");
    if (envWebBind && *envWebBind) _webConfig.bindAddress = envWebBind;
    const char *envWebEndpoints = getenv("NETMON_WEB_ENDPOINTS_ENABLED");
    if (envWebEndpoints && *envWebEndpoints) {
        std::string s(envWebEndpoints);
        _webConfig.endpointsEnabled = (s == "1" || s == "true" || s == "TRUE" || s == "yes");
    }

    const char *envSnmpTarget = getenv("NETMON_SNMP_TARGET");
    if (envSnmpTarget && *envSnmpTarget) {
        bool found = false;
        for (auto &t : _snmpTargets) {
            if (t.ip == envSnmpTarget) {
                found = true;
                break;
            }
        }
        if (!found) {
            SnmpTargetConfig t;
            t.ip = envSnmpTarget;
            t.name = "default";
            _snmpTargets.push_back(t);
        }
    }

    const char *envSnmpComm = getenv("NETMON_SNMP_COMMUNITY");
    if (envSnmpComm && *envSnmpComm) {
        for (auto &t : _snmpTargets) {
            t.community = envSnmpComm;
        }
    }
    const char *envSnmpVer = getenv("NETMON_SNMP_VERSION");
    if (envSnmpVer && *envSnmpVer) {
        for (auto &t : _snmpTargets) {
            t.version = envSnmpVer;
        }
    }
    const char *envSnmpPort = getenv("NETMON_SNMP_PORT");
    if (envSnmpPort && *envSnmpPort) {
        int p = std::atoi(envSnmpPort);
        for (auto &t : _snmpTargets) {
            t.port = p;
        }
    }

    return true;
}

bool Config::save() {
    if (_configPath.empty()) {
        _configPath = resolveHomePath("~/.config/netmon/netmon.cfg");
    }

    libconfig::Config cfg;
    libconfig::Setting &root = cfg.getRoot();

    try {
        if (!root.exists("interface")) {
            root.add("interface", libconfig::Setting::TypeString) = _interface;
        } else {
            root["interface"] = _interface;
        }

        if (!root.exists("gateway_host")) {
            root.add("gateway_host", libconfig::Setting::TypeString) = _gatewayHost;
        } else {
            root["gateway_host"] = _gatewayHost;
        }

        if (!root.exists("gateway_port")) {
            root.add("gateway_port", libconfig::Setting::TypeInt) = _gatewayPort;
        } else {
            root["gateway_port"] = _gatewayPort;
        }

        if (!root.exists("reconnect_interval_sec")) {
            root.add("reconnect_interval_sec", libconfig::Setting::TypeInt) = _reconnectIntervalSec;
        } else {
            root["reconnect_interval_sec"] = _reconnectIntervalSec;
        }

        if (!root.exists("devices_file")) {
            root.add("devices_file", libconfig::Setting::TypeString) = _devicesFile;
        } else {
            root["devices_file"] = _devicesFile;
        }

        if (!root.exists("log_level")) {
            root.add("log_level", libconfig::Setting::TypeString) = _logLevel;
        } else {
            root["log_level"] = _logLevel;
        }

        if (!root.exists("allow_ai_block_ip")) {
            root.add("allow_ai_block_ip", libconfig::Setting::TypeBoolean) = _allowAiBlockIp;
        } else {
            root["allow_ai_block_ip"] = _allowAiBlockIp;
        }

        if (!root.exists("allow_ai_raw_exec")) {
            root.add("allow_ai_raw_exec", libconfig::Setting::TypeBoolean) = _allowAiRawExec;
        } else {
            root["allow_ai_raw_exec"] = _allowAiRawExec;
        }

        if (!_routerUser.empty()) {
            if (!root.exists("router_user")) {
                root.add("router_user", libconfig::Setting::TypeString) = _routerUser;
            } else {
                root["router_user"] = _routerUser;
            }
        }

        if (!_routerKeyPath.empty()) {
            if (!root.exists("router_key_path")) {
                root.add("router_key_path", libconfig::Setting::TypeString) = _routerKeyPath;
            } else {
                root["router_key_path"] = _routerKeyPath;
            }
        }

        cfg.writeFile(_configPath.c_str());
        chmod(_configPath.c_str(), 0600);
        return true;
    } catch (const std::exception &ex) {
        std::cerr << "Failed to write config: " << ex.what() << std::endl;
        return false;
    }
}

const std::string &Config::getInterface() const {
    return _interface;
}

void Config::setInterface(const std::string &interface) {
    _interface = interface;
}

const std::string &Config::getGatewayHost() const {
    return _gatewayHost;
}

void Config::setGatewayHost(const std::string &host) {
    _gatewayHost = host;
}

int Config::getGatewayPort() const {
    return _gatewayPort;
}

void Config::setGatewayPort(int port) {
    _gatewayPort = port;
}

int Config::getReconnectIntervalSec() const {
    return _reconnectIntervalSec;
}

void Config::setReconnectIntervalSec(int sec) {
    _reconnectIntervalSec = sec;
}

const std::string &Config::getDevicesFile() const {
    return _devicesFile;
}

void Config::setDevicesFile(const std::string &file) {
    _devicesFile = file;
}

const std::string &Config::getLogLevel() const {
    return _logLevel;
}

void Config::setLogLevel(const std::string &level) {
    _logLevel = level;
}

bool Config::getAllowAiBlockIp() const {
    return _allowAiBlockIp;
}

void Config::setAllowAiBlockIp(bool allow) {
    _allowAiBlockIp = allow;
}

bool Config::getAllowAiRawExec() const {
    return _allowAiRawExec;
}

void Config::setAllowAiRawExec(bool allow) {
    _allowAiRawExec = allow;
}

const std::string &Config::getConfigPath() const {
    return _configPath;
}

const std::string &Config::getRouterUser() const {
    return _routerUser;
}

void Config::setRouterUser(const std::string &user) {
    _routerUser = user;
}

const std::string &Config::getRouterPassword() const {
    return _routerPassword;
}

void Config::setRouterPassword(const std::string &pass) {
    _routerPassword = pass;
}

const std::string &Config::getRouterKeyPath() const {
    return _routerKeyPath;
}

void Config::setRouterKeyPath(const std::string &path) {
    _routerKeyPath = path;
}

const std::vector<SnmpTargetConfig> &Config::getSnmpTargets() const {
    return _snmpTargets;
}

void Config::setSnmpTargets(const std::vector<SnmpTargetConfig> &targets) {
    _snmpTargets = targets;
}

void Config::addSnmpTarget(const SnmpTargetConfig &target) {
    _snmpTargets.push_back(target);
}

int Config::getSnmpPollIntervalSec() const {
    return _snmpPollIntervalSec;
}

void Config::setSnmpPollIntervalSec(int sec) {
    _snmpPollIntervalSec = sec;
}

const std::string &Config::getDatabaseFile() const {
    return _databaseFile;
}

void Config::setDatabaseFile(const std::string &file) {
    _databaseFile = file;
}

int Config::getRawRetentionDays() const {
    return _rawRetentionDays;
}

void Config::setRawRetentionDays(int days) {
    _rawRetentionDays = days;
}

const WebConfig &Config::getWebConfig() const {
    return _webConfig;
}

void Config::setWebConfig(const WebConfig &web) {
    _webConfig = web;
}

int Config::getWebPort() const {
    return _webConfig.port;
}

void Config::setWebPort(int port) {
    _webConfig.port = port;
}

int Config::getAdminPort() const {
    return _webConfig.adminPort;
}

void Config::setAdminPort(int port) {
    _webConfig.adminPort = port;
}

bool Config::isWebEnabled() const {
    return _webConfig.enabled;
}

void Config::setWebEnabled(bool enabled) {
    _webConfig.enabled = enabled;
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
