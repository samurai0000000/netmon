/*
 * TestZyxelSystemCmd.cxx
 *
 * Copyright (C) 2026, Charles Chiou
 */

#include "zyxel/ZyxelSystemCmd.hxx"
#include <CppUTest/TestHarness.h>

TEST_GROUP(ZyxelSystemCmdTest) {
    void setup() {}
    void teardown() {}
};

TEST(ZyxelSystemCmdTest, CommandGeneratorsReturnExpectedStrings) {
    STRCMP_EQUAL("show version", ZyxelSystemCmd::cmdShowVersion().c_str());
    STRCMP_EQUAL("show cpu status", ZyxelSystemCmd::cmdShowCpuStatus().c_str());
    STRCMP_EQUAL("show mem status", ZyxelSystemCmd::cmdShowMemStatus().c_str());
    STRCMP_EQUAL("show conn status", ZyxelSystemCmd::cmdShowConnStatus().c_str());
    STRCMP_EQUAL("ping 1.1.1.1 count 4", ZyxelSystemCmd::cmdPing("1.1.1.1", 4).c_str());
    STRCMP_EQUAL("traceroute 8.8.8.8", ZyxelSystemCmd::cmdTraceroute("8.8.8.8").c_str());
    STRCMP_EQUAL("write", ZyxelSystemCmd::cmdWrite().c_str());
    STRCMP_EQUAL("reboot", ZyxelSystemCmd::cmdReboot().c_str());
}

TEST(ZyxelSystemCmdTest, ParseVersionManualPage51Transcript) {
    // Official transcript from ZyWALL ZLD CLI Reference Guide, page 51
    std::string raw =
        "Zyxel Communications Corp.\n"
        "model           : ZyWALL USG 110\n"
        "firmware version: 2.20(AQQ.0)b3\n"
        "BM version      : 1.08\n"
        "build date      : 2014-01-21 01:18:06\n";

    ZyxelVersionInfo info;
    CHECK_TRUE(ZyxelSystemCmd::parseVersion(raw, info));
    STRCMP_EQUAL("ZyWALL USG 110", info.model.c_str());
    STRCMP_EQUAL("2.20(AQQ.0)b3", info.firmwareVersion.c_str());
    STRCMP_EQUAL("2014-01-21 01:18:06", info.buildDate.c_str());
}

TEST(ZyxelSystemCmdTest, ParseVersionDualFirmwareTablePage547) {
    // Official transcript from manual page 547
    std::string raw =
        "Zyxel Communications Corp.\n"
        "image number model                            firmware version\n"
        "build date           boot status\n"
        "===============================================================================\n"
        "1            USG110                           V4.11(AAPH.0)b3s1\n"
        "2015-01-11 21:53:44  Standby\n"
        "2            USG110                           V4.11(AAPH.0)\n"
        "2015-03-13 03:47:52  Running\n";

    ZyxelVersionInfo info;
    CHECK_TRUE(ZyxelSystemCmd::parseVersion(raw, info));
    STRCMP_EQUAL("USG110", info.model.c_str());
    STRCMP_EQUAL("V4.11(AAPH.0)", info.firmwareVersion.c_str());
    STRCMP_EQUAL("2015-03-13 03:47:52", info.buildDate.c_str());
}

TEST(ZyxelSystemCmdTest, ParseVersionUsgFlex200LiveTranscript) {
    // Live transcript from physical USG FLEX 200
    std::string raw =
        "Zyxel Communications Corp.\n"
        "image number model                            firmware version                                                  build date           boot status         \n"
        "===============================================================================\n"
        "1            USG FLEX 200                     V5.43(ABUI.0)                                                     2026-07-25 02:30:50  Running              \n"
        "2            USG FLEX 200                     V5.42(ABUI.1)                                                     2026-02-08 02:21:09  Standby              \n"
        "Router> ";

    ZyxelVersionInfo info;
    CHECK_TRUE(ZyxelSystemCmd::parseVersion(raw, info));
    STRCMP_EQUAL("USG FLEX 200", info.model.c_str());
    STRCMP_EQUAL("V5.43(ABUI.0)", info.firmwareVersion.c_str());
    STRCMP_EQUAL("2026-07-25 02:30:50", info.buildDate.c_str());
}

TEST(ZyxelSystemCmdTest, ParseCpuStatusManualPage48) {
    // Official transcript from manual page 48
    std::string raw =
        "Router> show cpu status\n"
        "CPU utilization: 11 %\n"
        "CPU utilization for 1 min: 2 %\n"
        "CPU utilization for 5 min: 2 %\n";

    double cpuPercent = 0.0;
    CHECK_TRUE(ZyxelSystemCmd::parseCpuStatus(raw, cpuPercent));
    DOUBLES_EQUAL(11.0, cpuPercent, 0.01);
}

TEST(ZyxelSystemCmdTest, ParseMemStatusManualPage48) {
    // Official transcript from manual page 48
    std::string raw =
        "Router(config)# show mem status\n"
        "memory usage: 39%\n";

    double memPercent = 0.0;
    CHECK_TRUE(ZyxelSystemCmd::parseMemStatus(raw, memPercent));
    DOUBLES_EQUAL(39.0, memPercent, 0.01);
}

TEST(ZyxelSystemCmdTest, ParseConnStatusStandardAndSlashFormat) {
    std::string raw1 =
        "Active sessions: 142\n"
        "Max sessions: 1000000\n";
    ZyxelSessionSummary sum1;
    CHECK_TRUE(ZyxelSystemCmd::parseConnStatus(raw1, sum1));
    LONGS_EQUAL(142, sum1.activeSessions);
    LONGS_EQUAL(1000000, sum1.maxSessions);
    DOUBLES_EQUAL(0.0142, sum1.sessionUsagePercent, 0.0001);

    std::string raw2 = "Current sessions: 250 / 50000\n";
    ZyxelSessionSummary sum2;
    CHECK_TRUE(ZyxelSystemCmd::parseConnStatus(raw2, sum2));
    LONGS_EQUAL(250, sum2.activeSessions);
    LONGS_EQUAL(50000, sum2.maxSessions);
    DOUBLES_EQUAL(0.5, sum2.sessionUsagePercent, 0.01);

    std::string raw3 =
        "Active Session Number: 1319\n"
        "Optimized DPI Session Number: 600000\n"
        "Support Session Number: 600000\n"
        "Router> \n";
    ZyxelSessionSummary sum3;
    CHECK_TRUE(ZyxelSystemCmd::parseConnStatus(raw3, sum3));
    LONGS_EQUAL(1319, sum3.activeSessions);
    LONGS_EQUAL(600000, sum3.maxSessions);
    DOUBLES_EQUAL(0.2198, sum3.sessionUsagePercent, 0.001);
}

TEST(ZyxelSystemCmdTest, ParsePingTranscript) {
    std::string raw =
        "PING 1.1.1.1 (1.1.1.1) 56(84) bytes of data.\n"
        "64 bytes from 1.1.1.1: icmp_seq=1 ttl=57 time=12.4 ms\n"
        "64 bytes from 1.1.1.1: icmp_seq=2 ttl=57 time=11.8 ms\n"
        "\n"
        "--- 1.1.1.1 ping statistics ---\n"
        "4 packets transmitted, 4 received, 0% packet loss, time 3004ms\n"
        "rtt min/avg/max/mdev = 11.821/12.145/12.482/0.250 ms\n";

    ZyxelDiagnosticResult res;
    CHECK_TRUE(ZyxelSystemCmd::parsePing(raw, res));
    STRCMP_EQUAL("ping", res.type.c_str());
    LONGS_EQUAL(4, res.packetsTransmitted);
    LONGS_EQUAL(4, res.packetsReceived);
    DOUBLES_EQUAL(0.0, res.packetLossPercent, 0.01);
    DOUBLES_EQUAL(11.821, res.minLatencyMs, 0.001);
    DOUBLES_EQUAL(12.145, res.avgLatencyMs, 0.001);
    DOUBLES_EQUAL(12.482, res.maxLatencyMs, 0.001);
}

TEST(ZyxelSystemCmdTest, ParseTracerouteTranscript) {
    std::string raw =
        "traceroute to 1.1.1.1 (1.1.1.1), 30 hops max, 60 byte packets\n"
        " 1  192.0.2.1 (192.0.2.1)  0.612 ms  0.590 ms  0.580 ms\n"
        " 2  10.0.0.1 (10.0.0.1)  2.145 ms  2.120 ms  2.110 ms\n"
        " 3  1.1.1.1 (1.1.1.1)  11.512 ms  11.480 ms  11.450 ms\n";

    ZyxelDiagnosticResult res;
    CHECK_TRUE(ZyxelSystemCmd::parseTraceroute(raw, res));
    STRCMP_EQUAL("traceroute", res.type.c_str());
    LONGS_EQUAL(3, res.packetsReceived);
}

TEST(ZyxelSystemCmdTest, ParseTracerouteStripsTrailingPrompt) {
    std::string rawWithPrompt =
        "traceroute to 8.8.8.8 (8.8.8.8), 30 hops max, 38 byte packets\n"
        " 1  * * *\n"
        " 2  168.95.105.138  10.370 ms  11.521 ms  11.538 ms\n"
        " 7  8.8.8.8  10.749 ms  10.792 ms  12.105 ms\n"
        "Router> ";

    ZyxelDiagnosticResult res;
    CHECK_TRUE(ZyxelSystemCmd::parseTraceroute(rawWithPrompt, res));
    LONGS_EQUAL(3, res.packetsReceived);
    CHECK_TRUE(res.rawOutput.find("Router>") == std::string::npos);
    CHECK_TRUE(res.rawOutput.find("8.8.8.8") != std::string::npos);
}

TEST(ZyxelSystemCmdTest, ParseTracerouteMultiGatewayECMPAndFlags) {
    std::string raw =
        "--- Zyxel Router Traceroute: 8.8.8.8 ---\n"
        "\n"
        " 1  * * *\n"
        " 2  168.95.105.138  10.370 ms  11.521 ms  11.538 ms\n"
        " 3  220.128.9.82  11.454 ms 220.128.9.214  13.442 ms  13.809 ms\n"
        " 4  * * 220.128.10.125  14.515 ms\n"
        " 5  72.14.209.178  16.490 ms 142.250.169.120  16.564 ms  16.419 ms\n"
        " 6  * * *\n"
        " 7  8.8.8.8  10.749 ms  10.792 ms  12.105 ms\n"
        " 8  192.168.1.1  10.200 ms !H\n"
        "Router> \n";

    ZyxelDiagnosticResult res;
    CHECK_TRUE(ZyxelSystemCmd::parseTraceroute(raw, res));
    LONGS_EQUAL(8, res.hops.size());

    // Hop 1: full timeout
    LONGS_EQUAL(1, res.hops[0].hopIndex);
    CHECK_TRUE(res.hops[0].isCompleteTimeout);
    LONGS_EQUAL(3, res.hops[0].probes.size());
    CHECK_TRUE(res.hops[0].probes[0].timeout);

    // Hop 2: single gateway 3 probes
    LONGS_EQUAL(2, res.hops[1].hopIndex);
    STRCMP_EQUAL("168.95.105.138", res.hops[1].primaryIp.c_str());
    CHECK_FALSE(res.hops[1].isCompleteTimeout);
    LONGS_EQUAL(3, res.hops[1].probes.size());

    // Hop 3: ECMP multi-gateway
    LONGS_EQUAL(3, res.hops[2].hopIndex);
    LONGS_EQUAL(3, res.hops[2].probes.size());
    STRCMP_EQUAL("220.128.9.82", res.hops[2].probes[0].ip.c_str());
    STRCMP_EQUAL("220.128.9.214", res.hops[2].probes[1].ip.c_str());
    STRCMP_EQUAL("220.128.9.214", res.hops[2].probes[2].ip.c_str());

    // Hop 4: partial drop
    LONGS_EQUAL(4, res.hops[3].hopIndex);
    LONGS_EQUAL(3, res.hops[3].probes.size());
    CHECK_TRUE(res.hops[3].probes[0].timeout);
    CHECK_TRUE(res.hops[3].probes[1].timeout);
    CHECK_FALSE(res.hops[3].probes[2].timeout);
    STRCMP_EQUAL("220.128.10.125", res.hops[3].primaryIp.c_str());

    // Hop 8: ICMP flag !H
    LONGS_EQUAL(8, res.hops[7].hopIndex);
    LONGS_EQUAL(1, res.hops[7].probes.size());
    STRCMP_EQUAL("!H", res.hops[7].probes[0].icmpFlag.c_str());

    // Verify JSON serialization includes hops
    auto j = res.toJson();
    CHECK_TRUE(j.contains("hops"));
    LONGS_EQUAL(8, j["hops"].size());
}

TEST(ZyxelSystemCmdTest, ParsePing100PercentPacketLossUnreachable) {
    std::string raw =
        "PING 192.0.2.99 (192.0.2.99) 56(84) bytes of data.\n"
        "From 192.0.2.1 icmp_seq=1 Destination Host Unreachable\n"
        "From 192.0.2.1 icmp_seq=2 Destination Host Unreachable\n"
        "\n"
        "--- 192.0.2.99 ping statistics ---\n"
        "2 packets transmitted, 0 received, +2 errors, 100% packet loss, time 1000ms\n";

    ZyxelDiagnosticResult res;
    CHECK_TRUE(ZyxelSystemCmd::parsePing(raw, res));
    STRCMP_EQUAL("ping", res.type.c_str());
    LONGS_EQUAL(2, res.packetsTransmitted);
    LONGS_EQUAL(0, res.packetsReceived);
    DOUBLES_EQUAL(100.0, res.packetLossPercent, 0.01);
}

TEST(ZyxelSystemCmdTest, ParseErrorsOnCorruptedBuffers) {
    ZyxelVersionInfo v;
    CHECK_FALSE(ZyxelSystemCmd::parseVersion("Random garbage % syntax error", v));

    double cpu = 0.0;
    CHECK_FALSE(ZyxelSystemCmd::parseCpuStatus("Unknown command error", cpu));

    double mem = 0.0;
    CHECK_FALSE(ZyxelSystemCmd::parseMemStatus("Cannot retrieve memory", mem));

    ZyxelSessionSummary s;
    CHECK_FALSE(ZyxelSystemCmd::parseConnStatus("No sessions active", s));

    ZyxelDiagnosticResult p;
    CHECK_FALSE(ZyxelSystemCmd::parsePing("Host unreachable: 100% loss without stats", p));
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
