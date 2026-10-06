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

    struct ClearanceCall {
        std::string command;
        int timeoutMs;
    };

    void reset();
    void setBlockIpSuccess(bool success);
    void setUnblockIpSuccess(bool success);
    size_t getCallCount() const;
    std::vector<CallRecord> getCalls() const;

    void setNextClearanceResponse(SshResult result, const std::string &output, const std::string &matchedPrompt);
    void setCurrentPrompt(const std::string &prompt);
    std::vector<ClearanceCall> getClearanceCalls() const;
    size_t getUnwindCalls() const;

    virtual nlohmann::json getStatus() override;
    virtual nlohmann::json getSessions() override;
    virtual nlohmann::json blockIp(const std::string &ip, const std::string &reason) override;
    virtual nlohmann::json unblockIp(const std::string &ip) override;
    virtual nlohmann::json getSecurityMetrics() override;

    virtual SshResult executeClearanceCommand(const std::string &command,
                                            std::string &outputOut,
                                            std::string &matchedPromptOut,
                                            int timeoutMs = 5000,
                                            bool isDiagnostic = false) override;
    virtual SshResult unwindToRootPrompt() override;
    virtual std::string getLastMatchedPrompt() const override;
    virtual void cancelActiveCommand() override;
    size_t getCancelCalls() const;

    void setExecutionDelayMs(int delayMs);

private:
    RecordingRouter();
    mutable std::mutex _mutex;
    std::vector<CallRecord> _calls;
    std::vector<ClearanceCall> _clearanceCalls;
    size_t _unwindCalls = 0;
    size_t _cancelCalls = 0;
    bool _blockIpSuccess = true;
    bool _unblockIpSuccess = true;
    SshResult _nextClearanceResult = SshResult::SUCCESS;
    std::string _nextClearanceOutput;
    std::string _nextClearancePrompt = "#";
    std::string _currentPrompt = "#";
    bool _customResponseSet = false;
    int _executionDelayMs = 0;
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
