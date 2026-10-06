/*
 * RecordingRouter.cxx
 *
 * Copyright (C) 2026, Charles Chiou
 */

#include "RecordingRouter.hxx"

RecordingRouter &RecordingRouter::getInstance() {
    static RecordingRouter instance;
    return instance;
}

RecordingRouter::RecordingRouter() : ZyxelDriver() {
}

void RecordingRouter::reset() {
    std::lock_guard<std::mutex> lock(_mutex);
    std::vector<CallRecord>().swap(_calls);
    std::vector<ClearanceCall>().swap(_clearanceCalls);
    _unwindCalls = 0;
    _cancelCalls = 0;
    _blockIpSuccess = true;
    _unblockIpSuccess = true;
    _nextClearanceResult = SshResult::SUCCESS;
    std::string().swap(_nextClearanceOutput);
    std::string("#").swap(_nextClearancePrompt);
    std::string("#").swap(_currentPrompt);
    _customResponseSet = false;
    _executionDelayMs = 0;
}

void RecordingRouter::setBlockIpSuccess(bool success) {
    std::lock_guard<std::mutex> lock(_mutex);
    _blockIpSuccess = success;
}

void RecordingRouter::setUnblockIpSuccess(bool success) {
    std::lock_guard<std::mutex> lock(_mutex);
    _unblockIpSuccess = success;
}

size_t RecordingRouter::getCallCount() const {
    std::lock_guard<std::mutex> lock(_mutex);
    return _calls.size();
}

std::vector<RecordingRouter::CallRecord> RecordingRouter::getCalls() const {
    std::lock_guard<std::mutex> lock(_mutex);
    return _calls;
}

nlohmann::json RecordingRouter::getStatus() {
    return {{"status", "online"}, {"driver", "RecordingRouter"}};
}

nlohmann::json RecordingRouter::getSessions() {
    return {{"sessions", nlohmann::json::array()}};
}

nlohmann::json RecordingRouter::getSecurityMetrics() {
    return {{"status", "ok"}, {"driver", "RecordingRouter"}};
}

nlohmann::json RecordingRouter::blockIp(const std::string &ip, const std::string &reason) {
    std::lock_guard<std::mutex> lock(_mutex);
    _calls.push_back({"blockIp", ip, reason});
    if (!_blockIpSuccess) {
        return {{"status", "error"}, {"error", "Simulated router failure"}};
    }
    return {{"status", "success"}, {"action", "block"}, {"ip", ip}};
}

nlohmann::json RecordingRouter::unblockIp(const std::string &ip) {
    std::lock_guard<std::mutex> lock(_mutex);
    _calls.push_back({"unblockIp", ip, ""});
    if (!_unblockIpSuccess) {
        return {{"status", "error"}, {"error", "Simulated router failure"}};
    }
    return {{"status", "success"}, {"action", "unblock"}, {"ip", ip}};
}

void RecordingRouter::setNextClearanceResponse(SshResult result, const std::string &output, const std::string &matchedPrompt) {
    std::lock_guard<std::mutex> lock(_mutex);
    _nextClearanceResult = result;
    _nextClearanceOutput = output;
    _nextClearancePrompt = matchedPrompt;
    _customResponseSet = true;
}

void RecordingRouter::setCurrentPrompt(const std::string &prompt) {
    std::lock_guard<std::mutex> lock(_mutex);
    _currentPrompt = prompt;
}

std::vector<RecordingRouter::ClearanceCall> RecordingRouter::getClearanceCalls() const {
    std::lock_guard<std::mutex> lock(_mutex);
    return _clearanceCalls;
}

size_t RecordingRouter::getUnwindCalls() const {
    std::lock_guard<std::mutex> lock(_mutex);
    return _unwindCalls;
}

void RecordingRouter::setExecutionDelayMs(int delayMs) {
    std::lock_guard<std::mutex> lock(_mutex);
    _executionDelayMs = delayMs;
}

SshResult RecordingRouter::executeClearanceCommand(const std::string &command,
                                                 std::string &outputOut,
                                                 std::string &matchedPromptOut,
                                                 int timeoutMs,
                                                 bool isDiagnostic) {
    int delay = 0;
    {
        std::lock_guard<std::mutex> lock(_mutex);
        _clearanceCalls.push_back({command, timeoutMs});
        delay = _executionDelayMs;
    }

    if (delay > 0) {
        int slept = 0;
        while (slept < delay) {
            {
                std::lock_guard<std::mutex> lock(_mutex);
                if (_cancelCalls > 0) {
                    outputOut = "Command cancelled";
                    matchedPromptOut = _currentPrompt;
                    return SshResult::ERR_INTERRUPTED;
                }
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
            slept += 10;
        }
    }

    std::lock_guard<std::mutex> lock(_mutex);
    if (_cancelCalls > 0) {
        outputOut = "Command cancelled";
        matchedPromptOut = _currentPrompt;
        return SshResult::ERR_INTERRUPTED;
    }
    if (_customResponseSet) {
        _customResponseSet = false;
        outputOut = _nextClearanceOutput;
        matchedPromptOut = _nextClearancePrompt;
        _currentPrompt = _nextClearancePrompt;
        return _nextClearanceResult;
    }
    outputOut = "[simulated] " + command;
    matchedPromptOut = _currentPrompt;
    return SshResult::SUCCESS;
}

SshResult RecordingRouter::unwindToRootPrompt() {
    std::lock_guard<std::mutex> lock(_mutex);
    _unwindCalls++;
    _currentPrompt = "#";
    return SshResult::SUCCESS;
}

std::string RecordingRouter::getLastMatchedPrompt() const {
    std::lock_guard<std::mutex> lock(_mutex);
    return _currentPrompt;
}

// Implementations of ZyxelDriver when RecordingRouter replaces src/ZyxelDriver.cxx
ZyxelDriver::ZyxelDriver()
    : _running(false),
      _configured(true),
      _pendingFlashWrite(false),
      _authFailed(false) {
}

ZyxelDriver::~ZyxelDriver() {
}

bool ZyxelDriver::isConfigured() const {
    return _configured;
}

bool ZyxelDriver::isConnected() const {
    return true;
}

nlohmann::json ZyxelDriver::getStatus() {
    return RecordingRouter::getInstance().getStatus();
}

nlohmann::json ZyxelDriver::getSessions() {
    return RecordingRouter::getInstance().getSessions();
}

nlohmann::json ZyxelDriver::blockIp(const std::string &ip, const std::string &reason) {
    return RecordingRouter::getInstance().blockIp(ip, reason);
}

nlohmann::json ZyxelDriver::unblockIp(const std::string &ip) {
    return RecordingRouter::getInstance().unblockIp(ip);
}

nlohmann::json ZyxelDriver::ping(const std::string &target, int count) {
    return {
        {"status", "ok"},
        {"type", "ping"},
        {"target", target},
        {"packets_transmitted", count},
        {"packets_received", count},
        {"packet_loss_percent", 0.0},
        {"min_latency_ms", 1.0},
        {"avg_latency_ms", 2.0},
        {"max_latency_ms", 3.0}
    };
}

nlohmann::json ZyxelDriver::traceroute(const std::string &target) {
    return {
        {"status", "ok"},
        {"type", "traceroute"},
        {"target", target},
        {"packets_received", 3}
    };
}

void RecordingRouter::cancelActiveCommand() {
    std::lock_guard<std::mutex> lock(_mutex);
    _cancelCalls++;
}

size_t RecordingRouter::getCancelCalls() const {
    std::lock_guard<std::mutex> lock(_mutex);
    return _cancelCalls;
}

ZyxelDriver &ZyxelDriver::getInstance() {
    return RecordingRouter::getInstance();
}

void ZyxelDriver::clearAuthFailure() {
}

void ZyxelDriver::cancelActiveCommand() {
    RecordingRouter::getInstance().cancelActiveCommand();
}

SshResult ZyxelDriver::executeClearanceCommand(const std::string &command,
                                              std::string &outputOut,
                                              std::string &matchedPromptOut,
                                              int timeoutMs,
                                              bool isDiagnostic) {
    return RecordingRouter::getInstance().executeClearanceCommand(command, outputOut, matchedPromptOut, timeoutMs, isDiagnostic);
}

SshResult ZyxelDriver::unwindToRootPrompt() {
    return RecordingRouter::getInstance().unwindToRootPrompt();
}

std::string ZyxelDriver::getLastMatchedPrompt() const {
    return RecordingRouter::getInstance().getLastMatchedPrompt();
}

nlohmann::json ZyxelDriver::getSecurityMetrics() {
    return RecordingRouter::getInstance().getSecurityMetrics();
}

ZyxelSecurityTelemetry ZyxelDriver::getSecurityTelemetry() const {
    return ZyxelSecurityTelemetry();
}

void ZyxelDriver::setSecurityTelemetryForTesting(const ZyxelSecurityTelemetry &telem) {
    (void)telem;
}

int runLiveFirewallDiagnostic(std::ostream &os, const std::string &, const std::string &) {
    os << "Error: Firewall driver is not connected to remote host.\n";
    return -2;
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
