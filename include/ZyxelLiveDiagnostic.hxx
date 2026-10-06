/*
 * ZyxelLiveDiagnostic.hxx
 *
 * Copyright (C) 2026, Charles Chiou
 */

#ifndef NETMON_ZYXEL_LIVE_DIAGNOSTIC_HXX
#define NETMON_ZYXEL_LIVE_DIAGNOSTIC_HXX

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
