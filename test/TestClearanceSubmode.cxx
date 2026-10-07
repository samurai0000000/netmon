/*
 * TestClearanceSubmode.cxx
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

static bool sentPrefix(const ZyshSimulator &sim, const std::string &prefix) {
    for (const auto &got : sim.lines()) {
        if (got.compare(0, prefix.size(), prefix) == 0) {
            return true;
        }
    }
    return false;
}

TEST_GROUP(ClearanceSubmode) {
    std::string pinPath = "test_data/ssh_e3/router_hostkey.pin";
    std::string vaultDir = "test_data/ssh_e3/vault";
    ZyshSimulator *sim = nullptr;

    void setup() {
        ZyxelDriver::getInstance().resetForTesting();
        AuthManager::getInstance().resetForTesting();
        fs::create_directories(vaultDir);
        fs::remove(pinPath);
        AuthManager::getInstance().setVaultDir(vaultDir);
        sim = new ZyshSimulator();
        sim->setHostKey(kPinHex);
        CHECK_EQUAL(static_cast<int>(SshResult::SUCCESS),
                    static_cast<int>(ZyxelSshClient::writeHostKeyPin(pinPath, kPinHex)));
        CHECK_TRUE(AuthManager::getInstance().setRouterPassword("router-test-pass"));
        ZyxelDriver &driver = ZyxelDriver::getInstance();
        driver.setLiveEnabled(true);
        driver.configure("192.0.2.1", 22, "admin", pinPath);
        driver.getSshClient().setTransport(sim->transport());
    }

    void teardown() {
        ZyxelDriver::getInstance().resetForTesting();
        delete sim;
        sim = nullptr;
        AuthManager::getInstance().resetForTesting();
        fs::remove_all("test_data/ssh_e3");
    }

    SshResult run(const std::string &command, std::string &out, std::string &prompt) {
        return ZyxelDriver::getInstance().executeClearanceCommand(command, out, prompt, 1000);
    }
};

TEST(ClearanceSubmode, ClearanceInsertDoesNotExitActiveSubmode) {
    std::string out;
    std::string prompt;
    SshResult res = run("secure-policy insert 1", out, prompt);
    CHECK_EQUAL(static_cast<int>(SshResult::SUCCESS), static_cast<int>(res));
    CHECK_TRUE(sentExact(*sim, "secure-policy insert 1"));
    CHECK_FALSE(sentExact(*sim, "exit"));
    CHECK_FALSE(sentPrefix(*sim, "no secure-policy"));
    CHECK_TRUE(sim->runningConfig().find("Policy-Control_") == std::string::npos);
    STRCMP_EQUAL("Router(secure-policy)#", prompt.c_str());
    CHECK_EQUAL(static_cast<int>(PromptState::POLICY_SUBMODE),
                static_cast<int>(ZyxelDriver::getInstance().getSshClient().getPromptState()));
    CHECK_TRUE(ZyxelDriver::getInstance().isConnected());
}

TEST(ClearanceSubmode, GrantExpiryMidSubmodeSendsNothing) {
    std::string out;
    std::string prompt;
    CHECK_EQUAL(static_cast<int>(SshResult::SUCCESS),
                static_cast<int>(run("secure-policy insert 1", out, prompt)));
    size_t before = sim->lines().size();
    SshResult abandoned = ZyxelDriver::getInstance().abandonPolicySubmode();
    CHECK_EQUAL(static_cast<int>(SshResult::ERR_DISCONNECTED), static_cast<int>(abandoned));
    LONGS_EQUAL(before, sim->lines().size());
    CHECK_FALSE(sentExact(*sim, "exit"));
    CHECK_FALSE(sentPrefix(*sim, "no secure-policy"));
    CHECK_TRUE(sim->runningConfig().find("Policy-Control_") == std::string::npos);
    CHECK_FALSE(ZyxelDriver::getInstance().isConnected());
}

TEST(ClearanceSubmode, GrantExpiryAfterNoActivateSendsNoDelete) {
    std::string out;
    std::string prompt;
    CHECK_EQUAL(static_cast<int>(SshResult::SUCCESS),
                static_cast<int>(run("secure-policy insert 1", out, prompt)));
    CHECK_EQUAL(static_cast<int>(SshResult::SUCCESS),
                static_cast<int>(run("no activate", out, prompt)));
    CHECK_TRUE(ZyxelDriver::getInstance().getSshClient().policyInactiveAcknowledged());
    size_t before = sim->lines().size();
    SshResult abandoned = ZyxelDriver::getInstance().abandonPolicySubmode();
    CHECK_EQUAL(static_cast<int>(SshResult::ERR_EXEC_FAILED), static_cast<int>(abandoned));
    LONGS_EQUAL(before, sim->lines().size());
    CHECK_FALSE(sentExact(*sim, "exit"));
    CHECK_FALSE(sentPrefix(*sim, "no secure-policy"));
    CHECK_TRUE(sim->runningConfig().find("Policy-Control_") == std::string::npos);
    CHECK_TRUE(ZyxelDriver::getInstance().isConnected());
}

TEST(ClearanceSubmode, SocketCloseMidSubmode) {
    std::string out;
    std::string prompt;
    CHECK_EQUAL(static_cast<int>(SshResult::SUCCESS),
                static_cast<int>(run("secure-policy insert 1", out, prompt)));
    int closes = sim->closeCount();
    size_t before = sim->lines().size();
    SshResult abandoned = ZyxelDriver::getInstance().abandonPolicySubmode();
    CHECK_EQUAL(static_cast<int>(SshResult::ERR_DISCONNECTED), static_cast<int>(abandoned));
    LONGS_EQUAL(before, sim->lines().size());
    CHECK_FALSE(sentExact(*sim, "exit"));
    CHECK_FALSE(sentPrefix(*sim, "no secure-policy"));
    CHECK_TRUE(sim->closeCount() > closes);
    CHECK_FALSE(ZyxelDriver::getInstance().isConnected());
}

TEST(ClearanceSubmode, QuestionMarkRejected) {
    std::string out;
    std::string prompt;
    CHECK_EQUAL(static_cast<int>(SshResult::SUCCESS),
                static_cast<int>(run("show version", out, prompt)));
    const char *bad[] = {"router ospf ?", "?", "show ?"};
    for (const char *cmd : bad) {
        SshResult res = run(cmd, out, prompt);
        CHECK_EQUAL(static_cast<int>(SshResult::ERR_REJECTED), static_cast<int>(res));
        CHECK_FALSE(sentExact(*sim, cmd));
    }
    CHECK_TRUE(sim->runningConfig().find("router ospf") == std::string::npos);
    CHECK_TRUE(ZyxelDriver::getInstance().isConnected());
}

TEST(ClearanceSubmode, ControlCharsRejected) {
    std::string out;
    std::string prompt;
    CHECK_EQUAL(static_cast<int>(SshResult::SUCCESS),
                static_cast<int>(run("show version", out, prompt)));
    std::string withNl = "show version\nreboot";
    std::string withCtl = std::string("show") + '\x01' + "version";
    std::string withDel = std::string("show") + '\x7f' + "version";
    std::string tooLong(513, 'A');
    const std::string *bad[] = {&withNl, &withCtl, &withDel, &tooLong};
    for (const std::string *cmd : bad) {
        SshResult res = run(*cmd, out, prompt);
        CHECK_EQUAL(static_cast<int>(SshResult::ERR_REJECTED), static_cast<int>(res));
        CHECK_FALSE(sentExact(*sim, *cmd));
    }
    CHECK_FALSE(sentExact(*sim, "reboot"));
    CHECK_TRUE(ZyxelDriver::getInstance().isConnected());
}

TEST(ClearanceSubmode, WriteRejectedAtDriver) {
    std::string out;
    std::string prompt;
    CHECK_EQUAL(static_cast<int>(SshResult::SUCCESS),
                static_cast<int>(run("show version", out, prompt)));
    const char *bad[] = {
        "write",
        "reboot",
        "copy running-config startup-config",
        "boot",
        "delete flash",
        "shutdown",
        "run",
        "apply"
    };
    for (const char *cmd : bad) {
        SshResult res = run(cmd, out, prompt);
        CHECK_EQUAL(static_cast<int>(SshResult::ERR_REJECTED), static_cast<int>(res));
        CHECK_FALSE(sentExact(*sim, cmd));
    }
    CHECK_TRUE(ZyxelDriver::getInstance().isConnected());
}

TEST(ClearanceSubmode, FirstSubmodeLineMustBeNoActivate) {
    std::string out;
    std::string prompt;
    CHECK_EQUAL(static_cast<int>(SshResult::SUCCESS),
                static_cast<int>(run("secure-policy insert 1", out, prompt)));
    SshResult named = run("name RULE_HTTP", out, prompt);
    CHECK_TRUE(named != SshResult::SUCCESS);
    CHECK_FALSE(sentExact(*sim, "name RULE_HTTP"));
    SshResult activated = run("activate", out, prompt);
    CHECK_TRUE(activated != SshResult::SUCCESS);
    CHECK_FALSE(sentExact(*sim, "activate"));
    CHECK_EQUAL(static_cast<int>(SshResult::SUCCESS),
                static_cast<int>(run("no activate", out, prompt)));
    CHECK_TRUE(sentExact(*sim, "no activate"));
    CHECK_FALSE(sentExact(*sim, "exit"));
    CHECK_TRUE(sim->runningConfig().find("Policy-Control_") == std::string::npos);
    CHECK_TRUE(ZyxelDriver::getInstance().getSshClient().policyInactiveAcknowledged());
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
