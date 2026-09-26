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
    _blockIpSuccess = true;
    _unblockIpSuccess = true;
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

// Implementations of ZyxelDriver when RecordingRouter replaces src/ZyxelDriver.cxx
ZyxelDriver::ZyxelDriver()
    : _configured(true) {
}

bool ZyxelDriver::isConfigured() const {
    return _configured;
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

ZyxelDriver &ZyxelDriver::getInstance() {
    return RecordingRouter::getInstance();
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
