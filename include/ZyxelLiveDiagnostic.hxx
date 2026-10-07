/*
 * ZyxelLiveDiagnostic.hxx
 *
 * Copyright (C) 2026, Charles Chiou
 */

#ifndef NETMON_ZYXEL_LIVE_DIAGNOSTIC_HXX
#define NETMON_ZYXEL_LIVE_DIAGNOSTIC_HXX

#include "ZyxelSshClient.hxx"

#include <string>
#include <vector>
#include <ostream>
#include <cstdint>
#include <cstddef>

struct DiagMetricEntry {
    std::string testGroup;
    std::string testName;
    std::string command;
    double rttMs = 0.0;
    uint64_t parseUs = 0;
    size_t bytes = 0;
    size_t entities = 0;
    bool passed = false;
    std::string statusMsg;
};

class ZyxelBenchmark {
public:
    static void record(const DiagMetricEntry &entry);
    static void clear();
    static const std::vector<DiagMetricEntry> &getEntries();
    static void renderScorecard(std::ostream &os,
                                const std::string &host = "gateway",
                                const std::string &model = "USG FLEX 200");
};

/**
 * Executes in-process CppUTest live diagnostic suite against the active SSH driver.
 *
 * @param os Output stream for diagnostic logs and benchmark report.
 * @param mode Diagnostic mode: "read", "write", or "all".
 * @param filter Optional specific test filter pattern.
 * @return 0 on full success, or number of failed assertions / negative error code.
 */
int runLiveFirewallDiagnostic(std::ostream &os,
                             const std::string &mode = "read",
                             const std::string &filter = "");

std::string diagLogPath();
void logDiag(const std::string &msg);
bool diagnosticLineIsConfig(const std::string &line);
std::vector<std::string> diagnosticReadCatalog();
std::vector<std::string> buildDiagnosticArgv(const std::string &mode,
                                             const std::string &filter);

class ZyxelDriver;

class DiagRestoreBracket {
public:
    DiagRestoreBracket(ZyxelDriver &driver,
                       std::vector<std::string> inverse,
                       std::string showCommand,
                       std::string objectToken);
    ~DiagRestoreBracket();

    SshResult send(const std::string &line);
    void restoreByUnblock(const std::string &ip);
    void ensureConfigMode();
    void restore();
    void dismiss();

    bool restored() const { return _restored; }
    bool restoreSucceeded() const { return _restoreSucceeded; }
    bool objectGone() const { return _objectGone; }
    bool policyAborted() const { return _policyAborted; }
    const std::string &showOutput() const { return _showOutput; }
    const std::string &restoreError() const { return _restoreError; }

private:
    ZyxelDriver &_driver;
    std::vector<std::string> _inverse;
    std::string _showCommand;
    std::string _objectToken;
    std::string _unblockIp;
    std::string _showOutput;
    std::string _restoreError;
    bool _restored;
    bool _restoreSucceeded;
    bool _objectGone;
    bool _policyAborted;
    bool _awaitingNoActivate;
};

#endif /* NETMON_ZYXEL_LIVE_DIAGNOSTIC_HXX */

/*
 * Local variables:
 * mode: C++
 * c-file-style: "BSD"
 * c-basic-offset: 4
 * tab-width: 4
 * indent-tabs-mode: nil
 * End:
 */
