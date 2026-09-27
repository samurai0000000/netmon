/*
 * ZyxelDriver.cxx
 *
 * Copyright (C) 2026, Charles Chiou
 */

#include "ZyxelDriver.hxx"
#include "Config.hxx"
#include "AuthManager.hxx"

#include <iostream>
#include <fstream>
#include <sstream>
#include <iomanip>
#include <vector>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#include <nlohmann/json.hpp>

using json = nlohmann::json;

ZyxelDriver &ZyxelDriver::getInstance() {
    static ZyxelDriver instance;
    return instance;
}

ZyxelDriver::ZyxelDriver()
    : _running(false),
      _configured(false),
      _pendingFlashWrite(false),
      _authFailed(false),
      _dryRun(false),
      _liveEnabled(false),
      _flashWriteEnabled(false) {
    _journalPath = Config::resolveHomePath("~/.config/netmon/router_journal.json");
    _lastMutationTime = std::chrono::steady_clock::now();
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

void ZyxelDriver::setJournalPath(const std::string &path) {
    std::lock_guard<std::mutex> lock(_journalMutex);
    _journalPath = Config::resolveHomePath(path);
}

std::string ZyxelDriver::getJournalPath() const {
    std::lock_guard<std::mutex> lock(_journalMutex);
    return _journalPath;
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

bool ZyxelDriver::isFlashWriteEnabled() const {
    return _flashWriteEnabled || Config::getInstance().getRouterFlashWrite();
}

void ZyxelDriver::setFlashWriteEnabled(bool enable) {
    _flashWriteEnabled = enable;
}

std::vector<std::string> ZyxelDriver::getDryRunLog() const {
    std::lock_guard<std::mutex> lock(_dryRunMutex);
    return _dryRunLog;
}

void ZyxelDriver::clearDryRunLog() {
    std::lock_guard<std::mutex> lock(_dryRunMutex);
    _dryRunLog.clear();
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
    replayJournalUnlocked();
    return true;
}

void ZyxelDriver::start() {
    if (_running.exchange(true)) {
        return;
    }
    _debounceThread = std::thread(&ZyxelDriver::debounceWorker, this);
    _keepaliveThread = std::thread(&ZyxelDriver::keepaliveWorker, this);

    if (isLiveEnabled()) {
        std::lock_guard<std::mutex> lock(_driverMutex);
        ensureConnectedUnlocked();
    }
}

void ZyxelDriver::stop() {
    if (!_running.exchange(false)) {
        return;
    }
    {
        std::lock_guard<std::mutex> lock(_debounceMutex);
        _debounceCv.notify_all();
    }
    if (_debounceThread.joinable()) {
        _debounceThread.join();
    }
    if (_keepaliveThread.joinable()) {
        _keepaliveThread.join();
    }
    flushFlashWrite();
    _sshClient.disconnect();
}

void ZyxelDriver::resetForTesting() {
    if (_running.exchange(false)) {
        {
            std::lock_guard<std::mutex> lock(_debounceMutex);
            _debounceCv.notify_all();
        }
        if (_debounceThread.joinable()) {
            _debounceThread.join();
        }
        if (_keepaliveThread.joinable()) {
            _keepaliveThread.join();
        }
    }
    std::lock_guard<std::mutex> lock(_driverMutex);
    std::lock_guard<std::mutex> jlock(_journalMutex);
    _configured = false;
    _pendingFlashWrite = false;
    _authFailed = false;
    _dryRun = false;
    _liveEnabled = false;
    _flashWriteEnabled = false;
    _sshClient.resetForTesting();
    _journalPath.clear();
    _journalPath.shrink_to_fit();
    {
        std::lock_guard<std::mutex> dlock(_dryRunMutex);
        _dryRunLog.clear();
        _dryRunLog.shrink_to_fit();
    }
}

bool ZyxelDriver::loadJournal(std::vector<RouterMutation> &mutations, bool &dirty) const {
    mutations.clear();
    dirty = false;

    if (_journalPath.empty()) {
        return false;
    }

    std::ifstream is(_journalPath);
    if (!is.is_open()) {
        return false;
    }

    try {
        json j;
        is >> j;
        dirty = j.value("dirty", false);
        if (j.contains("pending_mutations") && j["pending_mutations"].is_array()) {
            for (const auto &item : j["pending_mutations"]) {
                RouterMutation m;
                m.id = item.value("id", "");
                m.op = item.value("op", "");
                m.ip = item.value("ip", "");
                m.sanitizedName = item.value("sanitized_name", "");
                m.reason = item.value("reason", "");
                m.state = item.value("state", "pending");
                if (m.state.empty()) {
                    m.state = "pending";
                }
                m.timestamp = item.value("timestamp", 0);
                mutations.push_back(m);
            }
        }
        return true;
    } catch (...) {
        return false;
    }
}

bool ZyxelDriver::saveJournal(const std::vector<RouterMutation> &mutations, bool dirty) const {
    if (_journalPath.empty()) {
        return false;
    }

    size_t lastSlash = _journalPath.find_last_of('/');
    if (lastSlash != std::string::npos) {
        std::string dir = _journalPath.substr(0, lastSlash);
        struct stat st;
        if (stat(dir.c_str(), &st) != 0) {
            mkdir(dir.c_str(), 0700);
        }
    }

    json j;
    j["version"] = 1;
    j["dirty"] = dirty;
    j["last_mutation_timestamp"] = time(nullptr);
    json arr = json::array();
    for (const auto &m : mutations) {
        json item = {
            {"id", m.id},
            {"op", m.op},
            {"ip", m.ip},
            {"sanitized_name", m.sanitizedName},
            {"reason", m.reason},
            {"state", m.state.empty() ? "pending" : m.state},
            {"timestamp", m.timestamp}
        };
        arr.push_back(item);
    }
    j["pending_mutations"] = arr;

    std::string tmpPath = _journalPath + ".tmp";
    int fd = open(tmpPath.c_str(), O_WRONLY | O_CREAT | O_TRUNC, 0600);
    if (fd < 0) {
        return false;
    }
    fchmod(fd, 0600);
    std::string payload = j.dump(2);
    ssize_t written = write(fd, payload.c_str(), payload.size());
    fsync(fd);
    close(fd);

    if (written == static_cast<ssize_t>(payload.size())) {
        return (rename(tmpPath.c_str(), _journalPath.c_str()) == 0);
    }
    unlink(tmpPath.c_str());
    return false;
}

bool ZyxelDriver::appendMutationToJournal(const RouterMutation &m) {
    std::vector<RouterMutation> mutations;
    bool dirty = false;
    loadJournal(mutations, dirty);
    mutations.push_back(m);
    return saveJournal(mutations, true);
}

bool ZyxelDriver::removeMutationFromJournal(const std::string &id) {
    std::lock_guard<std::mutex> journalLock(_journalMutex);
    std::vector<RouterMutation> mutations;
    bool dirty = false;
    if (!loadJournal(mutations, dirty)) {
        return false;
    }
    std::vector<RouterMutation> remaining;
    for (const auto &m : mutations) {
        if (m.id != id) {
            remaining.push_back(m);
        }
    }
    return saveJournal(remaining, !remaining.empty());
}

bool ZyxelDriver::clearJournal() {
    std::vector<RouterMutation> emptyMutations;
    return saveJournal(emptyMutations, false);
}

std::vector<RouterMutation> ZyxelDriver::getPendingMutationsForTesting() const {
    std::lock_guard<std::mutex> lock(_journalMutex);
    std::vector<RouterMutation> mutations;
    bool dirty = false;
    loadJournal(mutations, dirty);
    return mutations;
}

bool ZyxelDriver::removeMutationForTesting(const std::string &id) {
    return removeMutationFromJournal(id);
}

bool ZyxelDriver::addMutationForTesting(const RouterMutation &m) {
    std::lock_guard<std::mutex> lock(_journalMutex);
    {
        std::lock_guard<std::mutex> dlock(_debounceMutex);
        _pendingFlashWrite = true;
    }
    return appendMutationToJournal(m);
}

bool ZyxelDriver::saveJournalForTesting(const std::vector<RouterMutation> &mutations, bool dirty) {
    std::lock_guard<std::mutex> lock(_journalMutex);
    {
        std::lock_guard<std::mutex> dlock(_debounceMutex);
        _pendingFlashWrite = true;
    }
    return saveJournal(mutations, dirty);
}

bool ZyxelDriver::loadJournalForTesting(std::vector<RouterMutation> &mutations, bool &dirty) const {
    std::lock_guard<std::mutex> lock(_journalMutex);
    return loadJournal(mutations, dirty);
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

SshResult ZyxelDriver::executeBlockSequence(const std::string &ip,
                                           const std::string &sanitizedName,
                                           const std::string &reason) {
    std::string objName = "NETMON_BLK_" + sanitizedName;
    std::string ruleName = "NETMON_RULE_" + sanitizedName;
    std::string cleanReason = ZyxelSshClient::sanitizeReason(reason);

    std::string out;
    SshResult res = _sshClient.executeCommand("configure terminal", out);
    if (res != SshResult::SUCCESS) {
        if (res == SshResult::ERR_LOCKED || ZyxelSshClient::isConfigLocked(out)) {
            _sshClient.unwindToRootPrompt();
            return SshResult::ERR_LOCKED;
        }
        return res;
    }

    // Step 1: Create or update address-object (treat duplicate as success)
    std::string cmdObj = "address-object " + objName + " host " + ip;
    res = _sshClient.executeCommand(cmdObj, out);
    if (res != SshResult::SUCCESS &&
        out.find("already exists") == std::string::npos &&
        out.find("duplicated") == std::string::npos) {
        _sshClient.unwindToRootPrompt();
        if (ZyxelSshClient::isSyntaxError(out)) {
            return SshResult::ERR_SYNTAX;
        }
        return res;
    }
    _sshClient.executeCommand("exit", out);

    // Step 2: Insert policy-control rule at position 1
    res = _sshClient.executeCommand("policy-control rule-insert 1", out);
    if (res != SshResult::SUCCESS) {
        if (out.find("already exists") != std::string::npos ||
            out.find("duplicated") != std::string::npos) {
            _sshClient.unwindToRootPrompt();
            return SshResult::SUCCESS;
        }
        executeRollback(sanitizedName);
        if (ZyxelSshClient::isSyntaxError(out)) {
            return SshResult::ERR_SYNTAX;
        }
        return res;
    }

    res = _sshClient.executeCommand("name " + ruleName, out);
    if (res != SshResult::SUCCESS ||
        out.find("already exists") != std::string::npos ||
        out.find("duplicated") != std::string::npos) {
        if (out.find("already exists") != std::string::npos ||
            out.find("duplicated") != std::string::npos) {
            _sshClient.unwindToRootPrompt();
            return SshResult::SUCCESS;
        }
        executeRollback(sanitizedName);
        if (ZyxelSshClient::isSyntaxError(out)) {
            return SshResult::ERR_SYNTAX;
        }
        return res;
    }

    res = _sshClient.executeCommand("action deny", out);
    if (res != SshResult::SUCCESS) {
        executeRollback(sanitizedName);
        if (ZyxelSshClient::isSyntaxError(out)) return SshResult::ERR_SYNTAX;
        return res;
    }
    res = _sshClient.executeCommand("source-ip " + objName, out);
    if (res != SshResult::SUCCESS) {
        executeRollback(sanitizedName);
        if (ZyxelSshClient::isSyntaxError(out)) return SshResult::ERR_SYNTAX;
        return res;
    }
    if (!cleanReason.empty()) {
        res = _sshClient.executeCommand("description \"NetMon Auto-Block: " + cleanReason + "\"", out);
        if (res != SshResult::SUCCESS) {
            executeRollback(sanitizedName);
            if (ZyxelSshClient::isSyntaxError(out)) return SshResult::ERR_SYNTAX;
            return res;
        }
    }
    res = _sshClient.executeCommand("activate", out);
    if (res != SshResult::SUCCESS) {
        executeRollback(sanitizedName);
        if (ZyxelSshClient::isSyntaxError(out)) return SshResult::ERR_SYNTAX;
        return res;
    }
    _sshClient.executeCommand("exit", out);
    _sshClient.executeCommand("exit", out);
    _sshClient.unwindToRootPrompt();

    return SshResult::SUCCESS;
}

SshResult ZyxelDriver::executeUnblockSequence(const std::string &sanitizedName) {
    std::string objName = "NETMON_BLK_" + sanitizedName;
    std::string ruleName = "NETMON_RULE_" + sanitizedName;

    std::string out;
    SshResult res = _sshClient.executeCommand("configure terminal", out);
    if (res != SshResult::SUCCESS) {
        if (res == SshResult::ERR_LOCKED || ZyxelSshClient::isConfigLocked(out)) {
            _sshClient.unwindToRootPrompt();
            return SshResult::ERR_LOCKED;
        }
        return res;
    }

    // Rule deleted FIRST, releasing reference to address-object
    res = _sshClient.executeCommand("no policy-control " + ruleName, out);
    if (res != SshResult::SUCCESS) {
        if (ZyxelSshClient::isSyntaxError(out)) {
            _sshClient.unwindToRootPrompt();
            return SshResult::ERR_SYNTAX;
        }
        if (res == SshResult::ERR_TIMEOUT ||
            res == SshResult::ERR_DISCONNECTED ||
            res == SshResult::ERR_CHANNEL_FAILED) {
            _sshClient.unwindToRootPrompt();
            return res;
        }
    }

    res = _sshClient.executeCommand("no address-object " + objName, out);
    if (res != SshResult::SUCCESS) {
        if (ZyxelSshClient::isSyntaxError(out)) {
            _sshClient.unwindToRootPrompt();
            return SshResult::ERR_SYNTAX;
        }
        if (res == SshResult::ERR_TIMEOUT ||
            res == SshResult::ERR_DISCONNECTED ||
            res == SshResult::ERR_CHANNEL_FAILED) {
            _sshClient.unwindToRootPrompt();
            return res;
        }
    }

    _sshClient.executeCommand("exit", out);
    _sshClient.unwindToRootPrompt();

    return SshResult::SUCCESS;
}

void ZyxelDriver::executeRollback(const std::string &sanitizedName) {
    std::string objName = "NETMON_BLK_" + sanitizedName;
    std::string ruleName = "NETMON_RULE_" + sanitizedName;
    std::string out;
    _sshClient.executeCommand("no policy-control " + ruleName, out);
    _sshClient.executeCommand("no address-object " + objName, out);
    _sshClient.unwindToRootPrompt();
}

void ZyxelDriver::clearAuthFailure() {
    std::lock_guard<std::mutex> lock(_driverMutex);
    _authFailed = false;
}

nlohmann::json ZyxelDriver::blockIp(const std::string &ip, const std::string &reason) {
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

    RouterMutation m;
    m.id = "blk_" + sanitizedName;
    m.op = "block";
    m.ip = ip;
    m.sanitizedName = sanitizedName;
    m.reason = reason;
    m.state = "pending";
    m.timestamp = time(nullptr);

    // Step 1: Append new mutation to journal on disk before running-config changes
    {
        std::lock_guard<std::mutex> journalLock(_journalMutex);
        appendMutationToJournal(m);
    }

    if (isDryRun()) {
        std::string objName = "NETMON_BLK_" + sanitizedName;
        std::string ruleName = "NETMON_RULE_" + sanitizedName;
        std::string cleanReason = ZyxelSshClient::sanitizeReason(reason);

        logDryRunCommand("configure terminal");
        logDryRunCommand("address-object " + objName + " host " + ip);
        logDryRunCommand("exit");
        logDryRunCommand("policy-control rule-insert 1");
        logDryRunCommand("name " + ruleName);
        logDryRunCommand("action deny");
        logDryRunCommand("source-ip " + objName);
        if (!cleanReason.empty()) {
            logDryRunCommand("description \"NetMon Auto-Block: " + cleanReason + "\"");
        }
        logDryRunCommand("activate");
        logDryRunCommand("exit");
        logDryRunCommand("exit");

        res["status"] = "queued";
        res["action"] = "block";
        res["ip"] = ip;
        res["rule"] = ruleName;
        res["dry_run"] = true;
        return res;
    }

    if (!isLiveEnabled()) {
        {
            std::lock_guard<std::mutex> debounceLock(_debounceMutex);
            _pendingFlashWrite = true;
        }
        res["status"] = "queued";
        res["action"] = "block";
        res["ip"] = ip;
        res["reason"] = reason;
        res["live_enabled"] = false;
        res["note"] = "Router driver disabled; mutation journaled for replay";
        return res;
    }

    ensureConnectedUnlocked();

    // Step 2: If session is up, apply once to running-config
    if (_sshClient.isConnected()) {
        SshResult ok = executeBlockSequence(ip, sanitizedName, reason);
        if (ok == SshResult::SUCCESS) {
            {
                std::lock_guard<std::mutex> journalLock(_journalMutex);
                std::vector<RouterMutation> mutations;
                bool dirty = false;
                if (loadJournal(mutations, dirty)) {
                    for (auto &mut : mutations) {
                        if (mut.id == m.id) {
                            mut.state = "applied_running";
                            break;
                        }
                    }
                    saveJournal(mutations, true);
                }
            }
            {
                std::lock_guard<std::mutex> debounceLock(_debounceMutex);
                _pendingFlashWrite = true;
                _lastMutationTime = std::chrono::steady_clock::now();
                _debounceCv.notify_all();
            }
            res["status"] = "success";
            res["action"] = "block";
            res["ip"] = ip;
            res["rule"] = "NETMON_RULE_" + sanitizedName;
            res["flash_save"] = "scheduled";
            return res;
        } else if (ok == SshResult::ERR_LOCKED) {
            // Configuration is locked; leave mutation in journal as "pending"
            res["status"] = "locked";
            res["action"] = "block";
            res["ip"] = ip;
            res["reason"] = reason;
            res["message"] = "Router configuration locked by another session";
            return res;
        } else if (ok == SshResult::ERR_SYNTAX) {
            // Explicit syntax rejection; rollback executed, remove poison entry
            removeMutationFromJournal(m.id);
            res["status"] = "error";
            res["error"] = "Failed to execute ZySH block sequence on router (syntax error)";
            res["ip"] = ip;
            return res;
        } else {
            // Transport / channel / timeout error: retain mutation in journal as "pending"
            {
                std::lock_guard<std::mutex> debounceLock(_debounceMutex);
                _pendingFlashWrite = true;
            }
            res["status"] = "queued";
            res["action"] = "block";
            res["ip"] = ip;
            res["reason"] = reason;
            res["note"] = "Connection dropped or transport error; mutation queued for replay";
            return res;
        }
    }

    // Router offline; mutation journaled for recovery replay
    {
        std::lock_guard<std::mutex> debounceLock(_debounceMutex);
        _pendingFlashWrite = true;
    }
    res["status"] = "queued";
    res["action"] = "block";
    res["ip"] = ip;
    res["reason"] = reason;
    res["note"] = "Router offline; mutation journaled for replay";
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

    RouterMutation m;
    m.id = "unblk_" + sanitizedName;
    m.op = "unblock";
    m.ip = ip;
    m.sanitizedName = sanitizedName;
    m.reason = "";
    m.state = "pending";
    m.timestamp = time(nullptr);

    // Step 1: Append new mutation to journal on disk before running-config changes
    {
        std::lock_guard<std::mutex> journalLock(_journalMutex);
        appendMutationToJournal(m);
    }

    if (isDryRun()) {
        std::string objName = "NETMON_BLK_" + sanitizedName;
        std::string ruleName = "NETMON_RULE_" + sanitizedName;

        logDryRunCommand("configure terminal");
        logDryRunCommand("no policy-control " + ruleName);
        logDryRunCommand("no address-object " + objName);
        logDryRunCommand("exit");

        res["status"] = "queued";
        res["action"] = "unblock";
        res["ip"] = ip;
        res["dry_run"] = true;
        return res;
    }

    if (!isLiveEnabled()) {
        {
            std::lock_guard<std::mutex> debounceLock(_debounceMutex);
            _pendingFlashWrite = true;
        }
        res["status"] = "queued";
        res["action"] = "unblock";
        res["ip"] = ip;
        res["live_enabled"] = false;
        res["note"] = "Router driver disabled; mutation journaled for replay";
        return res;
    }

    ensureConnectedUnlocked();

    // Step 2: If session is up, apply once to running-config
    if (_sshClient.isConnected()) {
        SshResult ok = executeUnblockSequence(sanitizedName);
        if (ok == SshResult::SUCCESS) {
            {
                std::lock_guard<std::mutex> journalLock(_journalMutex);
                std::vector<RouterMutation> mutations;
                bool dirty = false;
                if (loadJournal(mutations, dirty)) {
                    for (auto &mut : mutations) {
                        if (mut.id == m.id) {
                            mut.state = "applied_running";
                            break;
                        }
                    }
                    saveJournal(mutations, true);
                }
            }
            {
                std::lock_guard<std::mutex> debounceLock(_debounceMutex);
                _pendingFlashWrite = true;
                _lastMutationTime = std::chrono::steady_clock::now();
                _debounceCv.notify_all();
            }
            res["status"] = "success";
            res["action"] = "unblock";
            res["ip"] = ip;
            res["flash_save"] = "scheduled";
            return res;
        } else if (ok == SshResult::ERR_LOCKED) {
            // Configuration is locked; leave mutation in journal as "pending"
            res["status"] = "locked";
            res["action"] = "unblock";
            res["ip"] = ip;
            res["message"] = "Router configuration locked by another session";
            return res;
        } else if (ok == SshResult::ERR_SYNTAX) {
            // Explicit syntax rejection; rollback executed, remove poison entry
            removeMutationFromJournal(m.id);
            res["status"] = "error";
            res["error"] = "Failed to execute ZySH unblock sequence on router (syntax error)";
            res["ip"] = ip;
            return res;
        } else {
            // Transport / channel / timeout error: retain mutation in journal as "pending"
            {
                std::lock_guard<std::mutex> debounceLock(_debounceMutex);
                _pendingFlashWrite = true;
            }
            res["status"] = "queued";
            res["action"] = "unblock";
            res["ip"] = ip;
            res["note"] = "Connection dropped or transport error; mutation queued for replay";
            return res;
        }
    }

    // Router offline; mutation journaled for recovery replay
    {
        std::lock_guard<std::mutex> debounceLock(_debounceMutex);
        _pendingFlashWrite = true;
    }
    res["status"] = "queued";
    res["action"] = "unblock";
    res["ip"] = ip;
    res["note"] = "Router offline; mutation journaled for replay";
    return res;
}

nlohmann::json ZyxelDriver::getStatus() {
    nlohmann::json res;
    std::lock_guard<std::mutex> lock(_driverMutex);

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
        res["flash_write"] = isFlashWriteEnabled();
        res["note"] = "Router driver disabled by configuration (router_live_enabled = false)";
        return res;
    }

    ensureConnectedUnlocked();

    if (_sshClient.isConnected()) {
        res["status"] = "online";
        res["driver"] = "ZyxelDriver (USG FLEX 200)";
        res["transport"] = "SSH-2.0 (libssh2)";

        std::string out;
        SshResult rc = _sshClient.executeCommand("show version", out, 5000);
        if (rc == SshResult::SUCCESS) {
            res["raw_version"] = out;
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
    } else {
        SshClientState state = _sshClient.getState();
        if (state == SshClientState::DEGRADED_AUTH_FAILED) {
            res["status"] = "degraded";
            res["error"] = "Authentication failed (invalid router password)";
        } else if (state == SshClientState::DEGRADED_HOSTKEY_MISMATCH) {
            res["status"] = "degraded";
            res["error"] = "Host key fingerprint mismatch";
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
    SshResult rc = _sshClient.executeCommand("show session summary", out, 5000);
    if (rc != SshResult::SUCCESS) {
        res["status"] = "error";
        res["error"] = "Failed to query session summary";
        res["sessions"] = json::array();
        return res;
    }

    res["status"] = "ok";
    res["raw_summary"] = out;
    res["sessions"] = json::array();
    return res;
}

bool ZyxelDriver::replayJournalUnlocked() {
    std::vector<RouterMutation> mutations;
    bool dirty = false;

    {
        std::lock_guard<std::mutex> journalLock(_journalMutex);
        if (!loadJournal(mutations, dirty) || mutations.empty()) {
            return true;
        }
    }

    if (isDryRun()) {
        for (auto &m : mutations) {
            std::string objName = "NETMON_BLK_" + m.sanitizedName;
            std::string ruleName = "NETMON_RULE_" + m.sanitizedName;
            std::string cleanReason = ZyxelSshClient::sanitizeReason(m.reason);

            if (m.op == "block") {
                logDryRunCommand("configure terminal");
                logDryRunCommand("no policy-control " + ruleName);
                logDryRunCommand("no address-object " + objName);
                logDryRunCommand("address-object " + objName + " host " + m.ip);
                logDryRunCommand("exit");
                logDryRunCommand("policy-control rule-insert 1");
                logDryRunCommand("name " + ruleName);
                logDryRunCommand("action deny");
                logDryRunCommand("source-ip " + objName);
                if (!cleanReason.empty()) {
                    logDryRunCommand("description \"NetMon Auto-Block: " + cleanReason + "\"");
                }
                logDryRunCommand("activate");
                logDryRunCommand("exit");
                logDryRunCommand("exit");
            } else if (m.op == "unblock") {
                logDryRunCommand("configure terminal");
                logDryRunCommand("no policy-control " + ruleName);
                logDryRunCommand("no address-object " + objName);
                logDryRunCommand("exit");
            }
        }
        return true;
    }

    if (!isLiveEnabled() || !_sshClient.isConnected()) {
        return false;
    }

    std::vector<std::string> newlyAppliedIds;
    bool stopReplayNoWrite = false;

    for (auto &m : mutations) {
        if (m.op == "block") {
            std::string objName = "NETMON_BLK_" + m.sanitizedName;
            std::string ruleName = "NETMON_RULE_" + m.sanitizedName;
            std::string cleanReason = ZyxelSshClient::sanitizeReason(m.reason);

            std::string out;
            SshResult res = _sshClient.executeCommand("configure terminal", out);
            if (res != SshResult::SUCCESS) {
                if (res == SshResult::ERR_LOCKED || ZyxelSshClient::isConfigLocked(out)) {
                    _sshClient.unwindToRootPrompt();
                    break;
                }
                break;
            }

            // Step 1: Delete by name first (rule, then address-object). Missing object is success.
            res = _sshClient.executeCommand("no policy-control " + ruleName, out);
            if (res == SshResult::ERR_TIMEOUT ||
                res == SshResult::ERR_DISCONNECTED ||
                res == SshResult::ERR_CHANNEL_FAILED) {
                _sshClient.unwindToRootPrompt();
                break;
            }

            // As soon as no policy-control returns a prompt, demote row to pending on disk
            // because the live rule is no longer in running-config.
            m.state = "pending";
            {
                std::lock_guard<std::mutex> journalLock(_journalMutex);
                std::vector<RouterMutation> currentMutations;
                bool cdirty = false;
                if (loadJournal(currentMutations, cdirty)) {
                    for (auto &cm : currentMutations) {
                        if (cm.id == m.id) {
                            cm.state = "pending";
                            break;
                        }
                    }
                    saveJournal(currentMutations, true);
                }
            }

            res = _sshClient.executeCommand("no address-object " + objName, out);
            if (res == SshResult::ERR_TIMEOUT ||
                res == SshResult::ERR_DISCONNECTED ||
                res == SshResult::ERR_CHANNEL_FAILED) {
                _sshClient.unwindToRootPrompt();
                break;
            }

            // Step 2: Frozen insert sequence
            std::string cmdObj = "address-object " + objName + " host " + m.ip;
            res = _sshClient.executeCommand(cmdObj, out);
            if (res == SshResult::ERR_TIMEOUT ||
                res == SshResult::ERR_DISCONNECTED ||
                res == SshResult::ERR_CHANNEL_FAILED) {
                _sshClient.unwindToRootPrompt();
                break;
            }
            if (res != SshResult::SUCCESS &&
                out.find("already exists") == std::string::npos &&
                out.find("duplicated") == std::string::npos) {
                if (ZyxelSshClient::isSyntaxError(out)) {
                    removeMutationFromJournal(m.id);
                }
                _sshClient.unwindToRootPrompt();
                break;
            }
            _sshClient.executeCommand("exit", out);

            res = _sshClient.executeCommand("policy-control rule-insert 1", out);
            if (res == SshResult::ERR_TIMEOUT ||
                res == SshResult::ERR_DISCONNECTED ||
                res == SshResult::ERR_CHANNEL_FAILED) {
                _sshClient.unwindToRootPrompt();
                break;
            }
            if (res != SshResult::SUCCESS) {
                if (out.find("already exists") != std::string::npos ||
                    out.find("duplicated") != std::string::npos) {
                    _sshClient.unwindToRootPrompt();
                    stopReplayNoWrite = true;
                    break;
                }
                executeRollback(m.sanitizedName);
                if (ZyxelSshClient::isSyntaxError(out)) {
                    removeMutationFromJournal(m.id);
                }
                break;
            }

            res = _sshClient.executeCommand("name " + ruleName, out);
            if (out.find("already exists") != std::string::npos ||
                out.find("duplicated") != std::string::npos) {
                // Duplicate name after by-name delete means delete did not remove live rule.
                // Unwind, do not delete by index, leave row as pending, stop replay and do not write.
                _sshClient.unwindToRootPrompt();
                stopReplayNoWrite = true;
                break;
            }
            if (res != SshResult::SUCCESS) {
                executeRollback(m.sanitizedName);
                if (ZyxelSshClient::isSyntaxError(out)) {
                    removeMutationFromJournal(m.id);
                }
                break;
            }

            res = _sshClient.executeCommand("action deny", out);
            if (res != SshResult::SUCCESS) {
                executeRollback(m.sanitizedName);
                if (ZyxelSshClient::isSyntaxError(out)) {
                    removeMutationFromJournal(m.id);
                }
                break;
            }

            res = _sshClient.executeCommand("source-ip " + objName, out);
            if (res != SshResult::SUCCESS) {
                executeRollback(m.sanitizedName);
                if (ZyxelSshClient::isSyntaxError(out)) {
                    removeMutationFromJournal(m.id);
                }
                break;
            }

            if (!cleanReason.empty()) {
                res = _sshClient.executeCommand("description \"NetMon Auto-Block: " + cleanReason + "\"", out);
                if (res != SshResult::SUCCESS) {
                    executeRollback(m.sanitizedName);
                    if (ZyxelSshClient::isSyntaxError(out)) {
                        removeMutationFromJournal(m.id);
                    }
                    break;
                }
            }

            res = _sshClient.executeCommand("activate", out);
            if (res != SshResult::SUCCESS) {
                executeRollback(m.sanitizedName);
                if (ZyxelSshClient::isSyntaxError(out)) {
                    removeMutationFromJournal(m.id);
                }
                break;
            }

            _sshClient.executeCommand("exit", out);
            _sshClient.executeCommand("exit", out);
            _sshClient.unwindToRootPrompt();

            m.state = "applied_running";
            newlyAppliedIds.push_back(m.id);
        } else if (m.op == "unblock") {
            SshResult ok = executeUnblockSequence(m.sanitizedName);
            if (ok == SshResult::SUCCESS) {
                m.state = "applied_running";
                newlyAppliedIds.push_back(m.id);
            } else if (ok == SshResult::ERR_LOCKED) {
                break;
            } else if (ok == SshResult::ERR_SYNTAX) {
                removeMutationFromJournal(m.id);
            } else {
                break;
            }
        }
    }

    if (!newlyAppliedIds.empty()) {
        std::lock_guard<std::mutex> journalLock(_journalMutex);
        std::vector<RouterMutation> currentMutations;
        bool cdirty = false;
        if (loadJournal(currentMutations, cdirty)) {
            for (auto &cm : currentMutations) {
                for (const auto &aid : newlyAppliedIds) {
                    if (cm.id == aid) {
                        cm.state = "applied_running";
                        break;
                    }
                }
            }
            saveJournal(currentMutations, true);
        }
    }

    if (stopReplayNoWrite) {
        return false;
    }

    if (!isFlashWriteEnabled()) {
        return !newlyAppliedIds.empty();
    }

    if (!newlyAppliedIds.empty()) {
        std::string out;
        SshResult writeRes = _sshClient.executeCommand("write", out, 15000);
        if (writeRes == SshResult::SUCCESS) {
            std::lock_guard<std::mutex> journalLock(_journalMutex);
            std::vector<RouterMutation> remainingMutations;
            for (const auto &m : mutations) {
                bool wasApplied = false;
                for (const auto &aid : newlyAppliedIds) {
                    if (m.id == aid) {
                        wasApplied = true;
                        break;
                    }
                }
                if (!wasApplied) {
                    remainingMutations.push_back(m);
                }
            }
            saveJournal(remainingMutations, !remainingMutations.empty());
            if (remainingMutations.empty()) {
                std::lock_guard<std::mutex> debounceLock(_debounceMutex);
                _pendingFlashWrite = false;
            }
            return true;
        }
    } else {
        std::lock_guard<std::mutex> journalLock(_journalMutex);
        std::vector<RouterMutation> remainingMutations;
        bool rdirty = false;
        if (loadJournal(remainingMutations, rdirty) && remainingMutations.empty()) {
            std::lock_guard<std::mutex> debounceLock(_debounceMutex);
            _pendingFlashWrite = false;
            return true;
        }
    }

    return false;
}

bool ZyxelDriver::replayJournal() {
    std::lock_guard<std::mutex> driverLock(_driverMutex);
    return replayJournalUnlocked();
}

void ZyxelDriver::flushFlashWrite() {
    std::lock_guard<std::mutex> driverLock(_driverMutex);
    if (!_pendingFlashWrite) {
        return;
    }

    if (!isFlashWriteEnabled()) {
        return;
    }

    if (isDryRun()) {
        logDryRunCommand("write");
        std::lock_guard<std::mutex> journalLock(_journalMutex);
        std::vector<RouterMutation> activeMutations;
        bool activeDirty = false;
        if (loadJournal(activeMutations, activeDirty)) {
            std::vector<RouterMutation> remainingMutations;
            for (const auto &m : activeMutations) {
                if (m.state != "applied_running") {
                    remainingMutations.push_back(m);
                }
            }
            saveJournal(remainingMutations, !remainingMutations.empty());
            if (remainingMutations.empty()) {
                std::lock_guard<std::mutex> debounceLock(_debounceMutex);
                _pendingFlashWrite = false;
            }
        }
        return;
    }

    if (!isLiveEnabled() || !_sshClient.isConnected()) {
        return;
    }

    std::vector<RouterMutation> mutations;
    bool dirty = false;
    {
        std::lock_guard<std::mutex> journalLock(_journalMutex);
        loadJournal(mutations, dirty);
    }

    std::vector<std::string> newlyApplied;
    for (auto &m : mutations) {
        if (m.state == "pending" || m.state.empty()) {
            SshResult ok = SshResult::ERR_CHANNEL_FAILED;
            if (m.op == "block") {
                ok = executeBlockSequence(m.ip, m.sanitizedName, m.reason);
            } else if (m.op == "unblock") {
                ok = executeUnblockSequence(m.sanitizedName);
            }
            if (ok == SshResult::SUCCESS) {
                m.state = "applied_running";
                newlyApplied.push_back(m.id);
            } else if (ok == SshResult::ERR_LOCKED) {
                break;
            } else if (ok == SshResult::ERR_SYNTAX) {
                removeMutationFromJournal(m.id);
            } else {
                break;
            }
        }
    }

    if (!newlyApplied.empty()) {
        std::lock_guard<std::mutex> journalLock(_journalMutex);
        std::vector<RouterMutation> currentMutations;
        bool cdirty = false;
        if (loadJournal(currentMutations, cdirty)) {
            for (auto &cm : currentMutations) {
                for (const auto &aid : newlyApplied) {
                    if (cm.id == aid) {
                        cm.state = "applied_running";
                        break;
                    }
                }
            }
            saveJournal(currentMutations, true);
        }
    }

    // Check if any mutations are applied_running and ready for flash write
    std::vector<RouterMutation> activeMutations;
    bool activeDirty = false;
    {
        std::lock_guard<std::mutex> journalLock(_journalMutex);
        loadJournal(activeMutations, activeDirty);
    }

    std::vector<std::string> appliedToPrune;
    for (const auto &m : activeMutations) {
        if (m.state == "applied_running") {
            appliedToPrune.push_back(m.id);
        }
    }

    if (!appliedToPrune.empty()) {
        std::string out;
        SshResult writeRes = _sshClient.executeCommand("write", out, 15000);
        if (writeRes == SshResult::SUCCESS) {
            std::lock_guard<std::mutex> journalLock(_journalMutex);
            std::vector<RouterMutation> remainingMutations;
            for (const auto &m : activeMutations) {
                bool wasApplied = false;
                for (const auto &aid : appliedToPrune) {
                    if (m.id == aid) {
                        wasApplied = true;
                        break;
                    }
                }
                if (!wasApplied) {
                    remainingMutations.push_back(m);
                }
            }
            saveJournal(remainingMutations, !remainingMutations.empty());
            if (remainingMutations.empty()) {
                std::lock_guard<std::mutex> debounceLock(_debounceMutex);
                _pendingFlashWrite = false;
            }
        }
    } else if (activeMutations.empty()) {
        std::lock_guard<std::mutex> debounceLock(_debounceMutex);
        _pendingFlashWrite = false;
    }
}

void ZyxelDriver::debounceWorker() {
    while (_running) {
        std::unique_lock<std::mutex> debounceLock(_debounceMutex);
        _debounceCv.wait_for(debounceLock, std::chrono::seconds(5), [this]() {
            return !_running.load() || _pendingFlashWrite;
        });

        if (!_running) {
            break;
        }

        if (_pendingFlashWrite) {
            auto now = std::chrono::steady_clock::now();
            auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(now - _lastMutationTime).count();
            if (elapsed >= 30) {
                // 30 seconds of quiescence reached
                debounceLock.unlock();
                flushFlashWrite();
            }
        }
    }
}

void ZyxelDriver::keepaliveWorker() {
    while (_running) {
        for (int i = 0; i < 150 && _running; ++i) {
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }
        if (!_running) {
            break;
        }
        if (_sshClient.isConnected()) {
            _sshClient.sendKeepalive();
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
