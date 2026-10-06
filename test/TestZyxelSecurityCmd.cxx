/*
 * TestZyxelSecurityCmd.cxx
 *
 * Copyright (C) 2026, Charles Chiou
 */

#include "zyxel/ZyxelSecurityCmd.hxx"
#include <CppUTest/TestHarness.h>

TEST_GROUP(ZyxelSecurityCmdTest) {
    void setup() {}
    void teardown() {}
};

TEST(ZyxelSecurityCmdTest, CommandGeneratorsReturnExpectedStrings) {
    STRCMP_EQUAL("show app statistics summary", ZyxelSecurityCmd::cmdShowAppStatisticsSummary().c_str());
    STRCMP_EQUAL("show idp statistics summary", ZyxelSecurityCmd::cmdShowIdpStatisticsSummary().c_str());
    STRCMP_EQUAL("show conn status", ZyxelSecurityCmd::cmdShowConnStatus().c_str());
}

TEST(ZyxelSecurityCmdTest, ParseAppStatisticsSummaryKeyVal) {
    std::string raw =
        "Application Patrol Statistics Summary:\n"
        "Forwarded Traffic (KB)  : 1048576\n"
        "Dropped Traffic (KB)    : 4096\n"
        "Rejected Traffic (KB)   : 1024\n"
        "Matched Connections     : 58230\n";

    ZyxelAppPatrolSummary summary;
    CHECK_TRUE(ZyxelSecurityCmd::parseAppStatisticsSummary(raw, summary));
    CHECK_EQUAL(1048576ULL, summary.forwardedKb);
    CHECK_EQUAL(4096ULL, summary.droppedKb);
    CHECK_EQUAL(1024ULL, summary.rejectedKb);
    CHECK_EQUAL(58230ULL, summary.matchedConnections);

    nlohmann::json j = summary.toJson();
    CHECK_EQUAL(1048576ULL, j["forwarded_kb"].get<uint64_t>());
    CHECK_EQUAL(4096ULL, j["dropped_kb"].get<uint64_t>());
    CHECK_EQUAL(1024ULL, j["rejected_kb"].get<uint64_t>());
    CHECK_EQUAL(58230ULL, j["matched_connections"].get<uint64_t>());
}

TEST(ZyxelSecurityCmdTest, ParseAppStatisticsSummaryTable) {
    std::string raw =
        "App Patrol Summary\n"
        "forward: 500000 KB, drop: 250 KB, reject: 50 KB, match: 1200\n";

    ZyxelAppPatrolSummary summary;
    CHECK_TRUE(ZyxelSecurityCmd::parseAppStatisticsSummary(raw, summary));
    CHECK_EQUAL(500000ULL, summary.forwardedKb);
    CHECK_EQUAL(250ULL, summary.droppedKb);
    CHECK_EQUAL(50ULL, summary.rejectedKb);
    CHECK_EQUAL(1200ULL, summary.matchedConnections);
}

TEST(ZyxelSecurityCmdTest, ParseAppStatisticsSummaryInvalid) {
    ZyxelAppPatrolSummary summary;
    CHECK_FALSE(ZyxelSecurityCmd::parseAppStatisticsSummary("", summary));
    CHECK_FALSE(ZyxelSecurityCmd::parseAppStatisticsSummary("No matching stats available\n", summary));
}

TEST(ZyxelSecurityCmdTest, ParseIdpStatisticsSummaryKeyVal) {
    std::string raw =
        "IDP Statistics Summary:\n"
        "IDP Status              : Enabled\n"
        "Threats Detected        : 14\n"
        "Packets Dropped         : 88\n"
        "Connections Reset       : 3\n";

    ZyxelIdpSummary summary;
    CHECK_TRUE(ZyxelSecurityCmd::parseIdpStatisticsSummary(raw, summary));
    CHECK_TRUE(summary.enabled);
    CHECK_EQUAL(14ULL, summary.threatsDetected);
    CHECK_EQUAL(88ULL, summary.packetsDropped);
    CHECK_EQUAL(3ULL, summary.connectionsReset);

    nlohmann::json j = summary.toJson();
    CHECK_TRUE(j["enabled"].get<bool>());
    CHECK_EQUAL(14ULL, j["threats_detected"].get<uint64_t>());
    CHECK_EQUAL(88ULL, j["packets_dropped"].get<uint64_t>());
    CHECK_EQUAL(3ULL, j["connections_reset"].get<uint64_t>());
}

TEST(ZyxelSecurityCmdTest, ParseIdpStatisticsSummaryDisabled) {
    std::string raw =
        "IDP Status : Disabled\n"
        "Threats    : 0\n";

    ZyxelIdpSummary summary;
    CHECK_TRUE(ZyxelSecurityCmd::parseIdpStatisticsSummary(raw, summary));
    CHECK_FALSE(summary.enabled);
    CHECK_EQUAL(0ULL, summary.threatsDetected);
}

TEST(ZyxelSecurityCmdTest, ParseIdpStatisticsSummaryInvalid) {
    ZyxelIdpSummary summary;
    CHECK_FALSE(ZyxelSecurityCmd::parseIdpStatisticsSummary("", summary));
    CHECK_FALSE(ZyxelSecurityCmd::parseIdpStatisticsSummary("Command not supported on this firmware\n", summary));
}

TEST(ZyxelSecurityCmdTest, ParseConnStatusDelegation) {
    std::string raw = "Current sessions: 182 / 50000\n";
    ZyxelSessionSummary summary;
    CHECK_TRUE(ZyxelSecurityCmd::parseConnStatus(raw, summary));
    CHECK_EQUAL(182, summary.activeSessions);
    CHECK_EQUAL(50000, summary.maxSessions);
}

TEST(ZyxelSecurityCmdTest, SecurityTelemetryToJson) {
    ZyxelSecurityTelemetry telem;
    telem.timestamp = 1700000000;
    telem.sessionSummary.activeSessions = 120;
    telem.sessionSummary.maxSessions = 1000000;
    telem.hasSessionSummary = true;

    telem.appPatrolSummary.forwardedKb = 9999;
    telem.hasAppPatrol = true;

    telem.idpSummary.enabled = true;
    telem.idpSummary.threatsDetected = 7;
    telem.hasIdp = true;

    nlohmann::json j = telem.toJson();
    CHECK_EQUAL(1700000000, j["timestamp"].get<int64_t>());
    CHECK_TRUE(j["has_session_summary"].get<bool>());
    CHECK_TRUE(j["has_app_patrol"].get<bool>());
    CHECK_TRUE(j["has_idp"].get<bool>());
    CHECK_EQUAL(120, j["sessions"]["active_sessions"].get<int>());
    CHECK_EQUAL(9999ULL, j["app_patrol"]["forwarded_kb"].get<uint64_t>());
    CHECK_EQUAL(7ULL, j["idp"]["threats_detected"].get<uint64_t>());
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
