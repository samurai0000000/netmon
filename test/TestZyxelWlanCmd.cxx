/*
 * TestZyxelWlanCmd.cxx
 *
 * Copyright (C) 2026, Charles Chiou
 */

#include "zyxel/ZyxelWlanCmd.hxx"
#include <CppUTest/TestHarness.h>

TEST_GROUP(ZyxelWlanCmdTest) {
    void setup() {}
    void teardown() {}
};

TEST(ZyxelWlanCmdTest, CommandGeneratorsReturnExpectedStrings) {
    // Ch 6: AP Management
    STRCMP_EQUAL("show capwap ap all", ZyxelWlanCmd::cmdShowCapwapApAll().c_str());
    STRCMP_EQUAL("show capwap ap all config status", ZyxelWlanCmd::cmdShowCapwapApConfigStatus().c_str());
    STRCMP_EQUAL("show capwap ap all statistics", ZyxelWlanCmd::cmdShowCapwapApStatistics().c_str());
    STRCMP_EQUAL("show capwap ap fallback", ZyxelWlanCmd::cmdShowCapwapApFallback().c_str());
    STRCMP_EQUAL("show capwap ap fallback interval", ZyxelWlanCmd::cmdShowCapwapApFallbackInterval().c_str());
    STRCMP_EQUAL("show capwap ap idle timeout", ZyxelWlanCmd::cmdShowCapwapApIdleTimeout().c_str());
    STRCMP_EQUAL("show capwap ap wait-list", ZyxelWlanCmd::cmdShowCapwapApWaitList().c_str());
    STRCMP_EQUAL("show capwap manual-add", ZyxelWlanCmd::cmdShowCapwapManualAdd().c_str());
    STRCMP_EQUAL("show capwap station all", ZyxelWlanCmd::cmdShowCapwapStationAll().c_str());
    STRCMP_EQUAL("show country-code list", ZyxelWlanCmd::cmdShowCountryCodeList().c_str());
    STRCMP_EQUAL("show default country-code", ZyxelWlanCmd::cmdShowDefaultCountryCode().c_str());
    STRCMP_EQUAL("show vpn-policy-pool", ZyxelWlanCmd::cmdShowVpnPolicyPool().c_str());

    // Ch 8: AP Group
    STRCMP_EQUAL("show ap-group first-priority", ZyxelWlanCmd::cmdShowApGroupFirstPriority().c_str());
    STRCMP_EQUAL("show ap-group-profile all", ZyxelWlanCmd::cmdShowApGroupProfileAll().c_str());
    STRCMP_EQUAL("show ap-group-profile rule_count", ZyxelWlanCmd::cmdShowApGroupProfileRuleCount().c_str());

    // Ch 9: WLAN Profiles
    STRCMP_EQUAL("show wlan-macfilter-profile all", ZyxelWlanCmd::cmdShowWlanMacfilterProfileAll().c_str());
    STRCMP_EQUAL("show wlan-monitor-profile all", ZyxelWlanCmd::cmdShowWlanMonitorProfileAll().c_str());
    STRCMP_EQUAL("show wlan-radio-profile all", ZyxelWlanCmd::cmdShowWlanRadioProfileAll().c_str());
    STRCMP_EQUAL("show wlan-security-profile all", ZyxelWlanCmd::cmdShowWlanSecurityProfileAll().c_str());
    STRCMP_EQUAL("show wlan-ssid-profile all", ZyxelWlanCmd::cmdShowWlanSsidProfileAll().c_str());
    STRCMP_EQUAL("show zymesh ap info", ZyxelWlanCmd::cmdShowZymeshApInfo().c_str());
    STRCMP_EQUAL("show zymesh provision-group", ZyxelWlanCmd::cmdShowZymeshProvisionGroup().c_str());
    STRCMP_EQUAL("show zymesh-profile all", ZyxelWlanCmd::cmdShowZymeshProfileAll().c_str());

    // Ch 10: Rogue AP
    STRCMP_EQUAL("show rogue-ap containment config", ZyxelWlanCmd::cmdShowRogueApContainmentConfig().c_str());
    STRCMP_EQUAL("show rogue-ap containment list", ZyxelWlanCmd::cmdShowRogueApContainmentList().c_str());
    STRCMP_EQUAL("show rogue-ap detection info", ZyxelWlanCmd::cmdShowRogueApDetectionInfo().c_str());
    STRCMP_EQUAL("show rogue-ap detection list all", ZyxelWlanCmd::cmdShowRogueApDetectionListAll().c_str());
    STRCMP_EQUAL("show rogue-ap detection monitoring", ZyxelWlanCmd::cmdShowRogueApDetectionMonitoring().c_str());
    STRCMP_EQUAL("show rogue-ap detection status", ZyxelWlanCmd::cmdShowRogueApDetectionStatus().c_str());

    // Ch 11: Wireless Health
    STRCMP_EQUAL("show wireless-health-action", ZyxelWlanCmd::cmdShowWirelessHealthAction().c_str());
    STRCMP_EQUAL("show ap-info top 10 alert all", ZyxelWlanCmd::cmdShowApInfoTopAlert().c_str());
    STRCMP_EQUAL("show sta-info top 10 alert 2.4G", ZyxelWlanCmd::cmdShowStaInfoTopAlert("2.4G").c_str());

    // Ch 12: Frame Capture
    STRCMP_EQUAL("show frame-capture config", ZyxelWlanCmd::cmdShowFrameCaptureConfig().c_str());
    STRCMP_EQUAL("show frame-capture status", ZyxelWlanCmd::cmdShowFrameCaptureStatus().c_str());

    // Ch 14: Auto-Healing
    STRCMP_EQUAL("show auto-healing config", ZyxelWlanCmd::cmdShowAutoHealingConfig().c_str());

    // Ch 73: Managed AP
    STRCMP_EQUAL("show capwap ap ac-ip", ZyxelWlanCmd::cmdShowCapwapApAcIp().c_str());
    STRCMP_EQUAL("show capwap ap discovery-type", ZyxelWlanCmd::cmdShowCapwapApDiscoveryType().c_str());
    STRCMP_EQUAL("show capwap ap info", ZyxelWlanCmd::cmdShowCapwapApInfo().c_str());
}

TEST(ZyxelWlanCmdTest, Tier2MutationCommandsFormattedCorrectly) {
    STRCMP_EQUAL("wlan-ssid-profile NETMON_QA_SSID",
                 ZyxelWlanCmd::cmdWlanSsidProfile("NETMON_QA_SSID").c_str());
    STRCMP_EQUAL("no wlan-ssid-profile NETMON_QA_SSID",
                 ZyxelWlanCmd::cmdNoWlanSsidProfile("NETMON_QA_SSID").c_str());

    STRCMP_EQUAL("wlan-security-profile NETMON_QA_SEC",
                 ZyxelWlanCmd::cmdWlanSecurityProfile("NETMON_QA_SEC").c_str());
    STRCMP_EQUAL("no wlan-security-profile NETMON_QA_SEC",
                 ZyxelWlanCmd::cmdNoWlanSecurityProfile("NETMON_QA_SEC").c_str());
}

TEST(ZyxelWlanCmdTest, ParseCapwapApInfoSucceedsOnSampleOutput) {
    std::string sample =
        "Online mgnt ap: 2\r\n"
        "Offline mgnt ap: 1\r\n"
        "Un-mgnt ap: 0\r\n"
        "Station : 15\r\n"
        "Remote AP : 0\r\n"
        "Update available : Yes\r\n";

    ZyxelCapwapApInfo info;
    bool ok = ZyxelWlanCmd::parseCapwapApInfo(sample, info);
    CHECK_TRUE(ok);
    LONGS_EQUAL(2, info.onlineMgntAp);
    LONGS_EQUAL(1, info.offlineMgntAp);
    LONGS_EQUAL(0, info.unMgntAp);
    LONGS_EQUAL(15, info.stationCount);
    LONGS_EQUAL(0, info.remoteApCount);
    CHECK_TRUE(info.updateAvailable);

    nlohmann::json j = info.toJson();
    LONGS_EQUAL(2, j["online_mgnt_ap"].get<int>());
    CHECK_TRUE(j["update_available"].get<bool>());
}

TEST(ZyxelWlanCmdTest, ParseCapwapApInfoFailsOnGarbage) {
    ZyxelCapwapApInfo info;
    bool ok = ZyxelWlanCmd::parseCapwapApInfo("random corrupted text % parse error", info);
    CHECK_FALSE(ok);
}

TEST(ZyxelWlanCmdTest, ParseCapwapFallbackSucceedsOnSampleOutput) {
    std::string sample =
        "Fallback: enable\r\n"
        "Fallback Interval: 45\r\n"
        "Idle timeout: capwap default\r\n";

    ZyxelCapwapFallback fallback;
    bool ok = ZyxelWlanCmd::parseCapwapFallback(sample, fallback);
    CHECK_TRUE(ok);
    CHECK_TRUE(fallback.fallbackEnabled);
    LONGS_EQUAL(45, fallback.fallbackInterval);
    STRCMP_EQUAL("capwap default", fallback.idleTimeout.c_str());

    nlohmann::json j = fallback.toJson();
    CHECK_TRUE(j["fallback_enabled"].get<bool>());
    LONGS_EQUAL(45, j["fallback_interval"].get<int>());
}

TEST(ZyxelWlanCmdTest, ParseCapwapFallbackFailsOnGarbage) {
    ZyxelCapwapFallback fallback;
    bool ok = ZyxelWlanCmd::parseCapwapFallback("% invalid command syntax", fallback);
    CHECK_FALSE(ok);
}

TEST(ZyxelWlanCmdTest, ParseRogueApInfoSucceedsOnSampleOutput) {
    std::string sample =
        "rogue ap: 1\r\n"
        "friendly ap: 3\r\n"
        "suspected rogue ap: 0\r\n"
        "adhoc: 0\r\n"
        "unclassified ap: 4\r\n"
        "total devices: 8\r\n";

    ZyxelRogueApInfo info;
    bool ok = ZyxelWlanCmd::parseRogueApInfo(sample, info);
    CHECK_TRUE(ok);
    LONGS_EQUAL(1, info.rogueApCount);
    LONGS_EQUAL(3, info.friendlyApCount);
    LONGS_EQUAL(0, info.suspectedRogueApCount);
    LONGS_EQUAL(0, info.adhocCount);
    LONGS_EQUAL(4, info.unclassifiedApCount);
    LONGS_EQUAL(8, info.totalDevices);

    nlohmann::json j = info.toJson();
    LONGS_EQUAL(1, j["rogue_ap_count"].get<int>());
    LONGS_EQUAL(8, j["total_devices"].get<int>());
}

TEST(ZyxelWlanCmdTest, ParseRogueApInfoFailsOnGarbage) {
    ZyxelRogueApInfo info;
    bool ok = ZyxelWlanCmd::parseRogueApInfo("Corrupted table buffer", info);
    CHECK_FALSE(ok);
}

TEST(ZyxelWlanCmdTest, ParseRogueApStatusSucceedsOnSampleOutput) {
    std::string sample =
        "rogue-ap detection status: on\r\n"
        "ap-mode detection interval: 30\r\n"
        "rogue-rule weak-security: yes\r\n"
        "rogue-rule unmanaged-ap: yes\r\n"
        "rogue-rule hidden-ssid: no\r\n"
        "rogue-rule ssid-keyword: yes\r\n";

    ZyxelRogueApStatus status;
    bool ok = ZyxelWlanCmd::parseRogueApStatus(sample, status);
    CHECK_TRUE(ok);
    CHECK_TRUE(status.detectionStatus);
    LONGS_EQUAL(30, status.detectionInterval);
    CHECK_TRUE(status.weakSecurity);
    CHECK_TRUE(status.unmanagedAp);
    CHECK_FALSE(status.hiddenSsid);
    CHECK_TRUE(status.ssidKeyword);

    nlohmann::json j = status.toJson();
    CHECK_TRUE(j["detection_status"].get<bool>());
    CHECK_FALSE(j["hidden_ssid"].get<bool>());
}

TEST(ZyxelWlanCmdTest, ParseRogueApStatusFailsOnGarbage) {
    ZyxelRogueApStatus status;
    bool ok = ZyxelWlanCmd::parseRogueApStatus("unknown syntax error", status);
    CHECK_FALSE(ok);
}

TEST(ZyxelWlanCmdTest, ParseAutoHealingConfigSucceedsOnSampleOutput) {
    std::string sample =
        "auto-healing activate: yes\r\n"
        "auto-healing interval: 15\r\n"
        "auto-healing power threshold: -75 dBm\r\n"
        "auto-healing healing threshold: -80 dBm\r\n"
        "auto-healing margin: 3\r\n";

    ZyxelAutoHealingConfig config;
    bool ok = ZyxelWlanCmd::parseAutoHealingConfig(sample, config);
    CHECK_TRUE(ok);
    CHECK_TRUE(config.activated);
    LONGS_EQUAL(15, config.interval);
    LONGS_EQUAL(-75, config.powerThresholdDbm);
    LONGS_EQUAL(-80, config.healingThresholdDbm);
    LONGS_EQUAL(3, config.margin);

    nlohmann::json j = config.toJson();
    CHECK_TRUE(j["activated"].get<bool>());
    LONGS_EQUAL(-75, j["power_threshold_dbm"].get<int>());
}

TEST(ZyxelWlanCmdTest, ParseAutoHealingConfigFailsOnGarbage) {
    ZyxelAutoHealingConfig config;
    bool ok = ZyxelWlanCmd::parseAutoHealingConfig("no auto healing available", config);
    CHECK_FALSE(ok);
}

TEST(ZyxelWlanCmdTest, ParseFrameCaptureConfigSucceedsOnSampleOutput) {
    std::string sample =
        "capture source: local-ap\r\n"
        "file prefix: capture_trace\r\n"
        "file size: 2048\r\n";

    ZyxelFrameCaptureConfig config;
    bool ok = ZyxelWlanCmd::parseFrameCaptureConfig(sample, config);
    CHECK_TRUE(ok);
    STRCMP_EQUAL("local-ap", config.captureSource.c_str());
    STRCMP_EQUAL("capture_trace", config.filePrefix.c_str());
    LONGS_EQUAL(2048, config.fileSize);

    nlohmann::json j = config.toJson();
    STRCMP_EQUAL("local-ap", j["capture_source"].get<std::string>().c_str());
    LONGS_EQUAL(2048, j["file_size"].get<int>());
}

TEST(ZyxelWlanCmdTest, ParseFrameCaptureConfigFailsOnGarbage) {
    ZyxelFrameCaptureConfig config;
    bool ok = ZyxelWlanCmd::parseFrameCaptureConfig("some parse error", config);
    CHECK_FALSE(ok);
}

TEST(ZyxelWlanCmdTest, ParseZyMeshInfoSucceedsOnSampleOutput) {
    std::string sample =
        "Online Root AP: 2\r\n"
        "Online Repeater AP: 1\r\n"
        "Offline Root AP: 0\r\n"
        "Offline Repeater AP: 0\r\n"
        "Provision Group MAC: 02:00:00:00:00:01\r\n";

    ZyxelZyMeshInfo info;
    bool ok = ZyxelWlanCmd::parseZyMeshInfo(sample, info);
    CHECK_TRUE(ok);
    LONGS_EQUAL(2, info.onlineRootAp);
    LONGS_EQUAL(1, info.onlineRepeaterAp);
    LONGS_EQUAL(0, info.offlineRootAp);
    LONGS_EQUAL(0, info.offlineRepeaterAp);
    STRCMP_EQUAL("02:00:00:00:00:01", info.provisionGroupMac.c_str());

    nlohmann::json j = info.toJson();
    LONGS_EQUAL(2, j["online_root_ap"].get<int>());
    STRCMP_EQUAL("02:00:00:00:00:01", j["provision_group_mac"].get<std::string>().c_str());
}

TEST(ZyxelWlanCmdTest, ParseZyMeshInfoFailsOnGarbage) {
    ZyxelZyMeshInfo info;
    bool ok = ZyxelWlanCmd::parseZyMeshInfo("zymesh error % not ready", info);
    CHECK_FALSE(ok);
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
