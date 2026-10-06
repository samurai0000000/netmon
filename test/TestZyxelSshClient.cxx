/*
 * TestZyxelSshClient.cxx
 *
 * Copyright (C) 2026, Charles Chiou
 */

#include <iostream>
#include <string>
#include <vector>
#include <cstring>

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

TEST(ZyxelSshClientTest, TrailingPromptStripping) {
    // Strips Router> prompt and trims trailing whitespace
    std::string out1 = " 1  * * *\n 2  168.95.105.138  10.370 ms\nRouter> ";
    STRCMP_EQUAL(" 1  * * *\n 2  168.95.105.138  10.370 ms",
                 ZyxelSshClient::stripTrailingPrompt(out1).c_str());

    // Strips Router# prompt
    std::string out2 = "ZyWALL USG FLEX 200\r\nRouter#";
    STRCMP_EQUAL("ZyWALL USG FLEX 200",
                 ZyxelSshClient::stripTrailingPrompt(out2).c_str());

    // Strips submode prompt Router(config)#
    std::string out3 = "address-object host NETMON_BLK_1_2_3_4\nRouter(config)# ";
    STRCMP_EQUAL("address-object host NETMON_BLK_1_2_3_4",
                 ZyxelSshClient::stripTrailingPrompt(out3).c_str());

    // Output without prompt is preserved
    std::string out4 = "Standard output with no prompt\n";
    STRCMP_EQUAL("Standard output with no prompt",
                 ZyxelSshClient::stripTrailingPrompt(out4).c_str());

    // Empty string
    STRCMP_EQUAL("", ZyxelSshClient::stripTrailingPrompt("").c_str());
}

TEST(ZyxelSshClientTest, CommandCancellationState) {
    ZyxelSshClient client;
    CHECK_FALSE(client.isCancelled());

    client.cancelActiveCommand();
    CHECK_TRUE(client.isCancelled());

    client.resetForTesting();
    CHECK_FALSE(client.isCancelled());
}

TEST(ZyxelSshClientTest, DrainUntilPromptHandlesZeroByteReadsWithoutPrematureExit) {
    ZyxelSshClient client;
    int callCount = 0;
    std::string payload = "traceroute to 8.8.8.8\n 1  192.168.1.1  1.2 ms\nRouter> ";

    auto reader = [&](char *buf, size_t buflen, bool &eofOut) -> ssize_t {
        (void)buflen;
        eofOut = false;
        callCount++;
        // First 3 calls return 0 (no payload yet, but not EOF)
        if (callCount <= 3) {
            return 0;
        }
        // 4th call delivers full payload
        if (callCount == 4) {
            std::memcpy(buf, payload.data(), payload.size());
            return static_cast<ssize_t>(payload.size());
        }
        return 0;
    };

    auto writer = [](const char *data, size_t len) {
        (void)data;
        (void)len;
    };

    std::string out;
    SshResult res = client.drainUntilPromptForTesting(reader, writer, out, 1000);
    CHECK_EQUAL(static_cast<int>(SshResult::SUCCESS), static_cast<int>(res));
    CHECK_TRUE(out.find("192.168.1.1") != std::string::npos);
    CHECK_TRUE(out.find("Router>") != std::string::npos);
    CHECK_TRUE(callCount >= 4);
}

TEST(ZyxelSshClientTest, DrainUntilPromptDetectsTrueEofAsChannelFailed) {
    ZyxelSshClient client;
    auto reader = [](char *buf, size_t buflen, bool &eofOut) -> ssize_t {
        (void)buf;
        (void)buflen;
        eofOut = true; // True channel EOF without matching prompt
        return 0;
    };

    auto writer = [](const char *data, size_t len) {
        (void)data;
        (void)len;
    };

    std::string out;
    SshResult res = client.drainUntilPromptForTesting(reader, writer, out, 500);
    CHECK_EQUAL(static_cast<int>(SshResult::ERR_CHANNEL_FAILED), static_cast<int>(res));
}

TEST(ZyxelSshClientTest, DrainUntilPromptHandlesEagainAndChunkedDelivery) {
    ZyxelSshClient client;
    int step = 0;
    std::string chunk1 = " 1  192.168.1.1 1.2 ms\n";
    std::string chunk2 = " 2  8.8.8.8 12.3 ms\nRouter> ";

    auto reader = [&](char *buf, size_t buflen, bool &eofOut) -> ssize_t {
        (void)buflen;
        eofOut = false;
        step++;
        if (step == 1) {
            return LIBSSH2_ERROR_EAGAIN;
        } else if (step == 2) {
            std::memcpy(buf, chunk1.data(), chunk1.size());
            return static_cast<ssize_t>(chunk1.size());
        } else if (step == 3) {
            return 0; // zero-byte read between chunks
        } else if (step == 4) {
            std::memcpy(buf, chunk2.data(), chunk2.size());
            return static_cast<ssize_t>(chunk2.size());
        }
        return 0;
    };

    auto writer = [](const char *data, size_t len) {
        (void)data;
        (void)len;
    };

    std::string out;
    SshResult res = client.drainUntilPromptForTesting(reader, writer, out, 1000);
    CHECK_EQUAL(static_cast<int>(SshResult::SUCCESS), static_cast<int>(res));
    CHECK_TRUE(out.find("192.168.1.1") != std::string::npos);
    CHECK_TRUE(out.find("8.8.8.8") != std::string::npos);
    CHECK_TRUE(out.find("Router>") != std::string::npos);
}

TEST(ZyxelSshClientTest, DrainUntilPromptTimesOut) {
    ZyxelSshClient client;
    auto reader = [](char *buf, size_t buflen, bool &eofOut) -> ssize_t {
        (void)buf;
        (void)buflen;
        eofOut = false;
        return 0; // endless non-blocking empty packets
    };

    auto writer = [](const char *data, size_t len) {
        (void)data;
        (void)len;
    };

    std::string out;
    SshResult res = client.drainUntilPromptForTesting(reader, writer, out, 50);
    CHECK_EQUAL(static_cast<int>(SshResult::ERR_TIMEOUT), static_cast<int>(res));
}

TEST(ZyxelSshClientTest, DrainUntilPromptHandlesCancellation) {
    ZyxelSshClient client;
    client.cancelActiveCommand();

    std::string writtenData;
    auto reader = [&](char *buf, size_t buflen, bool &eofOut) -> ssize_t {
        (void)buflen;
        eofOut = false;
        if (!writtenData.empty()) {
            std::memcpy(buf, "Router> ", 8);
            return 8;
        }
        return 0;
    };

    auto writer = [&](const char *data, size_t len) {
        writtenData.append(data, len);
    };

    std::string out;
    SshResult res = client.drainUntilPromptForTesting(reader, writer, out, 500);
    CHECK_EQUAL(static_cast<int>(SshResult::ERR_INTERRUPTED), static_cast<int>(res));
    STRCMP_EQUAL("\x03\n", writtenData.c_str());
}

TEST(ZyxelSshClientTest, DrainUntilPromptHandlesAnsiMorePagination) {
    ZyxelSshClient client;
    int step = 0;
    std::string writtenData;
    std::string page1 = "Line 1\r\nLine 2\r\n\x1b[7m--More--\x1b[m";
    std::string page2 = "Line 3\r\nLine 4\r\nRouter> ";

    auto reader = [&](char *buf, size_t buflen, bool &eofOut) -> ssize_t {
        (void)buflen;
        eofOut = false;
        step++;
        if (step == 1) {
            std::memcpy(buf, page1.data(), page1.size());
            return static_cast<ssize_t>(page1.size());
        } else if (step == 2) {
            if (writtenData == " ") {
                std::memcpy(buf, page2.data(), page2.size());
                return static_cast<ssize_t>(page2.size());
            }
            return 0;
        }
        return 0;
    };

    auto writer = [&](const char *data, size_t len) {
        writtenData.append(data, len);
    };

    std::string out;
    SshResult res = client.drainUntilPromptForTesting(reader, writer, out, 1000);
    CHECK_EQUAL(static_cast<int>(SshResult::SUCCESS), static_cast<int>(res));
    STRCMP_EQUAL(" ", writtenData.c_str());
    CHECK_TRUE(out.find("Line 1") != std::string::npos);
    CHECK_TRUE(out.find("Line 2") != std::string::npos);
    CHECK_TRUE(out.find("Line 3") != std::string::npos);
    CHECK_TRUE(out.find("Line 4") != std::string::npos);
    CHECK_TRUE(out.find("--More--") == std::string::npos);
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
