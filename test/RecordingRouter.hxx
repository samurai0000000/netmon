/*
 * RecordingRouter.hxx
 *
 * Copyright (C) 2026, Charles Chiou
 */

#ifndef NETMON_RECORDING_ROUTER_HXX
#define NETMON_RECORDING_ROUTER_HXX

#include "ZyxelDriver.hxx"
#include <mutex>
#include <vector>
#include <string>

class RecordingRouter : public ZyxelDriver {
public:
    static RecordingRouter &getInstance();

    struct CallRecord {
        std::string method;
        std::string ip;
        std::string reason;
    };

    void reset();
    void setBlockIpSuccess(bool success);
    void setUnblockIpSuccess(bool success);
    size_t getCallCount() const;
    std::vector<CallRecord> getCalls() const;

    virtual nlohmann::json getStatus() override;
    virtual nlohmann::json getSessions() override;
    virtual nlohmann::json blockIp(const std::string &ip, const std::string &reason) override;
    virtual nlohmann::json unblockIp(const std::string &ip) override;

private:
    RecordingRouter();
    mutable std::mutex _mutex;
    std::vector<CallRecord> _calls;
    bool _blockIpSuccess = true;
    bool _unblockIpSuccess = true;
};

#endif /* NETMON_RECORDING_ROUTER_HXX */

/*
 * Local variables:
 * mode: C++
 * c-file-style: "BSD"
 * c-basic-offset: 4
 * tab-width: 4
 * indent-tabs-mode: nil
 * End:
 */
