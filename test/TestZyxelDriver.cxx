/*
 * TestZyxelDriver.cxx
 *
 * Copyright (C) 2026, Charles Chiou
 */

#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <filesystem>
#include <cstdlib>

#include "ZyxelDriver.hxx"
#include "AuthManager.hxx"
#include "Config.hxx"

#include <CppUTest/TestHarness.h>

namespace fs = std::filesystem;

static void checkNoWriteLine(const std::vector<std::string> &log) {
    for (const auto &line : log) {
        CHECK(line.rfind("write", 0) != 0);
    }
}

TEST_GROUP(ZyxelDriverTest) {
    std::string testDir = "test_data/test_zyxel_driver";

    void setup() {
        ZyxelDriver::getInstance().resetForTesting();
        AuthManager::getInstance().resetForTesting();
        Config::getInstance().resetForTesting();
        if (fs::exists(testDir)) {
            fs::remove_all(testDir);
        }
        fs::create_directories(testDir);
    }

    void teardown() {
        ZyxelDriver::getInstance().resetForTesting();
        AuthManager::getInstance().resetForTesting();
        Config::getInstance().resetForTesting();
        if (fs::exists(testDir)) {
            fs::remove_all(testDir);
        }
    }
};

TEST(ZyxelDriverTest, InitialStatusIsUnconfigured) {
    auto status = ZyxelDriver::getInstance().getStatus();
    STRCMP_EQUAL("unconfigured", status.value("status", "").c_str());
    CHECK_FALSE(ZyxelDriver::getInstance().isConfigured());
    CHECK_FALSE(ZyxelDriver::getInstance().isConnected());
}

TEST(ZyxelDriverTest, RejectsInvalidIpAddress) {
    auto res1 = ZyxelDriver::getInstance().blockIp("invalid-ip", "Test");
    STRCMP_EQUAL("error", res1.value("status", "").c_str());

    auto res2 = ZyxelDriver::getInstance().blockIp("300.1.2.3", "Test");
    STRCMP_EQUAL("error", res2.value("status", "").c_str());

    auto res3 = ZyxelDriver::getInstance().unblockIp("192.168.1");
    STRCMP_EQUAL("error", res3.value("status", "").c_str());

    auto log = ZyxelDriver::getInstance().getDryRunLog();
    LONGS_EQUAL(0, log.size());
}

TEST(ZyxelDriverTest, GetStatusConfiguredOfflineReportsOffline) {
    ZyxelDriver::getInstance().setLiveEnabled(true);
    ZyxelDriver::getInstance().configure("127.0.0.1", 2222, "admin", "test_data/test_pin.pin");
    auto status = ZyxelDriver::getInstance().getStatus();
    STRCMP_EQUAL("offline", status.value("status", "").c_str());
    CHECK_TRUE(status.contains("error"));
    CHECK_FALSE(status.contains("flash_write"));
}

TEST(ZyxelDriverTest, GetSessionsConfiguredOfflineReturnsEmptySessionsArray) {
    ZyxelDriver::getInstance().setLiveEnabled(true);
    ZyxelDriver::getInstance().configure("127.0.0.1", 2222, "admin", "test_data/test_pin.pin");
    auto sessions = ZyxelDriver::getInstance().getSessions();
    STRCMP_EQUAL("offline", sessions.value("status", "").c_str());
    CHECK_TRUE(sessions["sessions"].is_array());
    LONGS_EQUAL(0, sessions["sessions"].size());
}

TEST(ZyxelDriverTest, ClearAuthFailureResetsState) {
    ZyxelDriver::getInstance().setLiveEnabled(true);
    ZyxelDriver::getInstance().configure("127.0.0.1", 2222, "admin", "test_data/test_pin.pin");
    ZyxelDriver::getInstance().clearAuthFailure();
    auto status = ZyxelDriver::getInstance().getStatus();
    STRCMP_EQUAL("offline", status.value("status", "").c_str());
}

TEST(ZyxelDriverTest, SetRouterPasswordClearsAuthFailureBackoff) {
    AuthManager::getInstance().resetForTesting();
    std::string vaultDir = "test_data/test_zyxel_driver_vault";
    if (fs::exists(vaultDir)) {
        fs::remove_all(vaultDir);
    }
    fs::create_directories(vaultDir);
    AuthManager::getInstance().setVaultDir(vaultDir);

    CHECK_TRUE(AuthManager::getInstance().setRouterPassword("FreshTestPassword123!"));

    std::string retrieved;
    CHECK_TRUE(AuthManager::getInstance().getRouterPassword(retrieved));
    STRCMP_EQUAL("FreshTestPassword123!", retrieved.c_str());

    AuthManager::getInstance().resetForTesting();
    if (fs::exists(vaultDir)) {
        fs::remove_all(vaultDir);
    }
}

TEST(ZyxelDriverTest, ResetForTestingDoesNotCreateOrTouchFiles) {
    const char *homeEnv = getenv("HOME");
    CHECK_TRUE(homeEnv != nullptr);
    std::string home(homeEnv);
    std::string defaultJournal = home + "/.config/netmon/router_journal.json";
    std::string defaultPin = home + "/.config/netmon/router_hostkey.pin";

    ZyxelDriver::getInstance().resetForTesting();

    CHECK_FALSE(fs::exists(defaultJournal));
    CHECK_FALSE(fs::exists(defaultPin));
}

TEST(ZyxelDriverTest, RouterLiveEnabledDefaultsToFalseAndNeverConnects) {
    CHECK_FALSE(Config::getInstance().getRouterLiveEnabled());
    CHECK_FALSE(ZyxelDriver::getInstance().isLiveEnabled());

    ZyxelDriver::getInstance().configure("192.0.2.1", 22, "admin");

    auto status = ZyxelDriver::getInstance().getStatus();
    STRCMP_EQUAL("disabled", status.value("status", "").c_str());
    CHECK_FALSE(status.value("live_enabled", true));
    CHECK_FALSE(status.contains("flash_write"));
    CHECK_FALSE(ZyxelDriver::getInstance().isConnected());

    auto sessions = ZyxelDriver::getInstance().getSessions();
    STRCMP_EQUAL("disabled", sessions.value("status", "").c_str());
    CHECK_FALSE(ZyxelDriver::getInstance().isConnected());

    auto blockRes = ZyxelDriver::getInstance().blockIp("192.0.2.101", "Live Disabled Test");
    STRCMP_EQUAL("disabled", blockRes.value("status", "").c_str());
    CHECK_FALSE(blockRes.value("live_enabled", true));
    CHECK_FALSE(ZyxelDriver::getInstance().isConnected());

    auto unblockRes = ZyxelDriver::getInstance().unblockIp("192.0.2.101");
    STRCMP_EQUAL("disabled", unblockRes.value("status", "").c_str());
    CHECK_FALSE(unblockRes.value("live_enabled", true));
    CHECK_FALSE(ZyxelDriver::getInstance().isConnected());
}

TEST(ZyxelDriverTest, RouterDryRunLogsExactZyShBlockSequence) {
    ZyxelDriver::getInstance().setDryRun(true);
    CHECK_TRUE(ZyxelDriver::getInstance().isDryRun());

    auto res = ZyxelDriver::getInstance().blockIp("192.0.2.55", "Bandwidth Flood");
    STRCMP_EQUAL("dry_run", res.value("status", "").c_str());
    CHECK_TRUE(res.value("dry_run", false));
    STRCMP_EQUAL("NETMON_BLK_192_0_2_55", res.value("rule", "").c_str());
    CHECK_FALSE(ZyxelDriver::getInstance().isConnected());

    auto log = ZyxelDriver::getInstance().getDryRunLog();
    LONGS_EQUAL(13, log.size());
    STRCMP_EQUAL("show secure-policy", log[0].c_str());
    STRCMP_EQUAL("configure terminal", log[1].c_str());
    STRCMP_EQUAL("address-object NETMON_BLK_192_0_2_55 192.0.2.55", log[2].c_str());
    STRCMP_EQUAL("secure-policy insert 1", log[3].c_str());
    STRCMP_EQUAL("no activate", log[4].c_str());
    STRCMP_EQUAL("name NETMON_BLK_192_0_2_55", log[5].c_str());
    STRCMP_EQUAL("sourceip NETMON_BLK_192_0_2_55", log[6].c_str());
    STRCMP_EQUAL("action deny", log[7].c_str());
    STRCMP_EQUAL("description Bandwidth Flood", log[8].c_str());
    STRCMP_EQUAL("activate", log[9].c_str());
    STRCMP_EQUAL("exit", log[10].c_str());
    STRCMP_EQUAL("exit", log[11].c_str());
    STRCMP_EQUAL("show secure-policy", log[12].c_str());
    checkNoWriteLine(log);
}

TEST(ZyxelDriverTest, RouterDryRunLogsExactZyShUnblockSequence) {
    ZyxelDriver::getInstance().setDryRun(true);
    CHECK_TRUE(ZyxelDriver::getInstance().isDryRun());

    auto res = ZyxelDriver::getInstance().unblockIp("192.0.2.66");
    STRCMP_EQUAL("dry_run", res.value("status", "").c_str());
    CHECK_TRUE(res.value("dry_run", false));
    CHECK_FALSE(ZyxelDriver::getInstance().isConnected());

    auto log = ZyxelDriver::getInstance().getDryRunLog();
    LONGS_EQUAL(1, log.size());
    STRCMP_EQUAL("show secure-policy", log[0].c_str());
    checkNoWriteLine(log);
}

TEST(ZyxelDriverTest, DriverNeverTransmitsWrite) {
    auto &driver = ZyxelDriver::getInstance();
    driver.setDryRun(true);
    driver.blockIp("192.0.2.55", "Bandwidth Flood");
    driver.unblockIp("192.0.2.55");
    driver.start();
    driver.stop();

    auto log = driver.getDryRunLog();
    CHECK(log.size() > 0);
    checkNoWriteLine(log);
}

TEST(ZyxelDriverTest, ReconnectDoesNotReplay) {
    const char *oldHome = getenv("HOME");
    std::string savedHome = oldHome ? oldHome : "";
    setenv("HOME", testDir.c_str(), 1);

    std::string cfgDir = testDir + "/.config/netmon";
    fs::create_directories(cfgDir);
    std::string journal = cfgDir + "/router_journal.json";
    std::string payload =
        "{\"dirty\":true,\"pending_mutations\":["
        "{\"op\":\"block\",\"ip\":\"192.0.2.77\",\"state\":\"pending\"}]}\n";
    {
        std::ofstream ofs(journal);
        ofs << payload;
    }

    auto &driver = ZyxelDriver::getInstance();
    driver.setDryRun(true);
    driver.setLiveEnabled(true);
    driver.configure("127.0.0.1", 1, "admin", "");
    driver.clearDryRunLog();
    driver.start();
    auto log = driver.getDryRunLog();
    LONGS_EQUAL(0, log.size());
    driver.stop();

    std::ifstream ifs(journal);
    std::stringstream ss;
    ss << ifs.rdbuf();
    STRCMP_EQUAL(payload.c_str(), ss.str().c_str());

    if (savedHome.empty()) {
        unsetenv("HOME");
    } else {
        setenv("HOME", savedHome.c_str(), 1);
    }
}

TEST(ZyxelDriverTest, EnvironmentVariablesCannotEnableLiveOrFlashWrite) {
    setenv("NETMON_ROUTER_LIVE_ENABLED", "1", 1);
    setenv("NETMON_ROUTER_FLASH_WRITE", "1", 1);

    std::string cfgPath = testDir + "/with_flash.cfg";
    {
        std::ofstream ofs(cfgPath);
        ofs << "interface = \"br0\";\n";
        ofs << "router_flash_write = true;\n";
    }

    CHECK_TRUE(Config::getInstance().load(cfgPath));
    CHECK_FALSE(Config::getInstance().getRouterLiveEnabled());
    CHECK_FALSE(ZyxelDriver::getInstance().isLiveEnabled());
    CHECK_TRUE(Config::getInstance().save());

    std::ifstream ifs(cfgPath);
    std::stringstream ss;
    ss << ifs.rdbuf();
    CHECK(ss.str().find("router_flash_write") == std::string::npos);

    unsetenv("NETMON_ROUTER_LIVE_ENABLED");
    unsetenv("NETMON_ROUTER_FLASH_WRITE");
}

TEST(ZyxelDriverTest, PingAndTracerouteRejectInvalidHost) {
    auto res1 = ZyxelDriver::getInstance().ping("1.1.1.1; rm -rf /", 4);
    STRCMP_EQUAL("error", res1["status"].get<std::string>().c_str());

    auto res2 = ZyxelDriver::getInstance().traceroute("`whoami`.com");
    STRCMP_EQUAL("error", res2["status"].get<std::string>().c_str());

    auto res3 = ZyxelDriver::getInstance().ping("", 4);
    STRCMP_EQUAL("error", res3["status"].get<std::string>().c_str());
}

TEST(ZyxelDriverTest, PingAndTracerouteDryRunLogsCommands) {
    ZyxelDriver::getInstance().setDryRun(true);
    ZyxelDriver::getInstance().clearDryRunLog();

    auto resPing = ZyxelDriver::getInstance().ping("1.1.1.1", 3);
    STRCMP_EQUAL("ok", resPing["status"].get<std::string>().c_str());
    STRCMP_EQUAL("ping", resPing["type"].get<std::string>().c_str());
    LONGS_EQUAL(3, resPing["packets_transmitted"].get<int>());

    auto resTrace = ZyxelDriver::getInstance().traceroute("8.8.8.8");
    STRCMP_EQUAL("ok", resTrace["status"].get<std::string>().c_str());
    STRCMP_EQUAL("traceroute", resTrace["type"].get<std::string>().c_str());

    auto dryLog = ZyxelDriver::getInstance().getDryRunLog();
    LONGS_EQUAL(2, dryLog.size());
    STRCMP_EQUAL("ping 1.1.1.1 count 3", dryLog[0].c_str());
    STRCMP_EQUAL("traceroute 8.8.8.8", dryLog[1].c_str());
}

TEST(ZyxelDriverTest, PingAndTracerouteWhenDisabledReturnsDisabled) {
    ZyxelDriver::getInstance().configure("127.0.0.1", 22, "admin", "");
    ZyxelDriver::getInstance().setDryRun(false);
    ZyxelDriver::getInstance().setLiveEnabled(false);

    auto res1 = ZyxelDriver::getInstance().ping("1.1.1.1", 2);
    STRCMP_EQUAL("disabled", res1["status"].get<std::string>().c_str());

    auto res2 = ZyxelDriver::getInstance().traceroute("1.1.1.1");
    STRCMP_EQUAL("disabled", res2["status"].get<std::string>().c_str());
}

TEST(ZyxelDriverTest, DiagnosticLockLifecycleAndExclusiveAccess) {
    auto &driver = ZyxelDriver::getInstance();
    CHECK_FALSE(driver.isDiagnosticActive());

    CHECK_TRUE(driver.acquireDiagnosticLock());
    CHECK_TRUE(driver.isDiagnosticActive());

    CHECK_FALSE(driver.acquireDiagnosticLock());

    driver.releaseDiagnosticLock();
    CHECK_FALSE(driver.isDiagnosticActive());
}

TEST(ZyxelDriverTest, DiagnosticGuardRaiiManagesLock) {
    auto &driver = ZyxelDriver::getInstance();
    CHECK_FALSE(driver.isDiagnosticActive());

    {
        ZyxelDiagnosticGuard guard(driver);
        CHECK_TRUE(guard.isAcquired());
        CHECK_TRUE(driver.isDiagnosticActive());

        ZyxelDiagnosticGuard innerGuard(driver);
        CHECK_FALSE(innerGuard.isAcquired());
    }

    CHECK_FALSE(driver.isDiagnosticActive());
}

TEST(ZyxelDriverTest, ExecuteClearanceCommandReturnsBusyDuringDiagnostic) {
    auto &driver = ZyxelDriver::getInstance();
    CHECK_TRUE(driver.acquireDiagnosticLock());

    std::string out;
    std::string matchedPrompt;
    SshResult res = driver.executeClearanceCommand("show version", out, matchedPrompt, 1000);

    LONGS_EQUAL(static_cast<int>(SshResult::ERR_BUSY), static_cast<int>(res));
    STRCMP_EQUAL("Firewall diagnostic in progress; client requests temporarily suspended", out.c_str());
    CHECK(matchedPrompt.length() > 0);

    driver.releaseDiagnosticLock();
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
