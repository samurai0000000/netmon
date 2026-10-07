/*
 * TestForwardBlock.cxx
 *
 * Copyright (C) 2026, Charles Chiou
 */

#include <string>
#include <vector>
#include <filesystem>

#include "ZyxelDriver.hxx"
#include "ZyxelSshClient.hxx"
#include "ZyshSimulator.hxx"
#include "AuthManager.hxx"
#include "Config.hxx"
#include <CppUTest/TestHarness.h>

namespace fs = std::filesystem;

static const char *kPinHex =
    "00112233445566778899aabbccddeeff00112233445566778899aabbccddeeff";

static bool sentExact(const ZyshSimulator &sim, const std::string &line) {
    for (const auto &got : sim.lines()) {
        if (got == line) {
            return true;
        }
    }
    return false;
}

static int countExact(const ZyshSimulator &sim, const std::string &line) {
    int n = 0;
    for (const auto &got : sim.lines()) {
        if (got == line) {
            n++;
        }
    }
    return n;
}

TEST_GROUP(ForwardBlock) {
    std::string pinPath = "test_data/ssh_e4/router_hostkey.pin";
    std::string vaultDir = "test_data/ssh_e4/vault";
    ZyshSimulator *sim = nullptr;

    void setup() {
        Config::getInstance().resetForTesting();
        ZyxelDriver::getInstance().resetForTesting();
        AuthManager::getInstance().resetForTesting();
        fs::create_directories(vaultDir);
        fs::remove(pinPath);
        AuthManager::getInstance().setVaultDir(vaultDir);
        arm();
    }

    void teardown() {
        ZyxelDriver::getInstance().resetForTesting();
        delete sim;
        sim = nullptr;
        AuthManager::getInstance().resetForTesting();
        Config::getInstance().resetForTesting();
        fs::remove_all("test_data/ssh_e4");
    }

    void arm() {
        ZyxelDriver::getInstance().resetForTesting();
        delete sim;
        sim = new ZyshSimulator();
        sim->setHostKey(kPinHex);
        fs::remove(pinPath);
        CHECK_EQUAL(static_cast<int>(SshResult::SUCCESS),
                    static_cast<int>(ZyxelSshClient::writeHostKeyPin(pinPath, kPinHex)));
        CHECK_TRUE(AuthManager::getInstance().setRouterPassword("router-test-pass"));
        ZyxelDriver &driver = ZyxelDriver::getInstance();
        driver.setLiveEnabled(true);
        driver.configure("192.0.2.1", 22, "admin", pinPath);
        driver.getSshClient().setTransport(sim->transport());
    }
};

TEST(ForwardBlock, AlreadyExistsStopsWithNoFurtherLines) {
    sim->seedAddress("NETMON_BLK_192_0_2_55");
    auto res = ZyxelDriver::getInstance().blockIp("192.0.2.55", "Bandwidth Flood");
    STRCMP_EQUAL("error", res.value("status", "").c_str());
    STRCMP_EQUAL("address-object", res.value("step", "").c_str());
    CHECK_TRUE(sentExact(*sim, "address-object NETMON_BLK_192_0_2_55 192.0.2.55"));
    CHECK_FALSE(sentExact(*sim, "secure-policy insert 1"));
    CHECK_FALSE(sentExact(*sim, "no secure-policy 1"));
    CHECK_FALSE(sentExact(*sim, "no address-object NETMON_BLK_192_0_2_55"));
}

TEST(ForwardBlock, FaultAtStepSendsNoFurtherLines) {
    const char *steps[] = {
        "show secure-policy",
        "configure terminal",
        "address-object NETMON_BLK_192_0_2_55 192.0.2.55",
        "secure-policy insert 1",
        "no activate",
        "name NETMON_BLK_192_0_2_55",
        "sourceip NETMON_BLK_192_0_2_55",
        "action deny",
        "description Bandwidth Flood",
        "activate",
        "exit",
        "exit",
        "show secure-policy"
    };
    const int occurrences[] = {1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 2, 2};
    for (int i = 0; i < 13; ++i) {
        arm();
        sim->failOnOccurrence(steps[i], occurrences[i], "% Syntax error");
        auto res = ZyxelDriver::getInstance().blockIp("192.0.2.55", "Bandwidth Flood");
        CHECK_TRUE(std::string(res.value("status", "")) != "success");
        CHECK_EQUAL(occurrences[i], countExact(*sim, steps[i]));
        CHECK_FALSE(sentExact(*sim, "no secure-policy 1"));
        CHECK_FALSE(sentExact(*sim, "no address-object NETMON_BLK_192_0_2_55"));
        for (int j = i + 1; j < 13; ++j) {
            if (std::string(steps[j]) == steps[i]) {
                continue;
            }
            bool earlier = false;
            for (int h = 0; h <= i; ++h) {
                if (std::string(steps[h]) == steps[j]) {
                    earlier = true;
                }
            }
            if (!earlier) {
                CHECK_EQUAL(0, countExact(*sim, steps[j]));
            }
        }
    }
}

TEST(ForwardBlock, FaultBeforeNoActivateDisconnectsWithoutExit) {
    sim->failOnOccurrence("secure-policy insert 1", 1, "% Syntax error");
    auto res = ZyxelDriver::getInstance().blockIp("192.0.2.55", "Bandwidth Flood");
    STRCMP_EQUAL("error", res.value("status", "").c_str());
    CHECK_FALSE(sentExact(*sim, "exit"));
    CHECK_FALSE(sentExact(*sim, "no activate"));
    CHECK_FALSE(ZyxelDriver::getInstance().isConnected());
}

TEST(ForwardBlock, ReasonInjectionBlocked) {
    auto injected = ZyxelDriver::getInstance().blockIp("192.0.2.55", "x\nwrite");
    CHECK_TRUE(std::string(injected.value("status", "")) == "success" ||
               std::string(injected.value("status", "")) == "error");
    CHECK_TRUE(sentExact(*sim, "description xwrite"));
    CHECK_FALSE(sentExact(*sim, "write"));
    CHECK_FALSE(sentExact(*sim, "description x\nwrite"));

    arm();
    ZyxelDriver::getInstance().blockIp("192.0.2.56", "x?");
    CHECK_TRUE(sentExact(*sim, "description x"));
    CHECK_FALSE(sentExact(*sim, "description x?"));

    arm();
    std::string longReason(200, 'A');
    ZyxelDriver::getInstance().blockIp("192.0.2.57", longReason);
    CHECK_TRUE(sentExact(*sim, "description " + std::string(63, 'A')));
}

TEST(ForwardBlock, UnblockStopsWhenMatchIsAmbiguous) {
    auto none = ZyxelDriver::getInstance().unblockIp("192.0.2.55");
    STRCMP_EQUAL("error", none.value("status", "").c_str());
    STRCMP_EQUAL("match", none.value("step", "").c_str());
    CHECK_FALSE(sentExact(*sim, "configure terminal"));
    CHECK_FALSE(sentExact(*sim, "no secure-policy 1"));

    arm();
    sim->seedRule("NETMON_BLK_192_0_2_55", "NETMON_BLK_192_0_2_55", "deny", true);
    sim->seedRule("NETMON_BLK_192_0_2_55", "NETMON_BLK_192_0_2_55", "deny", true);
    auto many = ZyxelDriver::getInstance().unblockIp("192.0.2.55");
    STRCMP_EQUAL("error", many.value("status", "").c_str());
    CHECK_FALSE(sentExact(*sim, "configure terminal"));
    CHECK_FALSE(sentExact(*sim, "no secure-policy 1"));
    CHECK_FALSE(sentExact(*sim, "no address-object NETMON_BLK_192_0_2_55"));
}

TEST(ForwardBlock, UnblockStopsWhenRuleDeleteFails) {
    sim->seedRule("NETMON_BLK_192_0_2_55", "NETMON_BLK_192_0_2_55", "deny", true);
    sim->seedAddress("NETMON_BLK_192_0_2_55");
    sim->failOnOccurrence("no secure-policy 1", 1, "% Syntax error");
    auto res = ZyxelDriver::getInstance().unblockIp("192.0.2.55");
    STRCMP_EQUAL("error", res.value("status", "").c_str());
    STRCMP_EQUAL("delete-rule", res.value("step", "").c_str());
    CHECK_TRUE(sentExact(*sim, "no secure-policy 1"));
    CHECK_FALSE(sentExact(*sim, "no address-object NETMON_BLK_192_0_2_55"));
}

TEST(ForwardBlock, TicketPathReturnsRouterOutput) {
    auto res = ZyxelDriver::getInstance().blockIp("192.0.2.55", "Bandwidth Flood");
    STRCMP_EQUAL("success", res.value("status", "").c_str());
    CHECK_TRUE(res.value("show", "").find("NETMON_BLK_192_0_2_55") != std::string::npos);
    CHECK_FALSE(sentExact(*sim, "no secure-policy 1"));
}

TEST(ForwardBlock, StartupScanFlagsDefaultAllowRule) {
    sim->seedRule("Policy-Control_AAA", "any", "allow", true);
    ZyxelDriver::getInstance().start();
    STRCMP_EQUAL("Policy-Control_AAA", ZyxelDriver::getInstance().startupAlert().c_str());
    CHECK_TRUE(sentExact(*sim, "show secure-policy"));
    CHECK_FALSE(sentExact(*sim, "no secure-policy 1"));
    ZyxelDriver::getInstance().stop();
}

TEST(ForwardBlock, RefusesProtectedAddresses) {
    auto zero = ZyxelDriver::getInstance().blockIp("0.0.0.0", "x");
    STRCMP_EQUAL("error", zero.value("status", "").c_str());
    auto bcast = ZyxelDriver::getInstance().blockIp("255.255.255.255", "x");
    STRCMP_EQUAL("error", bcast.value("status", "").c_str());
    auto local = ZyxelDriver::getInstance().blockIp("127.0.0.1", "x");
    STRCMP_EQUAL("error", local.value("status", "").c_str());
    CHECK_FALSE(sentExact(*sim, "configure terminal"));
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
