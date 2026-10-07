/*
 * ZyxelDriver.cxx
 *
 * Copyright (C) 2026, Charles Chiou
 */

#include "ZyxelDriver.hxx"
#include "Config.hxx"
#include "AuthManager.hxx"
#include "zyxel/ZyxelTypes.hxx"
#include "zyxel/ZyxelScanner.hxx"
#include "zyxel/ZyxelSystemCmd.hxx"
#include "zyxel/ZyxelNetworkCmd.hxx"
#include "zyxel/ZyxelObjectCmd.hxx"
#include "zyxel/ZyxelFirewallCmd.hxx"
#include "zyxel/ZyxelNatCmd.hxx"
#include "zyxel/ZyxelSecurityCmd.hxx"
#include "AiSecurityClearance.hxx"

#include <iostream>
#include <sstream>
#include <vector>
#include <unistd.h>
#include <sys/stat.h>
#include <openssl/crypto.h>
#include <nlohmann/json.hpp>

using json = nlohmann::json;

ZyxelDriver &ZyxelDriver::getInstance() {
    static ZyxelDriver instance;
    return instance;
}

ZyxelDriver::ZyxelDriver()
    : _running(false),
      _configured(false),
      _authFailed(false),
      _dryRun(false),
      _liveEnabled(false),
      _diagnosticActive(false) {
    _lastAuthFailTime = std::chrono::steady_clock::now() - std::chrono::seconds(120);
}

ZyxelDriver::~ZyxelDriver() {
    stop();
}

void ZyxelDriver::configure(const std::string &host, int port,
                           const std::string &user,
                           const std::string &pinPath) {
    std::lock_guard<std::mutex> lock(_driverMutex);
    _sshClient.configure(host, port, user, pinPath);
    _configured = true;
    _authFailed = false;
}

bool ZyxelDriver::isConfigured() const {
    return _configured;
}

bool ZyxelDriver::isConnected() const {
    return _sshClient.isConnected();
}

bool ZyxelDriver::isDryRun() const {
    return _dryRun || Config::getInstance().getRouterDryRun();
}

void ZyxelDriver::setDryRun(bool enable) {
    _dryRun = enable;
}

bool ZyxelDriver::isLiveEnabled() const {
    return _liveEnabled || Config::getInstance().getRouterLiveEnabled();
}

void ZyxelDriver::setLiveEnabled(bool enable) {
    _liveEnabled = enable;
}

std::vector<std::string> ZyxelDriver::getDryRunLog() const {
    std::lock_guard<std::mutex> lock(_dryRunMutex);
    return _dryRunLog;
}

void ZyxelDriver::clearDryRunLog() {
    std::lock_guard<std::mutex> lock(_dryRunMutex);
    _dryRunLog.clear();
}

bool ZyxelDriver::acquireDiagnosticLock() {
    bool expected = false;
    return _diagnosticActive.compare_exchange_strong(expected, true);
}

void ZyxelDriver::releaseDiagnosticLock() {
    _diagnosticActive.store(false);
}

bool ZyxelDriver::isDiagnosticActive() const {
    return _diagnosticActive.load();
}

ZyxelSshClient &ZyxelDriver::getSshClient() {
    return _sshClient;
}

void ZyxelDriver::logDryRunCommand(const std::string &cmd) {
    std::cerr << "[router-dry-run] " << cmd << std::endl;
    std::lock_guard<std::mutex> lock(_dryRunMutex);
    _dryRunLog.push_back(cmd);
}

bool ZyxelDriver::ensureConnectedUnlocked() {
    if (!isLiveEnabled()) {
        return false;
    }
    if (_sshClient.isConnected()) {
        return true;
    }
    if (!_configured) {
        return false;
    }

    if (_authFailed) {
        auto now = std::chrono::steady_clock::now();
        auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(now - _lastAuthFailTime).count();
        if (elapsed < 60) {
            return false;
        }
        _authFailed = false;
    }

    std::string password;
    if (!AuthManager::getInstance().getRouterPassword(password) || password.empty()) {
        return false;
    }

    SshResult res = _sshClient.connect(password);
    if (res == SshResult::ERR_AUTH_FAILED) {
        _authFailed = true;
        _lastAuthFailTime = std::chrono::steady_clock::now();
        return false;
    } else if (res != SshResult::SUCCESS) {
        return false;
    }

    _authFailed = false;
    return true;
}

static void warnIfIgnoredRouterJournal() {
    std::string path = Config::resolveHomePath("~/.config/netmon/router_journal.json");
    struct stat st;
    if (lstat(path.c_str(), &st) == 0) {
        std::cerr << "WARNING: ignoring " << path
                  << "; netmon does not replay router commands" << std::endl;
    }
}

void ZyxelDriver::start() {
    if (_running.exchange(true)) {
        return;
    }
    warnIfIgnoredRouterJournal();
    _keepaliveThread = std::thread(&ZyxelDriver::keepaliveWorker, this);

    if (isLiveEnabled()) {
        std::lock_guard<std::mutex> lock(_driverMutex);
        if (ensureConnectedUnlocked() && _sshClient.isConnected()) {
            scanAllowAnyUnlocked();
        }
    }
}

void ZyxelDriver::scanAllowAnyUnlocked() {
    _startupAlert.clear();
    std::string out;
    SshResult res = transmitLine("show secure-policy", out, 15000);
    if (res != SshResult::SUCCESS || out.find("secure-policy rule:") == std::string::npos) {
        return;
    }
    std::vector<ZyxelFirewallRule> rules;
    if (!ZyxelFirewallCmd::parseSecurePolicy(out, rules)) {
        return;
    }
    std::string names;
    for (const auto &rule : rules) {
        const std::string prefix = "Policy-Control_";
        if (rule.action != "allow" || rule.name.size() != prefix.size() + 3) {
            continue;
        }
        if (rule.name.compare(0, prefix.size(), prefix) != 0) {
            continue;
        }
        bool autoName = true;
        for (size_t i = prefix.size(); i < rule.name.size(); ++i) {
            char c = rule.name[i];
            if (c < 'A' || c > 'Z') {
                autoName = false;
                break;
            }
        }
        if (!autoName) {
            continue;
        }
        if (rule.sourceIp != "any" && !rule.sourceIp.empty()) {
            continue;
        }
        if (!names.empty()) {
            names += ",";
        }
        names += rule.name;
    }
    if (names.empty()) {
        return;
    }
    _startupAlert = names;
    std::cerr << "netmon audit connId=0 grantType=startup reason=policy-control-allow prompt="
              << names << std::endl;
    std::cout << "WARNING: router has allow-any rule " << names << std::endl;
}

std::string ZyxelDriver::startupAlert() const {
    std::lock_guard<std::mutex> lock(_driverMutex);
    return _startupAlert;
}

void ZyxelDriver::stop() {
    if (!_running.exchange(false)) {
        return;
    }
    if (_keepaliveThread.joinable()) {
        _keepaliveThread.join();
    }
    _sshClient.disconnect();
}

void ZyxelDriver::resetForTesting() {
    if (_running.exchange(false)) {
        if (_keepaliveThread.joinable()) {
            _keepaliveThread.join();
        }
    }
    std::lock_guard<std::mutex> lock(_driverMutex);
    _configured = false;
    _authFailed = false;
    _dryRun = false;
    _liveEnabled = false;
    _diagnosticActive = false;
    std::string().swap(_startupAlert);
    _sshClient.resetForTesting();
    {
        std::lock_guard<std::mutex> dlock(_dryRunMutex);
        _dryRunLog.clear();
        _dryRunLog.shrink_to_fit();
    }
    {
        std::lock_guard<std::mutex> tlock(_telemetryMutex);
        _securityTelemetry = ZyxelSecurityTelemetry();
    }
}

static bool isValidIpv4(const std::string &ip) {
    if (ip.empty()) return false;
    int octets = 0;
    std::istringstream iss(ip);
    std::string segment;
    while (std::getline(iss, segment, '.')) {
        if (segment.empty() || segment.size() > 3) return false;
        for (char c : segment) {
            if (c < '0' || c > '9') return false;
        }
        int val = std::atoi(segment.c_str());
        if (val < 0 || val > 255) return false;
        ++octets;
    }
    return (octets == 4);
}

static bool isValidTargetHost(const std::string &target) {
    if (target.empty() || target.size() > 128) return false;
    for (char c : target) {
        if (!((c >= 'a' && c <= 'z') ||
              (c >= 'A' && c <= 'Z') ||
              (c >= '0' && c <= '9') ||
              c == '.' || c == '-')) {
            return false;
        }
    }
    return true;
}

static bool routerRejected(SshResult res, const std::string &out) {
    if (res != SshResult::SUCCESS) {
        return true;
    }
    if (ZyxelSshClient::isSyntaxError(out)) {
        return true;
    }
    if (out.find("already exists") != std::string::npos ||
        out.find("duplicated") != std::string::npos) {
        return true;
    }
    return false;
}

static bool sessionDropped(SshResult res) {
    return res == SshResult::ERR_TIMEOUT ||
           res == SshResult::ERR_DISCONNECTED ||
           res == SshResult::ERR_CHANNEL_FAILED;
}

SshResult ZyxelDriver::executeBlockSequence(const std::string &ip,
                                           const std::string &objName,
                                           const std::string &reason,
                                           std::string &stepOut,
                                           std::string &outputOut,
                                           std::string &showOut) {
    int position = Config::getInstance().getRouterBlockPosition();
    if (position < 1) {
        position = 1;
    }
    std::string cleanReason = ZyxelSshClient::sanitizeReason(reason);
    if (cleanReason.empty()) {
        cleanReason = "netmon-block";
    }

    struct Step {
        const char *name;
        std::string line;
        bool disconnectOnFail;
    };
    std::vector<Step> steps = {
        {"show", "show secure-policy", false},
        {"configure", "configure terminal", false},
        {"address-object", "address-object " + objName + " " + ip, false},
        {"insert", "secure-policy insert " + std::to_string(position), true},
        {"no-activate", "no activate", true},
        {"name", "name " + objName, false},
        {"sourceip", "sourceip " + objName, false},
        {"action", "action deny", false},
        {"description", "description " + cleanReason, false},
        {"activate", "activate", false},
        {"exit-config", "exit", false},
        {"exit-root", "exit", false},
        {"show-after", "show secure-policy", false}
    };

    for (const Step &step : steps) {
        bool ackClear = !_sshClient.policyInactiveAcknowledged();
        std::string out;
        SshResult res = transmitLine(step.line, out, step.name[0] == 's' ? 15000 : 5000);
        if (step.name == std::string("show-after")) {
            showOut = out;
        }
        if (!routerRejected(res, out)) {
            continue;
        }
        stepOut = step.name;
        outputOut = out;
        if ((step.disconnectOnFail && ackClear) || (sessionDropped(res) && ackClear)) {
            _sshClient.disconnect();
        }
        if (res == SshResult::SUCCESS) {
            return SshResult::ERR_EXEC_FAILED;
        }
        return res;
    }
    stepOut = "show-after";
    outputOut = showOut;
    return SshResult::SUCCESS;
}

SshResult ZyxelDriver::executeUnblockSequence(const std::string &objName,
                                             std::string &stepOut,
                                             std::string &outputOut,
                                             std::string &showOut) {
    std::string listed;
    SshResult res = transmitLine("show secure-policy", listed, 15000);
    if (routerRejected(res, listed)) {
        stepOut = "show";
        outputOut = listed;
        return res == SshResult::SUCCESS ? SshResult::ERR_EXEC_FAILED : res;
    }

    std::vector<ZyxelFirewallRule> rules;
    if (listed.find("secure-policy rule:") != std::string::npos) {
        ZyxelFirewallCmd::parseSecurePolicy(listed, rules);
    }
    int matches = 0;
    int position = 0;
    for (const auto &rule : rules) {
        if (rule.name == objName && rule.sourceIp == objName) {
            matches++;
            position = rule.index;
        }
    }
    if (matches != 1 || position < 1) {
        stepOut = "match";
        outputOut = listed;
        showOut = listed;
        return SshResult::ERR_EXEC_FAILED;
    }

    std::string out;
    res = transmitLine("configure terminal", out, 5000);
    if (routerRejected(res, out)) {
        stepOut = "configure";
        outputOut = out;
        return res == SshResult::SUCCESS ? SshResult::ERR_EXEC_FAILED : res;
    }

    std::string delRule = "no secure-policy " + std::to_string(position);
    res = transmitLine(delRule, out, 5000);
    if (routerRejected(res, out)) {
        stepOut = "delete-rule";
        outputOut = out;
        return res == SshResult::SUCCESS ? SshResult::ERR_EXEC_FAILED : res;
    }

    res = transmitLine("no address-object " + objName, out, 5000);
    if (routerRejected(res, out)) {
        stepOut = "delete-address";
        outputOut = out;
        return res == SshResult::SUCCESS ? SshResult::ERR_EXEC_FAILED : res;
    }

    res = transmitLine("exit", out, 5000);
    if (routerRejected(res, out)) {
        stepOut = "exit";
        outputOut = out;
        return res == SshResult::SUCCESS ? SshResult::ERR_EXEC_FAILED : res;
    }

    std::string afterPolicy;
    res = transmitLine("show secure-policy", afterPolicy, 15000);
    std::string afterAddress;
    SshResult addrRes = transmitLine("show address-object", afterAddress, 15000);
    showOut = afterPolicy + afterAddress;
    if (routerRejected(res, afterPolicy) || routerRejected(addrRes, afterAddress)) {
        stepOut = "show-after";
        outputOut = showOut;
        return SshResult::ERR_EXEC_FAILED;
    }
    stepOut = "show-after";
    outputOut = showOut;
    return SshResult::SUCCESS;
}

void ZyxelDriver::clearAuthFailure() {
    std::lock_guard<std::mutex> lock(_driverMutex);
    _authFailed = false;
}

void ZyxelDriver::cancelActiveCommand() {
    _sshClient.cancelActiveCommand();
}

static bool commandOpensPolicy(const std::string &command) {
    const char *forms[] = {"secure-policy insert", "secure-policy append"};
    for (const char *form : forms) {
        std::string prefix(form);
        if (command == prefix) {
            return true;
        }
        prefix.push_back(' ');
        if (command.compare(0, prefix.size(), prefix) == 0) {
            return true;
        }
    }
    return false;
}

static bool lineRejected(const std::string &line, std::string &reason) {
    if (line.size() > 512) {
        reason = "length";
        return true;
    }
    bool question = false;
    bool control = false;
    for (unsigned char uc : line) {
        if (uc == '?') {
            question = true;
        } else if (uc < 32 || uc == 127) {
            control = true;
        }
    }
    if (question) {
        reason = "question-mark";
        return true;
    }
    if (control) {
        reason = "control";
        return true;
    }
    std::string token;
    for (char c : line) {
        if (c == ' ' || c == '\t') {
            if (!token.empty()) {
                break;
            }
            continue;
        }
        token.push_back(c);
    }
    if (token == "write" || token == "reboot" || token == "copy" ||
        token == "boot" || token == "delete" || token == "shutdown" ||
        token == "run" || token == "apply") {
        reason = "denied-command";
        return true;
    }
    return false;
}

static void auditTransmitReject(const std::string &reason, const std::string &prompt) {
    std::cerr << "netmon audit connId=0 grantType=transmit reason=" << reason
              << " prompt=" << prompt << std::endl;
}

SshResult ZyxelDriver::sendDiagnosticLine(const std::string &line,
                                         std::string &outputOut,
                                         int timeoutMs) {
    std::lock_guard<std::mutex> driverLock(_driverMutex);
    if (!ensureConnectedUnlocked() || !_sshClient.isConnected()) {
        outputOut = "Not connected";
        return SshResult::ERR_DISCONNECTED;
    }
    return transmitLine(line, outputOut, timeoutMs);
}

SshResult ZyxelDriver::transmitLine(const std::string &line,
                                   std::string &outputOut,
                                   int timeoutMs) {
    std::string reason;
    if (lineRejected(line, reason)) {
        auditTransmitReject(reason, _sshClient.getLastMatchedPrompt());
        outputOut = "Command rejected";
        return SshResult::ERR_REJECTED;
    }
    if (_sshClient.getPromptState() == PromptState::POLICY_SUBMODE &&
        !_sshClient.policyInactiveAcknowledged() &&
        line != "no activate") {
        auditTransmitReject("first-submode-line", _sshClient.getLastMatchedPrompt());
        outputOut = "Command rejected";
        return SshResult::ERR_REJECTED;
    }
    return _sshClient.executeCommand(line, outputOut, timeoutMs);
}

SshResult ZyxelDriver::executeClearanceCommand(const std::string &command,
                                              std::string &outputOut,
                                              std::string &matchedPromptOut,
                                              int timeoutMs,
                                              bool isDiagnostic) {
    if (_diagnosticActive.load() && !isDiagnostic) {
        outputOut = "Firewall diagnostic in progress; client requests temporarily suspended";
        matchedPromptOut = _sshClient.getLastMatchedPrompt();
        if (matchedPromptOut.empty()) {
            matchedPromptOut = "Router>";
        }
        return SshResult::ERR_BUSY;
    }

    std::string denyReason;
    if (lineRejected(command, denyReason)) {
        auditTransmitReject(denyReason, _sshClient.getLastMatchedPrompt());
        outputOut = "Command rejected";
        matchedPromptOut = _sshClient.getLastMatchedPrompt();
        if (matchedPromptOut.empty()) {
            matchedPromptOut = "-";
        }
        return SshResult::ERR_REJECTED;
    }

    if (isDryRun()) {
        logDryRunCommand(command);
        outputOut = "[dry-run] " + command;
        matchedPromptOut = "Router#";
        return SshResult::SUCCESS;
    }

    std::lock_guard<std::mutex> driverLock(_driverMutex);

    if (!ensureConnectedUnlocked() || !_sshClient.isConnected()) {
        outputOut = "Router is offline";
        matchedPromptOut.clear();
        return SshResult::ERR_DISCONNECTED;
    }

    std::string curPrompt = _sshClient.getLastMatchedPrompt();
    int tOut = timeoutMs;
    std::string matchedMethod;
    bool acked = _sshClient.policyInactiveAcknowledged();
    LineClassification cls = AiSecurityClassifier::classify(command, curPrompt, tOut, matchedMethod, acked);

    if (cls == LineClassification::UNCLASSIFIED) {
        outputOut = "Command rejected: UNCLASSIFIED syntax";
        matchedPromptOut = curPrompt;
        return SshResult::ERR_SYNTAX;
    }
    if (cls == LineClassification::LEVEL1) {
        auditTransmitReject("denied-command", curPrompt);
        outputOut = "Command rejected";
        matchedPromptOut = curPrompt;
        return SshResult::ERR_REJECTED;
    }

    if (cls == LineClassification::LEVEL2_ROOT) {
        bool enteredConfig = false;
        PromptState state = _sshClient.getPromptState();
        if (state != PromptState::CONFIG &&
            state != PromptState::POLICY_SUBMODE &&
            state != PromptState::OTHER_SUBMODE) {
            std::string cfgOut;
            SshResult cRes = transmitLine("configure terminal", cfgOut, 5000);
            if (cRes != SshResult::SUCCESS) {
                outputOut = cfgOut;
                matchedPromptOut = _sshClient.getLastMatchedPrompt();
                if ((cRes == SshResult::ERR_LOCKED || ZyxelSshClient::isConfigLocked(cfgOut)) &&
                    _sshClient.getPromptState() != PromptState::POLICY_SUBMODE) {
                    _sshClient.unwindToRootPrompt();
                    matchedPromptOut = _sshClient.getLastMatchedPrompt();
                    return SshResult::ERR_LOCKED;
                }
                return cRes;
            }
            enteredConfig = true;
        }

        SshResult res = transmitLine(command, outputOut, timeoutMs);
        matchedPromptOut = _sshClient.getLastMatchedPrompt();
        if (commandOpensPolicy(command) ||
            _sshClient.getPromptState() == PromptState::POLICY_SUBMODE) {
            return res;
        }
        if (enteredConfig && _sshClient.getPromptState() == PromptState::CONFIG) {
            _sshClient.unwindToRootPrompt();
            matchedPromptOut = _sshClient.getLastMatchedPrompt();
        }
        if (res != SshResult::SUCCESS &&
            _sshClient.getPromptState() == PromptState::CONFIG) {
            _sshClient.unwindToRootPrompt();
            matchedPromptOut = _sshClient.getLastMatchedPrompt();
        }
        return res;
    }

    SshResult res = transmitLine(command, outputOut, timeoutMs);
    matchedPromptOut = _sshClient.getLastMatchedPrompt();
    if (res != SshResult::SUCCESS &&
        _sshClient.getPromptState() != PromptState::POLICY_SUBMODE &&
        _sshClient.getPromptState() != PromptState::ROOT &&
        _sshClient.getPromptState() != PromptState::USER &&
        _sshClient.getPromptState() != PromptState::UNKNOWN) {
        _sshClient.unwindToRootPrompt();
        matchedPromptOut = _sshClient.getLastMatchedPrompt();
    }
    return res;
}

SshResult ZyxelDriver::unwindToRootPrompt() {
    if (isDryRun()) {
        logDryRunCommand("unwindToRootPrompt");
        return SshResult::SUCCESS;
    }
    std::lock_guard<std::mutex> driverLock(_driverMutex);
    return _sshClient.unwindToRootPrompt();
}

SshResult ZyxelDriver::abandonPolicySubmode() {
    std::lock_guard<std::mutex> driverLock(_driverMutex);
    if (_sshClient.getPromptState() == PromptState::POLICY_SUBMODE &&
        !_sshClient.policyInactiveAcknowledged()) {
        _sshClient.disconnect();
        return SshResult::ERR_DISCONNECTED;
    }
    if (_sshClient.getPromptState() == PromptState::POLICY_SUBMODE) {
        return SshResult::ERR_EXEC_FAILED;
    }
    return SshResult::SUCCESS;
}

std::string ZyxelDriver::getLastMatchedPrompt() const {
    if (isDryRun()) {
        return "#";
    }
    std::lock_guard<std::mutex> driverLock(_driverMutex);
    return _sshClient.getLastMatchedPrompt();
}

nlohmann::json ZyxelDriver::blockIp(const std::string &ip, const std::string &reason) {
    nlohmann::json res;

    if (!isValidIpv4(ip) || ip == "0.0.0.0" || ip == "255.255.255.255" || ip == "127.0.0.1" ||
        ip == Config::getInstance().getGatewayHost()) {
        res["status"] = "error";
        res["error"] = "Refusing to block this address";
        res["ip"] = ip;
        return res;
    }
    for (const auto &target : Config::getInstance().getSnmpTargets()) {
        if (!target.ip.empty() && target.ip == ip) {
            res["status"] = "error";
            res["error"] = "Refusing to block this address";
            res["ip"] = ip;
            return res;
        }
    }

    std::lock_guard<std::mutex> driverLock(_driverMutex);

    std::string sanitizedName = ZyxelSshClient::sanitizeIpToObjectName(ip);
    if (sanitizedName.rfind("NETMON_BLK_", 0) == 0) {
        sanitizedName = sanitizedName.substr(11);
    }

    std::string objName = "NETMON_BLK_" + sanitizedName;
    int position = Config::getInstance().getRouterBlockPosition();
    if (position < 1) {
        position = 1;
    }
    std::string cleanReason = ZyxelSshClient::sanitizeReason(reason);
    if (cleanReason.empty()) {
        cleanReason = "netmon-block";
    }

    if (isDryRun()) {
        logDryRunCommand("show secure-policy");
        logDryRunCommand("configure terminal");
        logDryRunCommand("address-object " + objName + " " + ip);
        logDryRunCommand("secure-policy insert " + std::to_string(position));
        logDryRunCommand("no activate");
        logDryRunCommand("name " + objName);
        logDryRunCommand("sourceip " + objName);
        logDryRunCommand("action deny");
        logDryRunCommand("description " + cleanReason);
        logDryRunCommand("activate");
        logDryRunCommand("exit");
        logDryRunCommand("exit");
        logDryRunCommand("show secure-policy");

        res["status"] = "dry_run";
        res["action"] = "block";
        res["ip"] = ip;
        res["rule"] = objName;
        res["dry_run"] = true;
        return res;
    }

    if (!isLiveEnabled()) {
        res["status"] = "disabled";
        res["action"] = "block";
        res["ip"] = ip;
        res["live_enabled"] = false;
        res["error"] = "Router live mode disabled (router_live_enabled = false)";
        return res;
    }

    if (!ensureConnectedUnlocked() || !_sshClient.isConnected()) {
        res["status"] = "error";
        res["action"] = "block";
        res["ip"] = ip;
        res["error"] = "Router is not connected";
        return res;
    }

    std::string step;
    std::string output;
    std::string show;
    SshResult ok = executeBlockSequence(ip, objName, reason, step, output, show);
    res["step"] = step;
    res["output"] = output;
    res["show"] = show;
    res["rule"] = objName;
    if (ok == SshResult::SUCCESS) {
        res["status"] = "success";
        res["action"] = "block";
        res["ip"] = ip;
        return res;
    }
    if (ok == SshResult::ERR_LOCKED) {
        res["status"] = "locked";
        res["action"] = "block";
        res["ip"] = ip;
        res["message"] = "Router configuration locked by another session";
        return res;
    }
    res["status"] = "error";
    res["action"] = "block";
    res["ip"] = ip;
    res["error"] = "Block stopped at " + step;
    return res;
}

nlohmann::json ZyxelDriver::unblockIp(const std::string &ip) {
    nlohmann::json res;

    if (!isValidIpv4(ip)) {
        res["status"] = "error";
        res["error"] = "Invalid IPv4 address format";
        res["ip"] = ip;
        return res;
    }

    std::lock_guard<std::mutex> driverLock(_driverMutex);

    std::string sanitizedName = ZyxelSshClient::sanitizeIpToObjectName(ip);
    if (sanitizedName.rfind("NETMON_BLK_", 0) == 0) {
        sanitizedName = sanitizedName.substr(11);
    }

    std::string objName = "NETMON_BLK_" + sanitizedName;

    if (isDryRun()) {
        logDryRunCommand("show secure-policy");

        res["status"] = "dry_run";
        res["action"] = "unblock";
        res["ip"] = ip;
        res["dry_run"] = true;
        return res;
    }

    if (!isLiveEnabled()) {
        res["status"] = "disabled";
        res["action"] = "unblock";
        res["ip"] = ip;
        res["live_enabled"] = false;
        res["error"] = "Router live mode disabled (router_live_enabled = false)";
        return res;
    }

    if (!ensureConnectedUnlocked() || !_sshClient.isConnected()) {
        res["status"] = "error";
        res["action"] = "unblock";
        res["ip"] = ip;
        res["error"] = "Router is not connected";
        return res;
    }

    std::string step;
    std::string output;
    std::string show;
    SshResult ok = executeUnblockSequence(objName, step, output, show);
    res["step"] = step;
    res["output"] = output;
    res["show"] = show;
    if (ok == SshResult::SUCCESS) {
        res["status"] = "success";
        res["action"] = "unblock";
        res["ip"] = ip;
        return res;
    }
    if (ok == SshResult::ERR_LOCKED) {
        res["status"] = "locked";
        res["action"] = "unblock";
        res["ip"] = ip;
        res["message"] = "Router configuration locked by another session";
        return res;
    }
    res["status"] = "error";
    res["action"] = "unblock";
    res["ip"] = ip;
    res["error"] = "Unblock stopped at " + step;
    return res;
}

nlohmann::json ZyxelDriver::getStatus() {
    std::unique_lock<std::mutex> lock(_driverMutex, std::try_to_lock);
    if (!lock.owns_lock()) {
        if (!_cachedStatus.empty()) {
            return _cachedStatus;
        }
        nlohmann::json busyRes;
        busyRes["status"] = "online";
        busyRes["driver"] = "ZyxelDriver (USG FLEX 200)";
        busyRes["transport"] = "SSH-2.0 (libssh2)";
        busyRes["busy"] = true;
        return busyRes;
    }

    nlohmann::json res;

    if (!_configured) {
        res["status"] = "unconfigured";
        res["driver"] = "ZyxelDriver (USG FLEX 200)";
        res["note"] = "Pending configuration in netmon.cfg or vault.enc";
        return res;
    }

    if (!isLiveEnabled()) {
        res["status"] = "disabled";
        res["driver"] = "ZyxelDriver (USG FLEX 200)";
        res["live_enabled"] = false;
        res["dry_run"] = isDryRun();
        res["note"] = "Router driver disabled by configuration (router_live_enabled = false)";
        return res;
    }

    ensureConnectedUnlocked();

    if (_sshClient.isConnected()) {
        if (_cachedStatus.contains("model")) {
            return _cachedStatus;
        }

        res["status"] = "online";
        res["driver"] = "ZyxelDriver (USG FLEX 200)";
        res["transport"] = "SSH-2.0 (libssh2)";

        std::string out;
        SshResult rc = transmitLine(ZyxelSystemCmd::cmdShowVersion(), out, 5000);
        if (rc == SshResult::SUCCESS) {
            res["raw_version"] = out;
            ZyxelVersionInfo verInfo;
            if (ZyxelSystemCmd::parseVersion(out, verInfo)) {
                res["model"] = verInfo.model;
                res["firmware"] = verInfo.firmwareVersion;
                res["build_date"] = verInfo.buildDate;
                res["serial_number"] = verInfo.serialNumber;
            } else {
                std::istringstream iss(out);
                std::string line;
                while (std::getline(iss, line)) {
                    if (line.find("model") != std::string::npos || line.find("Model") != std::string::npos) {
                        res["model"] = line;
                    } else if (line.find("firmware") != std::string::npos || line.find("Firmware") != std::string::npos || line.find("ZyNOS") != std::string::npos) {
                        res["firmware"] = line;
                    } else if (line.find("uptime") != std::string::npos || line.find("Uptime") != std::string::npos) {
                        res["uptime"] = line;
                    }
                }
            }
            _cachedStatus = res;
        }
    } else {
        _cachedStatus.clear();
        res["driver"] = "ZyxelDriver (USG FLEX 200)";
        SshClientState state = _sshClient.getState();
        std::string storedPassword;
        bool havePassword = AuthManager::getInstance().getRouterPassword(storedPassword) &&
                            !storedPassword.empty();
        if (!storedPassword.empty()) {
            OPENSSL_cleanse(&storedPassword[0], storedPassword.size());
            std::string().swap(storedPassword);
        }
        if (!havePassword) {
            res["status"] = "offline";
            res["error"] = "No router password in the vault";
        } else if (state == SshClientState::DEGRADED_AUTH_FAILED) {
            res["status"] = "degraded";
            res["error"] = "Authentication failed (invalid router password)";
        } else if (state == SshClientState::DEGRADED_HOSTKEY_MISMATCH) {
            res["status"] = "degraded";
            res["error"] = "Host key fingerprint mismatch";
        } else if (state == SshClientState::ERROR_DISCONNECTED) {
            res["status"] = "offline";
            res["error"] = "SSH handshake or TCP connect failed";
        } else if (state == SshClientState::DISCONNECTED) {
            res["status"] = "offline";
            res["error"] = "SSH client did not open a session";
        } else {
            res["status"] = "offline";
            res["error"] = "SSH connection down";
        }
    }
    return res;
}

nlohmann::json ZyxelDriver::getSessions() {
    nlohmann::json res;
    std::lock_guard<std::mutex> lock(_driverMutex);

    if (!_configured) {
        res["status"] = "unconfigured";
        res["sessions"] = json::array();
        return res;
    }

    if (!isLiveEnabled()) {
        res["status"] = "disabled";
        res["sessions"] = json::array();
        return res;
    }

    ensureConnectedUnlocked();

    if (!_sshClient.isConnected()) {
        res["status"] = "offline";
        res["sessions"] = json::array();
        return res;
    }

    std::string out;
    SshResult rc = transmitLine(ZyxelSystemCmd::cmdShowConnStatus(), out, 5000);
    if (rc != SshResult::SUCCESS) {
        res["status"] = "error";
        res["error"] = "Failed to query session summary";
        res["sessions"] = json::array();
        return res;
    }

    res["status"] = "ok";
    res["raw_summary"] = out;
    res["sessions"] = json::array();
    ZyxelSessionSummary sum;
    if (ZyxelSystemCmd::parseConnStatus(out, sum)) {
        res["active_sessions"] = sum.activeSessions;
        res["max_sessions"] = sum.maxSessions;
        res["session_usage_percent"] = sum.sessionUsagePercent;

        std::lock_guard<std::mutex> tlock(_telemetryMutex);
        _securityTelemetry.sessionSummary = sum;
        _securityTelemetry.hasSessionSummary = true;
        _securityTelemetry.timestamp = time(nullptr);
    }
    return res;
}

nlohmann::json ZyxelDriver::getSecurityMetrics() {
    std::lock_guard<std::mutex> lock(_telemetryMutex);
    nlohmann::json j = _securityTelemetry.toJson();
    j["status"] = isConnected() ? "ok" : (_configured ? "offline" : "unconfigured");
    return j;
}

ZyxelSecurityTelemetry ZyxelDriver::getSecurityTelemetry() const {
    std::lock_guard<std::mutex> lock(_telemetryMutex);
    return _securityTelemetry;
}

void ZyxelDriver::setSecurityTelemetryForTesting(const ZyxelSecurityTelemetry &telem) {
    std::lock_guard<std::mutex> lock(_telemetryMutex);
    _securityTelemetry = telem;
}

nlohmann::json ZyxelDriver::ping(const std::string &target, int count) {
    nlohmann::json res;
    if (!isValidTargetHost(target)) {
        res["status"] = "error";
        res["error"] = "Invalid target host or IP format";
        res["target"] = target;
        return res;
    }

    if (count < 1) count = 1;
    if (count > 10) count = 10;

    std::lock_guard<std::mutex> lock(_driverMutex);

    if (isDryRun()) {
        std::string cmd = ZyxelSystemCmd::cmdPing(target, count);
        logDryRunCommand(cmd);
        res["status"] = "ok";
        res["type"] = "ping";
        res["target"] = target;
        res["packets_transmitted"] = count;
        res["packets_received"] = count;
        res["packet_loss_percent"] = 0.0;
        res["min_latency_ms"] = 1.0;
        res["avg_latency_ms"] = 2.0;
        res["max_latency_ms"] = 3.0;
        res["dry_run"] = true;
        return res;
    }

    if (!_configured) {
        res["status"] = "unconfigured";
        res["error"] = "Router driver unconfigured";
        return res;
    }

    if (!isLiveEnabled()) {
        res["status"] = "disabled";
        res["error"] = "Router live mode disabled (router_live_enabled = false)";
        return res;
    }

    ensureConnectedUnlocked();

    if (!_sshClient.isConnected()) {
        res["status"] = "offline";
        res["error"] = "SSH connection to router is offline";
        return res;
    }

    std::string cmd = ZyxelSystemCmd::cmdPing(target, count);
    std::string out;
    int timeoutMs = 10000 + (count * 2000);
    SshResult rc = transmitLine(cmd, out, timeoutMs);
    if (rc == SshResult::ERR_INTERRUPTED) {
        res["status"] = "canceled";
        res["error"] = "Ping probe canceled by operator (Ctrl+C)";
        return res;
    } else if (rc == SshResult::ERR_TIMEOUT) {
        res["status"] = "error";
        res["error"] = "Ping probe timed out on router";
        res["raw_output"] = ZyxelSshClient::stripTrailingPrompt(out);
        return res;
    } else if (rc != SshResult::SUCCESS) {
        res["status"] = "error";
        res["error"] = "Ping command failed on router";
        res["raw_output"] = ZyxelSshClient::stripTrailingPrompt(out);
        return res;
    }

    ZyxelDiagnosticResult diag;
    if (ZyxelSystemCmd::parsePing(out, diag)) {
        res = diag.toJson();
        res["status"] = "ok";
        res["target"] = target;
        res["raw_output"] = ZyxelSshClient::stripTrailingPrompt(out);
    } else {
        res["status"] = "ok";
        res["target"] = target;
        res["raw_output"] = ZyxelSshClient::stripTrailingPrompt(out);
    }
    return res;
}

nlohmann::json ZyxelDriver::traceroute(const std::string &target) {
    nlohmann::json res;
    if (!isValidTargetHost(target)) {
        res["status"] = "error";
        res["error"] = "Invalid target host or IP format";
        res["target"] = target;
        return res;
    }

    std::lock_guard<std::mutex> lock(_driverMutex);

    if (isDryRun()) {
        std::string cmd = ZyxelSystemCmd::cmdTraceroute(target);
        logDryRunCommand(cmd);
        res["status"] = "ok";
        res["type"] = "traceroute";
        res["target"] = target;
        res["packets_received"] = 3;
        res["min_latency_ms"] = 0.5;
        res["avg_latency_ms"] = 2.0;
        res["max_latency_ms"] = 12.0;
        res["dry_run"] = true;
        return res;
    }

    if (!_configured) {
        res["status"] = "unconfigured";
        res["error"] = "Router driver unconfigured";
        return res;
    }

    if (!isLiveEnabled()) {
        res["status"] = "disabled";
        res["error"] = "Router live mode disabled (router_live_enabled = false)";
        return res;
    }

    ensureConnectedUnlocked();

    if (!_sshClient.isConnected()) {
        res["status"] = "offline";
        res["error"] = "SSH connection to router is offline";
        return res;
    }

    std::string cmd = ZyxelSystemCmd::cmdTraceroute(target);
    std::string out;
    SshResult rc = transmitLine(cmd, out, 60000);
    if (rc == SshResult::ERR_INTERRUPTED) {
        res["status"] = "canceled";
        res["error"] = "Traceroute canceled by operator (Ctrl+C)";
        res["raw_output"] = ZyxelSshClient::stripTrailingPrompt(out);
        return res;
    } else if (rc == SshResult::ERR_TIMEOUT) {
        res["status"] = "error";
        res["error"] = "Traceroute command timed out on router (exceeded 60s)";
        res["raw_output"] = ZyxelSshClient::stripTrailingPrompt(out);
        return res;
    } else if (rc != SshResult::SUCCESS) {
        res["status"] = "error";
        res["error"] = "Traceroute command failed on router";
        res["raw_output"] = ZyxelSshClient::stripTrailingPrompt(out);
        return res;
    }

    ZyxelDiagnosticResult diag;
    if (ZyxelSystemCmd::parseTraceroute(out, diag)) {
        res = diag.toJson();
        res["status"] = "ok";
        res["target"] = target;
        res["raw_output"] = diag.rawOutput;
    } else {
        res["status"] = "ok";
        res["target"] = target;
        res["raw_output"] = ZyxelSshClient::stripTrailingPrompt(out);
    }
    return res;
}

void ZyxelDriver::keepaliveWorker() {
    uint32_t iteration = 0;
    while (_running) {
        for (int i = 0; i < 150 && _running; ++i) {
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }
        if (!_running) {
            break;
        }

        // Suspend keepalives and active telemetry while diagnostic is active
        if (_diagnosticActive.load()) {
            continue;
        }

        iteration++;

        if (_sshClient.isConnected()) {
            if (!_sshClient.sendKeepalive()) {
                _sshClient.disconnect();
            }
        }

        // Every 60 seconds (4 iterations of 15 seconds), execute active telemetry queries
        // to reset the ZLD CLI shell idle timeout and collect live security metrics.
        if ((iteration % 4) == 0 && _sshClient.isConnected() && !isDryRun()) {
            std::unique_lock<std::mutex> lock(_driverMutex, std::try_to_lock);
            if (lock.owns_lock()) {
                std::string curPrompt = _sshClient.getLastMatchedPrompt();
                if (curPrompt.find('(') == std::string::npos) {
                    ZyxelSecurityTelemetry telem;
                    telem.timestamp = time(nullptr);
                    std::string out;

                    if (transmitLine(ZyxelSecurityCmd::cmdShowConnStatus(), out, 5000) == SshResult::SUCCESS) {
                        if (ZyxelSecurityCmd::parseConnStatus(out, telem.sessionSummary)) {
                            telem.hasSessionSummary = true;
                        }
                    }
                    if (transmitLine(ZyxelSecurityCmd::cmdShowAppStatisticsSummary(), out, 5000) == SshResult::SUCCESS) {
                        if (ZyxelSecurityCmd::parseAppStatisticsSummary(out, telem.appPatrolSummary)) {
                            telem.hasAppPatrol = true;
                        }
                    }
                    if (transmitLine(ZyxelSecurityCmd::cmdShowIdpStatisticsSummary(), out, 5000) == SshResult::SUCCESS) {
                        if (ZyxelSecurityCmd::parseIdpStatisticsSummary(out, telem.idpSummary)) {
                            telem.hasIdp = true;
                        }
                    }

                    {
                        std::lock_guard<std::mutex> tlock(_telemetryMutex);
                        _securityTelemetry = telem;
                    }
                }
            }
        }
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
