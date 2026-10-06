/*
 * TestNetMonShell.cxx
 *
 * Copyright (C) 2026, Charles Chiou
 */

#include <sstream>
#include <iostream>
#include <vector>
#include <string>
#include "NetMonShell.hxx"
#include "ZyxelDriver.hxx"

#include <CppUTest/TestHarness.h>

TEST_GROUP(NetMonShellTest) {
    std::stringstream buffer;
    std::streambuf *oldCout;

    void setup() override {
        NetMonShell::getInstance().resetForTesting();
        oldCout = std::cout.rdbuf(buffer.rdbuf());
    }

    void teardown() override {
        std::cout.rdbuf(oldCout);
        NetMonShell::getInstance().resetForTesting();
    }

    void assertAllLinesWithinLimit(const std::string &output, size_t maxChars = 76) {
        std::istringstream iss(output);
        std::string line;
        size_t lineNum = 1;
        while (std::getline(iss, line)) {
            // Strip trailing carriage return if present
            if (!line.empty() && line.back() == '\r') {
                line.pop_back();
            }
            if (line.length() > maxChars) {
                std::string msg = "Line " + std::to_string(lineNum) + " (" +
                                  std::to_string(line.length()) + " chars) exceeds " +
                                  std::to_string(maxChars) + " chars: \"" + line + "\"";
                FAIL(msg.c_str());
            }
            lineNum++;
        }
    }
};

TEST(NetMonShellTest, HelpOutputConformsTo80x24Rule) {
    int ret = NetMonShell::getInstance().executeCommand("help");
    CHECK_EQUAL(0, ret);
    std::string out = buffer.str();
    CHECK_FALSE(out.empty());
    assertAllLinesWithinLimit(out, 76);
}

TEST(NetMonShellTest, RouterKeywordIsCompletelyPurgedWithZeroAliases) {
    // Attempting to invoke legacy "router" commands must result in unknown command error
    buffer.str("");
    int ret = NetMonShell::getInstance().executeCommand("router status");
    CHECK_EQUAL(0, ret);
    std::string out = buffer.str();
    CHECK_TRUE(out.find("Unknown command: 'router'") != std::string::npos);

    buffer.str("");
    ret = NetMonShell::getInstance().executeCommand("router ping 1.1.1.1");
    CHECK_EQUAL(0, ret);
    out = buffer.str();
    CHECK_TRUE(out.find("Unknown command: 'router'") != std::string::npos);

    buffer.str("");
    ret = NetMonShell::getInstance().executeCommand("router clear-password");
    CHECK_EQUAL(0, ret);
    out = buffer.str();
    CHECK_TRUE(out.find("Unknown command: 'router'") != std::string::npos);
}

TEST(NetMonShellTest, FirewallCommandsDispatchedNominally) {
    // Test firewall status dispatch
    buffer.str("");
    int ret = NetMonShell::getInstance().executeCommand("firewall status");
    CHECK_EQUAL(0, ret);
    std::string out = buffer.str();
    CHECK_TRUE(out.find("Zyxel Firewall Status") != std::string::npos);
    assertAllLinesWithinLimit(out, 76);
}

TEST(NetMonShellTest, FirewallDiagRejectsWhenNotConfiguredOrNotConnected) {
    buffer.str("");
    int ret = NetMonShell::getInstance().executeCommand("firewall diag read");
    // When offline, executes cleanly and emits connection error to stdout
    CHECK_EQUAL(0, ret);
    std::string out = buffer.str();
    CHECK_TRUE(out.find("Firewall driver is not") != std::string::npos);
    assertAllLinesWithinLimit(out, 76);
}

TEST(NetMonShellTest, AuthListConformsTo80x24Rule) {
    buffer.str("");
    int ret = NetMonShell::getInstance().executeCommand("auth list");
    CHECK_EQUAL(0, ret);
    std::string out = buffer.str();
    assertAllLinesWithinLimit(out, 76);
}

TEST(NetMonShellTest, DevicesAndUnregisteredConformTo80x24Rule) {
    buffer.str("");
    int ret = NetMonShell::getInstance().executeCommand("devices");
    CHECK_EQUAL(0, ret);
    assertAllLinesWithinLimit(buffer.str(), 76);

    buffer.str("");
    ret = NetMonShell::getInstance().executeCommand("unregistered");
    CHECK_EQUAL(0, ret);
    assertAllLinesWithinLimit(buffer.str(), 76);

    buffer.str("");
    ret = NetMonShell::getInstance().executeCommand("toptalkers");
    CHECK_EQUAL(0, ret);
    assertAllLinesWithinLimit(buffer.str(), 76);
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
