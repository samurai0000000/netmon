/*
 * TestDiagBracket.cxx
 *
 * Copyright (C) 2026, Charles Chiou
 */

#include <cstdlib>
#include <stdexcept>
#include <string>
#include <vector>
#include <filesystem>
#include <fstream>

#include <sys/stat.h>
#include <unistd.h>

#include "ZyxelDriver.hxx"
#include "ZyxelLiveDiagnostic.hxx"
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

static bool argsContain(const std::vector<std::string> &args, const std::string &token) {
    for (const auto &arg : args) {
        if (arg == token) {
            return true;
        }
    }
    return false;
}

TEST_GROUP(DiagWriteRestore) {
    std::string pinPath = "test_data/ssh_e5/router_hostkey.pin";
    std::string vaultDir = "test_data/ssh_e5/vault";
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
        fs::remove_all("test_data/ssh_e5");
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

TEST(DiagWriteRestore, DiagReadSendsNoConfigLines) {
    ZyxelDriver &driver = ZyxelDriver::getInstance();
    for (const auto &line : diagnosticReadCatalog()) {
        CHECK_FALSE(diagnosticLineIsConfig(line));
        std::string out;
        SshResult res = driver.sendDiagnosticLine(line, out, 5000);
        CHECK_EQUAL(static_cast<int>(SshResult::SUCCESS), static_cast<int>(res));
    }
    CHECK_TRUE(sim->lines().size() > 0);
    for (const auto &sent : sim->lines()) {
        CHECK_FALSE(diagnosticLineIsConfig(sent));
    }

    std::vector<std::string> readArgs = buildDiagnosticArgv("read", "");
    CHECK_TRUE(argsContain(readArgs, "LiveFirewallReadDiag"));
    CHECK_TRUE(argsContain(readArgs, "LiveFirewallE1"));
    CHECK_TRUE(argsContain(readArgs, "LiveFirewallE10"));
    CHECK_TRUE(argsContain(readArgs, "-xn"));
    CHECK_TRUE(argsContain(readArgs, "Write"));
    CHECK_FALSE(argsContain(readArgs, "-n"));

    std::vector<std::string> writeArgs = buildDiagnosticArgv("write", "");
    CHECK_TRUE(argsContain(writeArgs, "Write"));
    CHECK_TRUE(argsContain(writeArgs, "DriverBlockIpAndUnblockIp"));
    CHECK_FALSE(argsContain(writeArgs, "-sg"));

    std::vector<std::string> e4Args = buildDiagnosticArgv("e4", "");
    CHECK_TRUE(argsContain(e4Args, "LiveFirewallE4"));

    std::vector<std::string> allArgs = buildDiagnosticArgv("all", "");
    LONGS_EQUAL(2, allArgs.size());
}

TEST(DiagWriteRestore, DiagWriteRestoresEachObject) {
    ZyxelDriver &driver = ZyxelDriver::getInstance();
    {
        DiagRestoreBracket bracket(
            driver,
            {"configure terminal", "no address-object NETMON_QA_HOST", "exit"},
            "show address-object",
            "NETMON_QA_HOST");
        CHECK_EQUAL(static_cast<int>(SshResult::SUCCESS),
                    static_cast<int>(bracket.send("configure terminal")));
        CHECK_EQUAL(static_cast<int>(SshResult::SUCCESS),
                    static_cast<int>(bracket.send("address-object NETMON_QA_HOST 192.0.2.201")));
        std::string shown;
        driver.sendDiagnosticLine("show address-object", shown, 5000);
        CHECK_TRUE(shown.find("NETMON_QA_HOST") != std::string::npos);
        bracket.restore();
        CHECK_TRUE(bracket.objectGone());
    }
    CHECK_TRUE(sentExact(*sim, "no address-object NETMON_QA_HOST"));
    std::string after;
    ZyxelDriver::getInstance().sendDiagnosticLine("show address-object", after, 5000);
    CHECK_TRUE(after.find("NETMON_QA_HOST") == std::string::npos);
}

TEST(DiagWriteRestore, DiagWriteRestoreRunsWhenCheckFails) {
    ZyxelDriver &driver = ZyxelDriver::getInstance();
    bool threw = false;
    try {
        DiagRestoreBracket bracket(
            driver,
            {"no address-object NETMON_QA_HOST", "exit"},
            "show address-object",
            "NETMON_QA_HOST");
        CHECK_EQUAL(static_cast<int>(SshResult::SUCCESS),
                    static_cast<int>(bracket.send("configure terminal")));
        CHECK_EQUAL(static_cast<int>(SshResult::SUCCESS),
                    static_cast<int>(bracket.send("address-object NETMON_QA_HOST 192.0.2.201")));
        throw std::runtime_error("check failed");
    } catch (const std::runtime_error &ex) {
        threw = true;
        STRCMP_EQUAL("check failed", ex.what());
    }
    CHECK_TRUE(threw);
    CHECK_TRUE(sentExact(*sim, "no address-object NETMON_QA_HOST"));
    std::string after;
    driver.sendDiagnosticLine("show address-object", after, 5000);
    CHECK_TRUE(after.find("NETMON_QA_HOST") == std::string::npos);
}

TEST(DiagWriteRestore, DiagWriteDeletesProfileFromConfig) {
    ZyxelDriver &driver = ZyxelDriver::getInstance();
    {
        DiagRestoreBracket bracket(
            driver,
            {"no ip ddns profile NETMON_QA_DDNS", "exit"},
            "show ddns",
            "NETMON_QA_DDNS");
        CHECK_EQUAL(static_cast<int>(SshResult::SUCCESS),
                    static_cast<int>(bracket.send("configure terminal")));
        CHECK_EQUAL(static_cast<int>(SshResult::SUCCESS),
                    static_cast<int>(bracket.send("ip ddns profile NETMON_QA_DDNS")));
        bracket.restore();
        CHECK_TRUE(bracket.restoreSucceeded());
        CHECK_TRUE(bracket.objectGone());
    }
    CHECK_TRUE(sentExact(*sim, "no ip ddns profile NETMON_QA_DDNS"));
    std::string after;
    driver.sendDiagnosticLine("show ddns", after, 5000);
    CHECK_TRUE(after.find("NETMON_QA_DDNS") == std::string::npos);
}

TEST(DiagWriteRestore, DiagWriteReentersConfigAfterExit) {
    ZyxelDriver &driver = ZyxelDriver::getInstance();
    {
        DiagRestoreBracket bracket(
            driver,
            {"no address-object NETMON_QA_HOST", "exit"},
            "show address-object",
            "NETMON_QA_HOST");
        CHECK_EQUAL(static_cast<int>(SshResult::SUCCESS),
                    static_cast<int>(bracket.send("configure terminal")));
        CHECK_EQUAL(static_cast<int>(SshResult::SUCCESS),
                    static_cast<int>(bracket.send(
                        "address-object NETMON_QA_HOST 192.0.2.201")));
        CHECK_EQUAL(static_cast<int>(SshResult::SUCCESS),
                    static_cast<int>(bracket.send("exit")));
        bracket.restore();
        CHECK_TRUE(bracket.restoreSucceeded());
        CHECK_TRUE(bracket.objectGone());
    }
    bool sawExit = false;
    bool sawConfigureAfterExit = false;
    bool sawDeleteAfterConfigure = false;
    int exits = 0;
    for (const auto &line : sim->lines()) {
        if (line == "exit") {
            sawExit = true;
            exits++;
        }
        if (sawExit && line == "configure terminal") {
            sawConfigureAfterExit = true;
        }
        if (sawConfigureAfterExit && line == "no address-object NETMON_QA_HOST") {
            sawDeleteAfterConfigure = true;
        }
    }
    CHECK_TRUE(sawDeleteAfterConfigure);
    LONGS_EQUAL(2, exits);
}

TEST(DiagWriteRestore, DiagWriteReportsInverseFailure) {
    sim->failOnOccurrence("no address-object NETMON_QA_HOST", 1,
                          "% Syntax error");
    ZyxelDriver &driver = ZyxelDriver::getInstance();
    DiagRestoreBracket bracket(
        driver,
        {"no address-object NETMON_QA_HOST", "exit"},
        "show address-object",
        "NETMON_QA_HOST");
    CHECK_EQUAL(static_cast<int>(SshResult::SUCCESS),
                static_cast<int>(bracket.send("configure terminal")));
    CHECK_EQUAL(static_cast<int>(SshResult::SUCCESS),
                static_cast<int>(bracket.send(
                    "address-object NETMON_QA_HOST 192.0.2.201")));

    bracket.restore();

    CHECK_FALSE(bracket.restoreSucceeded());
    CHECK_FALSE(bracket.objectGone());
    CHECK_TRUE(bracket.restoreError().find(
        "no address-object NETMON_QA_HOST") != std::string::npos);
}

TEST(DiagWriteRestore, DiagWriteDoesNotExitActivePolicy) {
    sim->failOnOccurrence("no activate", 1, "% Syntax error");
    ZyxelDriver &driver = ZyxelDriver::getInstance();
    {
        DiagRestoreBracket bracket(
            driver,
            {"exit", "no secure-policy 1"},
            "show secure-policy",
            "NETMON_QA_RULE");
        CHECK_EQUAL(static_cast<int>(SshResult::SUCCESS),
                    static_cast<int>(bracket.send("configure terminal")));
        CHECK_EQUAL(static_cast<int>(SshResult::SUCCESS),
                    static_cast<int>(bracket.send("secure-policy insert 1")));
        bracket.send("no activate");
        CHECK_TRUE(bracket.policyAborted());
    }
    CHECK_TRUE(sentExact(*sim, "no activate"));
    CHECK_FALSE(sentExact(*sim, "exit"));
    CHECK_FALSE(sentExact(*sim, "no secure-policy 1"));
    CHECK_FALSE(ZyxelDriver::getInstance().isConnected());
}

TEST(DiagWriteRestore, DriverRefusesWithoutPassword) {
    ZyxelDriver &driver = ZyxelDriver::getInstance();
    CHECK_TRUE(AuthManager::getInstance().clearRouterPassword());
    auto res = driver.blockIp("192.0.2.88", "no-password");
    STRCMP_EQUAL("error", res.value("status", "").c_str());
    CHECK_FALSE(driver.isConnected());
    CHECK_FALSE(sentExact(*sim, "configure terminal"));
    std::string retrieved;
    CHECK_FALSE(AuthManager::getInstance().getRouterPassword(retrieved));
}

TEST(DiagWriteRestore, DiagLogNotInTmp) {
    const char *home = std::getenv("HOME");
    CHECK_TRUE(home != nullptr && home[0] != '\0');
    std::string marker = "diag-log-marker-e5-unique";
    std::string path = diagLogPath();
    CHECK_TRUE(path.find("/.config/netmon/logs/netmon_diag.log") != std::string::npos);
    CHECK_TRUE(path.find("/tmp/") == std::string::npos);

    logDiag(marker);
    std::ifstream in(path);
    CHECK_TRUE(in.good());
    std::string body((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
    CHECK_TRUE(body.find(marker) != std::string::npos);

    struct stat st;
    CHECK_EQUAL(0, stat(path.c_str(), &st));
    CHECK_EQUAL(0600, static_cast<int>(st.st_mode & 0777));

    std::string tmpPath = "/tmp/netmon_diag.log";
    std::ifstream tmp(tmpPath);
    if (tmp.good()) {
        std::string tmpBody((std::istreambuf_iterator<char>(tmp)),
                            std::istreambuf_iterator<char>());
        CHECK_TRUE(tmpBody.find(marker) == std::string::npos);
    }

    fs::remove(path);
    std::string sink = path + ".sink";
    {
        std::ofstream touch(sink);
        touch << "untouched\n";
    }
    CHECK_EQUAL(0, symlink(sink.c_str(), path.c_str()));
    logDiag(marker + "-through-link");
    std::ifstream sinkIn(sink);
    std::string sinkBody((std::istreambuf_iterator<char>(sinkIn)),
                         std::istreambuf_iterator<char>());
    CHECK_TRUE(sinkBody.find(marker + "-through-link") == std::string::npos);
    fs::remove(path);
    fs::remove(sink);
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
