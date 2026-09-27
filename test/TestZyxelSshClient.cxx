/*
 * TestZyxelSshClient.cxx
 *
 * Copyright (C) 2026, Charles Chiou
 */

#include <iostream>
#include <string>
#include <vector>

#include "ZyxelSshClient.hxx"
#include <CppUTest/TestHarness.h>

TEST_GROUP(ZyxelSshClientTest) {
    void setup() {
    }

    void teardown() {
    }
};

TEST(ZyxelSshClientTest, InitialStateIsDisconnected) {
    ZyxelSshClient client;
    CHECK_FALSE(client.isConnected());
    CHECK_EQUAL(static_cast<int>(SshClientState::DISCONNECTED),
                static_cast<int>(client.getState()));
}

TEST(ZyxelSshClientTest, TailPromptRegexMatching) {
    std::string prompt;

    // Standard user exec
    CHECK_TRUE(ZyxelSshClient::matchPrompt("Router>", prompt));
    STRCMP_EQUAL("Router>", prompt.c_str());

    // Standard privileged exec
    CHECK_TRUE(ZyxelSshClient::matchPrompt("Router#", prompt));
    STRCMP_EQUAL("Router#", prompt.c_str());

    // Config mode
    CHECK_TRUE(ZyxelSshClient::matchPrompt("Router(config)#", prompt));
    STRCMP_EQUAL("Router(config)#", prompt.c_str());

    // Submode (policy-control)
    CHECK_TRUE(ZyxelSshClient::matchPrompt("Router(config-policy-control)#", prompt));
    STRCMP_EQUAL("Router(config-policy-control)#", prompt.c_str());

    // Submode (address object)
    CHECK_TRUE(ZyxelSshClient::matchPrompt("usg-flex-200(config-address)#", prompt));
    STRCMP_EQUAL("usg-flex-200(config-address)#", prompt.c_str());

    // Multiline command output ending with prompt
    std::string output = "Building configuration...\n[OK]\nRouter(config)# ";
    CHECK_TRUE(ZyxelSshClient::matchPrompt(output, prompt));
}

TEST(ZyxelSshClientTest, PromptRegexDoesNotMatchMidBuffer) {
    std::string prompt;

    // Mid-buffer prompt in description text with trailing lines
    std::string midBuffer = "description \"Check host Router#1\"\nStatus: active\nPackets: 450";
    CHECK_FALSE(ZyxelSshClient::matchPrompt(midBuffer, prompt));

    // Greater-than in throughput log
    std::string logLine = "Throughput: eth0 > 1000 Mbps\nAnalyzing stream...";
    CHECK_FALSE(ZyxelSshClient::matchPrompt(logLine, prompt));
}

TEST(ZyxelSshClientTest, AnsiEscapeStripping) {
    // Green text color and reset
    std::string colorStr = "\033[32mSuccess\033[0m\r\n";
    std::string stripped = ZyxelSshClient::stripAnsiEscapes(colorStr);
    STRCMP_EQUAL("Success\n", stripped.c_str());

    // Cursor position escape codes
    std::string cursorStr = "\033[2K\033[1GRouter#";
    stripped = ZyxelSshClient::stripAnsiEscapes(cursorStr);
    STRCMP_EQUAL("Router#", stripped.c_str());
}

TEST(ZyxelSshClientTest, CommandEchoStripping) {
    std::string command = "show version";
    std::string rawOutput = "show version\r\nZyWALL USG FLEX 200\r\nFirmware: 5.37\r\nRouter#";

    std::string stripped = ZyxelSshClient::stripCommandEcho(rawOutput, command);
    STRCMP_EQUAL("ZyWALL USG FLEX 200\nFirmware: 5.37\nRouter#", stripped.c_str());
}

TEST(ZyxelSshClientTest, ConfigLockDetection) {
    std::string lockedOutput = "configure terminal\r\n% Configuration is locked by admin (Web GUI)\r\nRouter#";
    CHECK_TRUE(ZyxelSshClient::isConfigLocked(lockedOutput));

    std::string normalOutput = "configure terminal\r\nRouter(config)#";
    CHECK_FALSE(ZyxelSshClient::isConfigLocked(normalOutput));
}

TEST(ZyxelSshClientTest, SanitizeReasonFilter) {
    // Normal alphanumeric with spaces and dashes
    std::string clean = "High bandwidth violation - host 192.0.2.50";
    STRCMP_EQUAL(clean.c_str(), ZyxelSshClient::sanitizeReason(clean).c_str());

    // Command injection characters (quotes, backticks, semicolons, dollar signs)
    std::string dirty = "Blocked\"; reboot; echo `whoami` $VAR 'test'";
    std::string sanitized = ZyxelSshClient::sanitizeReason(dirty);
    STRCMP_EQUAL("Blocked reboot echo whoami VAR test", sanitized.c_str());

    // Length capping at 64 characters
    std::string longReason(100, 'A');
    std::string capped = ZyxelSshClient::sanitizeReason(longReason);
    LONGS_EQUAL(64, capped.size());
}

TEST(ZyxelSshClientTest, SanitizeIpToObjectName) {
    STRCMP_EQUAL("NETMON_BLK_192_0_2_50",
                 ZyxelSshClient::sanitizeIpToObjectName("192.0.2.50").c_str());
    STRCMP_EQUAL("NETMON_BLK_10_0_0_1",
                 ZyxelSshClient::sanitizeIpToObjectName("10.0.0.1").c_str());
}

TEST(ZyxelSshClientTest, SyntaxErrorDetectionMatchesExplicitZySHErrors) {
    CHECK_TRUE(ZyxelSshClient::isSyntaxError("% Invalid command at '^' marker"));
    CHECK_TRUE(ZyxelSshClient::isSyntaxError("% Incomplete command"));
    CHECK_TRUE(ZyxelSshClient::isSyntaxError("% Object name is invalid"));
    CHECK_TRUE(ZyxelSshClient::isSyntaxError("% Syntax error"));
    CHECK_TRUE(ZyxelSshClient::isSyntaxError("% Ambiguous command"));
    CHECK_TRUE(ZyxelSshClient::isSyntaxError("% Bad parameter for rule"));

    // Non-syntax outputs should not be detected as syntax errors
    CHECK_FALSE(ZyxelSshClient::isSyntaxError("% Configuration is locked by admin"));
    CHECK_FALSE(ZyxelSshClient::isSyntaxError("Router#"));
    CHECK_FALSE(ZyxelSshClient::isSyntaxError("Connection dropped by peer"));
    CHECK_FALSE(ZyxelSshClient::isSyntaxError("Channel read timeout"));
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
