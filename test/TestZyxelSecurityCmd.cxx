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

    // Envelope 7: BWM
    STRCMP_EQUAL("show bwm activation", ZyxelSecurityCmd::cmdShowBwmActivation().c_str());
    STRCMP_EQUAL("show bwm all", ZyxelSecurityCmd::cmdShowBwmAll().c_str());
    STRCMP_EQUAL("show bwm 1", ZyxelSecurityCmd::cmdShowBwm("1").c_str());
    STRCMP_EQUAL("show bwm control-tcp-ack", ZyxelSecurityCmd::cmdShowBwmControlTcpAck().c_str());
    STRCMP_EQUAL("show bwm default", ZyxelSecurityCmd::cmdShowBwmDefault().c_str());
    STRCMP_EQUAL("show bwm applications list", ZyxelSecurityCmd::cmdShowBwmApplicationsList().c_str());
    STRCMP_EQUAL("bwm append", ZyxelSecurityCmd::cmdBwmAppend().c_str());
    STRCMP_EQUAL("bwm delete 2", ZyxelSecurityCmd::cmdBwmDelete(2).c_str());
    STRCMP_EQUAL("bwm activate", ZyxelSecurityCmd::cmdBwmActivate(true).c_str());
    STRCMP_EQUAL("no bwm activate", ZyxelSecurityCmd::cmdBwmActivate(false).c_str());
    STRCMP_EQUAL("bwm control-tcp-ack activate", ZyxelSecurityCmd::cmdBwmControlTcpAckActivate(true).c_str());
    STRCMP_EQUAL("no bwm control-tcp-ack activate", ZyxelSecurityCmd::cmdBwmControlTcpAckActivate(false).c_str());
    STRCMP_EQUAL("bwm default inbound priority 4", ZyxelSecurityCmd::cmdBwmDefaultInboundPriority(4).c_str());
    STRCMP_EQUAL("bwm default outbound priority 4", ZyxelSecurityCmd::cmdBwmDefaultOutboundPriority(4).c_str());

    // Envelope 7: App Patrol
    STRCMP_EQUAL("show app profiles", ZyxelSecurityCmd::cmdShowAppProfiles().c_str());
    STRCMP_EQUAL("app test_profile", ZyxelSecurityCmd::cmdAppProfile("test_profile").c_str());
    STRCMP_EQUAL("no app test_profile", ZyxelSecurityCmd::cmdNoAppProfile("test_profile").c_str());

    // Envelope 7: Anti-Virus
    STRCMP_EQUAL("show anti-virus profile", ZyxelSecurityCmd::cmdShowAntiVirusProfile().c_str());
    STRCMP_EQUAL("show anti-virus update", ZyxelSecurityCmd::cmdShowAntiVirusUpdate().c_str());
    STRCMP_EQUAL("show anti-virus statistics", ZyxelSecurityCmd::cmdShowAntiVirusStatistics().c_str());
    STRCMP_EQUAL("anti-virus test_av", ZyxelSecurityCmd::cmdAntiVirusProfile("test_av").c_str());
    STRCMP_EQUAL("no anti-virus test_av", ZyxelSecurityCmd::cmdNoAntiVirusProfile("test_av").c_str());

    // Envelope 7: RTLS, Reputation Filter, Sandboxing, IDP
    STRCMP_EQUAL("show rtls", ZyxelSecurityCmd::cmdShowRtls().c_str());
    STRCMP_EQUAL("show reputation-filter status", ZyxelSecurityCmd::cmdShowReputationFilterStatus().c_str());
    STRCMP_EQUAL("show reputation-filter ip-reputation status", ZyxelSecurityCmd::cmdShowReputationFilterIpReputationStatus().c_str());
    STRCMP_EQUAL("show reputation-filter dns-threat-filter status", ZyxelSecurityCmd::cmdShowReputationFilterDnsThreatFilterStatus().c_str());
    STRCMP_EQUAL("show reputation-filter url-threat-filter status", ZyxelSecurityCmd::cmdShowReputationFilterUrlThreatFilterStatus().c_str());
    STRCMP_EQUAL("show sandboxing status", ZyxelSecurityCmd::cmdShowSandboxingStatus().c_str());
    STRCMP_EQUAL("show idp engine", ZyxelSecurityCmd::cmdShowIdpEngine().c_str());
    STRCMP_EQUAL("show idp signature", ZyxelSecurityCmd::cmdShowIdpSignature().c_str());

    // Envelope 7: Content Filter, Anti-Spam
    STRCMP_EQUAL("show content-filter profile", ZyxelSecurityCmd::cmdShowContentFilterProfile().c_str());
    STRCMP_EQUAL("show content-filter settings", ZyxelSecurityCmd::cmdShowContentFilterSettings().c_str());
    STRCMP_EQUAL("show content-filter statistics", ZyxelSecurityCmd::cmdShowContentFilterStatistics().c_str());
    STRCMP_EQUAL("content-filter cf-queue flush", ZyxelSecurityCmd::cmdContentFilterCfQueueFlush().c_str());
    STRCMP_EQUAL("show anti-spam profile", ZyxelSecurityCmd::cmdShowAntiSpamProfile().c_str());
    STRCMP_EQUAL("show anti-spam statistics", ZyxelSecurityCmd::cmdShowAntiSpamStatistics().c_str());
    STRCMP_EQUAL("anti-spam profile append", ZyxelSecurityCmd::cmdAntiSpamProfileAppend().c_str());
    STRCMP_EQUAL("anti-spam profile delete 1", ZyxelSecurityCmd::cmdAntiSpamProfileDelete(1).c_str());

    // Envelope 7: CDR, SSL Inspection
    STRCMP_EQUAL("show cdr status", ZyxelSecurityCmd::cmdShowCdrStatus().c_str());
    STRCMP_EQUAL("show cdr block-list", ZyxelSecurityCmd::cmdShowCdrBlockList().c_str());
    STRCMP_EQUAL("show cdr rules", ZyxelSecurityCmd::cmdShowCdrRules().c_str());
    STRCMP_EQUAL("cdr activate", ZyxelSecurityCmd::cmdCdrActivate(true).c_str());
    STRCMP_EQUAL("no cdr activate", ZyxelSecurityCmd::cmdCdrActivate(false).c_str());
    STRCMP_EQUAL("show ssl-inspection status", ZyxelSecurityCmd::cmdShowSslInspectionStatus().c_str());
    STRCMP_EQUAL("show ssl-inspection profile", ZyxelSecurityCmd::cmdShowSslInspectionProfile().c_str());
    STRCMP_EQUAL("show ssl-inspection statistics", ZyxelSecurityCmd::cmdShowSslInspectionStatistics().c_str());
    STRCMP_EQUAL("ssl-inspection profile qa_ssl", ZyxelSecurityCmd::cmdSslInspectionProfile("qa_ssl").c_str());
    STRCMP_EQUAL("no ssl-inspection profile qa_ssl", ZyxelSecurityCmd::cmdNoSslInspectionProfile("qa_ssl").c_str());

    // Dry-fire helpers
    STRCMP_EQUAL("show bwm 9999", ZyxelSecurityCmd::cmdInvalidBwmDryFire().c_str());
    STRCMP_EQUAL("show app invalid_subcommand_dry_fire", ZyxelSecurityCmd::cmdInvalidAppDryFire().c_str());
    STRCMP_EQUAL("show anti-virus invalid_subcommand_dry_fire", ZyxelSecurityCmd::cmdInvalidAntiVirusDryFire().c_str());
    STRCMP_EQUAL("show ssl-inspection invalid_subcommand_dry_fire", ZyxelSecurityCmd::cmdInvalidSslInspectionDryFire().c_str());
}

TEST(ZyxelSecurityCmdTest, ParseBwmActivation) {
    bool active = false;
    CHECK_TRUE(ZyxelSecurityCmd::parseBwmActivation("bwm activation: yes\n", active));
    CHECK_TRUE(active);

    CHECK_TRUE(ZyxelSecurityCmd::parseBwmActivation("bwm activation: no\n", active));
    CHECK_FALSE(active);

    CHECK_FALSE(ZyxelSecurityCmd::parseBwmActivation("", active));
    CHECK_FALSE(ZyxelSecurityCmd::parseBwmActivation("unknown output\n", active));
}

TEST(ZyxelSecurityCmdTest, ParseBwmControlTcpAck) {
    bool active = false;
    CHECK_TRUE(ZyxelSecurityCmd::parseBwmControlTcpAck("bwm handle tcp ack activation: no\n", active));
    CHECK_FALSE(active);

    CHECK_TRUE(ZyxelSecurityCmd::parseBwmControlTcpAck("bwm handle tcp ack activation: yes\n", active));
    CHECK_TRUE(active);
}

TEST(ZyxelSecurityCmdTest, ParseBwmAll) {
    std::string transcript =
        "index: 1\n"
        "   Activate: yes\n"
        "   Description: mail_2048kbps\n"
        "   BWM Type: shared\n"
        "   Src: mail.example.com\n"
        "   Dst: any\n"
        "   Inbound: 2048\n"
        "   Outbound: 2048\n"
        "index: default\n"
        "   Activate: yes\n"
        "   Description:\n"
        "   BWM Type: shared\n"
        "   Src: any\n"
        "   Dst: any\n"
        "   Inbound: 0\n"
        "   Outbound: 0\n";

    std::vector<ZyxelBwmRule> rules;
    CHECK_TRUE(ZyxelSecurityCmd::parseBwmAll(transcript, rules));
    CHECK_EQUAL(2, rules.size());

    CHECK_EQUAL("1", rules[0].index);
    CHECK_TRUE(rules[0].active);
    CHECK_EQUAL("mail_2048kbps", rules[0].description);
    CHECK_EQUAL("shared", rules[0].bwmType);
    CHECK_EQUAL(2048U, rules[0].inboundKbps);
    CHECK_EQUAL(2048U, rules[0].outboundKbps);

    CHECK_EQUAL("default", rules[1].index);
    CHECK_TRUE(rules[1].active);
    CHECK_EQUAL(0U, rules[1].inboundKbps);

    nlohmann::json j = rules[0].toJson();
    CHECK_EQUAL("1", j["index"].get<std::string>());
    CHECK_TRUE(j["active"].get<bool>());
    CHECK_EQUAL(2048U, j["inbound_kbps"].get<uint32_t>());
}

TEST(ZyxelSecurityCmdTest, ParseCdrStatus) {
    std::string transcript =
        "activate: no\n"
        "blocked-by: ip\n"
        "counter-reset: no\n"
        "email-alert: admin@selfso.com\n"
        "block period: 60\n"
        "block url: message\n";

    ZyxelCdrStatus status;
    CHECK_TRUE(ZyxelSecurityCmd::parseCdrStatus(transcript, status));
    CHECK_FALSE(status.active);
    CHECK_EQUAL("ip", status.blockedBy);
    CHECK_EQUAL(60, status.blockPeriod);
    CHECK_EQUAL("admin@selfso.com", status.emailAlert);

    nlohmann::json j = status.toJson();
    CHECK_FALSE(j["active"].get<bool>());
    CHECK_EQUAL("ip", j["blocked_by"].get<std::string>());
    CHECK_EQUAL(60, j["block_period"].get<int>());
}

TEST(ZyxelSecurityCmdTest, ParseCdrRules) {
    std::string transcript =
        "No.  Category            Occurence    Duration     Containment         Event Type\n"
        "===============================================================================\n"
        "1    Malware             5            60           Alert               Malware detected\n"
        "2    IPS                 2            10           Alert               Vulnerability exploit detected\n"
        "3    Web Threat          3            30           Alert               Connections to malicious web sites detected\n";

    std::vector<ZyxelCdrRule> rules;
    CHECK_TRUE(ZyxelSecurityCmd::parseCdrRules(transcript, rules));
    CHECK_EQUAL(3, rules.size());

    CHECK_EQUAL(1, rules[0].index);
    CHECK_EQUAL("Malware", rules[0].category);
    CHECK_EQUAL(5, rules[0].occurrence);
    CHECK_EQUAL(60, rules[0].duration);
    CHECK_EQUAL("Alert", rules[0].containment);
    CHECK_EQUAL("Malware detected", rules[0].eventType);

    CHECK_EQUAL(2, rules[1].index);
    CHECK_EQUAL("IPS", rules[1].category);

    nlohmann::json j = rules[0].toJson();
    CHECK_EQUAL(1, j["index"].get<int>());
    CHECK_EQUAL("Malware", j["category"].get<std::string>());
}

TEST(ZyxelSecurityCmdTest, ParseSslInspectionStatus) {
    std::string transcript =
        "server sign cert mode: ecdsa-rsa-2048\n"
        "server query timeout action: pass\n"
        "server query timeout sec: 2\n"
        "TLS 1.3 activate: yes\n"
        "TLS 1.2 aesgcm activate: yes\n";

    ZyxelSslInspectionStatus status;
    CHECK_TRUE(ZyxelSecurityCmd::parseSslInspectionStatus(transcript, status));
    CHECK_EQUAL("ecdsa-rsa-2048", status.certMode);
    CHECK_EQUAL("pass", status.timeoutAction);
    CHECK_EQUAL(2, status.timeoutSec);
    CHECK_TRUE(status.tls13Active);

    nlohmann::json j = status.toJson();
    CHECK_EQUAL("ecdsa-rsa-2048", j["cert_mode"].get<std::string>());
    CHECK_TRUE(j["tls13_active"].get<bool>());
    CHECK_EQUAL(2, j["timeout_sec"].get<int>());
}

TEST(ZyxelSecurityCmdTest, ParseContentFilterSettings) {
    std::string transcript =
        "default block  : no\n"
        "license key    : default\n"
        "service timeout: 10\n"
        "block message  : Web access is restricted. Please contact the administrator.\n"
        "block redirect : \n";

    ZyxelContentFilterSettings settings;
    CHECK_TRUE(ZyxelSecurityCmd::parseContentFilterSettings(transcript, settings));
    CHECK_FALSE(settings.defaultBlock);
    CHECK_EQUAL("default", settings.licenseKey);
    CHECK_EQUAL(10, settings.serviceTimeout);
    CHECK_EQUAL("Web access is restricted. Please contact the administrator.", settings.blockMessage);

    nlohmann::json j = settings.toJson();
    CHECK_FALSE(j["default_block"].get<bool>());
    CHECK_EQUAL("default", j["license_key"].get<std::string>());
    CHECK_EQUAL(10, j["service_timeout"].get<int>());
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
