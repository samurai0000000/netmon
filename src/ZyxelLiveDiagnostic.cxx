/*
 * ZyxelLiveDiagnostic.cxx
 *
 * Copyright (C) 2026, Charles Chiou
 */

#include "ZyxelLiveDiagnostic.hxx"
#include "ZyxelDriver.hxx"
#include "ZyxelSshClient.hxx"
#include "AuthManager.hxx"
#include "Config.hxx"
#include "zyxel/ZyxelSystemCmd.hxx"
#include "zyxel/ZyxelNetworkCmd.hxx"
#include "zyxel/ZyxelObjectCmd.hxx"
#include "zyxel/ZyxelFirewallCmd.hxx"
#include "zyxel/ZyxelNatCmd.hxx"
#include "zyxel/ZyxelSecurityCmd.hxx"
#include "zyxel/ZyxelVpnCmd.hxx"
#include "zyxel/ZyxelAuthCmd.hxx"
#include "zyxel/ZyxelWlanCmd.hxx"

#include "CppUTest/TestHarness.h"
#include "CppUTest/CommandLineTestRunner.h"
#include "CppUTest/TestRegistry.h"
#include "CppUTest/MemoryLeakWarningPlugin.h"

__attribute__((constructor(101))) static void disableCppUTestLeakOverloads() {
    MemoryLeakWarningPlugin::turnOffNewDeleteOverloads();
}

#include <chrono>
#include <fstream>
#include <iomanip>
#include <sstream>
#include <iostream>
#include <mutex>
#include <vector>
#include <string>
#include <ctime>
#include <thread>
#include <algorithm>

static void logDiag(const std::string &msg) {
    std::ofstream ofs("/tmp/netmon_diag.log", std::ios::app);
    if (ofs.is_open()) {
        ofs << msg << std::endl;
    }
}

static std::vector<DiagMetricEntry> s_metrics;
static std::mutex s_metricsMutex;

void ZyxelBenchmark::record(const DiagMetricEntry &entry) {
    std::lock_guard<std::mutex> lock(s_metricsMutex);
    s_metrics.push_back(entry);
}

void ZyxelBenchmark::clear() {
    std::lock_guard<std::mutex> lock(s_metricsMutex);
    s_metrics.clear();
}

const std::vector<DiagMetricEntry> &ZyxelBenchmark::getEntries() {
    std::lock_guard<std::mutex> lock(s_metricsMutex);
    return s_metrics;
}

void ZyxelBenchmark::renderScorecard(std::ostream &os,
                                     const std::string &host,
                                     const std::string &model) {
    std::lock_guard<std::mutex> lock(s_metricsMutex);

    std::time_t now = std::time(nullptr);
    char timeBuf[64];
    std::strftime(timeBuf, sizeof(timeBuf), "%Y-%m-%d %H:%M:%S", std::localtime(&now));

    // Calculate summary statistics
    size_t passedCount = 0;
    size_t failedCount = 0;
    double totalTimeSec = 0.0;
    for (const auto &m : s_metrics) {
        if (m.passed) {
            passedCount++;
        } else {
            failedCount++;
        }
        totalTimeSec += (m.rttMs / 1000.0);
    }

    // Format strictly constrained to 76 characters width for 80x24 terminals
    os << "+--------------------------------------------------------------------------+\n";
    os << "| LIVE FIREWALL QUALIFICATION & BENCHMARK REPORT                           |\n";
    os << "+--------------------------------------------------------------------------+\n";

    std::ostringstream hostLine;
    hostLine << "| Target Host : " << host << " (" << model << ")";
    std::string hStr = hostLine.str();
    if (hStr.length() < 75) {
        hStr.append(75 - hStr.length(), ' ');
    }
    hStr += "|\n";
    os << hStr;

    std::ostringstream timeLine;
    timeLine << "| Timestamp   : " << timeBuf;
    std::string tStr = timeLine.str();
    if (tStr.length() < 75) {
        tStr.append(75 - tStr.length(), ' ');
    }
    tStr += "|\n";
    os << tStr;

    os << "+--------------------------------------------------------------------------+\n";
    os << "| Test Case            | RTT (ms) | Parse(us) | Bytes | Yield | Result     |\n";
    os << "+----------------------+----------+-----------+-------+-------+------------+\n";

    for (const auto &m : s_metrics) {
        char row[80];
        std::string testDisplay = m.testName;
        if (testDisplay.length() > 20) {
            testDisplay = testDisplay.substr(0, 20);
        }
        std::snprintf(row, sizeof(row),
                      "| %-20s | %8.1f | %9lu | %5zu | %5zu | %-10s |",
                      testDisplay.c_str(),
                      m.rttMs,
                      static_cast<unsigned long>(m.parseUs),
                      m.bytes > 99999 ? 99999 : m.bytes,
                      m.entities > 99999 ? 99999 : m.entities,
                      m.statusMsg.substr(0, 10).c_str());
        os << row << "\n";
    }

    os << "+----------------------+----------+-----------+-------+-------+------------+\n";

    char sumBuf[80];
    std::snprintf(sumBuf, sizeof(sumBuf),
                  "| SUMMARY: %zu passed, %zu failed, %zu regressions (Total: %.2fs)",
                  passedCount, failedCount, failedCount, totalTimeSec);
    std::string sLine(sumBuf);
    if (sLine.length() < 75) {
        sLine.append(75 - sLine.length(), ' ');
    }
    sLine += "|\n";
    os << sLine;

    os << "+--------------------------------------------------------------------------+\n";
}

// ============================================================================
// CppUTest Diagnostic Test Group: LiveFirewallReadDiag (12 Read Tests)
// ============================================================================

TEST_GROUP(LiveFirewallReadDiag) {
    void setup() override {
    }

    void teardown() override {
    }
};

TEST(LiveFirewallReadDiag, ReadVersion) {
    ZyxelDriver &driver = ZyxelDriver::getInstance();
    if (!driver.isConnected()) return;

    auto t0 = std::chrono::steady_clock::now();
    std::string out;
    SshResult res = driver.getSshClient().executeCommand(ZyxelSystemCmd::cmdShowVersion(), out);
    auto t1 = std::chrono::steady_clock::now();

    CHECK_EQUAL(static_cast<int>(SshResult::SUCCESS), static_cast<int>(res));
    ZyxelVersionInfo info;
    bool parsed = ZyxelSystemCmd::parseVersion(out, info);
    auto t2 = std::chrono::steady_clock::now();

    double rttMs = std::chrono::duration<double, std::milli>(t1 - t0).count();
    uint64_t parseUs = std::chrono::duration_cast<std::chrono::microseconds>(t2 - t1).count();
    bool passed = parsed && !info.model.empty() && rttMs <= 1500.0 && parseUs <= 5000;

    ZyxelBenchmark::record({
        "LiveFirewallReadDiag", "ReadVersion", "show version",
        rttMs, parseUs, out.size(), parsed ? 1U : 0U, passed,
        passed ? "PASS (SLA)" : "FAIL"
    });

    CHECK_TRUE(parsed);
    CHECK_FALSE(info.model.empty());
    CHECK_TRUE(rttMs <= 1500.0);
}

TEST(LiveFirewallReadDiag, ReadCpuStatus) {
    ZyxelDriver &driver = ZyxelDriver::getInstance();
    if (!driver.isConnected()) return;

    auto t0 = std::chrono::steady_clock::now();
    std::string out;
    SshResult res = driver.getSshClient().executeCommand(ZyxelSystemCmd::cmdShowCpuStatus(), out);
    auto t1 = std::chrono::steady_clock::now();

    CHECK_EQUAL(static_cast<int>(SshResult::SUCCESS), static_cast<int>(res));
    double cpuPct = 0.0;
    bool parsed = ZyxelSystemCmd::parseCpuStatus(out, cpuPct);
    auto t2 = std::chrono::steady_clock::now();

    double rttMs = std::chrono::duration<double, std::milli>(t1 - t0).count();
    uint64_t parseUs = std::chrono::duration_cast<std::chrono::microseconds>(t2 - t1).count();
    bool passed = parsed && cpuPct >= 0.0 && cpuPct <= 100.0 && rttMs <= 1500.0 && parseUs <= 5000;

    ZyxelBenchmark::record({
        "LiveFirewallReadDiag", "ReadCpuStatus", "show cpu status",
        rttMs, parseUs, out.size(), parsed ? 1U : 0U, passed,
        passed ? "PASS (SLA)" : "FAIL"
    });

    CHECK_TRUE(parsed);
    CHECK_TRUE(cpuPct >= 0.0 && cpuPct <= 100.0);
    CHECK_TRUE(rttMs <= 1500.0);
}

TEST(LiveFirewallReadDiag, ReadMemStatus) {
    ZyxelDriver &driver = ZyxelDriver::getInstance();
    if (!driver.isConnected()) return;

    auto t0 = std::chrono::steady_clock::now();
    std::string out;
    SshResult res = driver.getSshClient().executeCommand(ZyxelSystemCmd::cmdShowMemStatus(), out);
    auto t1 = std::chrono::steady_clock::now();

    CHECK_EQUAL(static_cast<int>(SshResult::SUCCESS), static_cast<int>(res));
    double memPct = 0.0;
    bool parsed = ZyxelSystemCmd::parseMemStatus(out, memPct);
    auto t2 = std::chrono::steady_clock::now();

    double rttMs = std::chrono::duration<double, std::milli>(t1 - t0).count();
    uint64_t parseUs = std::chrono::duration_cast<std::chrono::microseconds>(t2 - t1).count();
    bool passed = parsed && memPct >= 0.0 && memPct <= 100.0 && rttMs <= 1500.0 && parseUs <= 5000;

    ZyxelBenchmark::record({
        "LiveFirewallReadDiag", "ReadMemStatus", "show mem status",
        rttMs, parseUs, out.size(), parsed ? 1U : 0U, passed,
        passed ? "PASS (SLA)" : "FAIL"
    });

    CHECK_TRUE(parsed);
    CHECK_TRUE(memPct >= 0.0 && memPct <= 100.0);
    CHECK_TRUE(rttMs <= 1500.0);
}

TEST(LiveFirewallReadDiag, ReadConnStatus) {
    ZyxelDriver &driver = ZyxelDriver::getInstance();
    if (!driver.isConnected()) return;

    auto t0 = std::chrono::steady_clock::now();
    std::string out;
    SshResult res = driver.getSshClient().executeCommand(ZyxelSystemCmd::cmdShowConnStatus(), out);
    auto t1 = std::chrono::steady_clock::now();

    CHECK_EQUAL(static_cast<int>(SshResult::SUCCESS), static_cast<int>(res));
    ZyxelSessionSummary sess;
    bool parsed = ZyxelSystemCmd::parseConnStatus(out, sess);
    auto t2 = std::chrono::steady_clock::now();

    double rttMs = std::chrono::duration<double, std::milli>(t1 - t0).count();
    uint64_t parseUs = std::chrono::duration_cast<std::chrono::microseconds>(t2 - t1).count();
    bool passed = parsed && sess.activeSessions > 0 && rttMs <= 1500.0 && parseUs <= 5000;

    ZyxelBenchmark::record({
        "LiveFirewallReadDiag", "ReadConnStatus", "show conn status",
        rttMs, parseUs, out.size(), parsed ? static_cast<size_t>(sess.activeSessions) : 0U, passed,
        passed ? "PASS (SLA)" : "FAIL"
    });

    CHECK_TRUE(parsed);
    CHECK_TRUE(sess.activeSessions > 0);
    CHECK_TRUE(rttMs <= 1500.0);
}

TEST(LiveFirewallReadDiag, ReadInterfaceSummary) {
    ZyxelDriver &driver = ZyxelDriver::getInstance();
    if (!driver.isConnected()) return;

    auto t0 = std::chrono::steady_clock::now();
    std::string out;
    SshResult res = driver.getSshClient().executeCommand(ZyxelNetworkCmd::cmdShowInterfaces(), out);
    auto t1 = std::chrono::steady_clock::now();

    CHECK_EQUAL(static_cast<int>(SshResult::SUCCESS), static_cast<int>(res));
    std::vector<ZyxelInterfaceInfo> ifaces;
    bool parsed = ZyxelNetworkCmd::parseInterfaces(out, ifaces);
    auto t2 = std::chrono::steady_clock::now();

    double rttMs = std::chrono::duration<double, std::milli>(t1 - t0).count();
    uint64_t parseUs = std::chrono::duration_cast<std::chrono::microseconds>(t2 - t1).count();
    bool passed = parsed && !ifaces.empty() && rttMs <= 2500.0 && parseUs <= 5000;

    ZyxelBenchmark::record({
        "LiveFirewallReadDiag", "ReadInterfaceSummary", "show interface summary all",
        rttMs, parseUs, out.size(), ifaces.size(), passed,
        passed ? "PASS (SLA)" : "FAIL"
    });

    CHECK_TRUE(parsed);
    CHECK_FALSE(ifaces.empty());
    CHECK_TRUE(rttMs <= 2500.0);
}

TEST(LiveFirewallReadDiag, ReadIpRoutes) {
    ZyxelDriver &driver = ZyxelDriver::getInstance();
    if (!driver.isConnected()) return;

    auto t0 = std::chrono::steady_clock::now();
    std::string out;
    SshResult res = driver.getSshClient().executeCommand(ZyxelNetworkCmd::cmdShowIpRoute(), out);
    auto t1 = std::chrono::steady_clock::now();

    CHECK_EQUAL(static_cast<int>(SshResult::SUCCESS), static_cast<int>(res));
    std::vector<ZyxelRouteEntry> routes;
    bool parsed = ZyxelNetworkCmd::parseIpRoutes(out, routes);
    auto t2 = std::chrono::steady_clock::now();

    double rttMs = std::chrono::duration<double, std::milli>(t1 - t0).count();
    uint64_t parseUs = std::chrono::duration_cast<std::chrono::microseconds>(t2 - t1).count();
    bool passed = parsed && !routes.empty() && rttMs <= 2500.0 && parseUs <= 5000;

    ZyxelBenchmark::record({
        "LiveFirewallReadDiag", "ReadIpRoutes", "show ip route",
        rttMs, parseUs, out.size(), routes.size(), passed,
        passed ? "PASS (SLA)" : "FAIL"
    });

    CHECK_TRUE(parsed);
    CHECK_FALSE(routes.empty());
    CHECK_TRUE(rttMs <= 2500.0);
}

TEST(LiveFirewallReadDiag, ReadAddressObjects) {
    ZyxelDriver &driver = ZyxelDriver::getInstance();
    if (!driver.isConnected()) return;

    auto t0 = std::chrono::steady_clock::now();
    std::string out;
    SshResult res = driver.getSshClient().executeCommand(ZyxelObjectCmd::cmdShowAddressObjects(), out);
    auto t1 = std::chrono::steady_clock::now();

    CHECK_EQUAL(static_cast<int>(SshResult::SUCCESS), static_cast<int>(res));
    std::vector<ZyxelAddressObject> objs;
    bool parsed = ZyxelObjectCmd::parseAddressObjects(out, objs);
    auto t2 = std::chrono::steady_clock::now();

    double rttMs = std::chrono::duration<double, std::milli>(t1 - t0).count();
    uint64_t parseUs = std::chrono::duration_cast<std::chrono::microseconds>(t2 - t1).count();
    bool passed = parsed && !objs.empty() && rttMs <= 2500.0 && parseUs <= 5000;

    ZyxelBenchmark::record({
        "LiveFirewallReadDiag", "ReadAddressObjects", "show address-object status",
        rttMs, parseUs, out.size(), objs.size(), passed,
        passed ? "PASS (SLA)" : "FAIL"
    });

    CHECK_TRUE(parsed);
    CHECK_FALSE(objs.empty());
    CHECK_TRUE(rttMs <= 2500.0);
}

TEST(LiveFirewallReadDiag, ReadServiceObjects) {
    ZyxelDriver &driver = ZyxelDriver::getInstance();
    if (!driver.isConnected()) return;

    auto t0 = std::chrono::steady_clock::now();
    std::string out;
    SshResult res = driver.getSshClient().executeCommand(ZyxelObjectCmd::cmdShowServiceObjects(), out);
    auto t1 = std::chrono::steady_clock::now();

    CHECK_EQUAL(static_cast<int>(SshResult::SUCCESS), static_cast<int>(res));
    std::vector<ZyxelServiceObject> svcs;
    bool parsed = ZyxelObjectCmd::parseServiceObjects(out, svcs);
    auto t2 = std::chrono::steady_clock::now();

    double rttMs = std::chrono::duration<double, std::milli>(t1 - t0).count();
    uint64_t parseUs = std::chrono::duration_cast<std::chrono::microseconds>(t2 - t1).count();
    bool passed = parsed && !svcs.empty() && rttMs <= 2500.0 && parseUs <= 5000;

    ZyxelBenchmark::record({
        "LiveFirewallReadDiag", "ReadServiceObjects", "show service-object status",
        rttMs, parseUs, out.size(), svcs.size(), passed,
        passed ? "PASS (SLA)" : "FAIL"
    });

    CHECK_TRUE(parsed);
    CHECK_FALSE(svcs.empty());
    CHECK_TRUE(rttMs <= 2500.0);
}

TEST(LiveFirewallReadDiag, ReadFirewallRules) {
    ZyxelDriver &driver = ZyxelDriver::getInstance();
    if (!driver.isConnected()) return;

    auto t0 = std::chrono::steady_clock::now();
    std::string out;
    SshResult res = driver.getSshClient().executeCommand(ZyxelFirewallCmd::cmdShowSecurePolicy(), out);
    auto t1 = std::chrono::steady_clock::now();

    CHECK_EQUAL(static_cast<int>(SshResult::SUCCESS), static_cast<int>(res));
    std::vector<ZyxelFirewallRule> rules;
    bool parsed = ZyxelFirewallCmd::parseSecurePolicy(out, rules);
    auto t2 = std::chrono::steady_clock::now();

    double rttMs = std::chrono::duration<double, std::milli>(t1 - t0).count();
    uint64_t parseUs = std::chrono::duration_cast<std::chrono::microseconds>(t2 - t1).count();
    bool passed = parsed && !rules.empty() && rttMs <= 3500.0 && parseUs <= 5000;

    ZyxelBenchmark::record({
        "LiveFirewallReadDiag", "ReadFirewallRules", "show secure-policy status",
        rttMs, parseUs, out.size(), rules.size(), passed,
        passed ? "PASS (SLA)" : "FAIL"
    });

    CHECK_TRUE(parsed);
    CHECK_FALSE(rules.empty());
    CHECK_TRUE(rttMs <= 3500.0);
}

TEST(LiveFirewallReadDiag, ReadVirtualServers) {
    ZyxelDriver &driver = ZyxelDriver::getInstance();
    if (!driver.isConnected()) return;

    auto t0 = std::chrono::steady_clock::now();
    std::string out;
    SshResult res = driver.getSshClient().executeCommand(ZyxelNatCmd::cmdShowVirtualServers(), out);
    auto t1 = std::chrono::steady_clock::now();

    CHECK_EQUAL(static_cast<int>(SshResult::SUCCESS), static_cast<int>(res));
    std::vector<ZyxelVirtualServerRule> vs;
    bool parsed = ZyxelNatCmd::parseVirtualServers(out, vs);
    auto t2 = std::chrono::steady_clock::now();

    double rttMs = std::chrono::duration<double, std::milli>(t1 - t0).count();
    uint64_t parseUs = std::chrono::duration_cast<std::chrono::microseconds>(t2 - t1).count();
    bool passed = parsed && rttMs <= 2500.0;

    ZyxelBenchmark::record({
        "LiveFirewallReadDiag", "ReadVirtualServers", "show virtual-server status",
        rttMs, parseUs, out.size(), vs.size(), passed,
        passed ? "PASS (SLA)" : "FAIL"
    });

    CHECK_TRUE(parsed);
    CHECK_TRUE(rttMs <= 2500.0);
}

TEST(LiveFirewallReadDiag, PingGateway) {
    ZyxelDriver &driver = ZyxelDriver::getInstance();
    if (!driver.isConnected()) return;

    std::string targetHost = "127.0.0.1";
    if (!Config::getInstance().getSnmpTargets().empty()) {
        targetHost = Config::getInstance().getSnmpTargets()[0].ip;
    }

    auto t0 = std::chrono::steady_clock::now();
    std::string out;
    SshResult res = driver.getSshClient().executeCommand(ZyxelSystemCmd::cmdPing(targetHost, 2), out);
    auto t1 = std::chrono::steady_clock::now();

    CHECK_EQUAL(static_cast<int>(SshResult::SUCCESS), static_cast<int>(res));
    ZyxelDiagnosticResult diag;
    bool parsed = ZyxelSystemCmd::parsePing(out, diag);
    auto t2 = std::chrono::steady_clock::now();

    double rttMs = std::chrono::duration<double, std::milli>(t1 - t0).count();
    uint64_t parseUs = std::chrono::duration_cast<std::chrono::microseconds>(t2 - t1).count();
    bool passed = parsed && diag.packetLossPercent == 0.0 && rttMs <= 5000.0;

    ZyxelBenchmark::record({
        "LiveFirewallReadDiag", "PingGateway", "ping " + targetHost + " count 2",
        rttMs, parseUs, out.size(), parsed ? 1U : 0U, passed,
        passed ? "PASS (SLA)" : "FAIL"
    });

    CHECK_TRUE(parsed);
    CHECK_TRUE(diag.packetLossPercent == 0.0);
    CHECK_TRUE(rttMs <= 5000.0);
}

// ============================================================================
// CppUTest Diagnostic Test Group: LiveFirewallWriteDiag (10 Non-Destructive Tests)
// ============================================================================

TEST_GROUP(LiveFirewallWriteDiag) {
    void setup() override {
        ZyxelDriver &driver = ZyxelDriver::getInstance();
        if (driver.isConnected()) {
            driver.getSshClient().unwindToRootPrompt();
            ZyxelSshClient &ssh = driver.getSshClient();
            std::string out;

            // Dynamically sweep and remove any orphaned QA rules from prior runs
            ssh.executeCommand(ZyxelFirewallCmd::cmdShowSecurePolicy(), out);
            std::vector<ZyxelFirewallRule> rules;
            if (ZyxelFirewallCmd::parseSecurePolicy(out, rules)) {
                ssh.executeCommand("configure terminal", out);
                for (const auto &r : rules) {
                    if (r.sourceIp.find("NETMON_") != std::string::npos ||
                        r.name.find("NETMON_") != std::string::npos ||
                        r.description.find("NETMON_") != std::string::npos ||
                        r.description.find("Live diag") != std::string::npos) {
                        ssh.executeCommand(ZyxelFirewallCmd::cmdDeleteRule(r.name), out);
                    }
                }
                ssh.unwindToRootPrompt();
            }

            ssh.executeCommand("configure terminal", out);
            ssh.executeCommand(ZyxelFirewallCmd::cmdDeleteRule("NETMON_QA_RULE"), out);
            ssh.executeCommand(ZyxelFirewallCmd::cmdDeleteRule("NETMON_RULE_192_0_2_220"), out);
            ssh.executeCommand("no ip virtual-server TEST", out);
            ssh.executeCommand(ZyxelNatCmd::cmdDeleteVirtualServer("NETMON_QA_VS"), out);
            ssh.executeCommand("no address-object NETMON_QA_HOST", out);
            ssh.executeCommand("no address-object NETMON_QA_RNG", out);
            ssh.executeCommand("no address-object NETMON_QA_SUBNET", out);
            ssh.executeCommand("no address-object NETMON_QA_RULE_HOST", out);
            ssh.executeCommand("no address-object NETMON_BLK_192_0_2_220", out);
            ssh.executeCommand("no address-object NETMON_QA_WRAP", out);
            ssh.executeCommand("no service-object NETMON_QA_TCP", out);
            ssh.executeCommand("no service-object NETMON_QA_UDP", out);
            ssh.executeCommand("no service-object NETMON_QA_VS_SVC", out);
            ssh.unwindToRootPrompt();
        }
    }

    void teardown() override {
        // Enforce absolute RAII safety: unwind config mode and delete any residual test objects
        ZyxelDriver &driver = ZyxelDriver::getInstance();
        if (!driver.isConnected()) return;

        ZyxelSshClient &ssh = driver.getSshClient();
        std::string out;

        // Dynamically sweep and remove any remaining QA rules
        ssh.executeCommand(ZyxelFirewallCmd::cmdShowSecurePolicy(), out);
        std::vector<ZyxelFirewallRule> rules;
        if (ZyxelFirewallCmd::parseSecurePolicy(out, rules)) {
            ssh.executeCommand("configure terminal", out);
            for (const auto &r : rules) {
                if (r.sourceIp.find("NETMON_") != std::string::npos ||
                    r.name.find("NETMON_") != std::string::npos ||
                    r.description.find("NETMON_") != std::string::npos ||
                    r.description.find("Live diag") != std::string::npos) {
                    ssh.executeCommand(ZyxelFirewallCmd::cmdDeleteRule(r.name), out);
                }
            }
            ssh.unwindToRootPrompt();
        }

        ssh.executeCommand("configure terminal", out);
        ssh.executeCommand(ZyxelFirewallCmd::cmdDeleteRule("NETMON_QA_RULE"), out);
        ssh.executeCommand(ZyxelFirewallCmd::cmdDeleteRule("NETMON_RULE_192_0_2_220"), out);
        ssh.executeCommand("no ip virtual-server TEST", out);
        ssh.executeCommand(ZyxelNatCmd::cmdDeleteVirtualServer("NETMON_QA_VS"), out);
        ssh.executeCommand("no address-object NETMON_QA_HOST", out);
        ssh.executeCommand("no address-object NETMON_QA_RNG", out);
        ssh.executeCommand("no address-object NETMON_QA_SUBNET", out);
        ssh.executeCommand("no address-object NETMON_QA_RULE_HOST", out);
        ssh.executeCommand("no address-object NETMON_BLK_192_0_2_220", out);
        ssh.executeCommand("no address-object NETMON_QA_WRAP", out);
        ssh.executeCommand("no service-object NETMON_QA_TCP", out);
        ssh.executeCommand("no service-object NETMON_QA_UDP", out);
        ssh.executeCommand("no service-object NETMON_QA_VS_SVC", out);
        ssh.unwindToRootPrompt();
    }
};

TEST(LiveFirewallWriteDiag, WriteAddressHostRollback) {
    ZyxelDriver &driver = ZyxelDriver::getInstance();
    if (!driver.isConnected()) return;
    ZyxelSshClient &ssh = driver.getSshClient();

    auto t0 = std::chrono::steady_clock::now();
    std::string out;

    // 1. Enter config and create host
    ssh.executeCommand("configure terminal", out);
    SshResult resAdd = ssh.executeCommand("address-object NETMON_QA_HOST 192.0.2.201", out);
    ssh.unwindToRootPrompt();
    CHECK_EQUAL(static_cast<int>(SshResult::SUCCESS), static_cast<int>(resAdd));

    // 2. Verify creation in address-object table
    ssh.executeCommand(ZyxelObjectCmd::cmdShowAddressObjects(), out);
    CHECK_TRUE(out.find("NETMON_QA_HOST") != std::string::npos);

    // 3. Rollback
    ssh.executeCommand("configure terminal", out);
    SshResult resDel = ssh.executeCommand("no address-object NETMON_QA_HOST", out);
    ssh.unwindToRootPrompt();
    CHECK_EQUAL(static_cast<int>(SshResult::SUCCESS), static_cast<int>(resDel));

    // 4. Verify clean deletion
    ssh.executeCommand(ZyxelObjectCmd::cmdShowAddressObjects(), out);
    bool deleted = (out.find("NETMON_QA_HOST") == std::string::npos);
    CHECK_TRUE(deleted);

    auto t1 = std::chrono::steady_clock::now();
    double rttMs = std::chrono::duration<double, std::milli>(t1 - t0).count();
    bool passed = deleted && (rttMs <= 4500.0);

    ZyxelBenchmark::record({
        "LiveFirewallWriteDiag", "WriteAddressHostRollback", "address-object NETMON_QA_HOST",
        rttMs, 0, out.size(), 1, passed,
        passed ? "PASS (SLA)" : "FAIL"
    });
    CHECK_TRUE(rttMs <= 4500.0);
}

TEST(LiveFirewallWriteDiag, WriteAddressRangeRollback) {
    ZyxelDriver &driver = ZyxelDriver::getInstance();
    if (!driver.isConnected()) return;
    ZyxelSshClient &ssh = driver.getSshClient();

    auto t0 = std::chrono::steady_clock::now();
    std::string out;

    // 1. Create range
    ssh.executeCommand("configure terminal", out);
    SshResult resAdd = ssh.executeCommand("address-object NETMON_QA_RNG 192.0.2.202-192.0.2.205", out);
    ssh.unwindToRootPrompt();
    CHECK_EQUAL(static_cast<int>(SshResult::SUCCESS), static_cast<int>(resAdd));

    // 2. Verify creation
    ssh.executeCommand(ZyxelObjectCmd::cmdShowAddressObjects(), out);
    CHECK_TRUE(out.find("NETMON_QA_RNG") != std::string::npos);

    // 3. Rollback
    ssh.executeCommand("configure terminal", out);
    SshResult resDel = ssh.executeCommand("no address-object NETMON_QA_RNG", out);
    ssh.unwindToRootPrompt();
    CHECK_EQUAL(static_cast<int>(SshResult::SUCCESS), static_cast<int>(resDel));

    // 4. Verify clean deletion
    ssh.executeCommand(ZyxelObjectCmd::cmdShowAddressObjects(), out);
    bool deleted = (out.find("NETMON_QA_RNG") == std::string::npos);
    CHECK_TRUE(deleted);

    auto t1 = std::chrono::steady_clock::now();
    double rttMs = std::chrono::duration<double, std::milli>(t1 - t0).count();
    bool passed = deleted && (rttMs <= 4500.0);

    ZyxelBenchmark::record({
        "LiveFirewallWriteDiag", "WriteAddressRangeRollback", "address-object NETMON_QA_RNG",
        rttMs, 0, out.size(), 1, passed,
        passed ? "PASS (SLA)" : "FAIL"
    });
    CHECK_TRUE(rttMs <= 4500.0);
}

TEST(LiveFirewallWriteDiag, WriteAddressSubnetRollback) {
    ZyxelDriver &driver = ZyxelDriver::getInstance();
    if (!driver.isConnected()) return;
    ZyxelSshClient &ssh = driver.getSshClient();

    auto t0 = std::chrono::steady_clock::now();
    std::string out;

    // 1. Create subnet
    ssh.executeCommand("configure terminal", out);
    SshResult resAdd = ssh.executeCommand("address-object NETMON_QA_SUBNET 192.0.2.224/28", out);
    ssh.unwindToRootPrompt();
    CHECK_EQUAL(static_cast<int>(SshResult::SUCCESS), static_cast<int>(resAdd));

    // 2. Verify creation
    ssh.executeCommand(ZyxelObjectCmd::cmdShowAddressObjects(), out);
    CHECK_TRUE(out.find("NETMON_QA_SUBNET") != std::string::npos);

    // 3. Rollback
    ssh.executeCommand("configure terminal", out);
    SshResult resDel = ssh.executeCommand("no address-object NETMON_QA_SUBNET", out);
    ssh.unwindToRootPrompt();
    CHECK_EQUAL(static_cast<int>(SshResult::SUCCESS), static_cast<int>(resDel));

    // 4. Verify clean deletion
    ssh.executeCommand(ZyxelObjectCmd::cmdShowAddressObjects(), out);
    bool deleted = (out.find("NETMON_QA_SUBNET") == std::string::npos);
    CHECK_TRUE(deleted);

    auto t1 = std::chrono::steady_clock::now();
    double rttMs = std::chrono::duration<double, std::milli>(t1 - t0).count();
    bool passed = deleted && (rttMs <= 4500.0);

    ZyxelBenchmark::record({
        "LiveFirewallWriteDiag", "WriteAddressSubnetRollback", "address-object NETMON_QA_SUBNET",
        rttMs, 0, out.size(), 1, passed,
        passed ? "PASS (SLA)" : "FAIL"
    });
    CHECK_TRUE(rttMs <= 4500.0);
}

TEST(LiveFirewallWriteDiag, WriteServiceTcpRollback) {
    ZyxelDriver &driver = ZyxelDriver::getInstance();
    if (!driver.isConnected()) return;
    ZyxelSshClient &ssh = driver.getSshClient();

    auto t0 = std::chrono::steady_clock::now();
    std::string out;

    // 1. Create TCP service
    ssh.executeCommand("configure terminal", out);
    SshResult resAdd = ssh.executeCommand("service-object NETMON_QA_TCP tcp eq 65432", out);
    ssh.unwindToRootPrompt();
    CHECK_EQUAL(static_cast<int>(SshResult::SUCCESS), static_cast<int>(resAdd));

    // 2. Verify creation
    ssh.executeCommand(ZyxelObjectCmd::cmdShowServiceObjects(), out);
    CHECK_TRUE(out.find("NETMON_QA_TCP") != std::string::npos);

    // 3. Rollback
    ssh.executeCommand("configure terminal", out);
    SshResult resDel = ssh.executeCommand("no service-object NETMON_QA_TCP", out);
    ssh.unwindToRootPrompt();
    CHECK_EQUAL(static_cast<int>(SshResult::SUCCESS), static_cast<int>(resDel));

    // 4. Verify clean deletion
    ssh.executeCommand(ZyxelObjectCmd::cmdShowServiceObjects(), out);
    bool deleted = (out.find("NETMON_QA_TCP") == std::string::npos);
    CHECK_TRUE(deleted);

    auto t1 = std::chrono::steady_clock::now();
    double rttMs = std::chrono::duration<double, std::milli>(t1 - t0).count();
    bool passed = deleted && (rttMs <= 4500.0);

    ZyxelBenchmark::record({
        "LiveFirewallWriteDiag", "WriteServiceTcpRollback", "service-object NETMON_QA_TCP",
        rttMs, 0, out.size(), 1, passed,
        passed ? "PASS (SLA)" : "FAIL"
    });
    CHECK_TRUE(rttMs <= 4500.0);
}

TEST(LiveFirewallWriteDiag, WriteServiceUdpRollback) {
    ZyxelDriver &driver = ZyxelDriver::getInstance();
    if (!driver.isConnected()) return;
    ZyxelSshClient &ssh = driver.getSshClient();

    auto t0 = std::chrono::steady_clock::now();
    std::string out;

    // 1. Create UDP service
    ssh.executeCommand("configure terminal", out);
    SshResult resAdd = ssh.executeCommand("service-object NETMON_QA_UDP udp eq 65432", out);
    ssh.unwindToRootPrompt();
    CHECK_EQUAL(static_cast<int>(SshResult::SUCCESS), static_cast<int>(resAdd));

    // 2. Verify creation
    ssh.executeCommand(ZyxelObjectCmd::cmdShowServiceObjects(), out);
    CHECK_TRUE(out.find("NETMON_QA_UDP") != std::string::npos);

    // 3. Rollback
    ssh.executeCommand("configure terminal", out);
    SshResult resDel = ssh.executeCommand("no service-object NETMON_QA_UDP", out);
    ssh.unwindToRootPrompt();
    CHECK_EQUAL(static_cast<int>(SshResult::SUCCESS), static_cast<int>(resDel));

    // 4. Verify clean deletion
    ssh.executeCommand(ZyxelObjectCmd::cmdShowServiceObjects(), out);
    bool deleted = (out.find("NETMON_QA_UDP") == std::string::npos);
    CHECK_TRUE(deleted);

    auto t1 = std::chrono::steady_clock::now();
    double rttMs = std::chrono::duration<double, std::milli>(t1 - t0).count();
    bool passed = deleted && (rttMs <= 4500.0);

    ZyxelBenchmark::record({
        "LiveFirewallWriteDiag", "WriteServiceUdpRollback", "service-object NETMON_QA_UDP",
        rttMs, 0, out.size(), 1, passed,
        passed ? "PASS (SLA)" : "FAIL"
    });
    CHECK_TRUE(rttMs <= 4500.0);
}

TEST(LiveFirewallWriteDiag, WriteVirtualServerRollback) {
    ZyxelDriver &driver = ZyxelDriver::getInstance();
    if (!driver.isConnected()) return;
    ZyxelSshClient &ssh = driver.getSshClient();

    auto t0 = std::chrono::steady_clock::now();
    std::string out;
    logDiag("=== WriteVirtualServerRollback START ===");

    // 1. Pre-create dummy service
    ssh.executeCommand("configure terminal", out);
    ssh.executeCommand(ZyxelObjectCmd::cmdAddService("NETMON_QA_VS_SVC", "tcp", 65433), out);
    logDiag("add service out: " + out);
    ssh.unwindToRootPrompt();

    // 2. Create virtual-server using ZyxelNatCmd::cmdAddVirtualServer
    ZyxelVirtualServerRule rule;
    rule.name = "NETMON_QA_VS";
    rule.interface = "wan1_ppp";
    rule.originalIp = "192.0.2.1";
    rule.mapToIp = "192.0.2.2";
    rule.originalService = "NETMON_QA_VS_SVC";
    rule.mappedService = "NETMON_QA_VS_SVC";
    rule.active = true;
    std::string cmdAdd = ZyxelNatCmd::cmdAddVirtualServer(rule);
    logDiag("cmdAdd: " + cmdAdd);
    ssh.executeCommand("configure terminal", out);
    SshResult resAdd = ssh.executeCommand(cmdAdd, out);
    logDiag("resAdd=" + std::to_string(static_cast<int>(resAdd)) + " out: " + out);
    ssh.unwindToRootPrompt();
    CHECK_EQUAL(static_cast<int>(SshResult::SUCCESS), static_cast<int>(resAdd));

    // 3. Verify creation
    ssh.executeCommand(ZyxelNatCmd::cmdShowVirtualServers(), out);
    logDiag("show vs: " + out);
    CHECK_TRUE(out.find("virtual server: NETMON_QA_VS") != std::string::npos);

    // 4. Rollback virtual server then service
    ssh.executeCommand("configure terminal", out);
    SshResult resDel = ssh.executeCommand(ZyxelNatCmd::cmdDeleteVirtualServer("NETMON_QA_VS"), out);
    logDiag("del vs out: " + out);
    ssh.executeCommand(ZyxelObjectCmd::cmdDeleteService("NETMON_QA_VS_SVC"), out);
    logDiag("del svc out: " + out);
    ssh.unwindToRootPrompt();
    CHECK_EQUAL(static_cast<int>(SshResult::SUCCESS), static_cast<int>(resDel));

    // 5. Verify deletion
    ssh.executeCommand(ZyxelNatCmd::cmdShowVirtualServers(), out);
    bool deleted = (out.find("virtual server: NETMON_QA_VS") == std::string::npos);
    logDiag("vs deleted=" + std::string(deleted ? "true" : "false"));
    CHECK_TRUE(deleted);

    auto t1 = std::chrono::steady_clock::now();
    double rttMs = std::chrono::duration<double, std::milli>(t1 - t0).count();
    bool passed = deleted && (rttMs <= 6000.0);

    ZyxelBenchmark::record({
        "LiveFirewallWriteDiag", "WriteVirtualServerRollback", "ip virtual-server NETMON_QA_VS",
        rttMs, 0, out.size(), 1, passed,
        passed ? "PASS (SLA)" : "FAIL"
    });
    CHECK_TRUE(rttMs <= 6000.0);
}

TEST(LiveFirewallWriteDiag, WriteFastDenyRuleRollback) {
    ZyxelDriver &driver = ZyxelDriver::getInstance();
    if (!driver.isConnected()) return;
    ZyxelSshClient &ssh = driver.getSshClient();

    auto t0 = std::chrono::steady_clock::now();
    std::string out;
    logDiag("=== WriteFastDenyRuleRollback START ===");

    // 1. Create source address object
    ssh.executeCommand("configure terminal", out);
    ssh.executeCommand("address-object NETMON_QA_RULE_HOST 192.0.2.210", out);
    ssh.unwindToRootPrompt();

    // 2. Append rule using ZyxelFirewallCmd::cmdAppendFastDeny
    ssh.executeCommand("configure terminal", out);
    std::vector<std::string> cmds = ZyxelFirewallCmd::cmdAppendFastDeny("NETMON_QA_RULE", "NETMON_QA_RULE_HOST", "Live diag QA test");
    for (const auto &c : cmds) {
        ssh.executeCommand(c, out);
        logDiag("fastDeny cmd [" + c + "] out: [" + out + "]");
    }
    ssh.unwindToRootPrompt();

    // 3. Verify rule creation
    ssh.executeCommand(ZyxelFirewallCmd::cmdShowSecurePolicy(), out);
    logDiag("show secure-policy: " + out);
    CHECK_TRUE(out.find("name: NETMON_QA_RULE") != std::string::npos);

    // 4. Rollback
    ssh.executeCommand("configure terminal", out);
    ssh.executeCommand(ZyxelFirewallCmd::cmdDeleteRuleByName("NETMON_QA_RULE"), out);
    logDiag("del rule out: " + out);
    ssh.executeCommand(ZyxelObjectCmd::cmdDeleteAddress("NETMON_QA_RULE_HOST"), out);
    logDiag("del addr out: " + out);
    ssh.unwindToRootPrompt();

    // 5. Verify deletion
    ssh.executeCommand(ZyxelFirewallCmd::cmdShowSecurePolicy(), out);
    bool deleted = (out.find("name: NETMON_QA_RULE") == std::string::npos);
    logDiag("rule deleted=" + std::string(deleted ? "true" : "false"));
    CHECK_TRUE(deleted);

    auto t1 = std::chrono::steady_clock::now();
    double rttMs = std::chrono::duration<double, std::milli>(t1 - t0).count();
    bool passed = deleted && (rttMs <= 6000.0);

    ZyxelBenchmark::record({
        "LiveFirewallWriteDiag", "WriteFastDenyRuleRollback", "secure-policy NETMON_QA_RULE",
        rttMs, 0, out.size(), 1, passed,
        passed ? "PASS (SLA)" : "FAIL"
    });
    CHECK_TRUE(rttMs <= 6000.0);
}

TEST(LiveFirewallWriteDiag, DriverBlockIpAndUnblockIp) {
    ZyxelDriver &driver = ZyxelDriver::getInstance();
    if (!driver.isConnected()) return;

    auto t0 = std::chrono::steady_clock::now();

    bool prevLive = driver.isLiveEnabled();
    struct LiveGuard {
        ZyxelDriver &d;
        bool prev;
        LiveGuard(ZyxelDriver &drv, bool p) : d(drv), prev(p) { d.setLiveEnabled(true); }
        ~LiveGuard() { d.setLiveEnabled(prev); }
    } liveGuard(driver, prevLive);

    // 1. Call high-level driver blockIp
    nlohmann::json blkRes = driver.blockIp("192.0.2.220", "Live diag qualification test");
    logDiag("blkRes: " + blkRes.dump());
    bool blocked = (blkRes.value("status", "") == "success" || blkRes.value("success", false));
    CHECK_TRUE(blocked);

    // 2. Verify presence in secure policy and address object
    std::string out;
    driver.getSshClient().executeCommand(ZyxelObjectCmd::cmdShowAddressObjects(), out);
    CHECK_TRUE(out.find("NETMON_BLK_192_0_2_220") != std::string::npos);
    driver.getSshClient().executeCommand(ZyxelFirewallCmd::cmdShowSecurePolicy(), out);
    CHECK_TRUE(out.find("name: NETMON_RULE_192_0_2_220") != std::string::npos);

    // 3. Call high-level driver unblockIp
    nlohmann::json unblkRes = driver.unblockIp("192.0.2.220");
    logDiag("unblkRes: " + unblkRes.dump());
    bool unblocked = (unblkRes.value("status", "") == "success" || unblkRes.value("success", false));
    CHECK_TRUE(unblocked);

    // 4. Verify clean deletion
    driver.getSshClient().executeCommand(ZyxelObjectCmd::cmdShowAddressObjects(), out);
    bool objGone = (out.find("NETMON_BLK_192_0_2_220") == std::string::npos);
    driver.getSshClient().executeCommand(ZyxelFirewallCmd::cmdShowSecurePolicy(), out);
    bool ruleGone = (out.find("name: NETMON_RULE_192_0_2_220") == std::string::npos);
    CHECK_TRUE(objGone);
    CHECK_TRUE(ruleGone);

    auto t1 = std::chrono::steady_clock::now();
    double rttMs = std::chrono::duration<double, std::milli>(t1 - t0).count();
    bool passed = blocked && unblocked && objGone && ruleGone && (rttMs <= 6000.0);

    ZyxelBenchmark::record({
        "LiveFirewallWriteDiag", "DriverBlockIpAndUnblockIp", "driver.blockIp / unblockIp",
        rttMs, 0, out.size(), 1, passed,
        passed ? "PASS (SLA)" : "FAIL"
    });
    CHECK_TRUE(rttMs <= 6000.0);
}

TEST(LiveFirewallWriteDiag, ConfigModeWrapping) {
    ZyxelDriver &driver = ZyxelDriver::getInstance();
    if (!driver.isConnected()) return;

    auto t0 = std::chrono::steady_clock::now();
    std::string out;
    std::string matchedPrompt;

    // Test driver transparent configure terminal wrapping for Level 2 command
    SshResult resAdd = driver.executeClearanceCommand("address-object NETMON_QA_WRAP 192.0.2.221", out, matchedPrompt, 5000, true);
    CHECK_EQUAL(static_cast<int>(SshResult::SUCCESS), static_cast<int>(resAdd));

    // Verify driver unwound prompt back to root Router>
    CHECK_TRUE(driver.getLastMatchedPrompt().find("(config") == std::string::npos);

    // Delete object via clearance command
    SshResult resDel = driver.executeClearanceCommand("no address-object NETMON_QA_WRAP", out, matchedPrompt, 5000, true);
    CHECK_EQUAL(static_cast<int>(SshResult::SUCCESS), static_cast<int>(resDel));

    // Verify prompt is root
    CHECK_TRUE(driver.getLastMatchedPrompt().find("(config") == std::string::npos);

    auto t1 = std::chrono::steady_clock::now();
    double rttMs = std::chrono::duration<double, std::milli>(t1 - t0).count();
    bool passed = (resAdd == SshResult::SUCCESS) && (resDel == SshResult::SUCCESS) && (rttMs <= 4500.0);

    ZyxelBenchmark::record({
        "LiveFirewallWriteDiag", "ConfigModeWrapping", "transparent configure terminal",
        rttMs, 0, out.size(), 1, passed,
        passed ? "PASS (SLA)" : "FAIL"
    });
    CHECK_TRUE(rttMs <= 4500.0);
}

TEST(LiveFirewallWriteDiag, SyntaxErrorUnwindAndRecovery) {
    ZyxelDriver &driver = ZyxelDriver::getInstance();
    if (!driver.isConnected()) return;

    auto t0 = std::chrono::steady_clock::now();
    std::string out;
    std::string matchedPrompt;

    // Send intentionally invalid command
    SshResult res = driver.executeClearanceCommand("address-object INVALID??? 999.999.999.999", out, matchedPrompt, 5000, true);
    // Clearance should reject as UNCLASSIFIED (SYNTAX error before transmission)
    CHECK_TRUE(res == SshResult::ERR_SYNTAX || res == SshResult::ERR_EXEC_FAILED || out.find("UNCLASSIFIED") != std::string::npos || out.find("SYNTAX") != std::string::npos);

    // Verify that subsequent read command executes normally and driver session is healthy
    std::string verOut;
    SshResult verRes = driver.getSshClient().executeCommand("show version", verOut);
    CHECK_EQUAL(static_cast<int>(SshResult::SUCCESS), static_cast<int>(verRes));

    auto t1 = std::chrono::steady_clock::now();
    double rttMs = std::chrono::duration<double, std::milli>(t1 - t0).count();
    bool passed = (verRes == SshResult::SUCCESS) && (rttMs <= 2500.0);

    ZyxelBenchmark::record({
        "LiveFirewallWriteDiag", "SyntaxErrorUnwindAndRecovery", "recovery from syntax error",
        rttMs, 0, verOut.size(), 1, passed,
        passed ? "PASS (SLA)" : "FAIL"
    });
    CHECK_TRUE(rttMs <= 2500.0);
}

// ============================================================================
// CppUTest Diagnostic Test Group: LiveFirewallE1 (Envelope 1: System Suite)
// ============================================================================

TEST_GROUP(LiveFirewallE1) {
    void setup() override {
        ZyxelDriver &driver = ZyxelDriver::getInstance();
        if (driver.isConnected()) {
            driver.getSshClient().unwindToRootPrompt();
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(25));
    }

    void teardown() override {
        ZyxelDriver &driver = ZyxelDriver::getInstance();
        if (driver.isConnected()) {
            driver.getSshClient().unwindToRootPrompt();
        }
    }
};

TEST(LiveFirewallE1, ReadVersion) {
    ZyxelDriver &driver = ZyxelDriver::getInstance();
    if (!driver.isConnected()) return;

    auto t0 = std::chrono::steady_clock::now();
    std::string out;
    SshResult res = driver.getSshClient().executeCommand(ZyxelSystemCmd::cmdShowVersion(), out);
    auto t1 = std::chrono::steady_clock::now();

    CHECK_EQUAL(static_cast<int>(SshResult::SUCCESS), static_cast<int>(res));
    ZyxelVersionInfo info;
    bool parsed = ZyxelSystemCmd::parseVersion(out, info);
    auto t2 = std::chrono::steady_clock::now();

    double rttMs = std::chrono::duration<double, std::milli>(t1 - t0).count();
    uint64_t parseUs = std::chrono::duration_cast<std::chrono::microseconds>(t2 - t1).count();
    bool passed = parsed && !info.model.empty() && rttMs <= 1500.0 && parseUs <= 5000;

    ZyxelBenchmark::record({
        "LiveFirewallE1", "ReadVersion", "show version",
        rttMs, parseUs, out.size(), parsed ? 1U : 0U, passed,
        passed ? "PASS (SLA)" : "FAIL"
    });

    CHECK_TRUE(parsed);
    CHECK_FALSE(info.model.empty());
    CHECK_TRUE(rttMs <= 1500.0);
}

TEST(LiveFirewallE1, ReadCpuStatus) {
    ZyxelDriver &driver = ZyxelDriver::getInstance();
    if (!driver.isConnected()) return;

    auto t0 = std::chrono::steady_clock::now();
    std::string out;
    SshResult res = driver.getSshClient().executeCommand(ZyxelSystemCmd::cmdShowCpuStatus(), out);
    auto t1 = std::chrono::steady_clock::now();

    CHECK_EQUAL(static_cast<int>(SshResult::SUCCESS), static_cast<int>(res));
    double cpuPct = 0.0;
    bool parsed = ZyxelSystemCmd::parseCpuStatus(out, cpuPct);
    auto t2 = std::chrono::steady_clock::now();

    double rttMs = std::chrono::duration<double, std::milli>(t1 - t0).count();
    uint64_t parseUs = std::chrono::duration_cast<std::chrono::microseconds>(t2 - t1).count();
    bool passed = parsed && cpuPct >= 0.0 && rttMs <= 1500.0 && parseUs <= 5000;

    ZyxelBenchmark::record({
        "LiveFirewallE1", "ReadCpuStatus", "show cpu status",
        rttMs, parseUs, out.size(), parsed ? 1U : 0U, passed,
        passed ? "PASS (SLA)" : "FAIL"
    });

    CHECK_TRUE(parsed);
    CHECK_TRUE(rttMs <= 1500.0);
}

TEST(LiveFirewallE1, ReadMemStatus) {
    ZyxelDriver &driver = ZyxelDriver::getInstance();
    if (!driver.isConnected()) return;

    auto t0 = std::chrono::steady_clock::now();
    std::string out;
    SshResult res = driver.getSshClient().executeCommand(ZyxelSystemCmd::cmdShowMemStatus(), out);
    auto t1 = std::chrono::steady_clock::now();

    CHECK_EQUAL(static_cast<int>(SshResult::SUCCESS), static_cast<int>(res));
    double memPct = 0.0;
    bool parsed = ZyxelSystemCmd::parseMemStatus(out, memPct);
    auto t2 = std::chrono::steady_clock::now();

    double rttMs = std::chrono::duration<double, std::milli>(t1 - t0).count();
    uint64_t parseUs = std::chrono::duration_cast<std::chrono::microseconds>(t2 - t1).count();
    bool passed = parsed && memPct > 0.0 && rttMs <= 1500.0 && parseUs <= 5000;

    ZyxelBenchmark::record({
        "LiveFirewallE1", "ReadMemStatus", "show mem status",
        rttMs, parseUs, out.size(), parsed ? 1U : 0U, passed,
        passed ? "PASS (SLA)" : "FAIL"
    });

    CHECK_TRUE(parsed);
    CHECK_TRUE(memPct > 0.0);
    CHECK_TRUE(rttMs <= 1500.0);
}

TEST(LiveFirewallE1, ReadConnStatus) {
    ZyxelDriver &driver = ZyxelDriver::getInstance();
    if (!driver.isConnected()) return;

    auto t0 = std::chrono::steady_clock::now();
    std::string out;
    SshResult res = driver.getSshClient().executeCommand(ZyxelSystemCmd::cmdShowConnStatus(), out);
    auto t1 = std::chrono::steady_clock::now();

    CHECK_EQUAL(static_cast<int>(SshResult::SUCCESS), static_cast<int>(res));
    ZyxelSessionSummary summary;
    bool parsed = ZyxelSystemCmd::parseConnStatus(out, summary);
    auto t2 = std::chrono::steady_clock::now();

    double rttMs = std::chrono::duration<double, std::milli>(t1 - t0).count();
    uint64_t parseUs = std::chrono::duration_cast<std::chrono::microseconds>(t2 - t1).count();
    bool passed = parsed && summary.maxSessions > 0 && rttMs <= 1500.0 && parseUs <= 5000;

    ZyxelBenchmark::record({
        "LiveFirewallE1", "ReadConnStatus", "show conn status",
        rttMs, parseUs, out.size(), parsed ? 1U : 0U, passed,
        passed ? "PASS (SLA)" : "FAIL"
    });

    CHECK_TRUE(parsed);
    CHECK_TRUE(summary.maxSessions > 0);
    CHECK_TRUE(rttMs <= 1500.0);
}

TEST(LiveFirewallE1, ReadIpDnsServerStatus) {
    ZyxelDriver &driver = ZyxelDriver::getInstance();
    if (!driver.isConnected()) return;

    auto t0 = std::chrono::steady_clock::now();
    std::string out;
    SshResult res = driver.getSshClient().executeCommand(ZyxelSystemCmd::cmdShowIpDnsServerStatus(), out);
    auto t1 = std::chrono::steady_clock::now();

    CHECK_EQUAL(static_cast<int>(SshResult::SUCCESS), static_cast<int>(res));
    bool active = false;
    bool parsed = ZyxelSystemCmd::parseIpDnsServerStatus(out, active);
    auto t2 = std::chrono::steady_clock::now();

    double rttMs = std::chrono::duration<double, std::milli>(t1 - t0).count();
    uint64_t parseUs = std::chrono::duration_cast<std::chrono::microseconds>(t2 - t1).count();
    bool passed = parsed && rttMs <= 1500.0 && parseUs <= 5000;

    ZyxelBenchmark::record({
        "LiveFirewallE1", "ReadIpDnsServerStatus", "show ip dns server status",
        rttMs, parseUs, out.size(), parsed ? 1U : 0U, passed,
        passed ? "PASS (SLA)" : "FAIL"
    });

    CHECK_TRUE(parsed);
    CHECK_TRUE(rttMs <= 1500.0);
}

TEST(LiveFirewallE1, ReadLoggingStatus) {
    ZyxelDriver &driver = ZyxelDriver::getInstance();
    if (!driver.isConnected()) return;

    auto t0 = std::chrono::steady_clock::now();
    std::string out;
    SshResult res = driver.getSshClient().executeCommand(ZyxelSystemCmd::cmdShowLoggingStatus(), out);
    auto t1 = std::chrono::steady_clock::now();

    CHECK_EQUAL(static_cast<int>(SshResult::SUCCESS), static_cast<int>(res));
    int eventsLogged = 0;
    bool suppression = false;
    bool parsed = ZyxelSystemCmd::parseLoggingStatus(out, eventsLogged, suppression);
    auto t2 = std::chrono::steady_clock::now();

    double rttMs = std::chrono::duration<double, std::milli>(t1 - t0).count();
    uint64_t parseUs = std::chrono::duration_cast<std::chrono::microseconds>(t2 - t1).count();
    bool passed = parsed && rttMs <= 1500.0 && parseUs <= 5000;

    ZyxelBenchmark::record({
        "LiveFirewallE1", "ReadLoggingStatus", "show logging status",
        rttMs, parseUs, out.size(), parsed ? 1U : 0U, passed,
        passed ? "PASS (SLA)" : "FAIL"
    });

    CHECK_TRUE(parsed);
    CHECK_TRUE(rttMs <= 1500.0);
}

TEST(LiveFirewallE1, ReadDisk) {
    ZyxelDriver &driver = ZyxelDriver::getInstance();
    if (!driver.isConnected()) return;

    auto t0 = std::chrono::steady_clock::now();
    std::string out;
    SshResult res = driver.getSshClient().executeCommand(ZyxelSystemCmd::cmdShowDisk(), out);
    auto t1 = std::chrono::steady_clock::now();

    CHECK_EQUAL(static_cast<int>(SshResult::SUCCESS), static_cast<int>(res));
    std::vector<ZyxelDiskEntry> disks;
    bool parsed = ZyxelSystemCmd::parseDisk(out, disks);
    auto t2 = std::chrono::steady_clock::now();

    double rttMs = std::chrono::duration<double, std::milli>(t1 - t0).count();
    uint64_t parseUs = std::chrono::duration_cast<std::chrono::microseconds>(t2 - t1).count();
    bool passed = parsed && !disks.empty() && rttMs <= 1500.0 && parseUs <= 5000;

    ZyxelBenchmark::record({
        "LiveFirewallE1", "ReadDisk", "show disk",
        rttMs, parseUs, out.size(), parsed ? static_cast<uint32_t>(disks.size()) : 0U, passed,
        passed ? "PASS (SLA)" : "FAIL"
    });

    CHECK_TRUE(parsed);
    CHECK_FALSE(disks.empty());
    CHECK_TRUE(rttMs <= 1500.0);
}

TEST(LiveFirewallE1, ReadMac) {
    ZyxelDriver &driver = ZyxelDriver::getInstance();
    if (!driver.isConnected()) return;

    auto t0 = std::chrono::steady_clock::now();
    std::string out;
    SshResult res = driver.getSshClient().executeCommand(ZyxelSystemCmd::cmdShowMac(), out);
    auto t1 = std::chrono::steady_clock::now();

    CHECK_EQUAL(static_cast<int>(SshResult::SUCCESS), static_cast<int>(res));
    std::string mac;
    bool parsed = ZyxelSystemCmd::parseMac(out, mac);
    auto t2 = std::chrono::steady_clock::now();

    double rttMs = std::chrono::duration<double, std::milli>(t1 - t0).count();
    uint64_t parseUs = std::chrono::duration_cast<std::chrono::microseconds>(t2 - t1).count();
    bool passed = parsed && !mac.empty() && rttMs <= 1500.0 && parseUs <= 5000;

    ZyxelBenchmark::record({
        "LiveFirewallE1", "ReadMac", "show mac",
        rttMs, parseUs, out.size(), parsed ? 1U : 0U, passed,
        passed ? "PASS (SLA)" : "FAIL"
    });

    CHECK_TRUE(parsed);
    CHECK_FALSE(mac.empty());
    CHECK_TRUE(rttMs <= 1500.0);
}

TEST(LiveFirewallE1, ReadLedStatus) {
    ZyxelDriver &driver = ZyxelDriver::getInstance();
    if (!driver.isConnected()) return;

    auto t0 = std::chrono::steady_clock::now();
    std::string out;
    SshResult res = driver.getSshClient().executeCommand(ZyxelSystemCmd::cmdShowLedStatus(), out);
    auto t1 = std::chrono::steady_clock::now();

    CHECK_EQUAL(static_cast<int>(SshResult::SUCCESS), static_cast<int>(res));
    std::string ledStatus;
    bool parsed = ZyxelSystemCmd::parseLedStatus(out, ledStatus);
    auto t2 = std::chrono::steady_clock::now();

    double rttMs = std::chrono::duration<double, std::milli>(t1 - t0).count();
    uint64_t parseUs = std::chrono::duration_cast<std::chrono::microseconds>(t2 - t1).count();
    bool passed = parsed && !ledStatus.empty() && rttMs <= 1500.0 && parseUs <= 5000;

    ZyxelBenchmark::record({
        "LiveFirewallE1", "ReadLedStatus", "show led status",
        rttMs, parseUs, out.size(), parsed ? 1U : 0U, passed,
        passed ? "PASS (SLA)" : "FAIL"
    });

    CHECK_TRUE(parsed);
    CHECK_FALSE(ledStatus.empty());
    CHECK_TRUE(rttMs <= 1500.0);
}

TEST(LiveFirewallE1, ReadExtensionSlot) {
    ZyxelDriver &driver = ZyxelDriver::getInstance();
    if (!driver.isConnected()) return;

    auto t0 = std::chrono::steady_clock::now();
    std::string out;
    SshResult res = driver.getSshClient().executeCommand(ZyxelSystemCmd::cmdShowExtensionSlot(), out);
    auto t1 = std::chrono::steady_clock::now();

    CHECK_EQUAL(static_cast<int>(SshResult::SUCCESS), static_cast<int>(res));
    std::vector<ZyxelExtensionSlotEntry> slots;
    bool parsed = ZyxelSystemCmd::parseExtensionSlot(out, slots);
    auto t2 = std::chrono::steady_clock::now();

    double rttMs = std::chrono::duration<double, std::milli>(t1 - t0).count();
    uint64_t parseUs = std::chrono::duration_cast<std::chrono::microseconds>(t2 - t1).count();
    bool passed = parsed && !slots.empty() && rttMs <= 1500.0 && parseUs <= 5000;

    ZyxelBenchmark::record({
        "LiveFirewallE1", "ReadExtensionSlot", "show extension-slot",
        rttMs, parseUs, out.size(), parsed ? static_cast<uint32_t>(slots.size()) : 0U, passed,
        passed ? "PASS (SLA)" : "FAIL"
    });

    CHECK_TRUE(parsed);
    CHECK_FALSE(slots.empty());
    CHECK_TRUE(rttMs <= 1500.0);
}

TEST(LiveFirewallE1, ReadSerialNumber) {
    ZyxelDriver &driver = ZyxelDriver::getInstance();
    if (!driver.isConnected()) return;

    auto t0 = std::chrono::steady_clock::now();
    std::string out;
    SshResult res = driver.getSshClient().executeCommand(ZyxelSystemCmd::cmdShowSerialNumber(), out);
    auto t1 = std::chrono::steady_clock::now();

    CHECK_EQUAL(static_cast<int>(SshResult::SUCCESS), static_cast<int>(res));
    std::string sn;
    bool parsed = ZyxelSystemCmd::parseSerialNumber(out, sn);
    auto t2 = std::chrono::steady_clock::now();

    double rttMs = std::chrono::duration<double, std::milli>(t1 - t0).count();
    uint64_t parseUs = std::chrono::duration_cast<std::chrono::microseconds>(t2 - t1).count();
    bool passed = parsed && !sn.empty() && rttMs <= 1500.0 && parseUs <= 5000;

    ZyxelBenchmark::record({
        "LiveFirewallE1", "ReadSerialNumber", "show serial-number",
        rttMs, parseUs, out.size(), parsed ? 1U : 0U, passed,
        passed ? "PASS (SLA)" : "FAIL"
    });

    CHECK_TRUE(parsed);
    CHECK_FALSE(sn.empty());
    CHECK_TRUE(rttMs <= 1500.0);
}

TEST(LiveFirewallE1, ReadBootStatus) {
    ZyxelDriver &driver = ZyxelDriver::getInstance();
    if (!driver.isConnected()) return;

    auto t0 = std::chrono::steady_clock::now();
    std::string out;
    SshResult res = driver.getSshClient().executeCommand(ZyxelSystemCmd::cmdShowBootStatus(), out);
    auto t1 = std::chrono::steady_clock::now();

    CHECK_EQUAL(static_cast<int>(SshResult::SUCCESS), static_cast<int>(res));
    int statusCode = 0;
    std::string statusMsg;
    bool parsed = ZyxelSystemCmd::parseBootStatus(out, statusCode, statusMsg);
    auto t2 = std::chrono::steady_clock::now();

    double rttMs = std::chrono::duration<double, std::milli>(t1 - t0).count();
    uint64_t parseUs = std::chrono::duration_cast<std::chrono::microseconds>(t2 - t1).count();
    bool passed = parsed && rttMs <= 1500.0 && parseUs <= 5000;

    ZyxelBenchmark::record({
        "LiveFirewallE1", "ReadBootStatus", "show boot status",
        rttMs, parseUs, out.size(), parsed ? 1U : 0U, passed,
        passed ? "PASS (SLA)" : "FAIL"
    });

    CHECK_TRUE(parsed);
    CHECK_TRUE(rttMs <= 1500.0);
}

TEST(LiveFirewallE1, ReadSocketListen) {
    ZyxelDriver &driver = ZyxelDriver::getInstance();
    if (!driver.isConnected()) return;

    auto t0 = std::chrono::steady_clock::now();
    std::string out;
    SshResult res = driver.getSshClient().executeCommand(ZyxelSystemCmd::cmdShowSocketListen(), out);
    auto t1 = std::chrono::steady_clock::now();

    CHECK_EQUAL(static_cast<int>(SshResult::SUCCESS), static_cast<int>(res));
    std::vector<ZyxelSocketEntry> sockets;
    bool parsed = ZyxelSystemCmd::parseSocketList(out, sockets);
    auto t2 = std::chrono::steady_clock::now();

    double rttMs = std::chrono::duration<double, std::milli>(t1 - t0).count();
    uint64_t parseUs = std::chrono::duration_cast<std::chrono::microseconds>(t2 - t1).count();
    bool passed = parsed && !sockets.empty() && rttMs <= 2500.0 && parseUs <= 5000;

    ZyxelBenchmark::record({
        "LiveFirewallE1", "ReadSocketListen", "show socket listen",
        rttMs, parseUs, out.size(), parsed ? static_cast<uint32_t>(sockets.size()) : 0U, passed,
        passed ? "PASS (SLA)" : "FAIL"
    });

    CHECK_TRUE(parsed);
    CHECK_FALSE(sockets.empty());
    CHECK_TRUE(rttMs <= 2500.0);
}

TEST(LiveFirewallE1, ReadSocketOpen) {
    ZyxelDriver &driver = ZyxelDriver::getInstance();
    if (!driver.isConnected()) return;

    auto t0 = std::chrono::steady_clock::now();
    std::string out;
    SshResult res = driver.getSshClient().executeCommand(ZyxelSystemCmd::cmdShowSocketOpen(), out);
    auto t1 = std::chrono::steady_clock::now();

    CHECK_EQUAL(static_cast<int>(SshResult::SUCCESS), static_cast<int>(res));
    std::vector<ZyxelSocketEntry> sockets;
    bool parsed = ZyxelSystemCmd::parseSocketList(out, sockets);
    auto t2 = std::chrono::steady_clock::now();

    double rttMs = std::chrono::duration<double, std::milli>(t1 - t0).count();
    uint64_t parseUs = std::chrono::duration_cast<std::chrono::microseconds>(t2 - t1).count();
    bool passed = parsed && !sockets.empty() && rttMs <= 2500.0 && parseUs <= 5000;

    ZyxelBenchmark::record({
        "LiveFirewallE1", "ReadSocketOpen", "show socket open",
        rttMs, parseUs, out.size(), parsed ? static_cast<uint32_t>(sockets.size()) : 0U, passed,
        passed ? "PASS (SLA)" : "FAIL"
    });

    CHECK_TRUE(parsed);
    CHECK_FALSE(sockets.empty());
    CHECK_TRUE(rttMs <= 2500.0);
}

TEST(LiveFirewallE1, ReadRamSize) {
    ZyxelDriver &driver = ZyxelDriver::getInstance();
    if (!driver.isConnected()) return;

    auto t0 = std::chrono::steady_clock::now();
    std::string out;
    SshResult res = driver.getSshClient().executeCommand(ZyxelSystemCmd::cmdShowRamSize(), out);
    auto t1 = std::chrono::steady_clock::now();

    CHECK_EQUAL(static_cast<int>(SshResult::SUCCESS), static_cast<int>(res));
    int ramMb = 0;
    bool parsed = ZyxelSystemCmd::parseRamSize(out, ramMb);
    auto t2 = std::chrono::steady_clock::now();

    double rttMs = std::chrono::duration<double, std::milli>(t1 - t0).count();
    uint64_t parseUs = std::chrono::duration_cast<std::chrono::microseconds>(t2 - t1).count();
    bool passed = parsed && ramMb > 0 && rttMs <= 1500.0 && parseUs <= 5000;

    ZyxelBenchmark::record({
        "LiveFirewallE1", "ReadRamSize", "show ram-size",
        rttMs, parseUs, out.size(), parsed ? 1U : 0U, passed,
        passed ? "PASS (SLA)" : "FAIL"
    });

    CHECK_TRUE(parsed);
    CHECK_TRUE(ramMb > 0);
    CHECK_TRUE(rttMs <= 1500.0);
}

TEST(LiveFirewallE1, ReadComportStatus) {
    ZyxelDriver &driver = ZyxelDriver::getInstance();
    if (!driver.isConnected()) return;

    auto t0 = std::chrono::steady_clock::now();
    std::string out;
    SshResult res = driver.getSshClient().executeCommand(ZyxelSystemCmd::cmdShowComportStatus(), out);
    auto t1 = std::chrono::steady_clock::now();

    CHECK_EQUAL(static_cast<int>(SshResult::SUCCESS), static_cast<int>(res));
    std::string comport;
    bool parsed = ZyxelSystemCmd::parseComportStatus(out, comport);
    auto t2 = std::chrono::steady_clock::now();

    double rttMs = std::chrono::duration<double, std::milli>(t1 - t0).count();
    uint64_t parseUs = std::chrono::duration_cast<std::chrono::microseconds>(t2 - t1).count();
    bool passed = parsed && !comport.empty() && rttMs <= 1500.0 && parseUs <= 5000;

    ZyxelBenchmark::record({
        "LiveFirewallE1", "ReadComportStatus", "show comport status",
        rttMs, parseUs, out.size(), parsed ? 1U : 0U, passed,
        passed ? "PASS (SLA)" : "FAIL"
    });

    CHECK_TRUE(parsed);
    CHECK_FALSE(comport.empty());
    CHECK_TRUE(rttMs <= 1500.0);
}

TEST(LiveFirewallE1, DryFireInvalidSystemCommand) {
    ZyxelDriver &driver = ZyxelDriver::getInstance();
    if (!driver.isConnected()) return;

    auto t0 = std::chrono::steady_clock::now();
    std::string out;
    SshResult res = driver.getSshClient().executeCommand(ZyxelSystemCmd::cmdInvalidSystemDryFire(), out);
    auto t1 = std::chrono::steady_clock::now();

    bool rejected = (res == SshResult::ERR_SYNTAX || out.find("Parse error") != std::string::npos);
    double rttMs = std::chrono::duration<double, std::milli>(t1 - t0).count();
    bool passed = rejected && rttMs <= 2500.0;

    ZyxelBenchmark::record({
        "LiveFirewallE1", "DryFireInvalidSystemCommand", "system illegal_probe_test_cmd_12345",
        rttMs, 0, out.size(), rejected ? 1U : 0U, passed,
        passed ? "PASS (REJECT)" : "FAIL"
    });

    CHECK_TRUE(rejected);
    CHECK_TRUE(rttMs <= 2500.0);
}

// ============================================================================
// CppUTest Diagnostic Test Group: LiveFirewallE2 (Envelope 2: Interfaces, Zones, Trunks)
// ============================================================================

TEST_GROUP(LiveFirewallE2) {
    void setup() override {
        ZyxelDriver &driver = ZyxelDriver::getInstance();
        if (driver.isConnected()) {
            driver.getSshClient().unwindToRootPrompt();
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(25));
    }

    void teardown() override {
        ZyxelDriver &driver = ZyxelDriver::getInstance();
        if (driver.isConnected()) {
            driver.getSshClient().unwindToRootPrompt();
        }
    }
};

TEST(LiveFirewallE2, ReadInterfaceSummaryAll) {
    ZyxelDriver &driver = ZyxelDriver::getInstance();
    if (!driver.isConnected()) return;

    auto t0 = std::chrono::steady_clock::now();
    std::string out;
    SshResult res = driver.getSshClient().executeCommand(ZyxelNetworkCmd::cmdShowInterfaces(), out);
    auto t1 = std::chrono::steady_clock::now();

    CHECK_EQUAL(static_cast<int>(SshResult::SUCCESS), static_cast<int>(res));

    auto p0 = std::chrono::steady_clock::now();
    std::vector<ZyxelInterfaceInfo> ifaces;
    bool parsed = ZyxelNetworkCmd::parseInterfaces(out, ifaces);
    auto p1 = std::chrono::steady_clock::now();

    double rttMs = std::chrono::duration<double, std::milli>(t1 - t0).count();
    uint64_t parseUs = std::chrono::duration_cast<std::chrono::microseconds>(p1 - p0).count();
    bool passed = (res == SshResult::SUCCESS) && parsed && (rttMs <= 3500.0);

    ZyxelBenchmark::record({
        "LiveFirewallE2", "ReadInterfaceSummaryAll", ZyxelNetworkCmd::cmdShowInterfaces(),
        rttMs, parseUs, out.size(), static_cast<uint32_t>(ifaces.size()), passed,
        passed ? "PASS (SLA)" : "FAIL"
    });

    CHECK_TRUE(parsed);
    CHECK_TRUE(rttMs <= 3500.0);
}

TEST(LiveFirewallE2, ReadInterfaceBase) {
    ZyxelDriver &driver = ZyxelDriver::getInstance();
    if (!driver.isConnected()) return;

    auto t0 = std::chrono::steady_clock::now();
    std::string out;
    SshResult res = driver.getSshClient().executeCommand(ZyxelNetworkCmd::cmdShowInterfaceBase(), out);
    auto t1 = std::chrono::steady_clock::now();

    CHECK_EQUAL(static_cast<int>(SshResult::SUCCESS), static_cast<int>(res));

    auto p0 = std::chrono::steady_clock::now();
    std::vector<ZyxelInterfaceInfo> ifaces;
    bool parsed = ZyxelNetworkCmd::parseInterfaces(out, ifaces);
    auto p1 = std::chrono::steady_clock::now();

    double rttMs = std::chrono::duration<double, std::milli>(t1 - t0).count();
    uint64_t parseUs = std::chrono::duration_cast<std::chrono::microseconds>(p1 - p0).count();
    bool passed = (res == SshResult::SUCCESS) && parsed && (rttMs <= 3500.0);

    ZyxelBenchmark::record({
        "LiveFirewallE2", "ReadInterfaceBase", ZyxelNetworkCmd::cmdShowInterfaceBase(),
        rttMs, parseUs, out.size(), static_cast<uint32_t>(ifaces.size()), passed,
        passed ? "PASS (SLA)" : "FAIL"
    });

    CHECK_TRUE(parsed);
    CHECK_TRUE(rttMs <= 3500.0);
}

TEST(LiveFirewallE2, ReadInterfaceDetailLan1) {
    ZyxelDriver &driver = ZyxelDriver::getInstance();
    if (!driver.isConnected()) return;

    auto t0 = std::chrono::steady_clock::now();
    std::string out;
    SshResult res = driver.getSshClient().executeCommand(ZyxelNetworkCmd::cmdShowInterface("lan1"), out);
    auto t1 = std::chrono::steady_clock::now();

    CHECK_EQUAL(static_cast<int>(SshResult::SUCCESS), static_cast<int>(res));

    auto p0 = std::chrono::steady_clock::now();
    std::vector<ZyxelInterfaceInfo> ifaces;
    bool parsed = ZyxelNetworkCmd::parseInterfaces(out, ifaces);
    auto p1 = std::chrono::steady_clock::now();

    double rttMs = std::chrono::duration<double, std::milli>(t1 - t0).count();
    uint64_t parseUs = std::chrono::duration_cast<std::chrono::microseconds>(p1 - p0).count();
    bool passed = (res == SshResult::SUCCESS) && parsed && (rttMs <= 1500.0);

    ZyxelBenchmark::record({
        "LiveFirewallE2", "ReadInterfaceDetailLan1", ZyxelNetworkCmd::cmdShowInterface("lan1"),
        rttMs, parseUs, out.size(), static_cast<uint32_t>(ifaces.size()), passed,
        passed ? "PASS (SLA)" : "FAIL"
    });

    CHECK_TRUE(parsed);
    CHECK_TRUE(rttMs <= 1500.0);
}

TEST(LiveFirewallE2, ReadInterfaceDetailWan1) {
    ZyxelDriver &driver = ZyxelDriver::getInstance();
    if (!driver.isConnected()) return;

    auto t0 = std::chrono::steady_clock::now();
    std::string out;
    SshResult res = driver.getSshClient().executeCommand(ZyxelNetworkCmd::cmdShowInterface("wan1"), out);
    auto t1 = std::chrono::steady_clock::now();

    CHECK_EQUAL(static_cast<int>(SshResult::SUCCESS), static_cast<int>(res));

    auto p0 = std::chrono::steady_clock::now();
    std::vector<ZyxelInterfaceInfo> ifaces;
    bool parsed = ZyxelNetworkCmd::parseInterfaces(out, ifaces);
    auto p1 = std::chrono::steady_clock::now();

    double rttMs = std::chrono::duration<double, std::milli>(t1 - t0).count();
    uint64_t parseUs = std::chrono::duration_cast<std::chrono::microseconds>(p1 - p0).count();
    bool passed = (res == SshResult::SUCCESS) && parsed && (rttMs <= 1500.0);

    ZyxelBenchmark::record({
        "LiveFirewallE2", "ReadInterfaceDetailWan1", ZyxelNetworkCmd::cmdShowInterface("wan1"),
        rttMs, parseUs, out.size(), static_cast<uint32_t>(ifaces.size()), passed,
        passed ? "PASS (SLA)" : "FAIL"
    });

    CHECK_TRUE(parsed);
    CHECK_TRUE(rttMs <= 1500.0);
}

TEST(LiveFirewallE2, ReadIpDhcpBinding) {
    ZyxelDriver &driver = ZyxelDriver::getInstance();
    if (!driver.isConnected()) return;

    auto t0 = std::chrono::steady_clock::now();
    std::string out;
    SshResult res = driver.getSshClient().executeCommand(ZyxelNetworkCmd::cmdShowIpDhcpBinding(), out);
    auto t1 = std::chrono::steady_clock::now();

    CHECK_EQUAL(static_cast<int>(SshResult::SUCCESS), static_cast<int>(res));

    auto p0 = std::chrono::steady_clock::now();
    std::vector<ZyxelDhcpBindingEntry> entries;
    bool parsed = ZyxelNetworkCmd::parseDhcpBindings(out, entries);
    auto p1 = std::chrono::steady_clock::now();

    double rttMs = std::chrono::duration<double, std::milli>(t1 - t0).count();
    uint64_t parseUs = std::chrono::duration_cast<std::chrono::microseconds>(p1 - p0).count();
    bool passed = (res == SshResult::SUCCESS) && parsed && (rttMs <= 2500.0);

    ZyxelBenchmark::record({
        "LiveFirewallE2", "ReadIpDhcpBinding", ZyxelNetworkCmd::cmdShowIpDhcpBinding(),
        rttMs, parseUs, out.size(), static_cast<uint32_t>(entries.size()), passed,
        passed ? "PASS (SLA)" : "FAIL"
    });

    CHECK_TRUE(parsed);
    CHECK_TRUE(rttMs <= 2500.0);
}

TEST(LiveFirewallE2, ReadArpGratuitous) {
    ZyxelDriver &driver = ZyxelDriver::getInstance();
    if (!driver.isConnected()) return;

    auto t0 = std::chrono::steady_clock::now();
    std::string out;
    SshResult res = driver.getSshClient().executeCommand(ZyxelNetworkCmd::cmdShowArpGratuitous(), out);
    auto t1 = std::chrono::steady_clock::now();

    CHECK_EQUAL(static_cast<int>(SshResult::SUCCESS), static_cast<int>(res));

    auto p0 = std::chrono::steady_clock::now();
    ZyxelArpGratuitousInfo info;
    bool parsed = ZyxelNetworkCmd::parseArpGratuitous(out, info);
    auto p1 = std::chrono::steady_clock::now();

    double rttMs = std::chrono::duration<double, std::milli>(t1 - t0).count();
    uint64_t parseUs = std::chrono::duration_cast<std::chrono::microseconds>(p1 - p0).count();
    bool passed = (res == SshResult::SUCCESS) && parsed && (rttMs <= 1500.0);

    ZyxelBenchmark::record({
        "LiveFirewallE2", "ReadArpGratuitous", ZyxelNetworkCmd::cmdShowArpGratuitous(),
        rttMs, parseUs, out.size(), 1, passed,
        passed ? "PASS (SLA)" : "FAIL"
    });

    CHECK_TRUE(parsed);
    CHECK_TRUE(rttMs <= 1500.0);
}

TEST(LiveFirewallE2, ReadZones) {
    ZyxelDriver &driver = ZyxelDriver::getInstance();
    if (!driver.isConnected()) return;

    auto t0 = std::chrono::steady_clock::now();
    std::string out;
    SshResult res = driver.getSshClient().executeCommand(ZyxelNetworkCmd::cmdShowZones(), out);
    auto t1 = std::chrono::steady_clock::now();

    CHECK_EQUAL(static_cast<int>(SshResult::SUCCESS), static_cast<int>(res));

    auto p0 = std::chrono::steady_clock::now();
    std::vector<ZyxelZoneInfo> zones;
    bool parsed = ZyxelNetworkCmd::parseZones(out, zones);
    auto p1 = std::chrono::steady_clock::now();

    double rttMs = std::chrono::duration<double, std::milli>(t1 - t0).count();
    uint64_t parseUs = std::chrono::duration_cast<std::chrono::microseconds>(p1 - p0).count();
    bool passed = (res == SshResult::SUCCESS) && parsed && (rttMs <= 1500.0);

    ZyxelBenchmark::record({
        "LiveFirewallE2", "ReadZones", ZyxelNetworkCmd::cmdShowZones(),
        rttMs, parseUs, out.size(), static_cast<uint32_t>(zones.size()), passed,
        passed ? "PASS (SLA)" : "FAIL"
    });

    CHECK_TRUE(parsed);
    CHECK_TRUE(rttMs <= 1500.0);
}

TEST(LiveFirewallE2, ReadZoneLan1) {
    ZyxelDriver &driver = ZyxelDriver::getInstance();
    if (!driver.isConnected()) return;

    auto t0 = std::chrono::steady_clock::now();
    std::string out;
    SshResult res = driver.getSshClient().executeCommand(ZyxelNetworkCmd::cmdShowZone("LAN1"), out);
    auto t1 = std::chrono::steady_clock::now();

    CHECK_EQUAL(static_cast<int>(SshResult::SUCCESS), static_cast<int>(res));

    double rttMs = std::chrono::duration<double, std::milli>(t1 - t0).count();
    bool passed = (res == SshResult::SUCCESS) && (out.find("lan1") != std::string::npos) && (rttMs <= 1500.0);

    ZyxelBenchmark::record({
        "LiveFirewallE2", "ReadZoneLan1", ZyxelNetworkCmd::cmdShowZone("LAN1"),
        rttMs, 0, out.size(), 1, passed,
        passed ? "PASS (SLA)" : "FAIL"
    });

    CHECK_TRUE(passed);
    CHECK_TRUE(rttMs <= 1500.0);
}

TEST(LiveFirewallE2, ReadZoneDefaultBinding) {
    ZyxelDriver &driver = ZyxelDriver::getInstance();
    if (!driver.isConnected()) return;

    auto t0 = std::chrono::steady_clock::now();
    std::string out;
    SshResult res = driver.getSshClient().executeCommand(ZyxelNetworkCmd::cmdShowZoneDefaultBinding(), out);
    auto t1 = std::chrono::steady_clock::now();

    CHECK_EQUAL(static_cast<int>(SshResult::SUCCESS), static_cast<int>(res));

    auto p0 = std::chrono::steady_clock::now();
    std::vector<ZyxelZoneBindingEntry> bindings;
    bool parsed = ZyxelNetworkCmd::parseZoneBindings(out, bindings);
    auto p1 = std::chrono::steady_clock::now();

    double rttMs = std::chrono::duration<double, std::milli>(t1 - t0).count();
    uint64_t parseUs = std::chrono::duration_cast<std::chrono::microseconds>(p1 - p0).count();
    bool passed = (res == SshResult::SUCCESS) && parsed && (rttMs <= 1500.0);

    ZyxelBenchmark::record({
        "LiveFirewallE2", "ReadZoneDefaultBinding", ZyxelNetworkCmd::cmdShowZoneDefaultBinding(),
        rttMs, parseUs, out.size(), static_cast<uint32_t>(bindings.size()), passed,
        passed ? "PASS (SLA)" : "FAIL"
    });

    CHECK_TRUE(parsed);
    CHECK_TRUE(rttMs <= 1500.0);
}

TEST(LiveFirewallE2, ReadZoneBindingIface) {
    ZyxelDriver &driver = ZyxelDriver::getInstance();
    if (!driver.isConnected()) return;

    auto t0 = std::chrono::steady_clock::now();
    std::string out;
    SshResult res = driver.getSshClient().executeCommand(ZyxelNetworkCmd::cmdShowZoneBindingIface(), out);
    auto t1 = std::chrono::steady_clock::now();

    CHECK_EQUAL(static_cast<int>(SshResult::SUCCESS), static_cast<int>(res));

    auto p0 = std::chrono::steady_clock::now();
    std::vector<ZyxelZoneBindingEntry> bindings;
    bool parsed = ZyxelNetworkCmd::parseZoneBindings(out, bindings);
    auto p1 = std::chrono::steady_clock::now();

    double rttMs = std::chrono::duration<double, std::milli>(t1 - t0).count();
    uint64_t parseUs = std::chrono::duration_cast<std::chrono::microseconds>(p1 - p0).count();
    bool passed = (res == SshResult::SUCCESS) && parsed && (rttMs <= 1500.0);

    ZyxelBenchmark::record({
        "LiveFirewallE2", "ReadZoneBindingIface", ZyxelNetworkCmd::cmdShowZoneBindingIface(),
        rttMs, parseUs, out.size(), static_cast<uint32_t>(bindings.size()), passed,
        passed ? "PASS (SLA)" : "FAIL"
    });

    CHECK_TRUE(parsed);
    CHECK_TRUE(rttMs <= 1500.0);
}

TEST(LiveFirewallE2, ReadL2Isolation) {
    ZyxelDriver &driver = ZyxelDriver::getInstance();
    if (!driver.isConnected()) return;

    auto t0 = std::chrono::steady_clock::now();
    std::string out;
    SshResult res = driver.getSshClient().executeCommand(ZyxelNetworkCmd::cmdShowL2Isolation(), out);
    auto t1 = std::chrono::steady_clock::now();

    CHECK_EQUAL(static_cast<int>(SshResult::SUCCESS), static_cast<int>(res));

    double rttMs = std::chrono::duration<double, std::milli>(t1 - t0).count();
    bool passed = (res == SshResult::SUCCESS) && (rttMs <= 1500.0);

    ZyxelBenchmark::record({
        "LiveFirewallE2", "ReadL2Isolation", ZyxelNetworkCmd::cmdShowL2Isolation(),
        rttMs, 0, out.size(), 1, passed,
        passed ? "PASS (SLA)" : "FAIL"
    });

    CHECK_TRUE(passed);
    CHECK_TRUE(rttMs <= 1500.0);
}

TEST(LiveFirewallE2, ReadL2IsolationActivation) {
    ZyxelDriver &driver = ZyxelDriver::getInstance();
    if (!driver.isConnected()) return;

    auto t0 = std::chrono::steady_clock::now();
    std::string out;
    SshResult res = driver.getSshClient().executeCommand(ZyxelNetworkCmd::cmdShowL2IsolationActivation(), out);
    auto t1 = std::chrono::steady_clock::now();

    CHECK_EQUAL(static_cast<int>(SshResult::SUCCESS), static_cast<int>(res));

    auto p0 = std::chrono::steady_clock::now();
    bool active = false;
    bool parsed = ZyxelNetworkCmd::parseL2IsolationActivation(out, active);
    auto p1 = std::chrono::steady_clock::now();

    double rttMs = std::chrono::duration<double, std::milli>(t1 - t0).count();
    uint64_t parseUs = std::chrono::duration_cast<std::chrono::microseconds>(p1 - p0).count();
    bool passed = (res == SshResult::SUCCESS) && parsed && (rttMs <= 1500.0);

    ZyxelBenchmark::record({
        "LiveFirewallE2", "ReadL2IsolationActivation", ZyxelNetworkCmd::cmdShowL2IsolationActivation(),
        rttMs, parseUs, out.size(), 1, passed,
        passed ? "PASS (SLA)" : "FAIL"
    });

    CHECK_TRUE(parsed);
    CHECK_TRUE(rttMs <= 1500.0);
}

TEST(LiveFirewallE2, ReadL2IsolationWhitelist) {
    ZyxelDriver &driver = ZyxelDriver::getInstance();
    if (!driver.isConnected()) return;

    auto t0 = std::chrono::steady_clock::now();
    std::string out;
    SshResult res = driver.getSshClient().executeCommand(ZyxelNetworkCmd::cmdShowL2IsolationWhitelist(), out);
    auto t1 = std::chrono::steady_clock::now();

    CHECK_EQUAL(static_cast<int>(SshResult::SUCCESS), static_cast<int>(res));

    double rttMs = std::chrono::duration<double, std::milli>(t1 - t0).count();
    bool passed = (res == SshResult::SUCCESS) && (rttMs <= 1500.0);

    ZyxelBenchmark::record({
        "LiveFirewallE2", "ReadL2IsolationWhitelist", ZyxelNetworkCmd::cmdShowL2IsolationWhitelist(),
        rttMs, 0, out.size(), 1, passed,
        passed ? "PASS (SLA)" : "FAIL"
    });

    CHECK_TRUE(passed);
    CHECK_TRUE(rttMs <= 1500.0);
}

TEST(LiveFirewallE2, NegativeReadNonExistentInterface) {
    ZyxelDriver &driver = ZyxelDriver::getInstance();
    if (!driver.isConnected()) return;

    auto t0 = std::chrono::steady_clock::now();
    std::string out;
    SshResult res = driver.getSshClient().executeCommand(ZyxelNetworkCmd::cmdShowInterface("non_existent_iface_999"), out);
    auto t1 = std::chrono::steady_clock::now();

    bool rejected = (res == SshResult::ERR_SYNTAX || res == SshResult::ERR_EXEC_FAILED ||
                     out.find("Parse error") != std::string::npos ||
                     out.find("ERROR") != std::string::npos);
    double rttMs = std::chrono::duration<double, std::milli>(t1 - t0).count();
    bool passed = rejected && rttMs <= 2500.0;

    ZyxelBenchmark::record({
        "LiveFirewallE2", "NegativeReadNonExistentInterface", ZyxelNetworkCmd::cmdShowInterface("non_existent_iface_999"),
        rttMs, 0, out.size(), rejected ? 1U : 0U, passed,
        passed ? "PASS (REJECT)" : "FAIL"
    });

    CHECK_TRUE(rejected);
    CHECK_TRUE(rttMs <= 2500.0);
}

TEST(LiveFirewallE2, DryFireInvalidInterfaceConfig) {
    ZyxelDriver &driver = ZyxelDriver::getInstance();
    if (!driver.isConnected()) return;

    auto t0 = std::chrono::steady_clock::now();
    std::string out;
    SshResult res = driver.getSshClient().executeCommand(ZyxelNetworkCmd::cmdInvalidInterfaceDryFire(), out);
    auto t1 = std::chrono::steady_clock::now();

    bool rejected = (res == SshResult::ERR_SYNTAX || res == SshResult::ERR_EXEC_FAILED ||
                     out.find("Parse error") != std::string::npos ||
                     out.find("ERROR") != std::string::npos);
    double rttMs = std::chrono::duration<double, std::milli>(t1 - t0).count();
    bool passed = rejected && rttMs <= 2500.0;

    ZyxelBenchmark::record({
        "LiveFirewallE2", "DryFireInvalidInterfaceConfig", ZyxelNetworkCmd::cmdInvalidInterfaceDryFire(),
        rttMs, 0, out.size(), rejected ? 1U : 0U, passed,
        passed ? "PASS (REJECT)" : "FAIL"
    });

    CHECK_TRUE(rejected);
    CHECK_TRUE(rttMs <= 2500.0);
}

// ============================================================================
// Test Group: LiveFirewallE3 (Routing & Routing Protocols)
// ============================================================================

TEST_GROUP(LiveFirewallE3) {
    void setup() override {
        ZyxelDriver &driver = ZyxelDriver::getInstance();
        if (driver.isConnected()) {
            driver.getSshClient().unwindToRootPrompt();
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(25));
    }

    void teardown() override {
        ZyxelDriver &driver = ZyxelDriver::getInstance();
        if (driver.isConnected()) {
            driver.getSshClient().unwindToRootPrompt();
        }
    }
};

TEST(LiveFirewallE3, ReadIpRouteKernel) {
    ZyxelDriver &driver = ZyxelDriver::getInstance();
    if (!driver.isConnected()) return;

    auto t0 = std::chrono::steady_clock::now();
    std::string out;
    SshResult res = driver.getSshClient().executeCommand(ZyxelNetworkCmd::cmdShowIpRouteFilter("kernel"), out);
    auto t1 = std::chrono::steady_clock::now();

    CHECK_EQUAL(static_cast<int>(SshResult::SUCCESS), static_cast<int>(res));
    auto p0 = std::chrono::steady_clock::now();
    std::vector<ZyxelRouteEntry> routes;
    bool parsed = ZyxelNetworkCmd::parseIpRoutes(out, routes);
    auto p1 = std::chrono::steady_clock::now();

    double rttMs = std::chrono::duration<double, std::milli>(t1 - t0).count();
    uint64_t parseUs = std::chrono::duration_cast<std::chrono::microseconds>(p1 - p0).count();
    bool passed = (res == SshResult::SUCCESS) && parsed && (rttMs <= 2500.0);

    ZyxelBenchmark::record({
        "LiveFirewallE3", "ReadIpRouteKernel", ZyxelNetworkCmd::cmdShowIpRouteFilter("kernel"),
        rttMs, parseUs, out.size(), static_cast<uint32_t>(routes.size()), passed,
        passed ? "PASS (SLA)" : "FAIL"
    });

    CHECK_TRUE(parsed);
    CHECK_TRUE(rttMs <= 2500.0);
}

TEST(LiveFirewallE3, ReadIpRouteConnected) {
    ZyxelDriver &driver = ZyxelDriver::getInstance();
    if (!driver.isConnected()) return;

    auto t0 = std::chrono::steady_clock::now();
    std::string out;
    SshResult res = driver.getSshClient().executeCommand(ZyxelNetworkCmd::cmdShowIpRouteFilter("connected"), out);
    auto t1 = std::chrono::steady_clock::now();

    CHECK_EQUAL(static_cast<int>(SshResult::SUCCESS), static_cast<int>(res));
    auto p0 = std::chrono::steady_clock::now();
    std::vector<ZyxelRouteEntry> routes;
    bool parsed = ZyxelNetworkCmd::parseIpRoutes(out, routes);
    auto p1 = std::chrono::steady_clock::now();

    double rttMs = std::chrono::duration<double, std::milli>(t1 - t0).count();
    uint64_t parseUs = std::chrono::duration_cast<std::chrono::microseconds>(p1 - p0).count();
    bool passed = (res == SshResult::SUCCESS) && parsed && !routes.empty() && (rttMs <= 2500.0);

    ZyxelBenchmark::record({
        "LiveFirewallE3", "ReadIpRouteConnected", ZyxelNetworkCmd::cmdShowIpRouteFilter("connected"),
        rttMs, parseUs, out.size(), static_cast<uint32_t>(routes.size()), passed,
        passed ? "PASS (SLA)" : "FAIL"
    });

    CHECK_TRUE(parsed);
    CHECK_FALSE(routes.empty());
    CHECK_TRUE(rttMs <= 2500.0);
}

TEST(LiveFirewallE3, ReadIpRouteStatic) {
    ZyxelDriver &driver = ZyxelDriver::getInstance();
    if (!driver.isConnected()) return;

    auto t0 = std::chrono::steady_clock::now();
    std::string out;
    SshResult res = driver.getSshClient().executeCommand(ZyxelNetworkCmd::cmdShowIpRouteFilter("static"), out);
    auto t1 = std::chrono::steady_clock::now();

    CHECK_EQUAL(static_cast<int>(SshResult::SUCCESS), static_cast<int>(res));
    auto p0 = std::chrono::steady_clock::now();
    std::vector<ZyxelRouteEntry> routes;
    bool parsed = ZyxelNetworkCmd::parseIpRoutes(out, routes);
    auto p1 = std::chrono::steady_clock::now();

    double rttMs = std::chrono::duration<double, std::milli>(t1 - t0).count();
    uint64_t parseUs = std::chrono::duration_cast<std::chrono::microseconds>(p1 - p0).count();
    bool passed = (res == SshResult::SUCCESS) && parsed && (rttMs <= 2500.0);

    ZyxelBenchmark::record({
        "LiveFirewallE3", "ReadIpRouteStatic", ZyxelNetworkCmd::cmdShowIpRouteFilter("static"),
        rttMs, parseUs, out.size(), static_cast<uint32_t>(routes.size()), passed,
        passed ? "PASS (SLA)" : "FAIL"
    });

    CHECK_TRUE(parsed);
    CHECK_TRUE(rttMs <= 2500.0);
}

TEST(LiveFirewallE3, ReadIpRouteOspf) {
    ZyxelDriver &driver = ZyxelDriver::getInstance();
    if (!driver.isConnected()) return;

    auto t0 = std::chrono::steady_clock::now();
    std::string out;
    SshResult res = driver.getSshClient().executeCommand(ZyxelNetworkCmd::cmdShowIpRouteFilter("ospf"), out);
    auto t1 = std::chrono::steady_clock::now();

    CHECK_EQUAL(static_cast<int>(SshResult::SUCCESS), static_cast<int>(res));
    double rttMs = std::chrono::duration<double, std::milli>(t1 - t0).count();
    bool passed = (res == SshResult::SUCCESS) && (rttMs <= 2500.0);

    ZyxelBenchmark::record({
        "LiveFirewallE3", "ReadIpRouteOspf", ZyxelNetworkCmd::cmdShowIpRouteFilter("ospf"),
        rttMs, 0, out.size(), 1, passed,
        passed ? "PASS (SLA)" : "FAIL"
    });

    CHECK_TRUE(rttMs <= 2500.0);
}

TEST(LiveFirewallE3, ReadIpRouteRip) {
    ZyxelDriver &driver = ZyxelDriver::getInstance();
    if (!driver.isConnected()) return;

    auto t0 = std::chrono::steady_clock::now();
    std::string out;
    SshResult res = driver.getSshClient().executeCommand(ZyxelNetworkCmd::cmdShowIpRouteFilter("rip"), out);
    auto t1 = std::chrono::steady_clock::now();

    CHECK_EQUAL(static_cast<int>(SshResult::SUCCESS), static_cast<int>(res));
    double rttMs = std::chrono::duration<double, std::milli>(t1 - t0).count();
    bool passed = (res == SshResult::SUCCESS) && (rttMs <= 2500.0);

    ZyxelBenchmark::record({
        "LiveFirewallE3", "ReadIpRouteRip", ZyxelNetworkCmd::cmdShowIpRouteFilter("rip"),
        rttMs, 0, out.size(), 1, passed,
        passed ? "PASS (SLA)" : "FAIL"
    });

    CHECK_TRUE(rttMs <= 2500.0);
}

TEST(LiveFirewallE3, ReadIpRouteBgp) {
    ZyxelDriver &driver = ZyxelDriver::getInstance();
    if (!driver.isConnected()) return;

    auto t0 = std::chrono::steady_clock::now();
    std::string out;
    SshResult res = driver.getSshClient().executeCommand(ZyxelNetworkCmd::cmdShowIpRouteFilter("bgp"), out);
    auto t1 = std::chrono::steady_clock::now();

    CHECK_EQUAL(static_cast<int>(SshResult::SUCCESS), static_cast<int>(res));
    double rttMs = std::chrono::duration<double, std::milli>(t1 - t0).count();
    bool passed = (res == SshResult::SUCCESS) && (rttMs <= 2500.0);

    ZyxelBenchmark::record({
        "LiveFirewallE3", "ReadIpRouteBgp", ZyxelNetworkCmd::cmdShowIpRouteFilter("bgp"),
        rttMs, 0, out.size(), 1, passed,
        passed ? "PASS (SLA)" : "FAIL"
    });

    CHECK_TRUE(rttMs <= 2500.0);
}

TEST(LiveFirewallE3, ReadIpRouteSettings) {
    ZyxelDriver &driver = ZyxelDriver::getInstance();
    if (!driver.isConnected()) return;

    auto t0 = std::chrono::steady_clock::now();
    std::string out;
    SshResult res = driver.getSshClient().executeCommand(ZyxelNetworkCmd::cmdShowIpRouteSettings(), out);
    auto t1 = std::chrono::steady_clock::now();

    CHECK_EQUAL(static_cast<int>(SshResult::SUCCESS), static_cast<int>(res));
    auto p0 = std::chrono::steady_clock::now();
    std::vector<ZyxelRouteSettingsEntry> settings;
    bool parsed = ZyxelNetworkCmd::parseRouteSettings(out, settings);
    auto p1 = std::chrono::steady_clock::now();

    double rttMs = std::chrono::duration<double, std::milli>(t1 - t0).count();
    uint64_t parseUs = std::chrono::duration_cast<std::chrono::microseconds>(p1 - p0).count();
    bool passed = (res == SshResult::SUCCESS) && parsed && (rttMs <= 2500.0);

    ZyxelBenchmark::record({
        "LiveFirewallE3", "ReadIpRouteSettings", ZyxelNetworkCmd::cmdShowIpRouteSettings(),
        rttMs, parseUs, out.size(), static_cast<uint32_t>(settings.size()), passed,
        passed ? "PASS (SLA)" : "FAIL"
    });

    CHECK_TRUE(parsed);
    CHECK_TRUE(rttMs <= 2500.0);
}

TEST(LiveFirewallE3, ReadIpRouteControlVirtualServer) {
    ZyxelDriver &driver = ZyxelDriver::getInstance();
    if (!driver.isConnected()) return;

    auto t0 = std::chrono::steady_clock::now();
    std::string out;
    SshResult res = driver.getSshClient().executeCommand(ZyxelNetworkCmd::cmdShowIpRouteControlVirtualServer(), out);
    auto t1 = std::chrono::steady_clock::now();

    CHECK_EQUAL(static_cast<int>(SshResult::SUCCESS), static_cast<int>(res));
    bool active = false;
    bool parsed = ZyxelNetworkCmd::parsePolicyRouteControlVirtualServer(out, active);
    double rttMs = std::chrono::duration<double, std::milli>(t1 - t0).count();
    bool passed = (res == SshResult::SUCCESS) && parsed && active && (rttMs <= 1500.0);

    ZyxelBenchmark::record({
        "LiveFirewallE3", "ReadIpRouteControlVirtualServer", ZyxelNetworkCmd::cmdShowIpRouteControlVirtualServer(),
        rttMs, 0, out.size(), active ? 1U : 0U, passed,
        passed ? "PASS (SLA)" : "FAIL"
    });

    CHECK_TRUE(parsed);
    CHECK_TRUE(active);
    CHECK_TRUE(rttMs <= 1500.0);
}

TEST(LiveFirewallE3, ReadPolicyRoute) {
    ZyxelDriver &driver = ZyxelDriver::getInstance();
    if (!driver.isConnected()) return;

    auto t0 = std::chrono::steady_clock::now();
    std::string out;
    SshResult res = driver.getSshClient().executeCommand(ZyxelNetworkCmd::cmdShowPolicyRoute(), out);
    auto t1 = std::chrono::steady_clock::now();

    CHECK_EQUAL(static_cast<int>(SshResult::SUCCESS), static_cast<int>(res));
    double rttMs = std::chrono::duration<double, std::milli>(t1 - t0).count();
    bool passed = (res == SshResult::SUCCESS) && (out.find("index:") != std::string::npos || out.find("Router>") != std::string::npos) && (rttMs <= 2500.0);

    ZyxelBenchmark::record({
        "LiveFirewallE3", "ReadPolicyRoute", ZyxelNetworkCmd::cmdShowPolicyRoute(),
        rttMs, 0, out.size(), 1, passed,
        passed ? "PASS (SLA)" : "FAIL"
    });

    CHECK_TRUE(rttMs <= 2500.0);
}

TEST(LiveFirewallE3, ReadPolicyRouteRuleCount) {
    ZyxelDriver &driver = ZyxelDriver::getInstance();
    if (!driver.isConnected()) return;

    auto t0 = std::chrono::steady_clock::now();
    std::string out;
    SshResult res = driver.getSshClient().executeCommand(ZyxelNetworkCmd::cmdShowPolicyRouteRuleCount(), out);
    auto t1 = std::chrono::steady_clock::now();

    CHECK_EQUAL(static_cast<int>(SshResult::SUCCESS), static_cast<int>(res));
    int count = 0;
    bool parsed = ZyxelNetworkCmd::parsePolicyRouteRuleCount(out, count);
    double rttMs = std::chrono::duration<double, std::milli>(t1 - t0).count();
    bool passed = (res == SshResult::SUCCESS) && parsed && (count >= 0) && (rttMs <= 1500.0);

    ZyxelBenchmark::record({
        "LiveFirewallE3", "ReadPolicyRouteRuleCount", ZyxelNetworkCmd::cmdShowPolicyRouteRuleCount(),
        rttMs, 0, out.size(), static_cast<uint32_t>(count), passed,
        passed ? "PASS (SLA)" : "FAIL"
    });

    CHECK_TRUE(parsed);
    CHECK_TRUE(count >= 0);
    CHECK_TRUE(rttMs <= 1500.0);
}

TEST(LiveFirewallE3, ReadPolicyRouteConnCheck) {
    ZyxelDriver &driver = ZyxelDriver::getInstance();
    if (!driver.isConnected()) return;

    auto t0 = std::chrono::steady_clock::now();
    std::string out;
    SshResult res = driver.getSshClient().executeCommand(ZyxelNetworkCmd::cmdShowPolicyRouteConnCheck(), out);
    auto t1 = std::chrono::steady_clock::now();

    CHECK_EQUAL(static_cast<int>(SshResult::SUCCESS), static_cast<int>(res));
    double rttMs = std::chrono::duration<double, std::milli>(t1 - t0).count();
    bool passed = (res == SshResult::SUCCESS) && (rttMs <= 1500.0);

    ZyxelBenchmark::record({
        "LiveFirewallE3", "ReadPolicyRouteConnCheck", ZyxelNetworkCmd::cmdShowPolicyRouteConnCheck(),
        rttMs, 0, out.size(), 1, passed,
        passed ? "PASS (SLA)" : "FAIL"
    });

    CHECK_TRUE(rttMs <= 1500.0);
}

TEST(LiveFirewallE3, ReadPolicyRouteConnCheckStatus) {
    ZyxelDriver &driver = ZyxelDriver::getInstance();
    if (!driver.isConnected()) return;

    auto t0 = std::chrono::steady_clock::now();
    std::string out;
    SshResult res = driver.getSshClient().executeCommand(ZyxelNetworkCmd::cmdShowPolicyRouteConnCheckStatus(), out);
    auto t1 = std::chrono::steady_clock::now();

    CHECK_EQUAL(static_cast<int>(SshResult::SUCCESS), static_cast<int>(res));
    double rttMs = std::chrono::duration<double, std::milli>(t1 - t0).count();
    bool passed = (res == SshResult::SUCCESS) && (out.find("Policy Index") != std::string::npos) && (rttMs <= 1500.0);

    ZyxelBenchmark::record({
        "LiveFirewallE3", "ReadPolicyRouteConnCheckStatus", ZyxelNetworkCmd::cmdShowPolicyRouteConnCheckStatus(),
        rttMs, 0, out.size(), 1, passed,
        passed ? "PASS (SLA)" : "FAIL"
    });

    CHECK_TRUE(out.find("Policy Index") != std::string::npos);
    CHECK_TRUE(rttMs <= 1500.0);
}

TEST(LiveFirewallE3, ReadPolicyRouteOverrideDirectRoute) {
    ZyxelDriver &driver = ZyxelDriver::getInstance();
    if (!driver.isConnected()) return;

    auto t0 = std::chrono::steady_clock::now();
    std::string out;
    SshResult res = driver.getSshClient().executeCommand(ZyxelNetworkCmd::cmdShowPolicyRouteOverrideDirectRoute(), out);
    auto t1 = std::chrono::steady_clock::now();

    CHECK_EQUAL(static_cast<int>(SshResult::SUCCESS), static_cast<int>(res));
    bool active = false;
    bool parsed = ZyxelNetworkCmd::parsePolicyRouteOverrideDirectRoute(out, active);
    double rttMs = std::chrono::duration<double, std::milli>(t1 - t0).count();
    bool passed = (res == SshResult::SUCCESS) && parsed && (rttMs <= 1500.0);

    ZyxelBenchmark::record({
        "LiveFirewallE3", "ReadPolicyRouteOverrideDirectRoute", ZyxelNetworkCmd::cmdShowPolicyRouteOverrideDirectRoute(),
        rttMs, 0, out.size(), active ? 1U : 0U, passed,
        passed ? "PASS (SLA)" : "FAIL"
    });

    CHECK_TRUE(parsed);
    CHECK_TRUE(rttMs <= 1500.0);
}

TEST(LiveFirewallE3, ReadPolicyRouteUnderlayerRules) {
    ZyxelDriver &driver = ZyxelDriver::getInstance();
    if (!driver.isConnected()) return;

    auto t0 = std::chrono::steady_clock::now();
    std::string out;
    SshResult res = driver.getSshClient().executeCommand(ZyxelNetworkCmd::cmdShowPolicyRouteUnderlayerRules(), out);
    auto t1 = std::chrono::steady_clock::now();

    CHECK_EQUAL(static_cast<int>(SshResult::SUCCESS), static_cast<int>(res));
    double rttMs = std::chrono::duration<double, std::milli>(t1 - t0).count();
    bool passed = (res == SshResult::SUCCESS) && (out.find("index:") != std::string::npos) && (rttMs <= 2500.0);

    ZyxelBenchmark::record({
        "LiveFirewallE3", "ReadPolicyRouteUnderlayerRules", ZyxelNetworkCmd::cmdShowPolicyRouteUnderlayerRules(),
        rttMs, 0, out.size(), 1, passed,
        passed ? "PASS (SLA)" : "FAIL"
    });

    CHECK_TRUE(out.find("index:") != std::string::npos);
    CHECK_TRUE(rttMs <= 2500.0);
}

TEST(LiveFirewallE3, ReadPolicyRouteControlVirtualServer) {
    ZyxelDriver &driver = ZyxelDriver::getInstance();
    if (!driver.isConnected()) return;

    auto t0 = std::chrono::steady_clock::now();
    std::string out;
    SshResult res = driver.getSshClient().executeCommand(ZyxelNetworkCmd::cmdShowPolicyRouteControlVirtualServer(), out);
    auto t1 = std::chrono::steady_clock::now();

    CHECK_EQUAL(static_cast<int>(SshResult::SUCCESS), static_cast<int>(res));
    bool active = false;
    bool parsed = ZyxelNetworkCmd::parsePolicyRouteControlVirtualServer(out, active);
    double rttMs = std::chrono::duration<double, std::milli>(t1 - t0).count();
    bool passed = (res == SshResult::SUCCESS) && parsed && (rttMs <= 1500.0);

    ZyxelBenchmark::record({
        "LiveFirewallE3", "ReadPolicyRouteControlVirtualServer", ZyxelNetworkCmd::cmdShowPolicyRouteControlVirtualServer(),
        rttMs, 0, out.size(), active ? 1U : 0U, passed,
        passed ? "PASS (SLA)" : "FAIL"
    });

    CHECK_TRUE(parsed);
    CHECK_TRUE(rttMs <= 1500.0);
}

TEST(LiveFirewallE3, ReadPolicyRouteControlIpsecDynamic) {
    ZyxelDriver &driver = ZyxelDriver::getInstance();
    if (!driver.isConnected()) return;

    auto t0 = std::chrono::steady_clock::now();
    std::string out;
    SshResult res = driver.getSshClient().executeCommand(ZyxelNetworkCmd::cmdShowPolicyRouteControlIpsecDynamic(), out);
    auto t1 = std::chrono::steady_clock::now();

    CHECK_EQUAL(static_cast<int>(SshResult::SUCCESS), static_cast<int>(res));
    bool active = false;
    bool parsed = ZyxelNetworkCmd::parsePolicyRouteControlVirtualServer(out, active);
    double rttMs = std::chrono::duration<double, std::milli>(t1 - t0).count();
    bool passed = (res == SshResult::SUCCESS) && parsed && (rttMs <= 1500.0);

    ZyxelBenchmark::record({
        "LiveFirewallE3", "ReadPolicyRouteControlIpsecDynamic", ZyxelNetworkCmd::cmdShowPolicyRouteControlIpsecDynamic(),
        rttMs, 0, out.size(), active ? 1U : 0U, passed,
        passed ? "PASS (SLA)" : "FAIL"
    });

    CHECK_TRUE(parsed);
    CHECK_TRUE(rttMs <= 1500.0);
}

TEST(LiveFirewallE3, ReadPolicyRouteBeginEnd) {
    ZyxelDriver &driver = ZyxelDriver::getInstance();
    if (!driver.isConnected()) return;

    auto t0 = std::chrono::steady_clock::now();
    std::string out;
    SshResult res = driver.getSshClient().executeCommand(ZyxelNetworkCmd::cmdShowPolicyRouteBeginEnd(1, 5), out);
    auto t1 = std::chrono::steady_clock::now();

    CHECK_EQUAL(static_cast<int>(SshResult::SUCCESS), static_cast<int>(res));
    double rttMs = std::chrono::duration<double, std::milli>(t1 - t0).count();
    bool passed = (res == SshResult::SUCCESS) && (out.find("index: 1") != std::string::npos || out.find("Router>") != std::string::npos) && (rttMs <= 2500.0);

    ZyxelBenchmark::record({
        "LiveFirewallE3", "ReadPolicyRouteBeginEnd", ZyxelNetworkCmd::cmdShowPolicyRouteBeginEnd(1, 5),
        rttMs, 0, out.size(), 1, passed,
        passed ? "PASS (SLA)" : "FAIL"
    });

    CHECK_TRUE(rttMs <= 2500.0);
}

TEST(LiveFirewallE3, ReadPolicyRoute6) {
    ZyxelDriver &driver = ZyxelDriver::getInstance();
    if (!driver.isConnected()) return;

    auto t0 = std::chrono::steady_clock::now();
    std::string out;
    SshResult res = driver.getSshClient().executeCommand(ZyxelNetworkCmd::cmdShowPolicyRoute6(), out);
    auto t1 = std::chrono::steady_clock::now();

    CHECK_EQUAL(static_cast<int>(SshResult::SUCCESS), static_cast<int>(res));
    double rttMs = std::chrono::duration<double, std::milli>(t1 - t0).count();
    bool passed = (res == SshResult::SUCCESS) && (rttMs <= 1500.0);

    ZyxelBenchmark::record({
        "LiveFirewallE3", "ReadPolicyRoute6", ZyxelNetworkCmd::cmdShowPolicyRoute6(),
        rttMs, 0, out.size(), 1, passed,
        passed ? "PASS (SLA)" : "FAIL"
    });

    CHECK_TRUE(rttMs <= 1500.0);
}

TEST(LiveFirewallE3, ReadPolicyRoute6RuleCount) {
    ZyxelDriver &driver = ZyxelDriver::getInstance();
    if (!driver.isConnected()) return;

    auto t0 = std::chrono::steady_clock::now();
    std::string out;
    SshResult res = driver.getSshClient().executeCommand(ZyxelNetworkCmd::cmdShowPolicyRoute6RuleCount(), out);
    auto t1 = std::chrono::steady_clock::now();

    CHECK_EQUAL(static_cast<int>(SshResult::SUCCESS), static_cast<int>(res));
    int count = 0;
    bool parsed = ZyxelNetworkCmd::parsePolicyRouteRuleCount(out, count);
    double rttMs = std::chrono::duration<double, std::milli>(t1 - t0).count();
    bool passed = (res == SshResult::SUCCESS) && parsed && (count >= 0) && (rttMs <= 1500.0);

    ZyxelBenchmark::record({
        "LiveFirewallE3", "ReadPolicyRoute6RuleCount", ZyxelNetworkCmd::cmdShowPolicyRoute6RuleCount(),
        rttMs, 0, out.size(), static_cast<uint32_t>(count), passed,
        passed ? "PASS (SLA)" : "FAIL"
    });

    CHECK_TRUE(parsed);
    CHECK_TRUE(count >= 0);
    CHECK_TRUE(rttMs <= 1500.0);
}

TEST(LiveFirewallE3, ReadPolicyRoute6OverrideDirectRoute) {
    ZyxelDriver &driver = ZyxelDriver::getInstance();
    if (!driver.isConnected()) return;

    auto t0 = std::chrono::steady_clock::now();
    std::string out;
    SshResult res = driver.getSshClient().executeCommand(ZyxelNetworkCmd::cmdShowPolicyRoute6OverrideDirectRoute(), out);
    auto t1 = std::chrono::steady_clock::now();

    CHECK_EQUAL(static_cast<int>(SshResult::SUCCESS), static_cast<int>(res));
    bool active = false;
    bool parsed = ZyxelNetworkCmd::parsePolicyRouteOverrideDirectRoute(out, active);
    double rttMs = std::chrono::duration<double, std::milli>(t1 - t0).count();
    bool passed = (res == SshResult::SUCCESS) && parsed && (rttMs <= 1500.0);

    ZyxelBenchmark::record({
        "LiveFirewallE3", "ReadPolicyRoute6OverrideDirectRoute", ZyxelNetworkCmd::cmdShowPolicyRoute6OverrideDirectRoute(),
        rttMs, 0, out.size(), active ? 1U : 0U, passed,
        passed ? "PASS (SLA)" : "FAIL"
    });

    CHECK_TRUE(parsed);
    CHECK_TRUE(rttMs <= 1500.0);
}

TEST(LiveFirewallE3, ReadPolicyRoute6ControlIpsecDynamic) {
    ZyxelDriver &driver = ZyxelDriver::getInstance();
    if (!driver.isConnected()) return;

    auto t0 = std::chrono::steady_clock::now();
    std::string out;
    SshResult res = driver.getSshClient().executeCommand(ZyxelNetworkCmd::cmdShowPolicyRoute6ControlIpsecDynamic(), out);
    auto t1 = std::chrono::steady_clock::now();

    CHECK_EQUAL(static_cast<int>(SshResult::SUCCESS), static_cast<int>(res));
    bool active = false;
    bool parsed = ZyxelNetworkCmd::parsePolicyRouteControlVirtualServer(out, active);
    double rttMs = std::chrono::duration<double, std::milli>(t1 - t0).count();
    bool passed = (res == SshResult::SUCCESS) && parsed && (rttMs <= 1500.0);

    ZyxelBenchmark::record({
        "LiveFirewallE3", "ReadPolicyRoute6ControlIpsecDynamic", ZyxelNetworkCmd::cmdShowPolicyRoute6ControlIpsecDynamic(),
        rttMs, 0, out.size(), active ? 1U : 0U, passed,
        passed ? "PASS (SLA)" : "FAIL"
    });

    CHECK_TRUE(parsed);
    CHECK_TRUE(rttMs <= 1500.0);
}

TEST(LiveFirewallE3, ReadBwmActivation) {
    ZyxelDriver &driver = ZyxelDriver::getInstance();
    if (!driver.isConnected()) return;

    auto t0 = std::chrono::steady_clock::now();
    std::string out;
    SshResult res = driver.getSshClient().executeCommand(ZyxelNetworkCmd::cmdShowBwmActivation(), out);
    auto t1 = std::chrono::steady_clock::now();

    CHECK_EQUAL(static_cast<int>(SshResult::SUCCESS), static_cast<int>(res));
    bool active = false;
    bool parsed = ZyxelNetworkCmd::parseBwmActivation(out, active);
    double rttMs = std::chrono::duration<double, std::milli>(t1 - t0).count();
    bool passed = (res == SshResult::SUCCESS) && parsed && active && (rttMs <= 1500.0);

    ZyxelBenchmark::record({
        "LiveFirewallE3", "ReadBwmActivation", ZyxelNetworkCmd::cmdShowBwmActivation(),
        rttMs, 0, out.size(), active ? 1U : 0U, passed,
        passed ? "PASS (SLA)" : "FAIL"
    });

    CHECK_TRUE(parsed);
    CHECK_TRUE(active);
    CHECK_TRUE(rttMs <= 1500.0);
}

TEST(LiveFirewallE3, ReadOspfGlobal) {
    ZyxelDriver &driver = ZyxelDriver::getInstance();
    if (!driver.isConnected()) return;

    auto t0 = std::chrono::steady_clock::now();
    std::string out;
    SshResult res = driver.getSshClient().executeCommand(ZyxelNetworkCmd::cmdShowOspfGlobal(), out);
    auto t1 = std::chrono::steady_clock::now();

    CHECK_EQUAL(static_cast<int>(SshResult::SUCCESS), static_cast<int>(res));
    ZyxelOspfGlobalInfo info;
    bool parsed = ZyxelNetworkCmd::parseOspfGlobal(out, info);
    double rttMs = std::chrono::duration<double, std::milli>(t1 - t0).count();
    bool passed = (res == SshResult::SUCCESS) && parsed && (info.routerId == "default") && (rttMs <= 1500.0);

    ZyxelBenchmark::record({
        "LiveFirewallE3", "ReadOspfGlobal", ZyxelNetworkCmd::cmdShowOspfGlobal(),
        rttMs, 0, out.size(), 1, passed,
        passed ? "PASS (SLA)" : "FAIL"
    });

    CHECK_TRUE(parsed);
    STRCMP_EQUAL("default", info.routerId.c_str());
    CHECK_TRUE(rttMs <= 1500.0);
}

TEST(LiveFirewallE3, ReadOspfDatabase) {
    ZyxelDriver &driver = ZyxelDriver::getInstance();
    if (!driver.isConnected()) return;

    auto t0 = std::chrono::steady_clock::now();
    std::string out;
    SshResult res = driver.getSshClient().executeCommand(ZyxelNetworkCmd::cmdShowOspfDatabase(), out);
    auto t1 = std::chrono::steady_clock::now();

    CHECK_EQUAL(static_cast<int>(SshResult::SUCCESS), static_cast<int>(res));
    double rttMs = std::chrono::duration<double, std::milli>(t1 - t0).count();
    bool passed = (res == SshResult::SUCCESS) && (out.find("Area") != std::string::npos) && (rttMs <= 2500.0);

    ZyxelBenchmark::record({
        "LiveFirewallE3", "ReadOspfDatabase", ZyxelNetworkCmd::cmdShowOspfDatabase(),
        rttMs, 0, out.size(), 1, passed,
        passed ? "PASS (SLA)" : "FAIL"
    });

    CHECK_TRUE(out.find("Area") != std::string::npos);
    CHECK_TRUE(rttMs <= 2500.0);
}

TEST(LiveFirewallE3, ReadOspfNeighbor) {
    ZyxelDriver &driver = ZyxelDriver::getInstance();
    if (!driver.isConnected()) return;

    auto t0 = std::chrono::steady_clock::now();
    std::string out;
    SshResult res = driver.getSshClient().executeCommand(ZyxelNetworkCmd::cmdShowOspfNeighbor(), out);
    auto t1 = std::chrono::steady_clock::now();

    CHECK_EQUAL(static_cast<int>(SshResult::SUCCESS), static_cast<int>(res));
    double rttMs = std::chrono::duration<double, std::milli>(t1 - t0).count();
    bool passed = (res == SshResult::SUCCESS) && (out.find("Neighbor ID") != std::string::npos) && (rttMs <= 2500.0);

    ZyxelBenchmark::record({
        "LiveFirewallE3", "ReadOspfNeighbor", ZyxelNetworkCmd::cmdShowOspfNeighbor(),
        rttMs, 0, out.size(), 1, passed,
        passed ? "PASS (SLA)" : "FAIL"
    });

    CHECK_TRUE(out.find("Neighbor ID") != std::string::npos);
    CHECK_TRUE(rttMs <= 2500.0);
}

TEST(LiveFirewallE3, ReadRipGlobal) {
    ZyxelDriver &driver = ZyxelDriver::getInstance();
    if (!driver.isConnected()) return;

    auto t0 = std::chrono::steady_clock::now();
    std::string out;
    SshResult res = driver.getSshClient().executeCommand(ZyxelNetworkCmd::cmdShowRipGlobal(), out);
    auto t1 = std::chrono::steady_clock::now();

    CHECK_EQUAL(static_cast<int>(SshResult::SUCCESS), static_cast<int>(res));
    ZyxelRipGlobalInfo info;
    bool parsed = ZyxelNetworkCmd::parseRipGlobal(out, info);
    double rttMs = std::chrono::duration<double, std::milli>(t1 - t0).count();
    bool passed = (res == SshResult::SUCCESS) && parsed && (info.authType == "none") && (rttMs <= 1500.0);

    ZyxelBenchmark::record({
        "LiveFirewallE3", "ReadRipGlobal", ZyxelNetworkCmd::cmdShowRipGlobal(),
        rttMs, 0, out.size(), 1, passed,
        passed ? "PASS (SLA)" : "FAIL"
    });

    CHECK_TRUE(parsed);
    STRCMP_EQUAL("none", info.authType.c_str());
    CHECK_TRUE(rttMs <= 1500.0);
}

TEST(LiveFirewallE3, ReadBgpGlobal) {
    ZyxelDriver &driver = ZyxelDriver::getInstance();
    if (!driver.isConnected()) return;

    auto t0 = std::chrono::steady_clock::now();
    std::string out;
    SshResult res = driver.getSshClient().executeCommand(ZyxelNetworkCmd::cmdShowBgpGlobal(), out);
    auto t1 = std::chrono::steady_clock::now();

    CHECK_EQUAL(static_cast<int>(SshResult::SUCCESS), static_cast<int>(res));
    ZyxelBgpGlobalInfo info;
    bool parsed = ZyxelNetworkCmd::parseBgpGlobal(out, info);
    double rttMs = std::chrono::duration<double, std::milli>(t1 - t0).count();
    bool passed = (res == SshResult::SUCCESS) && parsed && (info.asNumber == 64512) && (rttMs <= 1500.0);

    ZyxelBenchmark::record({
        "LiveFirewallE3", "ReadBgpGlobal", ZyxelNetworkCmd::cmdShowBgpGlobal(),
        rttMs, 0, out.size(), 1, passed,
        passed ? "PASS (SLA)" : "FAIL"
    });

    CHECK_TRUE(parsed);
    LONGS_EQUAL(64512, info.asNumber);
    CHECK_TRUE(rttMs <= 1500.0);
}

TEST(LiveFirewallE3, ReadBgpSummary) {
    ZyxelDriver &driver = ZyxelDriver::getInstance();
    if (!driver.isConnected()) return;

    auto t0 = std::chrono::steady_clock::now();
    std::string out;
    SshResult res = driver.getSshClient().executeCommand(ZyxelNetworkCmd::cmdShowBgpSummary(), out);
    auto t1 = std::chrono::steady_clock::now();

    CHECK_EQUAL(static_cast<int>(SshResult::SUCCESS), static_cast<int>(res));
    double rttMs = std::chrono::duration<double, std::milli>(t1 - t0).count();
    bool passed = (res == SshResult::SUCCESS) && (out.find("Neighbor") != std::string::npos) && (rttMs <= 1500.0);

    ZyxelBenchmark::record({
        "LiveFirewallE3", "ReadBgpSummary", ZyxelNetworkCmd::cmdShowBgpSummary(),
        rttMs, 0, out.size(), 1, passed,
        passed ? "PASS (SLA)" : "FAIL"
    });

    CHECK_TRUE(out.find("Neighbor") != std::string::npos);
    CHECK_TRUE(rttMs <= 1500.0);
}

TEST(LiveFirewallE3, ReadBgpRoute) {
    ZyxelDriver &driver = ZyxelDriver::getInstance();
    if (!driver.isConnected()) return;

    auto t0 = std::chrono::steady_clock::now();
    std::string out;
    SshResult res = driver.getSshClient().executeCommand(ZyxelNetworkCmd::cmdShowBgpRoute(), out);
    auto t1 = std::chrono::steady_clock::now();

    CHECK_EQUAL(static_cast<int>(SshResult::SUCCESS), static_cast<int>(res));
    double rttMs = std::chrono::duration<double, std::milli>(t1 - t0).count();
    bool passed = (res == SshResult::SUCCESS) && (out.find("Network") != std::string::npos) && (rttMs <= 1500.0);

    ZyxelBenchmark::record({
        "LiveFirewallE3", "ReadBgpRoute", ZyxelNetworkCmd::cmdShowBgpRoute(),
        rttMs, 0, out.size(), 1, passed,
        passed ? "PASS (SLA)" : "FAIL"
    });

    CHECK_TRUE(out.find("Network") != std::string::npos);
    CHECK_TRUE(rttMs <= 1500.0);
}

TEST(LiveFirewallE3, ReadBgpMem) {
    ZyxelDriver &driver = ZyxelDriver::getInstance();
    if (!driver.isConnected()) return;

    auto t0 = std::chrono::steady_clock::now();
    std::string out;
    SshResult res = driver.getSshClient().executeCommand(ZyxelNetworkCmd::cmdShowBgpMem(), out);
    auto t1 = std::chrono::steady_clock::now();

    CHECK_EQUAL(static_cast<int>(SshResult::SUCCESS), static_cast<int>(res));
    double rttMs = std::chrono::duration<double, std::milli>(t1 - t0).count();
    bool passed = (res == SshResult::SUCCESS) && (out.find("RIB nodes") != std::string::npos) && (rttMs <= 1500.0);

    ZyxelBenchmark::record({
        "LiveFirewallE3", "ReadBgpMem", ZyxelNetworkCmd::cmdShowBgpMem(),
        rttMs, 0, out.size(), 1, passed,
        passed ? "PASS (SLA)" : "FAIL"
    });

    CHECK_TRUE(out.find("RIB nodes") != std::string::npos);
    CHECK_TRUE(rttMs <= 1500.0);
}

TEST(LiveFirewallE3, ReadBgpNeighbor) {
    ZyxelDriver &driver = ZyxelDriver::getInstance();
    if (!driver.isConnected()) return;

    auto t0 = std::chrono::steady_clock::now();
    std::string out;
    SshResult res = driver.getSshClient().executeCommand(ZyxelNetworkCmd::cmdShowBgpNeighbor(), out);
    auto t1 = std::chrono::steady_clock::now();

    CHECK_EQUAL(static_cast<int>(SshResult::SUCCESS), static_cast<int>(res));
    double rttMs = std::chrono::duration<double, std::milli>(t1 - t0).count();
    bool passed = (res == SshResult::SUCCESS) && (rttMs <= 1500.0);

    ZyxelBenchmark::record({
        "LiveFirewallE3", "ReadBgpNeighbor", ZyxelNetworkCmd::cmdShowBgpNeighbor(),
        rttMs, 0, out.size(), 1, passed,
        passed ? "PASS (SLA)" : "FAIL"
    });

    CHECK_TRUE(rttMs <= 1500.0);
}

TEST(LiveFirewallE3, DryFireInvalidRoute) {
    ZyxelDriver &driver = ZyxelDriver::getInstance();
    if (!driver.isConnected()) return;

    auto t0 = std::chrono::steady_clock::now();
    std::string out;
    SshResult res = driver.getSshClient().executeCommand(ZyxelNetworkCmd::cmdInvalidRouteDryFire(), out);
    auto t1 = std::chrono::steady_clock::now();

    bool rejected = (res == SshResult::ERR_SYNTAX || res == SshResult::ERR_EXEC_FAILED ||
                     out.find("Parse error") != std::string::npos ||
                     out.find("ERROR") != std::string::npos);
    double rttMs = std::chrono::duration<double, std::milli>(t1 - t0).count();
    bool passed = rejected && (rttMs <= 1500.0);

    ZyxelBenchmark::record({
        "LiveFirewallE3", "DryFireInvalidRoute", ZyxelNetworkCmd::cmdInvalidRouteDryFire(),
        rttMs, 0, out.size(), rejected ? 1U : 0U, passed,
        passed ? "PASS (REJECT)" : "FAIL"
    });

    CHECK_TRUE(rejected);
    CHECK_TRUE(rttMs <= 1500.0);
}

TEST(LiveFirewallE3, DryFireInvalidPolicyRoute) {
    ZyxelDriver &driver = ZyxelDriver::getInstance();
    if (!driver.isConnected()) return;

    auto t0 = std::chrono::steady_clock::now();
    std::string out;
    SshResult res = driver.getSshClient().executeCommand(ZyxelNetworkCmd::cmdInvalidPolicyRouteDryFire(), out);
    auto t1 = std::chrono::steady_clock::now();

    bool rejected = (res == SshResult::ERR_SYNTAX || res == SshResult::ERR_EXEC_FAILED ||
                     out.find("Parse error") != std::string::npos ||
                     out.find("ERROR") != std::string::npos);
    double rttMs = std::chrono::duration<double, std::milli>(t1 - t0).count();
    bool passed = rejected && (rttMs <= 1500.0);

    ZyxelBenchmark::record({
        "LiveFirewallE3", "DryFireInvalidPolicyRoute", ZyxelNetworkCmd::cmdInvalidPolicyRouteDryFire(),
        rttMs, 0, out.size(), rejected ? 1U : 0U, passed,
        passed ? "PASS (REJECT)" : "FAIL"
    });

    CHECK_TRUE(rejected);
    CHECK_TRUE(rttMs <= 1500.0);
}

TEST(LiveFirewallE3, DryFireInvalidOspfArea) {
    ZyxelDriver &driver = ZyxelDriver::getInstance();
    if (!driver.isConnected()) return;

    auto t0 = std::chrono::steady_clock::now();
    std::string out;
    SshResult res = driver.getSshClient().executeCommand(ZyxelNetworkCmd::cmdInvalidOspfAreaDryFire(), out);
    auto t1 = std::chrono::steady_clock::now();

    bool rejected = (res == SshResult::ERR_SYNTAX || res == SshResult::ERR_EXEC_FAILED ||
                     out.find("Cannot find the designated OSPF area") != std::string::npos ||
                     out.find("ERROR") != std::string::npos);
    double rttMs = std::chrono::duration<double, std::milli>(t1 - t0).count();
    bool passed = rejected && (rttMs <= 1500.0);

    ZyxelBenchmark::record({
        "LiveFirewallE3", "DryFireInvalidOspfArea", ZyxelNetworkCmd::cmdInvalidOspfAreaDryFire(),
        rttMs, 0, out.size(), rejected ? 1U : 0U, passed,
        passed ? "PASS (REJECT)" : "FAIL"
    });

    CHECK_TRUE(rejected);
    CHECK_TRUE(rttMs <= 1500.0);
}

// ============================================================================
// Test Group: LiveFirewallE4 (Objects and Schedules)
// ============================================================================

TEST_GROUP(LiveFirewallE4) {
    void sweep() {
        ZyxelDriver &driver = ZyxelDriver::getInstance();
        if (!driver.isConnected()) return;
        ZyxelSshClient &ssh = driver.getSshClient();
        std::string out;
        ssh.unwindToRootPrompt();
        ssh.executeCommand("configure terminal", out);
        ssh.executeCommand("no object-group address6 NETMON_QA_GRP6", out);
        ssh.executeCommand("no address6-object NETMON_QA_SUBNET6", out);
        ssh.executeCommand("no address6-object NETMON_QA_ADDR6", out);
        ssh.executeCommand("no schedule-object NETMON_QA_SCHED", out);
        ssh.executeCommand("exit", out);
        ssh.unwindToRootPrompt();
    }

    void setup() override {
        sweep();
        std::this_thread::sleep_for(std::chrono::milliseconds(25));
    }

    void teardown() override {
        sweep();
    }
};

TEST(LiveFirewallE4, ReadAddressObjects) {
    ZyxelDriver &driver = ZyxelDriver::getInstance();
    if (!driver.isConnected()) return;

    auto t0 = std::chrono::steady_clock::now();
    std::string out;
    SshResult res = driver.getSshClient().executeCommand(ZyxelObjectCmd::cmdShowAddressObjects(), out);
    auto t1 = std::chrono::steady_clock::now();

    CHECK_EQUAL(static_cast<int>(SshResult::SUCCESS), static_cast<int>(res));
    auto p0 = std::chrono::steady_clock::now();
    std::vector<ZyxelAddressObject> objects;
    bool parsed = ZyxelObjectCmd::parseAddressObjects(out, objects);
    auto p1 = std::chrono::steady_clock::now();

    double rttMs = std::chrono::duration<double, std::milli>(t1 - t0).count();
    uint64_t parseUs = std::chrono::duration_cast<std::chrono::microseconds>(p1 - p0).count();
    bool passed = (res == SshResult::SUCCESS) && parsed && !objects.empty() && (rttMs <= 2500.0);

    ZyxelBenchmark::record({
        "LiveFirewallE4", "ReadAddressObjects", ZyxelObjectCmd::cmdShowAddressObjects(),
        rttMs, parseUs, out.size(), static_cast<uint32_t>(objects.size()), passed,
        passed ? "PASS (SLA)" : "FAIL"
    });

    CHECK_TRUE(parsed);
    CHECK_FALSE(objects.empty());
    CHECK_TRUE(rttMs <= 2500.0);
}

TEST(LiveFirewallE4, ReadAddressObjectSpecific) {
    ZyxelDriver &driver = ZyxelDriver::getInstance();
    if (!driver.isConnected()) return;

    auto t0 = std::chrono::steady_clock::now();
    std::string out;
    SshResult res = driver.getSshClient().executeCommand(ZyxelObjectCmd::cmdShowAddressObjects("LAN1_SUBNET"), out);
    auto t1 = std::chrono::steady_clock::now();

    CHECK_EQUAL(static_cast<int>(SshResult::SUCCESS), static_cast<int>(res));
    bool found = (out.find("LAN1_SUBNET") != std::string::npos);
    double rttMs = std::chrono::duration<double, std::milli>(t1 - t0).count();
    bool passed = (res == SshResult::SUCCESS) && found && (rttMs <= 2500.0);

    ZyxelBenchmark::record({
        "LiveFirewallE4", "ReadAddressObjectSpecific", ZyxelObjectCmd::cmdShowAddressObjects("LAN1_SUBNET"),
        rttMs, 0, out.size(), found ? 1U : 0U, passed,
        passed ? "PASS (SLA)" : "FAIL"
    });

    CHECK_TRUE(found);
    CHECK_TRUE(rttMs <= 2500.0);
}

TEST(LiveFirewallE4, ReadAddress6Objects) {
    ZyxelDriver &driver = ZyxelDriver::getInstance();
    if (!driver.isConnected()) return;

    auto t0 = std::chrono::steady_clock::now();
    std::string out;
    SshResult res = driver.getSshClient().executeCommand(ZyxelObjectCmd::cmdShowAddress6Objects(), out);
    auto t1 = std::chrono::steady_clock::now();

    CHECK_EQUAL(static_cast<int>(SshResult::SUCCESS), static_cast<int>(res));
    auto p0 = std::chrono::steady_clock::now();
    std::vector<ZyxelAddress6Object> objects;
    bool parsed = ZyxelObjectCmd::parseAddress6Objects(out, objects);
    auto p1 = std::chrono::steady_clock::now();

    double rttMs = std::chrono::duration<double, std::milli>(t1 - t0).count();
    uint64_t parseUs = std::chrono::duration_cast<std::chrono::microseconds>(p1 - p0).count();
    bool passed = (res == SshResult::SUCCESS) && parsed && !objects.empty() && (rttMs <= 2500.0);

    ZyxelBenchmark::record({
        "LiveFirewallE4", "ReadAddress6Objects", ZyxelObjectCmd::cmdShowAddress6Objects(),
        rttMs, parseUs, out.size(), static_cast<uint32_t>(objects.size()), passed,
        passed ? "PASS (SLA)" : "FAIL"
    });

    CHECK_TRUE(parsed);
    CHECK_FALSE(objects.empty());
    CHECK_TRUE(rttMs <= 2500.0);
}

TEST(LiveFirewallE4, ReadAddress6ObjectSpecific) {
    ZyxelDriver &driver = ZyxelDriver::getInstance();
    if (!driver.isConnected()) return;

    auto t0 = std::chrono::steady_clock::now();
    std::string out;
    SshResult res = driver.getSshClient().executeCommand(ZyxelObjectCmd::cmdShowAddress6Objects("LAN1_SUBNET_STATIC"), out);
    auto t1 = std::chrono::steady_clock::now();

    CHECK_EQUAL(static_cast<int>(SshResult::SUCCESS), static_cast<int>(res));
    bool found = (out.find("LAN1_SUBNET_STATIC") != std::string::npos);
    double rttMs = std::chrono::duration<double, std::milli>(t1 - t0).count();
    bool passed = (res == SshResult::SUCCESS) && found && (rttMs <= 2500.0);

    ZyxelBenchmark::record({
        "LiveFirewallE4", "ReadAddress6ObjectSpecific", ZyxelObjectCmd::cmdShowAddress6Objects("LAN1_SUBNET_STATIC"),
        rttMs, 0, out.size(), found ? 1U : 0U, passed,
        passed ? "PASS (SLA)" : "FAIL"
    });

    CHECK_TRUE(found);
    CHECK_TRUE(rttMs <= 2500.0);
}

TEST(LiveFirewallE4, ReadAddressGroups) {
    ZyxelDriver &driver = ZyxelDriver::getInstance();
    if (!driver.isConnected()) return;

    auto t0 = std::chrono::steady_clock::now();
    std::string out;
    SshResult res = driver.getSshClient().executeCommand(ZyxelObjectCmd::cmdShowAddressGroup(), out);
    auto t1 = std::chrono::steady_clock::now();

    CHECK_EQUAL(static_cast<int>(SshResult::SUCCESS), static_cast<int>(res));
    double rttMs = std::chrono::duration<double, std::milli>(t1 - t0).count();
    bool passed = (res == SshResult::SUCCESS) && (rttMs <= 2500.0);

    ZyxelBenchmark::record({
        "LiveFirewallE4", "ReadAddressGroups", ZyxelObjectCmd::cmdShowAddressGroup(),
        rttMs, 0, out.size(), 1, passed,
        passed ? "PASS (SLA)" : "FAIL"
    });

    CHECK_TRUE(rttMs <= 2500.0);
}

TEST(LiveFirewallE4, ReadAddress6Groups) {
    ZyxelDriver &driver = ZyxelDriver::getInstance();
    if (!driver.isConnected()) return;

    auto t0 = std::chrono::steady_clock::now();
    std::string out;
    SshResult res = driver.getSshClient().executeCommand(ZyxelObjectCmd::cmdShowAddress6Group(), out);
    auto t1 = std::chrono::steady_clock::now();

    CHECK_EQUAL(static_cast<int>(SshResult::SUCCESS), static_cast<int>(res));
    double rttMs = std::chrono::duration<double, std::milli>(t1 - t0).count();
    bool passed = (res == SshResult::SUCCESS) && (rttMs <= 2500.0);

    ZyxelBenchmark::record({
        "LiveFirewallE4", "ReadAddress6Groups", ZyxelObjectCmd::cmdShowAddress6Group(),
        rttMs, 0, out.size(), 1, passed,
        passed ? "PASS (SLA)" : "FAIL"
    });

    CHECK_TRUE(rttMs <= 2500.0);
}

TEST(LiveFirewallE4, ReadServiceObjects) {
    ZyxelDriver &driver = ZyxelDriver::getInstance();
    if (!driver.isConnected()) return;

    auto t0 = std::chrono::steady_clock::now();
    std::string out;
    SshResult res = driver.getSshClient().executeCommand(ZyxelObjectCmd::cmdShowServiceObjects(), out);
    auto t1 = std::chrono::steady_clock::now();

    CHECK_EQUAL(static_cast<int>(SshResult::SUCCESS), static_cast<int>(res));
    auto p0 = std::chrono::steady_clock::now();
    std::vector<ZyxelServiceObject> svcs;
    bool parsed = ZyxelObjectCmd::parseServiceObjects(out, svcs);
    auto p1 = std::chrono::steady_clock::now();

    double rttMs = std::chrono::duration<double, std::milli>(t1 - t0).count();
    uint64_t parseUs = std::chrono::duration_cast<std::chrono::microseconds>(p1 - p0).count();
    bool passed = (res == SshResult::SUCCESS) && parsed && !svcs.empty() && (rttMs <= 2500.0);

    ZyxelBenchmark::record({
        "LiveFirewallE4", "ReadServiceObjects", ZyxelObjectCmd::cmdShowServiceObjects(),
        rttMs, parseUs, out.size(), static_cast<uint32_t>(svcs.size()), passed,
        passed ? "PASS (SLA)" : "FAIL"
    });

    CHECK_TRUE(parsed);
    CHECK_FALSE(svcs.empty());
    CHECK_TRUE(rttMs <= 2500.0);
}

TEST(LiveFirewallE4, ReadServiceGroups) {
    ZyxelDriver &driver = ZyxelDriver::getInstance();
    if (!driver.isConnected()) return;

    auto t0 = std::chrono::steady_clock::now();
    std::string out;
    SshResult res = driver.getSshClient().executeCommand(ZyxelObjectCmd::cmdShowServiceGroup(), out);
    auto t1 = std::chrono::steady_clock::now();

    CHECK_EQUAL(static_cast<int>(SshResult::SUCCESS), static_cast<int>(res));
    auto p0 = std::chrono::steady_clock::now();
    std::vector<ZyxelServiceGroup> groups;
    bool parsed = ZyxelObjectCmd::parseServiceGroups(out, groups);
    auto p1 = std::chrono::steady_clock::now();

    double rttMs = std::chrono::duration<double, std::milli>(t1 - t0).count();
    uint64_t parseUs = std::chrono::duration_cast<std::chrono::microseconds>(p1 - p0).count();
    bool passed = (res == SshResult::SUCCESS) && parsed && !groups.empty() && (rttMs <= 2500.0);

    ZyxelBenchmark::record({
        "LiveFirewallE4", "ReadServiceGroups", ZyxelObjectCmd::cmdShowServiceGroup(),
        rttMs, parseUs, out.size(), static_cast<uint32_t>(groups.size()), passed,
        passed ? "PASS (SLA)" : "FAIL"
    });

    CHECK_TRUE(parsed);
    CHECK_FALSE(groups.empty());
    CHECK_TRUE(rttMs <= 2500.0);
}

TEST(LiveFirewallE4, ReadServiceGroupSpecific) {
    ZyxelDriver &driver = ZyxelDriver::getInstance();
    if (!driver.isConnected()) return;

    auto t0 = std::chrono::steady_clock::now();
    std::string out;
    SshResult res = driver.getSshClient().executeCommand(ZyxelObjectCmd::cmdShowServiceGroup("DNS"), out);
    auto t1 = std::chrono::steady_clock::now();

    CHECK_EQUAL(static_cast<int>(SshResult::SUCCESS), static_cast<int>(res));
    auto p0 = std::chrono::steady_clock::now();
    std::vector<ZyxelServiceGroup> detail;
    bool parsed = ZyxelObjectCmd::parseServiceGroups(out, detail);
    auto p1 = std::chrono::steady_clock::now();

    double rttMs = std::chrono::duration<double, std::milli>(t1 - t0).count();
    uint64_t parseUs = std::chrono::duration_cast<std::chrono::microseconds>(p1 - p0).count();
    bool passed = (res == SshResult::SUCCESS) && parsed && !detail.empty() && !detail[0].members.empty() && (rttMs <= 2500.0);

    ZyxelBenchmark::record({
        "LiveFirewallE4", "ReadServiceGroupSpecific", ZyxelObjectCmd::cmdShowServiceGroup("DNS"),
        rttMs, parseUs, out.size(), !detail.empty() ? static_cast<uint32_t>(detail[0].members.size()) : 0U, passed,
        passed ? "PASS (SLA)" : "FAIL"
    });

    CHECK_TRUE(parsed);
    CHECK_FALSE(detail.empty());
    CHECK_TRUE(rttMs <= 2500.0);
}

TEST(LiveFirewallE4, ReadScheduleObjects) {
    ZyxelDriver &driver = ZyxelDriver::getInstance();
    if (!driver.isConnected()) return;

    auto t0 = std::chrono::steady_clock::now();
    std::string out;
    SshResult res = driver.getSshClient().executeCommand(ZyxelObjectCmd::cmdShowScheduleObjects(), out);
    auto t1 = std::chrono::steady_clock::now();

    CHECK_EQUAL(static_cast<int>(SshResult::SUCCESS), static_cast<int>(res));
    auto p0 = std::chrono::steady_clock::now();
    std::vector<ZyxelScheduleObject> schedules;
    bool parsed = ZyxelObjectCmd::parseScheduleObjects(out, schedules);
    auto p1 = std::chrono::steady_clock::now();

    double rttMs = std::chrono::duration<double, std::milli>(t1 - t0).count();
    uint64_t parseUs = std::chrono::duration_cast<std::chrono::microseconds>(p1 - p0).count();
    bool passed = (res == SshResult::SUCCESS) && parsed && (rttMs <= 2500.0);

    ZyxelBenchmark::record({
        "LiveFirewallE4", "ReadScheduleObjects", ZyxelObjectCmd::cmdShowScheduleObjects(),
        rttMs, parseUs, out.size(), static_cast<uint32_t>(schedules.size()), passed,
        passed ? "PASS (SLA)" : "FAIL"
    });

    CHECK_TRUE(parsed);
    CHECK_TRUE(rttMs <= 2500.0);
}

TEST(LiveFirewallE4, ReadScheduleRecurring) {
    ZyxelDriver &driver = ZyxelDriver::getInstance();
    if (!driver.isConnected()) return;

    auto t0 = std::chrono::steady_clock::now();
    std::string out;
    SshResult res = driver.getSshClient().executeCommand(ZyxelObjectCmd::cmdShowScheduleRecurring(), out);
    auto t1 = std::chrono::steady_clock::now();

    // When no recurring schedule exists, firewall returns -43000 error, which is valid response
    bool valid = (res == SshResult::SUCCESS ||
                  out.find("does not exist") != std::string::npos ||
                  out.find("retval = -43000") != std::string::npos);
    double rttMs = std::chrono::duration<double, std::milli>(t1 - t0).count();
    bool passed = valid && (rttMs <= 2500.0);

    ZyxelBenchmark::record({
        "LiveFirewallE4", "ReadScheduleRecurring", ZyxelObjectCmd::cmdShowScheduleRecurring(),
        rttMs, 0, out.size(), 1, passed,
        passed ? "PASS (SLA)" : "FAIL"
    });

    CHECK_TRUE(valid);
    CHECK_TRUE(rttMs <= 2500.0);
}

TEST(LiveFirewallE4, ReadScheduleOneTime) {
    ZyxelDriver &driver = ZyxelDriver::getInstance();
    if (!driver.isConnected()) return;

    auto t0 = std::chrono::steady_clock::now();
    std::string out;
    SshResult res = driver.getSshClient().executeCommand(ZyxelObjectCmd::cmdShowScheduleOneTime(), out);
    auto t1 = std::chrono::steady_clock::now();

    bool valid = (res == SshResult::SUCCESS ||
                  out.find("does not exist") != std::string::npos ||
                  out.find("retval = -43000") != std::string::npos);
    double rttMs = std::chrono::duration<double, std::milli>(t1 - t0).count();
    bool passed = valid && (rttMs <= 2500.0);

    ZyxelBenchmark::record({
        "LiveFirewallE4", "ReadScheduleOneTime", ZyxelObjectCmd::cmdShowScheduleOneTime(),
        rttMs, 0, out.size(), 1, passed,
        passed ? "PASS (SLA)" : "FAIL"
    });

    CHECK_TRUE(valid);
    CHECK_TRUE(rttMs <= 2500.0);
}

TEST(LiveFirewallE4, ReadFqdn) {
    ZyxelDriver &driver = ZyxelDriver::getInstance();
    if (!driver.isConnected()) return;

    auto t0 = std::chrono::steady_clock::now();
    std::string out;
    SshResult res = driver.getSshClient().executeCommand(ZyxelObjectCmd::cmdShowFqdn(), out);
    auto t1 = std::chrono::steady_clock::now();

    CHECK_EQUAL(static_cast<int>(SshResult::SUCCESS), static_cast<int>(res));
    bool valid = (out.find("host name") != std::string::npos);
    double rttMs = std::chrono::duration<double, std::milli>(t1 - t0).count();
    bool passed = (res == SshResult::SUCCESS) && valid && (rttMs <= 2500.0);

    ZyxelBenchmark::record({
        "LiveFirewallE4", "ReadFqdn", ZyxelObjectCmd::cmdShowFqdn(),
        rttMs, 0, out.size(), valid ? 1U : 0U, passed,
        passed ? "PASS (SLA)" : "FAIL"
    });

    CHECK_TRUE(valid);
    CHECK_TRUE(rttMs <= 2500.0);
}

TEST(LiveFirewallE4, ReadFqdnObjects) {
    ZyxelDriver &driver = ZyxelDriver::getInstance();
    if (!driver.isConnected()) return;

    auto t0 = std::chrono::steady_clock::now();
    std::string out;
    SshResult res = driver.getSshClient().executeCommand(ZyxelObjectCmd::cmdShowFqdnObjects(), out);
    auto t1 = std::chrono::steady_clock::now();

    CHECK_EQUAL(static_cast<int>(SshResult::SUCCESS), static_cast<int>(res));
    bool valid = (out.find("Object name") != std::string::npos);
    double rttMs = std::chrono::duration<double, std::milli>(t1 - t0).count();
    bool passed = (res == SshResult::SUCCESS) && valid && (rttMs <= 2500.0);

    ZyxelBenchmark::record({
        "LiveFirewallE4", "ReadFqdnObjects", ZyxelObjectCmd::cmdShowFqdnObjects(),
        rttMs, 0, out.size(), valid ? 1U : 0U, passed,
        passed ? "PASS (SLA)" : "FAIL"
    });

    CHECK_TRUE(valid);
    CHECK_TRUE(rttMs <= 2500.0);
}

TEST(LiveFirewallE4, ReadFqdnQueryPeriod) {
    ZyxelDriver &driver = ZyxelDriver::getInstance();
    if (!driver.isConnected()) return;

    auto t0 = std::chrono::steady_clock::now();
    std::string out;
    SshResult res = driver.getSshClient().executeCommand(ZyxelObjectCmd::cmdShowFqdnQueryPeriod(), out);
    auto t1 = std::chrono::steady_clock::now();

    CHECK_EQUAL(static_cast<int>(SshResult::SUCCESS), static_cast<int>(res));
    bool valid = (out.find("Query Period") != std::string::npos);
    double rttMs = std::chrono::duration<double, std::milli>(t1 - t0).count();
    bool passed = (res == SshResult::SUCCESS) && valid && (rttMs <= 2500.0);

    ZyxelBenchmark::record({
        "LiveFirewallE4", "ReadFqdnQueryPeriod", ZyxelObjectCmd::cmdShowFqdnQueryPeriod(),
        rttMs, 0, out.size(), valid ? 1U : 0U, passed,
        passed ? "PASS (SLA)" : "FAIL"
    });

    CHECK_TRUE(valid);
    CHECK_TRUE(rttMs <= 2500.0);
}

TEST(LiveFirewallE4, ReadFqdnSyncPeriod) {
    ZyxelDriver &driver = ZyxelDriver::getInstance();
    if (!driver.isConnected()) return;

    auto t0 = std::chrono::steady_clock::now();
    std::string out;
    SshResult res = driver.getSshClient().executeCommand(ZyxelObjectCmd::cmdShowFqdnSyncPeriod(), out);
    auto t1 = std::chrono::steady_clock::now();

    CHECK_EQUAL(static_cast<int>(SshResult::SUCCESS), static_cast<int>(res));
    bool valid = (out.find("Sync Period") != std::string::npos);
    double rttMs = std::chrono::duration<double, std::milli>(t1 - t0).count();
    bool passed = (res == SshResult::SUCCESS) && valid && (rttMs <= 2500.0);

    ZyxelBenchmark::record({
        "LiveFirewallE4", "ReadFqdnSyncPeriod", ZyxelObjectCmd::cmdShowFqdnSyncPeriod(),
        rttMs, 0, out.size(), valid ? 1U : 0U, passed,
        passed ? "PASS (SLA)" : "FAIL"
    });

    CHECK_TRUE(valid);
    CHECK_TRUE(rttMs <= 2500.0);
}

TEST(LiveFirewallE4, ReadGeoIpCountryCode) {
    ZyxelDriver &driver = ZyxelDriver::getInstance();
    if (!driver.isConnected()) return;

    auto t0 = std::chrono::steady_clock::now();
    std::string out;
    SshResult res = driver.getSshClient().executeCommand(ZyxelObjectCmd::cmdShowGeoIpCountryCode(), out);
    auto t1 = std::chrono::steady_clock::now();

    CHECK_EQUAL(static_cast<int>(SshResult::SUCCESS), static_cast<int>(res));
    bool valid = (out.find("Country-Code") != std::string::npos);
    double rttMs = std::chrono::duration<double, std::milli>(t1 - t0).count();
    bool passed = (res == SshResult::SUCCESS) && valid && (rttMs <= 2500.0);

    ZyxelBenchmark::record({
        "LiveFirewallE4", "ReadGeoIpCountryCode", ZyxelObjectCmd::cmdShowGeoIpCountryCode(),
        rttMs, 0, out.size(), valid ? 1U : 0U, passed,
        passed ? "PASS (SLA)" : "FAIL"
    });

    CHECK_TRUE(valid);
    CHECK_TRUE(rttMs <= 2500.0);
}

TEST(LiveFirewallE4, ReadGeoIpDatabaseUpdate) {
    ZyxelDriver &driver = ZyxelDriver::getInstance();
    if (!driver.isConnected()) return;

    auto t0 = std::chrono::steady_clock::now();
    std::string out;
    SshResult res = driver.getSshClient().executeCommand(ZyxelObjectCmd::cmdShowGeoIpDatabaseUpdate(), out);
    auto t1 = std::chrono::steady_clock::now();

    CHECK_EQUAL(static_cast<int>(SshResult::SUCCESS), static_cast<int>(res));
    bool valid = (out.find("auto:") != std::string::npos);
    double rttMs = std::chrono::duration<double, std::milli>(t1 - t0).count();
    bool passed = (res == SshResult::SUCCESS) && valid && (rttMs <= 2500.0);

    ZyxelBenchmark::record({
        "LiveFirewallE4", "ReadGeoIpDatabaseUpdate", ZyxelObjectCmd::cmdShowGeoIpDatabaseUpdate(),
        rttMs, 0, out.size(), valid ? 1U : 0U, passed,
        passed ? "PASS (SLA)" : "FAIL"
    });

    CHECK_TRUE(valid);
    CHECK_TRUE(rttMs <= 2500.0);
}

TEST(LiveFirewallE4, ReadGeoIpDatabaseVersion) {
    ZyxelDriver &driver = ZyxelDriver::getInstance();
    if (!driver.isConnected()) return;

    auto t0 = std::chrono::steady_clock::now();
    std::string out;
    SshResult res = driver.getSshClient().executeCommand(ZyxelObjectCmd::cmdShowGeoIpDatabaseVersion(), out);
    auto t1 = std::chrono::steady_clock::now();

    CHECK_EQUAL(static_cast<int>(SshResult::SUCCESS), static_cast<int>(res));
    bool valid = (out.find("version") != std::string::npos);
    double rttMs = std::chrono::duration<double, std::milli>(t1 - t0).count();
    bool passed = (res == SshResult::SUCCESS) && valid && (rttMs <= 2500.0);

    ZyxelBenchmark::record({
        "LiveFirewallE4", "ReadGeoIpDatabaseVersion", ZyxelObjectCmd::cmdShowGeoIpDatabaseVersion(),
        rttMs, 0, out.size(), valid ? 1U : 0U, passed,
        passed ? "PASS (SLA)" : "FAIL"
    });

    CHECK_TRUE(valid);
    CHECK_TRUE(rttMs <= 2500.0);
}

TEST(LiveFirewallE4, ReadAccountPppoe) {
    ZyxelDriver &driver = ZyxelDriver::getInstance();
    if (!driver.isConnected()) return;

    auto t0 = std::chrono::steady_clock::now();
    std::string out;
    SshResult res = driver.getSshClient().executeCommand(ZyxelObjectCmd::cmdShowAccountPppoe(), out);
    auto t1 = std::chrono::steady_clock::now();

    CHECK_EQUAL(static_cast<int>(SshResult::SUCCESS), static_cast<int>(res));
    auto p0 = std::chrono::steady_clock::now();
    std::vector<ZyxelAccountPppoeEntry> pppoe;
    bool parsed = ZyxelObjectCmd::parseAccountPppoe(out, pppoe);
    auto p1 = std::chrono::steady_clock::now();

    double rttMs = std::chrono::duration<double, std::milli>(t1 - t0).count();
    uint64_t parseUs = std::chrono::duration_cast<std::chrono::microseconds>(p1 - p0).count();
    bool passed = (res == SshResult::SUCCESS) && parsed && !pppoe.empty() && (rttMs <= 2500.0);

    ZyxelBenchmark::record({
        "LiveFirewallE4", "ReadAccountPppoe", ZyxelObjectCmd::cmdShowAccountPppoe(),
        rttMs, parseUs, out.size(), static_cast<uint32_t>(pppoe.size()), passed,
        passed ? "PASS (SLA)" : "FAIL"
    });

    CHECK_TRUE(parsed);
    CHECK_FALSE(pppoe.empty());
    CHECK_TRUE(rttMs <= 2500.0);
}

TEST(LiveFirewallE4, ReadAccountPptp) {
    ZyxelDriver &driver = ZyxelDriver::getInstance();
    if (!driver.isConnected()) return;

    auto t0 = std::chrono::steady_clock::now();
    std::string out;
    SshResult res = driver.getSshClient().executeCommand(ZyxelObjectCmd::cmdShowAccountPptp(), out);
    auto t1 = std::chrono::steady_clock::now();

    CHECK_EQUAL(static_cast<int>(SshResult::SUCCESS), static_cast<int>(res));
    bool valid = (out.find("PPTP account") != std::string::npos);
    double rttMs = std::chrono::duration<double, std::milli>(t1 - t0).count();
    bool passed = (res == SshResult::SUCCESS) && valid && (rttMs <= 2500.0);

    ZyxelBenchmark::record({
        "LiveFirewallE4", "ReadAccountPptp", ZyxelObjectCmd::cmdShowAccountPptp(),
        rttMs, 0, out.size(), valid ? 1U : 0U, passed,
        passed ? "PASS (SLA)" : "FAIL"
    });

    CHECK_TRUE(valid);
    CHECK_TRUE(rttMs <= 2500.0);
}

TEST(LiveFirewallE4, ReadAccountCellular) {
    ZyxelDriver &driver = ZyxelDriver::getInstance();
    if (!driver.isConnected()) return;

    auto t0 = std::chrono::steady_clock::now();
    std::string out;
    SshResult res = driver.getSshClient().executeCommand(ZyxelObjectCmd::cmdShowAccountCellular(), out);
    auto t1 = std::chrono::steady_clock::now();

    CHECK_EQUAL(static_cast<int>(SshResult::SUCCESS), static_cast<int>(res));
    bool valid = (out.find("cellular account") != std::string::npos);
    double rttMs = std::chrono::duration<double, std::milli>(t1 - t0).count();
    bool passed = (res == SshResult::SUCCESS) && valid && (rttMs <= 2500.0);

    ZyxelBenchmark::record({
        "LiveFirewallE4", "ReadAccountCellular", ZyxelObjectCmd::cmdShowAccountCellular(),
        rttMs, 0, out.size(), valid ? 1U : 0U, passed,
        passed ? "PASS (SLA)" : "FAIL"
    });

    CHECK_TRUE(valid);
    CHECK_TRUE(rttMs <= 2500.0);
}

TEST(LiveFirewallE4, ReadSslvpnApplication) {
    ZyxelDriver &driver = ZyxelDriver::getInstance();
    if (!driver.isConnected()) return;

    auto t0 = std::chrono::steady_clock::now();
    std::string out;
    SshResult res = driver.getSshClient().executeCommand(ZyxelObjectCmd::cmdShowSslvpnApplication(), out);
    auto t1 = std::chrono::steady_clock::now();

    CHECK_EQUAL(static_cast<int>(SshResult::SUCCESS), static_cast<int>(res));
    double rttMs = std::chrono::duration<double, std::milli>(t1 - t0).count();
    bool passed = (res == SshResult::SUCCESS) && (rttMs <= 2500.0);

    ZyxelBenchmark::record({
        "LiveFirewallE4", "ReadSslvpnApplication", ZyxelObjectCmd::cmdShowSslvpnApplication(),
        rttMs, 0, out.size(), 1, passed,
        passed ? "PASS (SLA)" : "FAIL"
    });

    CHECK_TRUE(rttMs <= 2500.0);
}

TEST(LiveFirewallE4, ReadDhcp6Interface) {
    ZyxelDriver &driver = ZyxelDriver::getInstance();
    if (!driver.isConnected()) return;

    auto t0 = std::chrono::steady_clock::now();
    std::string out;
    SshResult res = driver.getSshClient().executeCommand(ZyxelObjectCmd::cmdShowDhcp6Interface(), out);
    auto t1 = std::chrono::steady_clock::now();

    CHECK_EQUAL(static_cast<int>(SshResult::SUCCESS), static_cast<int>(res));
    bool valid = (out.find("DHCP6 server list") != std::string::npos);
    double rttMs = std::chrono::duration<double, std::milli>(t1 - t0).count();
    bool passed = (res == SshResult::SUCCESS) && valid && (rttMs <= 2500.0);

    ZyxelBenchmark::record({
        "LiveFirewallE4", "ReadDhcp6Interface", ZyxelObjectCmd::cmdShowDhcp6Interface(),
        rttMs, 0, out.size(), valid ? 1U : 0U, passed,
        passed ? "PASS (SLA)" : "FAIL"
    });

    CHECK_TRUE(valid);
    CHECK_TRUE(rttMs <= 2500.0);
}

TEST(LiveFirewallE4, ReadDhcp6LeaseObjects) {
    ZyxelDriver &driver = ZyxelDriver::getInstance();
    if (!driver.isConnected()) return;

    auto t0 = std::chrono::steady_clock::now();
    std::string out;
    SshResult res = driver.getSshClient().executeCommand(ZyxelObjectCmd::cmdShowDhcp6LeaseObjects(), out);
    auto t1 = std::chrono::steady_clock::now();

    CHECK_EQUAL(static_cast<int>(SshResult::SUCCESS), static_cast<int>(res));
    double rttMs = std::chrono::duration<double, std::milli>(t1 - t0).count();
    bool passed = (res == SshResult::SUCCESS) && (rttMs <= 2500.0);

    ZyxelBenchmark::record({
        "LiveFirewallE4", "ReadDhcp6LeaseObjects", ZyxelObjectCmd::cmdShowDhcp6LeaseObjects(),
        rttMs, 0, out.size(), 1, passed,
        passed ? "PASS (SLA)" : "FAIL"
    });

    CHECK_TRUE(rttMs <= 2500.0);
}

TEST(LiveFirewallE4, ReadDhcp6RequestObjects) {
    ZyxelDriver &driver = ZyxelDriver::getInstance();
    if (!driver.isConnected()) return;

    auto t0 = std::chrono::steady_clock::now();
    std::string out;
    SshResult res = driver.getSshClient().executeCommand(ZyxelObjectCmd::cmdShowDhcp6RequestObjects(), out);
    auto t1 = std::chrono::steady_clock::now();

    CHECK_EQUAL(static_cast<int>(SshResult::SUCCESS), static_cast<int>(res));
    double rttMs = std::chrono::duration<double, std::milli>(t1 - t0).count();
    bool passed = (res == SshResult::SUCCESS) && (rttMs <= 2500.0);

    ZyxelBenchmark::record({
        "LiveFirewallE4", "ReadDhcp6RequestObjects", ZyxelObjectCmd::cmdShowDhcp6RequestObjects(),
        rttMs, 0, out.size(), 1, passed,
        passed ? "PASS (SLA)" : "FAIL"
    });

    CHECK_TRUE(rttMs <= 2500.0);
}

TEST(LiveFirewallE4, ReadIpv6Dhcp6Bindings) {
    ZyxelDriver &driver = ZyxelDriver::getInstance();
    if (!driver.isConnected()) return;

    auto t0 = std::chrono::steady_clock::now();
    std::string out;
    SshResult res = driver.getSshClient().executeCommand(ZyxelObjectCmd::cmdShowIpv6Dhcp6Bindings(), out);
    auto t1 = std::chrono::steady_clock::now();

    CHECK_EQUAL(static_cast<int>(SshResult::SUCCESS), static_cast<int>(res));
    bool valid = (out.find("Interface") != std::string::npos);
    double rttMs = std::chrono::duration<double, std::milli>(t1 - t0).count();
    bool passed = (res == SshResult::SUCCESS) && valid && (rttMs <= 2500.0);

    ZyxelBenchmark::record({
        "LiveFirewallE4", "ReadIpv6Dhcp6Bindings", ZyxelObjectCmd::cmdShowIpv6Dhcp6Bindings(),
        rttMs, 0, out.size(), valid ? 1U : 0U, passed,
        passed ? "PASS (SLA)" : "FAIL"
    });

    CHECK_TRUE(valid);
    CHECK_TRUE(rttMs <= 2500.0);
}

TEST(LiveFirewallE4, DryFireAddressObjectNonExistent) {
    ZyxelDriver &driver = ZyxelDriver::getInstance();
    if (!driver.isConnected()) return;

    auto t0 = std::chrono::steady_clock::now();
    std::string out;
    SshResult res = driver.getSshClient().executeCommand(ZyxelObjectCmd::cmdInvalidAddressObjectDryFire(), out);
    auto t1 = std::chrono::steady_clock::now();

    bool rejected = (res == SshResult::ERR_SYNTAX || res == SshResult::ERR_EXEC_FAILED ||
                     out.find("does not exist") != std::string::npos ||
                     out.find("retval = -43000") != std::string::npos ||
                     out.find("ERROR") != std::string::npos);
    double rttMs = std::chrono::duration<double, std::milli>(t1 - t0).count();
    bool passed = rejected && (rttMs <= 1500.0);

    ZyxelBenchmark::record({
        "LiveFirewallE4", "DryFireAddressObjectNonExistent", ZyxelObjectCmd::cmdInvalidAddressObjectDryFire(),
        rttMs, 0, out.size(), rejected ? 1U : 0U, passed,
        passed ? "PASS (REJECT)" : "FAIL"
    });

    CHECK_TRUE(rejected);
    CHECK_TRUE(rttMs <= 1500.0);
}

TEST(LiveFirewallE4, DryFireAddress6ObjectNonExistent) {
    ZyxelDriver &driver = ZyxelDriver::getInstance();
    if (!driver.isConnected()) return;

    auto t0 = std::chrono::steady_clock::now();
    std::string out;
    SshResult res = driver.getSshClient().executeCommand(ZyxelObjectCmd::cmdInvalidAddress6ObjectDryFire(), out);
    auto t1 = std::chrono::steady_clock::now();

    bool rejected = (res == SshResult::ERR_SYNTAX || res == SshResult::ERR_EXEC_FAILED ||
                     out.find("does not exist") != std::string::npos ||
                     out.find("retval = -43000") != std::string::npos ||
                     out.find("ERROR") != std::string::npos);
    double rttMs = std::chrono::duration<double, std::milli>(t1 - t0).count();
    bool passed = rejected && (rttMs <= 1500.0);

    ZyxelBenchmark::record({
        "LiveFirewallE4", "DryFireAddress6ObjectNonExistent", ZyxelObjectCmd::cmdInvalidAddress6ObjectDryFire(),
        rttMs, 0, out.size(), rejected ? 1U : 0U, passed,
        passed ? "PASS (REJECT)" : "FAIL"
    });

    CHECK_TRUE(rejected);
    CHECK_TRUE(rttMs <= 1500.0);
}

TEST(LiveFirewallE4, DryFireServiceObjectNonExistent) {
    ZyxelDriver &driver = ZyxelDriver::getInstance();
    if (!driver.isConnected()) return;

    auto t0 = std::chrono::steady_clock::now();
    std::string out;
    SshResult res = driver.getSshClient().executeCommand(ZyxelObjectCmd::cmdInvalidServiceObjectDryFire(), out);
    auto t1 = std::chrono::steady_clock::now();

    bool rejected = (res == SshResult::ERR_SYNTAX || res == SshResult::ERR_EXEC_FAILED ||
                     out.find("does not exist") != std::string::npos ||
                     out.find("retval = -43000") != std::string::npos ||
                     out.find("ERROR") != std::string::npos);
    double rttMs = std::chrono::duration<double, std::milli>(t1 - t0).count();
    bool passed = rejected && (rttMs <= 1500.0);

    ZyxelBenchmark::record({
        "LiveFirewallE4", "DryFireServiceObjectNonExistent", ZyxelObjectCmd::cmdInvalidServiceObjectDryFire(),
        rttMs, 0, out.size(), rejected ? 1U : 0U, passed,
        passed ? "PASS (REJECT)" : "FAIL"
    });

    CHECK_TRUE(rejected);
    CHECK_TRUE(rttMs <= 1500.0);
}

TEST(LiveFirewallE4, DryFireScheduleObjectNonExistent) {
    ZyxelDriver &driver = ZyxelDriver::getInstance();
    if (!driver.isConnected()) return;

    auto t0 = std::chrono::steady_clock::now();
    std::string out;
    SshResult res = driver.getSshClient().executeCommand(ZyxelObjectCmd::cmdInvalidScheduleObjectDryFire(), out);
    auto t1 = std::chrono::steady_clock::now();

    bool rejected = (res == SshResult::ERR_SYNTAX || res == SshResult::ERR_EXEC_FAILED ||
                     out.find("does not exist") != std::string::npos ||
                     out.find("retval = -43000") != std::string::npos ||
                     out.find("ERROR") != std::string::npos);
    double rttMs = std::chrono::duration<double, std::milli>(t1 - t0).count();
    bool passed = rejected && (rttMs <= 1500.0);

    ZyxelBenchmark::record({
        "LiveFirewallE4", "DryFireScheduleObjectNonExistent", ZyxelObjectCmd::cmdInvalidScheduleObjectDryFire(),
        rttMs, 0, out.size(), rejected ? 1U : 0U, passed,
        passed ? "PASS (REJECT)" : "FAIL"
    });

    CHECK_TRUE(rejected);
    CHECK_TRUE(rttMs <= 1500.0);
}

TEST(LiveFirewallE4, WriteScheduleOneTimeRollback) {
    ZyxelDriver &driver = ZyxelDriver::getInstance();
    if (!driver.isConnected()) return;
    ZyxelSshClient &ssh = driver.getSshClient();

    auto t0 = std::chrono::steady_clock::now();
    std::string out;
    ssh.unwindToRootPrompt();
    ssh.executeCommand("configure terminal", out);
    SshResult resAdd = ssh.executeCommand(
        ZyxelObjectCmd::cmdAddScheduleOneTime("NETMON_QA_SCHED", "2026-12-31", "00:00", "2026-12-31", "23:59"),
        out);
    CHECK_EQUAL(static_cast<int>(SshResult::SUCCESS), static_cast<int>(resAdd));

    ssh.executeCommand(ZyxelObjectCmd::cmdShowScheduleObjects("NETMON_QA_SCHED"), out);
    bool created = (out.find("NETMON_QA_SCHED") != std::string::npos);

    SshResult resDel = ssh.executeCommand(ZyxelObjectCmd::cmdDeleteSchedule("NETMON_QA_SCHED"), out);
    CHECK_EQUAL(static_cast<int>(SshResult::SUCCESS), static_cast<int>(resDel));

    ssh.executeCommand(ZyxelObjectCmd::cmdShowScheduleObjects("NETMON_QA_SCHED"), out);
    bool deleted = (out.find("NETMON_QA_SCHED") == std::string::npos ||
                    out.find("does not exist") != std::string::npos);

    ssh.executeCommand("exit", out);
    ssh.unwindToRootPrompt();
    auto t1 = std::chrono::steady_clock::now();

    double rttMs = std::chrono::duration<double, std::milli>(t1 - t0).count();
    bool passed = (resAdd == SshResult::SUCCESS) && (resDel == SshResult::SUCCESS) &&
                  created && deleted && (rttMs <= 4000.0);

    ZyxelBenchmark::record({
        "LiveFirewallE4", "WriteScheduleOneTimeRollback", "schedule-object NETMON_QA_SCHED",
        rttMs, 0, out.size(), 1, passed,
        passed ? "PASS (ROLLBACK)" : "FAIL"
    });

    CHECK_TRUE(created);
    CHECK_TRUE(deleted);
    CHECK_TRUE(rttMs <= 4000.0);
}

TEST(LiveFirewallE4, WriteAddress6HostRollback) {
    ZyxelDriver &driver = ZyxelDriver::getInstance();
    if (!driver.isConnected()) return;
    ZyxelSshClient &ssh = driver.getSshClient();

    auto t0 = std::chrono::steady_clock::now();
    std::string out;
    ssh.unwindToRootPrompt();
    ssh.executeCommand("configure terminal", out);
    SshResult resAdd = ssh.executeCommand(
        ZyxelObjectCmd::cmdAddAddress6Host("NETMON_QA_ADDR6", "2001:db8::1"),
        out);
    CHECK_EQUAL(static_cast<int>(SshResult::SUCCESS), static_cast<int>(resAdd));

    ssh.executeCommand(ZyxelObjectCmd::cmdShowAddress6Objects("NETMON_QA_ADDR6"), out);
    bool created = (out.find("NETMON_QA_ADDR6") != std::string::npos);

    SshResult resDel = ssh.executeCommand(ZyxelObjectCmd::cmdDeleteAddress6("NETMON_QA_ADDR6"), out);
    CHECK_EQUAL(static_cast<int>(SshResult::SUCCESS), static_cast<int>(resDel));

    ssh.executeCommand(ZyxelObjectCmd::cmdShowAddress6Objects("NETMON_QA_ADDR6"), out);
    bool deleted = (out.find("NETMON_QA_ADDR6") == std::string::npos ||
                    out.find("does not exist") != std::string::npos);

    ssh.executeCommand("exit", out);
    ssh.unwindToRootPrompt();
    auto t1 = std::chrono::steady_clock::now();

    double rttMs = std::chrono::duration<double, std::milli>(t1 - t0).count();
    bool passed = (resAdd == SshResult::SUCCESS) && (resDel == SshResult::SUCCESS) &&
                  created && deleted && (rttMs <= 4000.0);

    ZyxelBenchmark::record({
        "LiveFirewallE4", "WriteAddress6HostRollback", "address6-object NETMON_QA_ADDR6",
        rttMs, 0, out.size(), 1, passed,
        passed ? "PASS (ROLLBACK)" : "FAIL"
    });

    CHECK_TRUE(created);
    CHECK_TRUE(deleted);
    CHECK_TRUE(rttMs <= 4000.0);
}

TEST(LiveFirewallE4, WriteAddress6SubnetRollback) {
    ZyxelDriver &driver = ZyxelDriver::getInstance();
    if (!driver.isConnected()) return;
    ZyxelSshClient &ssh = driver.getSshClient();

    auto t0 = std::chrono::steady_clock::now();
    std::string out;
    ssh.unwindToRootPrompt();
    ssh.executeCommand("configure terminal", out);
    SshResult resAdd = ssh.executeCommand(
        ZyxelObjectCmd::cmdAddAddress6Subnet("NETMON_QA_SUBNET6", "2001:db8:1::", 64),
        out);
    CHECK_EQUAL(static_cast<int>(SshResult::SUCCESS), static_cast<int>(resAdd));

    ssh.executeCommand(ZyxelObjectCmd::cmdShowAddress6Objects("NETMON_QA_SUBNET6"), out);
    bool created = (out.find("NETMON_QA_SUBNET6") != std::string::npos);

    SshResult resDel = ssh.executeCommand(ZyxelObjectCmd::cmdDeleteAddress6("NETMON_QA_SUBNET6"), out);
    CHECK_EQUAL(static_cast<int>(SshResult::SUCCESS), static_cast<int>(resDel));

    ssh.executeCommand(ZyxelObjectCmd::cmdShowAddress6Objects("NETMON_QA_SUBNET6"), out);
    bool deleted = (out.find("NETMON_QA_SUBNET6") == std::string::npos ||
                    out.find("does not exist") != std::string::npos);

    ssh.executeCommand("exit", out);
    ssh.unwindToRootPrompt();
    auto t1 = std::chrono::steady_clock::now();

    double rttMs = std::chrono::duration<double, std::milli>(t1 - t0).count();
    bool passed = (resAdd == SshResult::SUCCESS) && (resDel == SshResult::SUCCESS) &&
                  created && deleted && (rttMs <= 4000.0);

    ZyxelBenchmark::record({
        "LiveFirewallE4", "WriteAddress6SubnetRollback", "address6-object NETMON_QA_SUBNET6",
        rttMs, 0, out.size(), 1, passed,
        passed ? "PASS (ROLLBACK)" : "FAIL"
    });

    CHECK_TRUE(created);
    CHECK_TRUE(deleted);
    CHECK_TRUE(rttMs <= 4000.0);
}

TEST(LiveFirewallE4, WriteAddress6GroupRollback) {
    ZyxelDriver &driver = ZyxelDriver::getInstance();
    if (!driver.isConnected()) return;
    ZyxelSshClient &ssh = driver.getSshClient();

    auto t0 = std::chrono::steady_clock::now();
    std::string out;
    ssh.unwindToRootPrompt();
    ssh.executeCommand("configure terminal", out);
    SshResult resAdd = ssh.executeCommand(
        ZyxelObjectCmd::cmdAddAddress6Group("NETMON_QA_GRP6"),
        out);
    CHECK_EQUAL(static_cast<int>(SshResult::SUCCESS), static_cast<int>(resAdd));
    ssh.executeCommand("exit", out);

    ssh.executeCommand(ZyxelObjectCmd::cmdShowAddress6Group(), out);
    bool created = (out.find("NETMON_QA_GRP6") != std::string::npos);

    SshResult resDel = ssh.executeCommand(ZyxelObjectCmd::cmdDeleteAddress6Group("NETMON_QA_GRP6"), out);
    CHECK_EQUAL(static_cast<int>(SshResult::SUCCESS), static_cast<int>(resDel));

    ssh.executeCommand(ZyxelObjectCmd::cmdShowAddress6Group(), out);
    bool deleted = (out.find("NETMON_QA_GRP6") == std::string::npos);

    ssh.executeCommand("exit", out);
    ssh.unwindToRootPrompt();
    auto t1 = std::chrono::steady_clock::now();

    double rttMs = std::chrono::duration<double, std::milli>(t1 - t0).count();
    bool passed = (resAdd == SshResult::SUCCESS) && (resDel == SshResult::SUCCESS) &&
                  created && deleted && (rttMs <= 4000.0);

    ZyxelBenchmark::record({
        "LiveFirewallE4", "WriteAddress6GroupRollback", "object-group address6 NETMON_QA_GRP6",
        rttMs, 0, out.size(), 1, passed,
        passed ? "PASS (ROLLBACK)" : "FAIL"
    });

    CHECK_TRUE(created);
    CHECK_TRUE(deleted);
    CHECK_TRUE(rttMs <= 4000.0);
}

// ============================================================================
// Group 7: Envelope 5 Live Diagnostics (NAT, DDNS, Virtual Servers, ALG, UPnP)
// ============================================================================

TEST_GROUP(LiveFirewallE5) {
    void sweep() {
        ZyxelDriver &driver = ZyxelDriver::getInstance();
        if (!driver.isConnected()) return;
        ZyxelSshClient &ssh = driver.getSshClient();
        std::string out;
        ssh.unwindToRootPrompt();
        ssh.executeCommand("configure terminal", out);
        ssh.executeCommand(ZyxelNatCmd::cmdDeleteDdnsProfile("NETMON_QA_DDNS"), out);
        ssh.executeCommand(ZyxelNatCmd::cmdDeleteVirtualServer("NETMON_QA_VS"), out);
        ssh.executeCommand(ZyxelObjectCmd::cmdDeleteService("NETMON_QA_VS_SVC"), out);
        ssh.executeCommand("exit", out);
        ssh.unwindToRootPrompt();
    }

    void setup() override {
        sweep();
        std::this_thread::sleep_for(std::chrono::milliseconds(25));
    }

    void teardown() override {
        sweep();
    }
};

TEST(LiveFirewallE5, ReadDdns) {
    ZyxelDriver &driver = ZyxelDriver::getInstance();
    if (!driver.isConnected()) return;

    auto t0 = std::chrono::steady_clock::now();
    std::string out;
    SshResult res = driver.getSshClient().executeCommand(ZyxelNatCmd::cmdShowDdns(), out);
    auto t1 = std::chrono::steady_clock::now();

    CHECK_EQUAL(static_cast<int>(SshResult::SUCCESS), static_cast<int>(res));
    bool found = (out.find("Profile_Name") != std::string::npos || out.find("No.") != std::string::npos);
    double rttMs = std::chrono::duration<double, std::milli>(t1 - t0).count();
    bool passed = (res == SshResult::SUCCESS) && found && (rttMs <= 2500.0);

    ZyxelBenchmark::record({
        "LiveFirewallE5", "ReadDdns", ZyxelNatCmd::cmdShowDdns(),
        rttMs, 0, out.size(), found ? 1U : 0U, passed,
        passed ? "PASS (SLA)" : "FAIL"
    });

    CHECK_TRUE(found);
    CHECK_TRUE(rttMs <= 2500.0);
}

TEST(LiveFirewallE5, ReadDdnsSpecific) {
    ZyxelDriver &driver = ZyxelDriver::getInstance();
    if (!driver.isConnected()) return;

    auto t0 = std::chrono::steady_clock::now();
    std::string out;
    SshResult res = driver.getSshClient().executeCommand(ZyxelNatCmd::cmdShowDdns("flamingo"), out);
    auto t1 = std::chrono::steady_clock::now();

    CHECK_EQUAL(static_cast<int>(SshResult::SUCCESS), static_cast<int>(res));
    bool found = (out.find("flamingo") != std::string::npos);
    double rttMs = std::chrono::duration<double, std::milli>(t1 - t0).count();
    bool passed = (res == SshResult::SUCCESS) && found && (rttMs <= 2500.0);

    ZyxelBenchmark::record({
        "LiveFirewallE5", "ReadDdnsSpecific", ZyxelNatCmd::cmdShowDdns("flamingo"),
        rttMs, 0, out.size(), found ? 1U : 0U, passed,
        passed ? "PASS (SLA)" : "FAIL"
    });

    CHECK_TRUE(found);
    CHECK_TRUE(rttMs <= 2500.0);
}

TEST(LiveFirewallE5, ReadDdnsStatus) {
    ZyxelDriver &driver = ZyxelDriver::getInstance();
    if (!driver.isConnected()) return;

    auto t0 = std::chrono::steady_clock::now();
    std::string out;
    SshResult res = driver.getSshClient().executeCommand(ZyxelNatCmd::cmdShowDdnsStatus(), out);
    auto t1 = std::chrono::steady_clock::now();

    CHECK_EQUAL(static_cast<int>(SshResult::SUCCESS), static_cast<int>(res));
    auto p0 = std::chrono::steady_clock::now();
    std::vector<ZyxelDdnsStatusEntry> entries;
    bool parsed = ZyxelNatCmd::parseDdnsStatus(out, entries);
    auto p1 = std::chrono::steady_clock::now();

    double rttMs = std::chrono::duration<double, std::milli>(t1 - t0).count();
    uint64_t parseUs = std::chrono::duration_cast<std::chrono::microseconds>(p1 - p0).count();
    bool passed = (res == SshResult::SUCCESS) && parsed && !entries.empty() && (rttMs <= 2500.0);

    ZyxelBenchmark::record({
        "LiveFirewallE5", "ReadDdnsStatus", ZyxelNatCmd::cmdShowDdnsStatus(),
        rttMs, parseUs, out.size(), static_cast<uint32_t>(entries.size()), passed,
        passed ? "PASS (SLA)" : "FAIL"
    });

    CHECK_TRUE(parsed);
    CHECK_FALSE(entries.empty());
    CHECK_TRUE(rttMs <= 2500.0);
}

TEST(LiveFirewallE5, ReadVirtualServers) {
    ZyxelDriver &driver = ZyxelDriver::getInstance();
    if (!driver.isConnected()) return;

    auto t0 = std::chrono::steady_clock::now();
    std::string out;
    SshResult res = driver.getSshClient().executeCommand(ZyxelNatCmd::cmdShowVirtualServers(), out);
    auto t1 = std::chrono::steady_clock::now();

    CHECK_EQUAL(static_cast<int>(SshResult::SUCCESS), static_cast<int>(res));
    auto p0 = std::chrono::steady_clock::now();
    std::vector<ZyxelVirtualServerRule> rules;
    bool parsed = ZyxelNatCmd::parseVirtualServers(out, rules);
    auto p1 = std::chrono::steady_clock::now();

    double rttMs = std::chrono::duration<double, std::milli>(t1 - t0).count();
    uint64_t parseUs = std::chrono::duration_cast<std::chrono::microseconds>(p1 - p0).count();
    bool passed = (res == SshResult::SUCCESS) && parsed && !rules.empty() && (rttMs <= 2500.0);

    ZyxelBenchmark::record({
        "LiveFirewallE5", "ReadVirtualServers", ZyxelNatCmd::cmdShowVirtualServers(),
        rttMs, parseUs, out.size(), static_cast<uint32_t>(rules.size()), passed,
        passed ? "PASS (SLA)" : "FAIL"
    });

    CHECK_TRUE(parsed);
    CHECK_FALSE(rules.empty());
    CHECK_TRUE(rttMs <= 2500.0);
}

TEST(LiveFirewallE5, ReadVirtualServerSpecific) {
    ZyxelDriver &driver = ZyxelDriver::getInstance();
    if (!driver.isConnected()) return;

    auto t0 = std::chrono::steady_clock::now();
    std::string out;
    SshResult res = driver.getSshClient().executeCommand(ZyxelNatCmd::cmdShowVirtualServers("smtp_wan1"), out);
    auto t1 = std::chrono::steady_clock::now();

    CHECK_EQUAL(static_cast<int>(SshResult::SUCCESS), static_cast<int>(res));
    bool found = (out.find("smtp_wan1") != std::string::npos);
    double rttMs = std::chrono::duration<double, std::milli>(t1 - t0).count();
    bool passed = (res == SshResult::SUCCESS) && found && (rttMs <= 2500.0);

    ZyxelBenchmark::record({
        "LiveFirewallE5", "ReadVirtualServerSpecific", ZyxelNatCmd::cmdShowVirtualServers("smtp_wan1"),
        rttMs, 0, out.size(), found ? 1U : 0U, passed,
        passed ? "PASS (SLA)" : "FAIL"
    });

    CHECK_TRUE(found);
    CHECK_TRUE(rttMs <= 2500.0);
}

TEST(LiveFirewallE5, ReadVirtualServerStatus) {
    ZyxelDriver &driver = ZyxelDriver::getInstance();
    if (!driver.isConnected()) return;

    auto t0 = std::chrono::steady_clock::now();
    std::string out;
    SshResult res = driver.getSshClient().executeCommand(ZyxelNatCmd::cmdShowVirtualServerStatus(), out);
    auto t1 = std::chrono::steady_clock::now();

    CHECK_EQUAL(static_cast<int>(SshResult::SUCCESS), static_cast<int>(res));
    double rttMs = std::chrono::duration<double, std::milli>(t1 - t0).count();
    bool passed = (res == SshResult::SUCCESS) && (rttMs <= 2500.0);

    ZyxelBenchmark::record({
        "LiveFirewallE5", "ReadVirtualServerStatus", ZyxelNatCmd::cmdShowVirtualServerStatus(),
        rttMs, 0, out.size(), 1, passed,
        passed ? "PASS (SLA)" : "FAIL"
    });

    CHECK_TRUE(rttMs <= 2500.0);
}

TEST(LiveFirewallE5, ReadVirtualServerLoadBalancer) {
    ZyxelDriver &driver = ZyxelDriver::getInstance();
    if (!driver.isConnected()) return;

    auto t0 = std::chrono::steady_clock::now();
    std::string out;
    SshResult res = driver.getSshClient().executeCommand(ZyxelNatCmd::cmdShowVirtualServerLoadBalancer(), out);
    auto t1 = std::chrono::steady_clock::now();

    CHECK_EQUAL(static_cast<int>(SshResult::SUCCESS), static_cast<int>(res));
    double rttMs = std::chrono::duration<double, std::milli>(t1 - t0).count();
    bool passed = (res == SshResult::SUCCESS) && (rttMs <= 2500.0);

    ZyxelBenchmark::record({
        "LiveFirewallE5", "ReadVirtualServerLoadBalancer", ZyxelNatCmd::cmdShowVirtualServerLoadBalancer(),
        rttMs, 0, out.size(), 1, passed,
        passed ? "PASS (SLA)" : "FAIL"
    });

    CHECK_TRUE(rttMs <= 2500.0);
}

TEST(LiveFirewallE5, ReadRedirectService) {
    ZyxelDriver &driver = ZyxelDriver::getInstance();
    if (!driver.isConnected()) return;

    auto t0 = std::chrono::steady_clock::now();
    std::string out;
    SshResult res = driver.getSshClient().executeCommand(ZyxelNatCmd::cmdShowRedirectService(), out);
    auto t1 = std::chrono::steady_clock::now();

    CHECK_EQUAL(static_cast<int>(SshResult::SUCCESS), static_cast<int>(res));
    double rttMs = std::chrono::duration<double, std::milli>(t1 - t0).count();
    bool passed = (res == SshResult::SUCCESS) && (rttMs <= 2500.0);

    ZyxelBenchmark::record({
        "LiveFirewallE5", "ReadRedirectService", ZyxelNatCmd::cmdShowRedirectService(),
        rttMs, 0, out.size(), 1, passed,
        passed ? "PASS (SLA)" : "FAIL"
    });

    CHECK_TRUE(rttMs <= 2500.0);
}

TEST(LiveFirewallE5, ReadAlgFtp) {
    ZyxelDriver &driver = ZyxelDriver::getInstance();
    if (!driver.isConnected()) return;

    auto t0 = std::chrono::steady_clock::now();
    std::string out;
    SshResult res = driver.getSshClient().executeCommand(ZyxelNatCmd::cmdShowAlgFtp(), out);
    auto t1 = std::chrono::steady_clock::now();

    CHECK_EQUAL(static_cast<int>(SshResult::SUCCESS), static_cast<int>(res));
    ZyxelAlgStatus status;
    bool parsed = ZyxelNatCmd::parseAlgStatus(out, "ftp", status);
    double rttMs = std::chrono::duration<double, std::milli>(t1 - t0).count();
    bool passed = (res == SshResult::SUCCESS) && parsed && (status.signalingPort == 21) && (rttMs <= 2500.0);

    ZyxelBenchmark::record({
        "LiveFirewallE5", "ReadAlgFtp", ZyxelNatCmd::cmdShowAlgFtp(),
        rttMs, 0, out.size(), parsed ? 1U : 0U, passed,
        passed ? "PASS (SLA)" : "FAIL"
    });

    CHECK_TRUE(parsed);
    LONGS_EQUAL(21, status.signalingPort);
    CHECK_TRUE(rttMs <= 2500.0);
}

TEST(LiveFirewallE5, ReadAlgSip) {
    ZyxelDriver &driver = ZyxelDriver::getInstance();
    if (!driver.isConnected()) return;

    auto t0 = std::chrono::steady_clock::now();
    std::string out;
    SshResult res = driver.getSshClient().executeCommand(ZyxelNatCmd::cmdShowAlgSip(), out);
    auto t1 = std::chrono::steady_clock::now();

    CHECK_EQUAL(static_cast<int>(SshResult::SUCCESS), static_cast<int>(res));
    ZyxelAlgStatus status;
    bool parsed = ZyxelNatCmd::parseAlgStatus(out, "sip", status);
    double rttMs = std::chrono::duration<double, std::milli>(t1 - t0).count();
    bool passed = (res == SshResult::SUCCESS) && parsed && (status.protocol == "sip") && (rttMs <= 2500.0);

    ZyxelBenchmark::record({
        "LiveFirewallE5", "ReadAlgSip", ZyxelNatCmd::cmdShowAlgSip(),
        rttMs, 0, out.size(), parsed ? 1U : 0U, passed,
        passed ? "PASS (SLA)" : "FAIL"
    });

    CHECK_TRUE(parsed);
    CHECK_TRUE(rttMs <= 2500.0);
}

TEST(LiveFirewallE5, ReadAlgH323) {
    ZyxelDriver &driver = ZyxelDriver::getInstance();
    if (!driver.isConnected()) return;

    auto t0 = std::chrono::steady_clock::now();
    std::string out;
    SshResult res = driver.getSshClient().executeCommand(ZyxelNatCmd::cmdShowAlgH323(), out);
    auto t1 = std::chrono::steady_clock::now();

    CHECK_EQUAL(static_cast<int>(SshResult::SUCCESS), static_cast<int>(res));
    ZyxelAlgStatus status;
    bool parsed = ZyxelNatCmd::parseAlgStatus(out, "h323", status);
    double rttMs = std::chrono::duration<double, std::milli>(t1 - t0).count();
    bool passed = (res == SshResult::SUCCESS) && parsed && (status.signalingPort == 1720) && (rttMs <= 2500.0);

    ZyxelBenchmark::record({
        "LiveFirewallE5", "ReadAlgH323", ZyxelNatCmd::cmdShowAlgH323(),
        rttMs, 0, out.size(), parsed ? 1U : 0U, passed,
        passed ? "PASS (SLA)" : "FAIL"
    });

    CHECK_TRUE(parsed);
    LONGS_EQUAL(1720, status.signalingPort);
    CHECK_TRUE(rttMs <= 2500.0);
}

TEST(LiveFirewallE5, DryFireDdnsNonExistent) {
    ZyxelDriver &driver = ZyxelDriver::getInstance();
    if (!driver.isConnected()) return;

    auto t0 = std::chrono::steady_clock::now();
    std::string out;
    SshResult res = driver.getSshClient().executeCommand(ZyxelNatCmd::cmdInvalidDdnsDryFire(), out);
    auto t1 = std::chrono::steady_clock::now();

    CHECK_EQUAL(static_cast<int>(SshResult::SUCCESS), static_cast<int>(res));
    bool rejected = (out.find("retval = -13002") != std::string::npos || out.find("ERROR:") != std::string::npos);
    double rttMs = std::chrono::duration<double, std::milli>(t1 - t0).count();
    bool passed = (res == SshResult::SUCCESS) && rejected && (rttMs <= 2500.0);

    ZyxelBenchmark::record({
        "LiveFirewallE5", "DryFireDdnsNonExistent", ZyxelNatCmd::cmdInvalidDdnsDryFire(),
        rttMs, 0, out.size(), rejected ? 1U : 0U, passed,
        passed ? "PASS (DRY-FIRE)" : "FAIL"
    });

    CHECK_TRUE(rejected);
    CHECK_TRUE(rttMs <= 2500.0);
}

TEST(LiveFirewallE5, DryFireVirtualServerNonExistent) {
    ZyxelDriver &driver = ZyxelDriver::getInstance();
    if (!driver.isConnected()) return;

    auto t0 = std::chrono::steady_clock::now();
    std::string out;
    SshResult res = driver.getSshClient().executeCommand("show ip virtual-server non_existent_vs_999", out);
    auto t1 = std::chrono::steady_clock::now();

    CHECK_EQUAL(static_cast<int>(SshResult::SUCCESS), static_cast<int>(res));
    double rttMs = std::chrono::duration<double, std::milli>(t1 - t0).count();
    bool passed = (res == SshResult::SUCCESS) && (rttMs <= 2500.0);

    ZyxelBenchmark::record({
        "LiveFirewallE5", "DryFireVirtualServerNonExistent", "show ip virtual-server non_existent_vs_999",
        rttMs, 0, out.size(), 1, passed,
        passed ? "PASS (DRY-FIRE)" : "FAIL"
    });

    CHECK_TRUE(rttMs <= 2500.0);
}

TEST(LiveFirewallE5, DryFireVirtualServerLoadBalancerNonExistent) {
    ZyxelDriver &driver = ZyxelDriver::getInstance();
    if (!driver.isConnected()) return;

    auto t0 = std::chrono::steady_clock::now();
    std::string out;
    SshResult res = driver.getSshClient().executeCommand(ZyxelNatCmd::cmdInvalidVirtualServerDryFire(), out);
    auto t1 = std::chrono::steady_clock::now();

    CHECK_EQUAL(static_cast<int>(SshResult::SUCCESS), static_cast<int>(res));
    double rttMs = std::chrono::duration<double, std::milli>(t1 - t0).count();
    bool passed = (res == SshResult::SUCCESS) && (rttMs <= 2500.0);

    ZyxelBenchmark::record({
        "LiveFirewallE5", "DryFireVirtualServerLoadBalancerNonExistent", ZyxelNatCmd::cmdInvalidVirtualServerDryFire(),
        rttMs, 0, out.size(), 1, passed,
        passed ? "PASS (DRY-FIRE)" : "FAIL"
    });

    CHECK_TRUE(rttMs <= 2500.0);
}

TEST(LiveFirewallE5, DryFireHttpRedirectUnsupported) {
    ZyxelDriver &driver = ZyxelDriver::getInstance();
    if (!driver.isConnected()) return;

    auto t0 = std::chrono::steady_clock::now();
    std::string out;
    SshResult res = driver.getSshClient().executeCommand(ZyxelNatCmd::cmdInvalidHttpRedirectDryFire(), out);
    auto t1 = std::chrono::steady_clock::now();

    CHECK_EQUAL(static_cast<int>(SshResult::SUCCESS), static_cast<int>(res));
    bool rejected = (out.find("Parse error") != std::string::npos);
    double rttMs = std::chrono::duration<double, std::milli>(t1 - t0).count();
    bool passed = (res == SshResult::SUCCESS) && rejected && (rttMs <= 2500.0);

    ZyxelBenchmark::record({
        "LiveFirewallE5", "DryFireHttpRedirectUnsupported", ZyxelNatCmd::cmdInvalidHttpRedirectDryFire(),
        rttMs, 0, out.size(), rejected ? 1U : 0U, passed,
        passed ? "PASS (DRY-FIRE)" : "FAIL"
    });

    CHECK_TRUE(rejected);
    CHECK_TRUE(rttMs <= 2500.0);
}

TEST(LiveFirewallE5, DryFireRedirectServiceOutOfRange) {
    ZyxelDriver &driver = ZyxelDriver::getInstance();
    if (!driver.isConnected()) return;

    auto t0 = std::chrono::steady_clock::now();
    std::string out;
    SshResult res = driver.getSshClient().executeCommand(ZyxelNatCmd::cmdInvalidRedirectServiceDryFire(), out);
    auto t1 = std::chrono::steady_clock::now();

    CHECK_EQUAL(static_cast<int>(SshResult::SUCCESS), static_cast<int>(res));
    bool rejected = (out.find("out of range") != std::string::npos || out.find("Parse error") != std::string::npos);
    double rttMs = std::chrono::duration<double, std::milli>(t1 - t0).count();
    bool passed = (res == SshResult::SUCCESS) && rejected && (rttMs <= 2500.0);

    ZyxelBenchmark::record({
        "LiveFirewallE5", "DryFireRedirectServiceOutOfRange", ZyxelNatCmd::cmdInvalidRedirectServiceDryFire(),
        rttMs, 0, out.size(), rejected ? 1U : 0U, passed,
        passed ? "PASS (DRY-FIRE)" : "FAIL"
    });

    CHECK_TRUE(rejected);
    CHECK_TRUE(rttMs <= 2500.0);
}

TEST(LiveFirewallE5, DryFireAlgInvalid) {
    ZyxelDriver &driver = ZyxelDriver::getInstance();
    if (!driver.isConnected()) return;

    auto t0 = std::chrono::steady_clock::now();
    std::string out;
    SshResult res = driver.getSshClient().executeCommand(ZyxelNatCmd::cmdInvalidAlgDryFire(), out);
    auto t1 = std::chrono::steady_clock::now();

    CHECK_EQUAL(static_cast<int>(SshResult::SUCCESS), static_cast<int>(res));
    bool rejected = (out.find("Parse error") != std::string::npos);
    double rttMs = std::chrono::duration<double, std::milli>(t1 - t0).count();
    bool passed = (res == SshResult::SUCCESS) && rejected && (rttMs <= 2500.0);

    ZyxelBenchmark::record({
        "LiveFirewallE5", "DryFireAlgInvalid", ZyxelNatCmd::cmdInvalidAlgDryFire(),
        rttMs, 0, out.size(), rejected ? 1U : 0U, passed,
        passed ? "PASS (DRY-FIRE)" : "FAIL"
    });

    CHECK_TRUE(rejected);
    CHECK_TRUE(rttMs <= 2500.0);
}

TEST(LiveFirewallE5, DryFireUpnpUnsupported) {
    ZyxelDriver &driver = ZyxelDriver::getInstance();
    if (!driver.isConnected()) return;

    auto t0 = std::chrono::steady_clock::now();
    std::string out;
    SshResult res = driver.getSshClient().executeCommand(ZyxelNatCmd::cmdShowUpnp(), out);
    auto t1 = std::chrono::steady_clock::now();

    CHECK_EQUAL(static_cast<int>(SshResult::SUCCESS), static_cast<int>(res));
    bool rejected = (out.find("Parse error") != std::string::npos);
    double rttMs = std::chrono::duration<double, std::milli>(t1 - t0).count();
    bool passed = (res == SshResult::SUCCESS) && rejected && (rttMs <= 2500.0);

    ZyxelBenchmark::record({
        "LiveFirewallE5", "DryFireUpnpUnsupported", ZyxelNatCmd::cmdShowUpnp(),
        rttMs, 0, out.size(), rejected ? 1U : 0U, passed,
        passed ? "PASS (DRY-FIRE)" : "FAIL"
    });

    CHECK_TRUE(rejected);
    CHECK_TRUE(rttMs <= 2500.0);
}

TEST(LiveFirewallE5, DryFireUpnpIgdUnsupported) {
    ZyxelDriver &driver = ZyxelDriver::getInstance();
    if (!driver.isConnected()) return;

    auto t0 = std::chrono::steady_clock::now();
    std::string out;
    SshResult res = driver.getSshClient().executeCommand(ZyxelNatCmd::cmdShowUpnpIgd(), out);
    auto t1 = std::chrono::steady_clock::now();

    CHECK_EQUAL(static_cast<int>(SshResult::SUCCESS), static_cast<int>(res));
    bool rejected = (out.find("Parse error") != std::string::npos);
    double rttMs = std::chrono::duration<double, std::milli>(t1 - t0).count();
    bool passed = (res == SshResult::SUCCESS) && rejected && (rttMs <= 2500.0);

    ZyxelBenchmark::record({
        "LiveFirewallE5", "DryFireUpnpIgdUnsupported", ZyxelNatCmd::cmdShowUpnpIgd(),
        rttMs, 0, out.size(), rejected ? 1U : 0U, passed,
        passed ? "PASS (DRY-FIRE)" : "FAIL"
    });

    CHECK_TRUE(rejected);
    CHECK_TRUE(rttMs <= 2500.0);
}

TEST(LiveFirewallE5, DryFireNatPmpUnsupported) {
    ZyxelDriver &driver = ZyxelDriver::getInstance();
    if (!driver.isConnected()) return;

    auto t0 = std::chrono::steady_clock::now();
    std::string out;
    SshResult res = driver.getSshClient().executeCommand(ZyxelNatCmd::cmdShowNatPmp(), out);
    auto t1 = std::chrono::steady_clock::now();

    CHECK_EQUAL(static_cast<int>(SshResult::SUCCESS), static_cast<int>(res));
    bool rejected = (out.find("Parse error") != std::string::npos);
    double rttMs = std::chrono::duration<double, std::milli>(t1 - t0).count();
    bool passed = (res == SshResult::SUCCESS) && rejected && (rttMs <= 2500.0);

    ZyxelBenchmark::record({
        "LiveFirewallE5", "DryFireNatPmpUnsupported", ZyxelNatCmd::cmdShowNatPmp(),
        rttMs, 0, out.size(), rejected ? 1U : 0U, passed,
        passed ? "PASS (DRY-FIRE)" : "FAIL"
    });

    CHECK_TRUE(rejected);
    CHECK_TRUE(rttMs <= 2500.0);
}

TEST(LiveFirewallE5, WriteDdnsProfileRollback) {
    ZyxelDriver &driver = ZyxelDriver::getInstance();
    if (!driver.isConnected()) return;
    ZyxelSshClient &ssh = driver.getSshClient();

    auto t0 = std::chrono::steady_clock::now();
    std::string out;

    ssh.executeCommand("configure terminal", out);
    SshResult resAdd = ssh.executeCommand(ZyxelNatCmd::cmdAddDdnsProfile("NETMON_QA_DDNS"), out);
    ssh.executeCommand("exit", out);
    ssh.unwindToRootPrompt();
    CHECK_EQUAL(static_cast<int>(SshResult::SUCCESS), static_cast<int>(resAdd));

    ssh.executeCommand(ZyxelNatCmd::cmdShowDdns(), out);
    bool created = (out.find("NETMON_QA_DDNS") != std::string::npos);

    ssh.executeCommand("configure terminal", out);
    SshResult resDel = ssh.executeCommand(ZyxelNatCmd::cmdDeleteDdnsProfile("NETMON_QA_DDNS"), out);
    CHECK_EQUAL(static_cast<int>(SshResult::SUCCESS), static_cast<int>(resDel));

    ssh.executeCommand("exit", out);
    ssh.unwindToRootPrompt();

    ssh.executeCommand(ZyxelNatCmd::cmdShowDdns(), out);
    bool deleted = (out.find("NETMON_QA_DDNS") == std::string::npos);
    auto t1 = std::chrono::steady_clock::now();

    double rttMs = std::chrono::duration<double, std::milli>(t1 - t0).count();
    bool passed = (resAdd == SshResult::SUCCESS) && (resDel == SshResult::SUCCESS) &&
                  created && deleted && (rttMs <= 4000.0);

    ZyxelBenchmark::record({
        "LiveFirewallE5", "WriteDdnsProfileRollback", "ip ddns profile NETMON_QA_DDNS",
        rttMs, 0, out.size(), 1, passed,
        passed ? "PASS (ROLLBACK)" : "FAIL"
    });

    CHECK_TRUE(created);
    CHECK_TRUE(deleted);
    CHECK_TRUE(rttMs <= 4000.0);
}

TEST(LiveFirewallE5, WriteVirtualServerRollback) {
    ZyxelDriver &driver = ZyxelDriver::getInstance();
    if (!driver.isConnected()) return;
    ZyxelSshClient &ssh = driver.getSshClient();

    auto t0 = std::chrono::steady_clock::now();
    std::string out;

    // 1. Pre-create dummy service
    ssh.executeCommand("configure terminal", out);
    ssh.executeCommand(ZyxelObjectCmd::cmdAddService("NETMON_QA_VS_SVC", "tcp", 65433), out);
    ssh.unwindToRootPrompt();

    ZyxelVirtualServerRule vs;
    vs.name = "NETMON_QA_VS";
    vs.interface = "wan1_ppp";
    vs.originalIp = "192.0.2.1";
    vs.mapToIp = "192.0.2.2";
    vs.originalService = "NETMON_QA_VS_SVC";
    vs.mappedService = "NETMON_QA_VS_SVC";
    vs.active = false;

    ssh.executeCommand("configure terminal", out);
    SshResult resAdd = ssh.executeCommand(ZyxelNatCmd::cmdAddVirtualServer(vs), out);
    ssh.executeCommand("exit", out);
    ssh.unwindToRootPrompt();
    CHECK_EQUAL(static_cast<int>(SshResult::SUCCESS), static_cast<int>(resAdd));

    ssh.executeCommand(ZyxelNatCmd::cmdShowVirtualServers(), out);
    bool created = (out.find("virtual server: NETMON_QA_VS") != std::string::npos);

    ssh.executeCommand("configure terminal", out);
    SshResult resDel = ssh.executeCommand(ZyxelNatCmd::cmdDeleteVirtualServer("NETMON_QA_VS"), out);
    ssh.executeCommand(ZyxelObjectCmd::cmdDeleteService("NETMON_QA_VS_SVC"), out);
    CHECK_EQUAL(static_cast<int>(SshResult::SUCCESS), static_cast<int>(resDel));

    ssh.executeCommand("exit", out);
    ssh.unwindToRootPrompt();

    ssh.executeCommand(ZyxelNatCmd::cmdShowVirtualServers(), out);
    bool deleted = (out.find("virtual server: NETMON_QA_VS") == std::string::npos);
    auto t1 = std::chrono::steady_clock::now();

    double rttMs = std::chrono::duration<double, std::milli>(t1 - t0).count();
    bool passed = (resAdd == SshResult::SUCCESS) && (resDel == SshResult::SUCCESS) &&
                  created && deleted && (rttMs <= 6000.0);

    ZyxelBenchmark::record({
        "LiveFirewallE5", "WriteVirtualServerRollback", "ip virtual-server NETMON_QA_VS",
        rttMs, 0, out.size(), 1, passed,
        passed ? "PASS (ROLLBACK)" : "FAIL"
    });

    CHECK_TRUE(created);
    CHECK_TRUE(deleted);
    CHECK_TRUE(rttMs <= 6000.0);
}

// ============================================================================
// Group 8: Envelope 6 Live Diagnostics (Security Policy & Device HA)
// ============================================================================

TEST_GROUP(LiveFirewallE6) {
    void sweep() {
        ZyxelDriver &driver = ZyxelDriver::getInstance();
        if (!driver.isConnected()) return;
        ZyxelSshClient &ssh = driver.getSshClient();
        std::string out;
        ssh.unwindToRootPrompt();
        ssh.executeCommand("configure terminal", out);
        ssh.executeCommand(ZyxelFirewallCmd::cmdDeleteRuleByName("NETMON_QA_RULE_E6"), out);
        ssh.executeCommand(ZyxelObjectCmd::cmdDeleteAddress("NETMON_QA_HOST_E6"), out);
        ssh.executeCommand("exit", out);
        ssh.unwindToRootPrompt();
    }

    void setup() override {
        sweep();
        std::this_thread::sleep_for(std::chrono::milliseconds(25));
    }

    void teardown() override {
        sweep();
    }
};

TEST(LiveFirewallE6, ReadSecurePolicy) {
    ZyxelDriver &driver = ZyxelDriver::getInstance();
    if (!driver.isConnected()) return;

    auto t0 = std::chrono::steady_clock::now();
    std::string out;
    SshResult res = driver.getSshClient().executeCommand(ZyxelFirewallCmd::cmdShowSecurePolicy(), out);
    auto t1 = std::chrono::steady_clock::now();

    CHECK_EQUAL(static_cast<int>(SshResult::SUCCESS), static_cast<int>(res));
    auto p0 = std::chrono::steady_clock::now();
    std::vector<ZyxelFirewallRule> rules;
    bool parsed = ZyxelFirewallCmd::parseSecurePolicy(out, rules);
    auto p1 = std::chrono::steady_clock::now();

    double rttMs = std::chrono::duration<double, std::milli>(t1 - t0).count();
    uint64_t parseUs = std::chrono::duration_cast<std::chrono::microseconds>(p1 - p0).count();
    bool passed = (res == SshResult::SUCCESS) && parsed && !rules.empty() && (rttMs <= 2500.0);

    ZyxelBenchmark::record({
        "LiveFirewallE6", "ReadSecurePolicy", ZyxelFirewallCmd::cmdShowSecurePolicy(),
        rttMs, parseUs, out.size(), static_cast<uint32_t>(rules.size()), passed,
        passed ? "PASS (SLA)" : "FAIL"
    });

    CHECK_TRUE(parsed);
    CHECK_FALSE(rules.empty());
    CHECK_TRUE(rttMs <= 2500.0);
}

TEST(LiveFirewallE6, ReadSecurePolicySpecific) {
    ZyxelDriver &driver = ZyxelDriver::getInstance();
    if (!driver.isConnected()) return;

    auto t0 = std::chrono::steady_clock::now();
    std::string out;
    SshResult res = driver.getSshClient().executeCommand(ZyxelFirewallCmd::cmdShowSecurePolicy("1"), out);
    auto t1 = std::chrono::steady_clock::now();

    CHECK_EQUAL(static_cast<int>(SshResult::SUCCESS), static_cast<int>(res));
    bool found = (out.find("secure-policy rule: 1") != std::string::npos);
    double rttMs = std::chrono::duration<double, std::milli>(t1 - t0).count();
    bool passed = (res == SshResult::SUCCESS) && found && (rttMs <= 2500.0);

    ZyxelBenchmark::record({
        "LiveFirewallE6", "ReadSecurePolicySpecific", ZyxelFirewallCmd::cmdShowSecurePolicy("1"),
        rttMs, 0, out.size(), found ? 1U : 0U, passed,
        passed ? "PASS (SLA)" : "FAIL"
    });

    CHECK_TRUE(found);
    CHECK_TRUE(rttMs <= 2500.0);
}

TEST(LiveFirewallE6, ReadSecurePolicyStatus) {
    ZyxelDriver &driver = ZyxelDriver::getInstance();
    if (!driver.isConnected()) return;

    auto t0 = std::chrono::steady_clock::now();
    std::string out;
    SshResult res = driver.getSshClient().executeCommand(ZyxelFirewallCmd::cmdShowSecurePolicyStatus(), out);
    auto t1 = std::chrono::steady_clock::now();

    CHECK_EQUAL(static_cast<int>(SshResult::SUCCESS), static_cast<int>(res));
    auto p0 = std::chrono::steady_clock::now();
    ZyxelSecurePolicyStatus status;
    bool parsed = ZyxelFirewallCmd::parseSecurePolicyStatus(out, status);
    auto p1 = std::chrono::steady_clock::now();

    double rttMs = std::chrono::duration<double, std::milli>(t1 - t0).count();
    uint64_t parseUs = std::chrono::duration_cast<std::chrono::microseconds>(p1 - p0).count();
    bool passed = (res == SshResult::SUCCESS) && parsed && status.active && (rttMs <= 2500.0);

    ZyxelBenchmark::record({
        "LiveFirewallE6", "ReadSecurePolicyStatus", ZyxelFirewallCmd::cmdShowSecurePolicyStatus(),
        rttMs, parseUs, out.size(), parsed ? 1U : 0U, passed,
        passed ? "PASS (SLA)" : "FAIL"
    });

    CHECK_TRUE(parsed);
    CHECK_TRUE(status.active);
    CHECK_TRUE(rttMs <= 2500.0);
}

TEST(LiveFirewallE6, ReadSecurePolicyBlockRules) {
    ZyxelDriver &driver = ZyxelDriver::getInstance();
    if (!driver.isConnected()) return;

    auto t0 = std::chrono::steady_clock::now();
    std::string out;
    SshResult res = driver.getSshClient().executeCommand(ZyxelFirewallCmd::cmdShowSecurePolicyBlockRules(), out);
    auto t1 = std::chrono::steady_clock::now();

    CHECK_EQUAL(static_cast<int>(SshResult::SUCCESS), static_cast<int>(res));
    double rttMs = std::chrono::duration<double, std::milli>(t1 - t0).count();
    bool passed = (res == SshResult::SUCCESS) && (rttMs <= 2500.0);

    ZyxelBenchmark::record({
        "LiveFirewallE6", "ReadSecurePolicyBlockRules", ZyxelFirewallCmd::cmdShowSecurePolicyBlockRules(),
        rttMs, 0, out.size(), 1, passed,
        passed ? "PASS (SLA)" : "FAIL"
    });

    CHECK_TRUE(rttMs <= 2500.0);
}

TEST(LiveFirewallE6, ReadSecurePolicy6) {
    ZyxelDriver &driver = ZyxelDriver::getInstance();
    if (!driver.isConnected()) return;

    auto t0 = std::chrono::steady_clock::now();
    std::string out;
    SshResult res = driver.getSshClient().executeCommand(ZyxelFirewallCmd::cmdShowSecurePolicy6(), out);
    auto t1 = std::chrono::steady_clock::now();

    CHECK_EQUAL(static_cast<int>(SshResult::SUCCESS), static_cast<int>(res));
    bool found = (out.find("secure-policy rule:") != std::string::npos);
    double rttMs = std::chrono::duration<double, std::milli>(t1 - t0).count();
    bool passed = (res == SshResult::SUCCESS) && found && (rttMs <= 2500.0);

    ZyxelBenchmark::record({
        "LiveFirewallE6", "ReadSecurePolicy6", ZyxelFirewallCmd::cmdShowSecurePolicy6(),
        rttMs, 0, out.size(), found ? 1U : 0U, passed,
        passed ? "PASS (SLA)" : "FAIL"
    });

    CHECK_TRUE(found);
    CHECK_TRUE(rttMs <= 2500.0);
}

TEST(LiveFirewallE6, ReadSecurePolicy6Specific) {
    ZyxelDriver &driver = ZyxelDriver::getInstance();
    if (!driver.isConnected()) return;

    auto t0 = std::chrono::steady_clock::now();
    std::string out;
    SshResult res = driver.getSshClient().executeCommand(ZyxelFirewallCmd::cmdShowSecurePolicy6("1"), out);
    auto t1 = std::chrono::steady_clock::now();

    CHECK_EQUAL(static_cast<int>(SshResult::SUCCESS), static_cast<int>(res));
    bool found = (out.find("secure-policy rule: 1") != std::string::npos);
    double rttMs = std::chrono::duration<double, std::milli>(t1 - t0).count();
    bool passed = (res == SshResult::SUCCESS) && found && (rttMs <= 2500.0);

    ZyxelBenchmark::record({
        "LiveFirewallE6", "ReadSecurePolicy6Specific", ZyxelFirewallCmd::cmdShowSecurePolicy6("1"),
        rttMs, 0, out.size(), found ? 1U : 0U, passed,
        passed ? "PASS (SLA)" : "FAIL"
    });

    CHECK_TRUE(found);
    CHECK_TRUE(rttMs <= 2500.0);
}

TEST(LiveFirewallE6, ReadDeviceHa) {
    ZyxelDriver &driver = ZyxelDriver::getInstance();
    if (!driver.isConnected()) return;

    auto t0 = std::chrono::steady_clock::now();
    std::string out;
    SshResult res = driver.getSshClient().executeCommand(ZyxelFirewallCmd::cmdShowDeviceHa(), out);
    auto t1 = std::chrono::steady_clock::now();

    CHECK_EQUAL(static_cast<int>(SshResult::SUCCESS), static_cast<int>(res));
    bool unsupported = (out.find("Parse error") != std::string::npos);
    double rttMs = std::chrono::duration<double, std::milli>(t1 - t0).count();
    bool passed = (res == SshResult::SUCCESS) && unsupported && (rttMs <= 2500.0);

    ZyxelBenchmark::record({
        "LiveFirewallE6", "ReadDeviceHa", ZyxelFirewallCmd::cmdShowDeviceHa(),
        rttMs, 0, out.size(), unsupported ? 1U : 0U, passed,
        passed ? "PASS (DRY-FIRE)" : "FAIL"
    });

    CHECK_TRUE(unsupported);
    CHECK_TRUE(rttMs <= 2500.0);
}

TEST(LiveFirewallE6, ReadDeviceHaStatus) {
    ZyxelDriver &driver = ZyxelDriver::getInstance();
    if (!driver.isConnected()) return;

    auto t0 = std::chrono::steady_clock::now();
    std::string out;
    SshResult res = driver.getSshClient().executeCommand(ZyxelFirewallCmd::cmdShowDeviceHaStatus(), out);
    auto t1 = std::chrono::steady_clock::now();

    CHECK_EQUAL(static_cast<int>(SshResult::SUCCESS), static_cast<int>(res));
    bool unsupported = (out.find("Parse error") != std::string::npos);
    double rttMs = std::chrono::duration<double, std::milli>(t1 - t0).count();
    bool passed = (res == SshResult::SUCCESS) && unsupported && (rttMs <= 2500.0);

    ZyxelBenchmark::record({
        "LiveFirewallE6", "ReadDeviceHaStatus", ZyxelFirewallCmd::cmdShowDeviceHaStatus(),
        rttMs, 0, out.size(), unsupported ? 1U : 0U, passed,
        passed ? "PASS (DRY-FIRE)" : "FAIL"
    });

    CHECK_TRUE(unsupported);
    CHECK_TRUE(rttMs <= 2500.0);
}

TEST(LiveFirewallE6, ReadDeviceHaMode) {
    ZyxelDriver &driver = ZyxelDriver::getInstance();
    if (!driver.isConnected()) return;

    auto t0 = std::chrono::steady_clock::now();
    std::string out;
    SshResult res = driver.getSshClient().executeCommand(ZyxelFirewallCmd::cmdShowDeviceHaMode(), out);
    auto t1 = std::chrono::steady_clock::now();

    CHECK_EQUAL(static_cast<int>(SshResult::SUCCESS), static_cast<int>(res));
    bool unsupported = (out.find("Parse error") != std::string::npos);
    double rttMs = std::chrono::duration<double, std::milli>(t1 - t0).count();
    bool passed = (res == SshResult::SUCCESS) && unsupported && (rttMs <= 2500.0);

    ZyxelBenchmark::record({
        "LiveFirewallE6", "ReadDeviceHaMode", ZyxelFirewallCmd::cmdShowDeviceHaMode(),
        rttMs, 0, out.size(), unsupported ? 1U : 0U, passed,
        passed ? "PASS (DRY-FIRE)" : "FAIL"
    });

    CHECK_TRUE(unsupported);
    CHECK_TRUE(rttMs <= 2500.0);
}

TEST(LiveFirewallE6, ReadDeviceHa2) {
    ZyxelDriver &driver = ZyxelDriver::getInstance();
    if (!driver.isConnected()) return;

    auto t0 = std::chrono::steady_clock::now();
    std::string out;
    SshResult res = driver.getSshClient().executeCommand(ZyxelFirewallCmd::cmdShowDeviceHa2(), out);
    auto t1 = std::chrono::steady_clock::now();

    CHECK_EQUAL(static_cast<int>(SshResult::SUCCESS), static_cast<int>(res));
    bool unsupported = (out.find("Parse error") != std::string::npos);
    double rttMs = std::chrono::duration<double, std::milli>(t1 - t0).count();
    bool passed = (res == SshResult::SUCCESS) && unsupported && (rttMs <= 2500.0);

    ZyxelBenchmark::record({
        "LiveFirewallE6", "ReadDeviceHa2", ZyxelFirewallCmd::cmdShowDeviceHa2(),
        rttMs, 0, out.size(), unsupported ? 1U : 0U, passed,
        passed ? "PASS (DRY-FIRE)" : "FAIL"
    });

    CHECK_TRUE(unsupported);
    CHECK_TRUE(rttMs <= 2500.0);
}

TEST(LiveFirewallE6, ReadDeviceHa2Interfaces) {
    ZyxelDriver &driver = ZyxelDriver::getInstance();
    if (!driver.isConnected()) return;

    auto t0 = std::chrono::steady_clock::now();
    std::string out;
    SshResult res = driver.getSshClient().executeCommand(ZyxelFirewallCmd::cmdShowDeviceHa2Interfaces(), out);
    auto t1 = std::chrono::steady_clock::now();

    CHECK_EQUAL(static_cast<int>(SshResult::SUCCESS), static_cast<int>(res));
    bool unsupported = (out.find("Parse error") != std::string::npos);
    double rttMs = std::chrono::duration<double, std::milli>(t1 - t0).count();
    bool passed = (res == SshResult::SUCCESS) && unsupported && (rttMs <= 2500.0);

    ZyxelBenchmark::record({
        "LiveFirewallE6", "ReadDeviceHa2Interfaces", ZyxelFirewallCmd::cmdShowDeviceHa2Interfaces(),
        rttMs, 0, out.size(), unsupported ? 1U : 0U, passed,
        passed ? "PASS (DRY-FIRE)" : "FAIL"
    });

    CHECK_TRUE(unsupported);
    CHECK_TRUE(rttMs <= 2500.0);
}

TEST(LiveFirewallE6, ReadDeviceHa2DeviceStatus) {
    ZyxelDriver &driver = ZyxelDriver::getInstance();
    if (!driver.isConnected()) return;

    auto t0 = std::chrono::steady_clock::now();
    std::string out;
    SshResult res = driver.getSshClient().executeCommand(ZyxelFirewallCmd::cmdShowDeviceHa2DeviceStatus(), out);
    auto t1 = std::chrono::steady_clock::now();

    CHECK_EQUAL(static_cast<int>(SshResult::SUCCESS), static_cast<int>(res));
    bool unsupported = (out.find("Parse error") != std::string::npos);
    double rttMs = std::chrono::duration<double, std::milli>(t1 - t0).count();
    bool passed = (res == SshResult::SUCCESS) && unsupported && (rttMs <= 2500.0);

    ZyxelBenchmark::record({
        "LiveFirewallE6", "ReadDeviceHa2DeviceStatus", ZyxelFirewallCmd::cmdShowDeviceHa2DeviceStatus(),
        rttMs, 0, out.size(), unsupported ? 1U : 0U, passed,
        passed ? "PASS (DRY-FIRE)" : "FAIL"
    });

    CHECK_TRUE(unsupported);
    CHECK_TRUE(rttMs <= 2500.0);
}

TEST(LiveFirewallE6, DryFireSecurePolicyNonExistent) {
    ZyxelDriver &driver = ZyxelDriver::getInstance();
    if (!driver.isConnected()) return;

    auto t0 = std::chrono::steady_clock::now();
    std::string out;
    SshResult res = driver.getSshClient().executeCommand(ZyxelFirewallCmd::cmdInvalidSecurePolicyDryFire(), out);
    auto t1 = std::chrono::steady_clock::now();

    CHECK_EQUAL(static_cast<int>(SshResult::SUCCESS), static_cast<int>(res));
    bool rejected = (out.find("Parse error") != std::string::npos || out.find("out of range") != std::string::npos);
    double rttMs = std::chrono::duration<double, std::milli>(t1 - t0).count();
    bool passed = (res == SshResult::SUCCESS) && rejected && (rttMs <= 2500.0);

    ZyxelBenchmark::record({
        "LiveFirewallE6", "DryFireSecurePolicyNonExistent", ZyxelFirewallCmd::cmdInvalidSecurePolicyDryFire(),
        rttMs, 0, out.size(), rejected ? 1U : 0U, passed,
        passed ? "PASS (DRY-FIRE)" : "FAIL"
    });

    CHECK_TRUE(rejected);
    CHECK_TRUE(rttMs <= 2500.0);
}

TEST(LiveFirewallE6, DryFireSecurePolicy6NonExistent) {
    ZyxelDriver &driver = ZyxelDriver::getInstance();
    if (!driver.isConnected()) return;

    auto t0 = std::chrono::steady_clock::now();
    std::string out;
    SshResult res = driver.getSshClient().executeCommand(ZyxelFirewallCmd::cmdInvalidSecurePolicy6DryFire(), out);
    auto t1 = std::chrono::steady_clock::now();

    CHECK_EQUAL(static_cast<int>(SshResult::SUCCESS), static_cast<int>(res));
    bool rejected = (out.find("Parse error") != std::string::npos || out.find("out of range") != std::string::npos);
    double rttMs = std::chrono::duration<double, std::milli>(t1 - t0).count();
    bool passed = (res == SshResult::SUCCESS) && rejected && (rttMs <= 2500.0);

    ZyxelBenchmark::record({
        "LiveFirewallE6", "DryFireSecurePolicy6NonExistent", ZyxelFirewallCmd::cmdInvalidSecurePolicy6DryFire(),
        rttMs, 0, out.size(), rejected ? 1U : 0U, passed,
        passed ? "PASS (DRY-FIRE)" : "FAIL"
    });

    CHECK_TRUE(rejected);
    CHECK_TRUE(rttMs <= 2500.0);
}

TEST(LiveFirewallE6, DryFireDeviceHaInvalid) {
    ZyxelDriver &driver = ZyxelDriver::getInstance();
    if (!driver.isConnected()) return;

    auto t0 = std::chrono::steady_clock::now();
    std::string out;
    SshResult res = driver.getSshClient().executeCommand(ZyxelFirewallCmd::cmdInvalidDeviceHaDryFire(), out);
    auto t1 = std::chrono::steady_clock::now();

    CHECK_EQUAL(static_cast<int>(SshResult::SUCCESS), static_cast<int>(res));
    bool rejected = (out.find("Parse error") != std::string::npos);
    double rttMs = std::chrono::duration<double, std::milli>(t1 - t0).count();
    bool passed = (res == SshResult::SUCCESS) && rejected && (rttMs <= 2500.0);

    ZyxelBenchmark::record({
        "LiveFirewallE6", "DryFireDeviceHaInvalid", ZyxelFirewallCmd::cmdInvalidDeviceHaDryFire(),
        rttMs, 0, out.size(), rejected ? 1U : 0U, passed,
        passed ? "PASS (DRY-FIRE)" : "FAIL"
    });

    CHECK_TRUE(rejected);
    CHECK_TRUE(rttMs <= 2500.0);
}

TEST(LiveFirewallE6, DryFireDeviceHa2Invalid) {
    ZyxelDriver &driver = ZyxelDriver::getInstance();
    if (!driver.isConnected()) return;

    auto t0 = std::chrono::steady_clock::now();
    std::string out;
    SshResult res = driver.getSshClient().executeCommand(ZyxelFirewallCmd::cmdInvalidDeviceHa2DryFire(), out);
    auto t1 = std::chrono::steady_clock::now();

    CHECK_EQUAL(static_cast<int>(SshResult::SUCCESS), static_cast<int>(res));
    bool rejected = (out.find("Parse error") != std::string::npos);
    double rttMs = std::chrono::duration<double, std::milli>(t1 - t0).count();
    bool passed = (res == SshResult::SUCCESS) && rejected && (rttMs <= 2500.0);

    ZyxelBenchmark::record({
        "LiveFirewallE6", "DryFireDeviceHa2Invalid", ZyxelFirewallCmd::cmdInvalidDeviceHa2DryFire(),
        rttMs, 0, out.size(), rejected ? 1U : 0U, passed,
        passed ? "PASS (DRY-FIRE)" : "FAIL"
    });

    CHECK_TRUE(rejected);
    CHECK_TRUE(rttMs <= 2500.0);
}

TEST(LiveFirewallE6, WriteAppendSecurePolicyRollback) {
    ZyxelDriver &driver = ZyxelDriver::getInstance();
    if (!driver.isConnected()) return;
    ZyxelSshClient &ssh = driver.getSshClient();

    auto t0 = std::chrono::steady_clock::now();
    std::string out;

    // 1. Create dummy source address object
    ssh.executeCommand("configure terminal", out);
    ssh.executeCommand("address-object NETMON_QA_HOST_E6 192.0.2.215", out);
    ssh.unwindToRootPrompt();

    // 2. Append fast deny rule to the end of the rule table (NEVER insert 1)
    ssh.executeCommand("configure terminal", out);
    auto cmds = ZyxelFirewallCmd::cmdAppendFastDeny("NETMON_QA_RULE_E6", "NETMON_QA_HOST_E6", "E6 live append test");
    for (const auto &c : cmds) {
        ssh.executeCommand(c, out);
    }
    ssh.unwindToRootPrompt();

    // 3. Verify rule creation
    ssh.executeCommand(ZyxelFirewallCmd::cmdShowSecurePolicy(), out);
    bool created = (out.find("name: NETMON_QA_RULE_E6") != std::string::npos);

    // 4. Rollback: delete rule by name, then address object
    ssh.executeCommand("configure terminal", out);
    SshResult resDelRule = ssh.executeCommand(ZyxelFirewallCmd::cmdDeleteRuleByName("NETMON_QA_RULE_E6"), out);
    SshResult resDelObj = ssh.executeCommand(ZyxelObjectCmd::cmdDeleteAddress("NETMON_QA_HOST_E6"), out);
    ssh.unwindToRootPrompt();

    CHECK_EQUAL(static_cast<int>(SshResult::SUCCESS), static_cast<int>(resDelRule));
    CHECK_EQUAL(static_cast<int>(SshResult::SUCCESS), static_cast<int>(resDelObj));

    // 5. Verify clean deletion
    ssh.executeCommand(ZyxelFirewallCmd::cmdShowSecurePolicy(), out);
    bool deleted = (out.find("name: NETMON_QA_RULE_E6") == std::string::npos);
    auto t1 = std::chrono::steady_clock::now();

    double rttMs = std::chrono::duration<double, std::milli>(t1 - t0).count();
    bool passed = created && deleted && (rttMs <= 6000.0);

    ZyxelBenchmark::record({
        "LiveFirewallE6", "WriteAppendSecurePolicyRollback", "secure-policy append NETMON_QA_RULE_E6",
        rttMs, 0, out.size(), 1, passed,
        passed ? "PASS (ROLLBACK)" : "FAIL"
    });

    CHECK_TRUE(created);
    CHECK_TRUE(deleted);
    CHECK_TRUE(rttMs <= 6000.0);
}

// ============================================================================
// Envelope 7: Live Firewall Diagnostic Tests (UTM: Ch 36..46)
// ============================================================================

TEST_GROUP(LiveFirewallE7) {
    void sweep() {
        ZyxelDriver &driver = ZyxelDriver::getInstance();
        if (!driver.isConnected()) return;
        ZyxelSshClient &ssh = driver.getSshClient();
        std::string out;
        ssh.unwindToRootPrompt();
        ssh.executeCommand("configure terminal", out);
        ssh.executeCommand("bwm delete 2", out);
        ssh.executeCommand(ZyxelSecurityCmd::cmdNoSslInspectionProfile("NETMON_QA_SSL_E7"), out);
        ssh.executeCommand(ZyxelSecurityCmd::cmdNoAntiVirusProfile("NETMON_QA_AV_E7"), out);
        ssh.executeCommand(ZyxelSecurityCmd::cmdNoAppProfile("NETMON_QA_APP_E7"), out);
        ssh.executeCommand("exit", out);
        ssh.unwindToRootPrompt();
    }

    void setup() override {
        sweep();
        std::this_thread::sleep_for(std::chrono::milliseconds(25));
    }

    void teardown() override {
        sweep();
    }
};

TEST(LiveFirewallE7, ReadBwmActivation) {
    ZyxelDriver &driver = ZyxelDriver::getInstance();
    if (!driver.isConnected()) return;

    auto t0 = std::chrono::steady_clock::now();
    std::string out;
    SshResult res = driver.getSshClient().executeCommand(ZyxelSecurityCmd::cmdShowBwmActivation(), out);
    auto t1 = std::chrono::steady_clock::now();

    CHECK_EQUAL(static_cast<int>(SshResult::SUCCESS), static_cast<int>(res));
    auto p0 = std::chrono::steady_clock::now();
    bool active = false;
    bool parsed = ZyxelSecurityCmd::parseBwmActivation(out, active);
    auto p1 = std::chrono::steady_clock::now();

    double rttMs = std::chrono::duration<double, std::milli>(t1 - t0).count();
    uint64_t parseUs = std::chrono::duration_cast<std::chrono::microseconds>(p1 - p0).count();
    bool passed = (res == SshResult::SUCCESS) && parsed && active && (rttMs <= 2500.0);

    ZyxelBenchmark::record({
        "LiveFirewallE7", "ReadBwmActivation", ZyxelSecurityCmd::cmdShowBwmActivation(),
        rttMs, parseUs, out.size(), parsed ? 1U : 0U, passed,
        passed ? "PASS (SLA)" : "FAIL"
    });

    CHECK_TRUE(parsed);
    CHECK_TRUE(active);
    CHECK_TRUE(rttMs <= 2500.0);
}

TEST(LiveFirewallE7, ReadBwmAll) {
    ZyxelDriver &driver = ZyxelDriver::getInstance();
    if (!driver.isConnected()) return;

    auto t0 = std::chrono::steady_clock::now();
    std::string out;
    SshResult res = driver.getSshClient().executeCommand(ZyxelSecurityCmd::cmdShowBwmAll(), out);
    auto t1 = std::chrono::steady_clock::now();

    CHECK_EQUAL(static_cast<int>(SshResult::SUCCESS), static_cast<int>(res));
    auto p0 = std::chrono::steady_clock::now();
    std::vector<ZyxelBwmRule> rules;
    bool parsed = ZyxelSecurityCmd::parseBwmAll(out, rules);
    auto p1 = std::chrono::steady_clock::now();

    double rttMs = std::chrono::duration<double, std::milli>(t1 - t0).count();
    uint64_t parseUs = std::chrono::duration_cast<std::chrono::microseconds>(p1 - p0).count();
    bool passed = (res == SshResult::SUCCESS) && parsed && !rules.empty() && (rttMs <= 2500.0);

    ZyxelBenchmark::record({
        "LiveFirewallE7", "ReadBwmAll", ZyxelSecurityCmd::cmdShowBwmAll(),
        rttMs, parseUs, out.size(), static_cast<uint32_t>(rules.size()), passed,
        passed ? "PASS (SLA)" : "FAIL"
    });

    CHECK_TRUE(parsed);
    CHECK_FALSE(rules.empty());
    CHECK_TRUE(rttMs <= 2500.0);
}

TEST(LiveFirewallE7, ReadBwmRule1) {
    ZyxelDriver &driver = ZyxelDriver::getInstance();
    if (!driver.isConnected()) return;

    auto t0 = std::chrono::steady_clock::now();
    std::string out;
    SshResult res = driver.getSshClient().executeCommand(ZyxelSecurityCmd::cmdShowBwm("1"), out);
    auto t1 = std::chrono::steady_clock::now();

    CHECK_EQUAL(static_cast<int>(SshResult::SUCCESS), static_cast<int>(res));
    bool found = (out.find("index: 1") != std::string::npos);
    double rttMs = std::chrono::duration<double, std::milli>(t1 - t0).count();
    bool passed = (res == SshResult::SUCCESS) && found && (rttMs <= 2500.0);

    ZyxelBenchmark::record({
        "LiveFirewallE7", "ReadBwmRule1", ZyxelSecurityCmd::cmdShowBwm("1"),
        rttMs, 0, out.size(), found ? 1U : 0U, passed,
        passed ? "PASS (SLA)" : "FAIL"
    });

    CHECK_TRUE(found);
    CHECK_TRUE(rttMs <= 2500.0);
}

TEST(LiveFirewallE7, ReadBwmControlTcpAck) {
    ZyxelDriver &driver = ZyxelDriver::getInstance();
    if (!driver.isConnected()) return;

    auto t0 = std::chrono::steady_clock::now();
    std::string out;
    SshResult res = driver.getSshClient().executeCommand(ZyxelSecurityCmd::cmdShowBwmControlTcpAck(), out);
    auto t1 = std::chrono::steady_clock::now();

    CHECK_EQUAL(static_cast<int>(SshResult::SUCCESS), static_cast<int>(res));
    auto p0 = std::chrono::steady_clock::now();
    bool active = false;
    bool parsed = ZyxelSecurityCmd::parseBwmControlTcpAck(out, active);
    auto p1 = std::chrono::steady_clock::now();

    double rttMs = std::chrono::duration<double, std::milli>(t1 - t0).count();
    uint64_t parseUs = std::chrono::duration_cast<std::chrono::microseconds>(p1 - p0).count();
    bool passed = (res == SshResult::SUCCESS) && parsed && (rttMs <= 2500.0);

    ZyxelBenchmark::record({
        "LiveFirewallE7", "ReadBwmControlTcpAck", ZyxelSecurityCmd::cmdShowBwmControlTcpAck(),
        rttMs, parseUs, out.size(), parsed ? 1U : 0U, passed,
        passed ? "PASS (SLA)" : "FAIL"
    });

    CHECK_TRUE(parsed);
    CHECK_TRUE(rttMs <= 2500.0);
}

TEST(LiveFirewallE7, ReadBwmDefault) {
    ZyxelDriver &driver = ZyxelDriver::getInstance();
    if (!driver.isConnected()) return;

    auto t0 = std::chrono::steady_clock::now();
    std::string out;
    SshResult res = driver.getSshClient().executeCommand(ZyxelSecurityCmd::cmdShowBwmDefault(), out);
    auto t1 = std::chrono::steady_clock::now();

    CHECK_EQUAL(static_cast<int>(SshResult::SUCCESS), static_cast<int>(res));
    bool found = (out.find("index: default") != std::string::npos);
    double rttMs = std::chrono::duration<double, std::milli>(t1 - t0).count();
    bool passed = (res == SshResult::SUCCESS) && found && (rttMs <= 2500.0);

    ZyxelBenchmark::record({
        "LiveFirewallE7", "ReadBwmDefault", ZyxelSecurityCmd::cmdShowBwmDefault(),
        rttMs, 0, out.size(), found ? 1U : 0U, passed,
        passed ? "PASS (SLA)" : "FAIL"
    });

    CHECK_TRUE(found);
    CHECK_TRUE(rttMs <= 2500.0);
}

TEST(LiveFirewallE7, ReadBwmApplicationsList) {
    ZyxelDriver &driver = ZyxelDriver::getInstance();
    if (!driver.isConnected()) return;

    auto t0 = std::chrono::steady_clock::now();
    std::string out;
    SshResult res = driver.getSshClient().executeCommand(ZyxelSecurityCmd::cmdShowBwmApplicationsList(), out);
    auto t1 = std::chrono::steady_clock::now();

    CHECK_EQUAL(static_cast<int>(SshResult::SUCCESS), static_cast<int>(res));
    bool found = (out.find("Application Name") != std::string::npos);
    double rttMs = std::chrono::duration<double, std::milli>(t1 - t0).count();
    bool passed = (res == SshResult::SUCCESS) && found && (rttMs <= 2500.0);

    ZyxelBenchmark::record({
        "LiveFirewallE7", "ReadBwmApplicationsList", ZyxelSecurityCmd::cmdShowBwmApplicationsList(),
        rttMs, 0, out.size(), found ? 1U : 0U, passed,
        passed ? "PASS (SLA)" : "FAIL"
    });

    CHECK_TRUE(found);
    CHECK_TRUE(rttMs <= 2500.0);
}

TEST(LiveFirewallE7, ReadAppProfiles) {
    ZyxelDriver &driver = ZyxelDriver::getInstance();
    if (!driver.isConnected()) return;

    auto t0 = std::chrono::steady_clock::now();
    std::string out;
    SshResult res = driver.getSshClient().executeCommand(ZyxelSecurityCmd::cmdShowAppProfiles(), out);
    auto t1 = std::chrono::steady_clock::now();

    CHECK_EQUAL(static_cast<int>(SshResult::SUCCESS), static_cast<int>(res));
    bool found = (out.find("profile name:") != std::string::npos);
    double rttMs = std::chrono::duration<double, std::milli>(t1 - t0).count();
    bool passed = (res == SshResult::SUCCESS) && found && (rttMs <= 2500.0);

    ZyxelBenchmark::record({
        "LiveFirewallE7", "ReadAppProfiles", ZyxelSecurityCmd::cmdShowAppProfiles(),
        rttMs, 0, out.size(), found ? 1U : 0U, passed,
        passed ? "PASS (SLA)" : "FAIL"
    });

    CHECK_TRUE(found);
    CHECK_TRUE(rttMs <= 2500.0);
}

TEST(LiveFirewallE7, ReadAppStatisticsSummary) {
    ZyxelDriver &driver = ZyxelDriver::getInstance();
    if (!driver.isConnected()) return;

    auto t0 = std::chrono::steady_clock::now();
    std::string out;
    SshResult res = driver.getSshClient().executeCommand(ZyxelSecurityCmd::cmdShowAppStatisticsSummary(), out);
    auto t1 = std::chrono::steady_clock::now();

    CHECK_EQUAL(static_cast<int>(SshResult::SUCCESS), static_cast<int>(res));
    auto p0 = std::chrono::steady_clock::now();
    ZyxelAppPatrolSummary summary;
    bool parsed = ZyxelSecurityCmd::parseAppStatisticsSummary(out, summary);
    auto p1 = std::chrono::steady_clock::now();

    double rttMs = std::chrono::duration<double, std::milli>(t1 - t0).count();
    uint64_t parseUs = std::chrono::duration_cast<std::chrono::microseconds>(p1 - p0).count();
    bool passed = (res == SshResult::SUCCESS) && parsed && (summary.forwardedKb > 0) && (rttMs <= 2500.0);

    ZyxelBenchmark::record({
        "LiveFirewallE7", "ReadAppStatisticsSummary", ZyxelSecurityCmd::cmdShowAppStatisticsSummary(),
        rttMs, parseUs, out.size(), parsed ? 1U : 0U, passed,
        passed ? "PASS (SLA)" : "FAIL"
    });

    CHECK_TRUE(parsed);
    CHECK_TRUE(rttMs <= 2500.0);
}

TEST(LiveFirewallE7, ReadAntiVirusProfile) {
    ZyxelDriver &driver = ZyxelDriver::getInstance();
    if (!driver.isConnected()) return;

    auto t0 = std::chrono::steady_clock::now();
    std::string out;
    SshResult res = driver.getSshClient().executeCommand(ZyxelSecurityCmd::cmdShowAntiVirusProfile(), out);
    auto t1 = std::chrono::steady_clock::now();

    CHECK_EQUAL(static_cast<int>(SshResult::SUCCESS), static_cast<int>(res));
    bool found = (out.find("name: default_profile") != std::string::npos);
    double rttMs = std::chrono::duration<double, std::milli>(t1 - t0).count();
    bool passed = (res == SshResult::SUCCESS) && found && (rttMs <= 2500.0);

    ZyxelBenchmark::record({
        "LiveFirewallE7", "ReadAntiVirusProfile", ZyxelSecurityCmd::cmdShowAntiVirusProfile(),
        rttMs, 0, out.size(), found ? 1U : 0U, passed,
        passed ? "PASS (SLA)" : "FAIL"
    });

    CHECK_TRUE(found);
    CHECK_TRUE(rttMs <= 2500.0);
}

TEST(LiveFirewallE7, ReadAntiVirusUpdate) {
    ZyxelDriver &driver = ZyxelDriver::getInstance();
    if (!driver.isConnected()) return;

    auto t0 = std::chrono::steady_clock::now();
    std::string out;
    SshResult res = driver.getSshClient().executeCommand(ZyxelSecurityCmd::cmdShowAntiVirusUpdate(), out);
    auto t1 = std::chrono::steady_clock::now();

    CHECK_EQUAL(static_cast<int>(SshResult::SUCCESS), static_cast<int>(res));
    bool found = (out.find("schedule:") != std::string::npos || out.find("auto:") != std::string::npos);
    double rttMs = std::chrono::duration<double, std::milli>(t1 - t0).count();
    bool passed = (res == SshResult::SUCCESS) && found && (rttMs <= 2500.0);

    ZyxelBenchmark::record({
        "LiveFirewallE7", "ReadAntiVirusUpdate", ZyxelSecurityCmd::cmdShowAntiVirusUpdate(),
        rttMs, 0, out.size(), found ? 1U : 0U, passed,
        passed ? "PASS (SLA)" : "FAIL"
    });

    CHECK_TRUE(found);
    CHECK_TRUE(rttMs <= 2500.0);
}

TEST(LiveFirewallE7, ReadIdpStatisticsSummary) {
    ZyxelDriver &driver = ZyxelDriver::getInstance();
    if (!driver.isConnected()) return;

    auto t0 = std::chrono::steady_clock::now();
    std::string out;
    SshResult res = driver.getSshClient().executeCommand(ZyxelSecurityCmd::cmdShowIdpStatisticsSummary(), out);
    auto t1 = std::chrono::steady_clock::now();

    CHECK_EQUAL(static_cast<int>(SshResult::SUCCESS), static_cast<int>(res));
    auto p0 = std::chrono::steady_clock::now();
    ZyxelIdpSummary summary;
    bool parsed = ZyxelSecurityCmd::parseIdpStatisticsSummary(out, summary);
    auto p1 = std::chrono::steady_clock::now();

    double rttMs = std::chrono::duration<double, std::milli>(t1 - t0).count();
    uint64_t parseUs = std::chrono::duration_cast<std::chrono::microseconds>(p1 - p0).count();
    bool passed = (res == SshResult::SUCCESS) && parsed && (rttMs <= 2500.0);

    ZyxelBenchmark::record({
        "LiveFirewallE7", "ReadIdpStatisticsSummary", ZyxelSecurityCmd::cmdShowIdpStatisticsSummary(),
        rttMs, parseUs, out.size(), parsed ? 1U : 0U, passed,
        passed ? "PASS (SLA)" : "FAIL"
    });

    CHECK_TRUE(parsed);
    CHECK_TRUE(rttMs <= 2500.0);
}

TEST(LiveFirewallE7, ReadContentFilterProfile) {
    ZyxelDriver &driver = ZyxelDriver::getInstance();
    if (!driver.isConnected()) return;

    auto t0 = std::chrono::steady_clock::now();
    std::string out;
    SshResult res = driver.getSshClient().executeCommand(ZyxelSecurityCmd::cmdShowContentFilterProfile(), out);
    auto t1 = std::chrono::steady_clock::now();

    CHECK_EQUAL(static_cast<int>(SshResult::SUCCESS), static_cast<int>(res));
    bool found = (out.find("Name") != std::string::npos);
    double rttMs = std::chrono::duration<double, std::milli>(t1 - t0).count();
    bool passed = (res == SshResult::SUCCESS) && found && (rttMs <= 2500.0);

    ZyxelBenchmark::record({
        "LiveFirewallE7", "ReadContentFilterProfile", ZyxelSecurityCmd::cmdShowContentFilterProfile(),
        rttMs, 0, out.size(), found ? 1U : 0U, passed,
        passed ? "PASS (SLA)" : "FAIL"
    });

    CHECK_TRUE(found);
    CHECK_TRUE(rttMs <= 2500.0);
}

TEST(LiveFirewallE7, ReadContentFilterSettings) {
    ZyxelDriver &driver = ZyxelDriver::getInstance();
    if (!driver.isConnected()) return;

    auto t0 = std::chrono::steady_clock::now();
    std::string out;
    SshResult res = driver.getSshClient().executeCommand(ZyxelSecurityCmd::cmdShowContentFilterSettings(), out);
    auto t1 = std::chrono::steady_clock::now();

    CHECK_EQUAL(static_cast<int>(SshResult::SUCCESS), static_cast<int>(res));
    auto p0 = std::chrono::steady_clock::now();
    ZyxelContentFilterSettings settings;
    bool parsed = ZyxelSecurityCmd::parseContentFilterSettings(out, settings);
    auto p1 = std::chrono::steady_clock::now();

    double rttMs = std::chrono::duration<double, std::milli>(t1 - t0).count();
    uint64_t parseUs = std::chrono::duration_cast<std::chrono::microseconds>(p1 - p0).count();
    bool passed = (res == SshResult::SUCCESS) && parsed && (settings.serviceTimeout > 0) && (rttMs <= 2500.0);

    ZyxelBenchmark::record({
        "LiveFirewallE7", "ReadContentFilterSettings", ZyxelSecurityCmd::cmdShowContentFilterSettings(),
        rttMs, parseUs, out.size(), parsed ? 1U : 0U, passed,
        passed ? "PASS (SLA)" : "FAIL"
    });

    CHECK_TRUE(parsed);
    CHECK_TRUE(settings.serviceTimeout > 0);
    CHECK_TRUE(rttMs <= 2500.0);
}

TEST(LiveFirewallE7, ReadAntiSpamProfile) {
    ZyxelDriver &driver = ZyxelDriver::getInstance();
    if (!driver.isConnected()) return;

    auto t0 = std::chrono::steady_clock::now();
    std::string out;
    SshResult res = driver.getSshClient().executeCommand(ZyxelSecurityCmd::cmdShowAntiSpamProfile(), out);
    auto t1 = std::chrono::steady_clock::now();

    CHECK_EQUAL(static_cast<int>(SshResult::SUCCESS), static_cast<int>(res));
    bool found = (out.find("profile name: default_profile") != std::string::npos);
    double rttMs = std::chrono::duration<double, std::milli>(t1 - t0).count();
    bool passed = (res == SshResult::SUCCESS) && found && (rttMs <= 2500.0);

    ZyxelBenchmark::record({
        "LiveFirewallE7", "ReadAntiSpamProfile", ZyxelSecurityCmd::cmdShowAntiSpamProfile(),
        rttMs, 0, out.size(), found ? 1U : 0U, passed,
        passed ? "PASS (SLA)" : "FAIL"
    });

    CHECK_TRUE(found);
    CHECK_TRUE(rttMs <= 2500.0);
}

TEST(LiveFirewallE7, ReadCdrStatus) {
    ZyxelDriver &driver = ZyxelDriver::getInstance();
    if (!driver.isConnected()) return;

    auto t0 = std::chrono::steady_clock::now();
    std::string out;
    SshResult res = driver.getSshClient().executeCommand(ZyxelSecurityCmd::cmdShowCdrStatus(), out);
    auto t1 = std::chrono::steady_clock::now();

    CHECK_EQUAL(static_cast<int>(SshResult::SUCCESS), static_cast<int>(res));
    auto p0 = std::chrono::steady_clock::now();
    ZyxelCdrStatus status;
    bool parsed = ZyxelSecurityCmd::parseCdrStatus(out, status);
    auto p1 = std::chrono::steady_clock::now();

    double rttMs = std::chrono::duration<double, std::milli>(t1 - t0).count();
    uint64_t parseUs = std::chrono::duration_cast<std::chrono::microseconds>(p1 - p0).count();
    bool passed = (res == SshResult::SUCCESS) && parsed && (status.blockedBy == "ip") && (rttMs <= 2500.0);

    ZyxelBenchmark::record({
        "LiveFirewallE7", "ReadCdrStatus", ZyxelSecurityCmd::cmdShowCdrStatus(),
        rttMs, parseUs, out.size(), parsed ? 1U : 0U, passed,
        passed ? "PASS (SLA)" : "FAIL"
    });

    CHECK_TRUE(parsed);
    CHECK_EQUAL("ip", status.blockedBy);
    CHECK_TRUE(rttMs <= 2500.0);
}

TEST(LiveFirewallE7, ReadCdrBlockList) {
    ZyxelDriver &driver = ZyxelDriver::getInstance();
    if (!driver.isConnected()) return;

    auto t0 = std::chrono::steady_clock::now();
    std::string out;
    SshResult res = driver.getSshClient().executeCommand(ZyxelSecurityCmd::cmdShowCdrBlockList(), out);
    auto t1 = std::chrono::steady_clock::now();

    CHECK_EQUAL(static_cast<int>(SshResult::SUCCESS), static_cast<int>(res));
    bool found = (out.find("Address") != std::string::npos || out.find("Date/Time") != std::string::npos);
    double rttMs = std::chrono::duration<double, std::milli>(t1 - t0).count();
    bool passed = (res == SshResult::SUCCESS) && found && (rttMs <= 2500.0);

    ZyxelBenchmark::record({
        "LiveFirewallE7", "ReadCdrBlockList", ZyxelSecurityCmd::cmdShowCdrBlockList(),
        rttMs, 0, out.size(), found ? 1U : 0U, passed,
        passed ? "PASS (SLA)" : "FAIL"
    });

    CHECK_TRUE(found);
    CHECK_TRUE(rttMs <= 2500.0);
}

TEST(LiveFirewallE7, ReadCdrRules) {
    ZyxelDriver &driver = ZyxelDriver::getInstance();
    if (!driver.isConnected()) return;

    auto t0 = std::chrono::steady_clock::now();
    std::string out;
    SshResult res = driver.getSshClient().executeCommand(ZyxelSecurityCmd::cmdShowCdrRules(), out);
    auto t1 = std::chrono::steady_clock::now();

    CHECK_EQUAL(static_cast<int>(SshResult::SUCCESS), static_cast<int>(res));
    auto p0 = std::chrono::steady_clock::now();
    std::vector<ZyxelCdrRule> rules;
    bool parsed = ZyxelSecurityCmd::parseCdrRules(out, rules);
    auto p1 = std::chrono::steady_clock::now();

    double rttMs = std::chrono::duration<double, std::milli>(t1 - t0).count();
    uint64_t parseUs = std::chrono::duration_cast<std::chrono::microseconds>(p1 - p0).count();
    bool passed = (res == SshResult::SUCCESS) && parsed && (rules.size() >= 3) && (rttMs <= 2500.0);

    ZyxelBenchmark::record({
        "LiveFirewallE7", "ReadCdrRules", ZyxelSecurityCmd::cmdShowCdrRules(),
        rttMs, parseUs, out.size(), static_cast<uint32_t>(rules.size()), passed,
        passed ? "PASS (SLA)" : "FAIL"
    });

    CHECK_TRUE(parsed);
    CHECK_TRUE(rules.size() >= 3);
    CHECK_TRUE(rttMs <= 2500.0);
}

TEST(LiveFirewallE7, ReadSslInspectionStatus) {
    ZyxelDriver &driver = ZyxelDriver::getInstance();
    if (!driver.isConnected()) return;

    auto t0 = std::chrono::steady_clock::now();
    std::string out;
    SshResult res = driver.getSshClient().executeCommand(ZyxelSecurityCmd::cmdShowSslInspectionStatus(), out);
    auto t1 = std::chrono::steady_clock::now();

    CHECK_EQUAL(static_cast<int>(SshResult::SUCCESS), static_cast<int>(res));
    auto p0 = std::chrono::steady_clock::now();
    ZyxelSslInspectionStatus status;
    bool parsed = ZyxelSecurityCmd::parseSslInspectionStatus(out, status);
    auto p1 = std::chrono::steady_clock::now();

    double rttMs = std::chrono::duration<double, std::milli>(t1 - t0).count();
    uint64_t parseUs = std::chrono::duration_cast<std::chrono::microseconds>(p1 - p0).count();
    bool passed = (res == SshResult::SUCCESS) && parsed && status.tls13Active && (rttMs <= 2500.0);

    ZyxelBenchmark::record({
        "LiveFirewallE7", "ReadSslInspectionStatus", ZyxelSecurityCmd::cmdShowSslInspectionStatus(),
        rttMs, parseUs, out.size(), parsed ? 1U : 0U, passed,
        passed ? "PASS (SLA)" : "FAIL"
    });

    CHECK_TRUE(parsed);
    CHECK_TRUE(status.tls13Active);
    CHECK_TRUE(rttMs <= 2500.0);
}

TEST(LiveFirewallE7, ReadSslInspectionProfile) {
    ZyxelDriver &driver = ZyxelDriver::getInstance();
    if (!driver.isConnected()) return;

    auto t0 = std::chrono::steady_clock::now();
    std::string out;
    SshResult res = driver.getSshClient().executeCommand(ZyxelSecurityCmd::cmdShowSslInspectionProfile(), out);
    auto t1 = std::chrono::steady_clock::now();

    CHECK_EQUAL(static_cast<int>(SshResult::SUCCESS), static_cast<int>(res));
    bool found = (out.find("profile name: default") != std::string::npos);
    double rttMs = std::chrono::duration<double, std::milli>(t1 - t0).count();
    bool passed = (res == SshResult::SUCCESS) && found && (rttMs <= 2500.0);

    ZyxelBenchmark::record({
        "LiveFirewallE7", "ReadSslInspectionProfile", ZyxelSecurityCmd::cmdShowSslInspectionProfile(),
        rttMs, 0, out.size(), found ? 1U : 0U, passed,
        passed ? "PASS (SLA)" : "FAIL"
    });

    CHECK_TRUE(found);
    CHECK_TRUE(rttMs <= 2500.0);
}

TEST(LiveFirewallE7, DryFireRtls) {
    ZyxelDriver &driver = ZyxelDriver::getInstance();
    if (!driver.isConnected()) return;

    auto t0 = std::chrono::steady_clock::now();
    std::string out;
    SshResult res = driver.getSshClient().executeCommand(ZyxelSecurityCmd::cmdShowRtls(), out);
    auto t1 = std::chrono::steady_clock::now();

    CHECK_EQUAL(static_cast<int>(SshResult::SUCCESS), static_cast<int>(res));
    bool unsupported = (out.find("Parse error") != std::string::npos);
    double rttMs = std::chrono::duration<double, std::milli>(t1 - t0).count();
    bool passed = (res == SshResult::SUCCESS) && unsupported && (rttMs <= 2500.0);

    ZyxelBenchmark::record({
        "LiveFirewallE7", "DryFireRtls", ZyxelSecurityCmd::cmdShowRtls(),
        rttMs, 0, out.size(), unsupported ? 1U : 0U, passed,
        passed ? "PASS (DRY-FIRE)" : "FAIL"
    });

    CHECK_TRUE(unsupported);
    CHECK_TRUE(rttMs <= 2500.0);
}

TEST(LiveFirewallE7, DryFireReputationFilter) {
    ZyxelDriver &driver = ZyxelDriver::getInstance();
    if (!driver.isConnected()) return;

    auto t0 = std::chrono::steady_clock::now();
    std::string out;
    SshResult res = driver.getSshClient().executeCommand(ZyxelSecurityCmd::cmdShowReputationFilterStatus(), out);
    auto t1 = std::chrono::steady_clock::now();

    CHECK_EQUAL(static_cast<int>(SshResult::SUCCESS), static_cast<int>(res));
    bool unsupported = (out.find("Parse error") != std::string::npos);
    double rttMs = std::chrono::duration<double, std::milli>(t1 - t0).count();
    bool passed = (res == SshResult::SUCCESS) && unsupported && (rttMs <= 2500.0);

    ZyxelBenchmark::record({
        "LiveFirewallE7", "DryFireReputationFilter", ZyxelSecurityCmd::cmdShowReputationFilterStatus(),
        rttMs, 0, out.size(), unsupported ? 1U : 0U, passed,
        passed ? "PASS (DRY-FIRE)" : "FAIL"
    });

    CHECK_TRUE(unsupported);
    CHECK_TRUE(rttMs <= 2500.0);
}

TEST(LiveFirewallE7, DryFireSandboxing) {
    ZyxelDriver &driver = ZyxelDriver::getInstance();
    if (!driver.isConnected()) return;

    auto t0 = std::chrono::steady_clock::now();
    std::string out;
    SshResult res = driver.getSshClient().executeCommand(ZyxelSecurityCmd::cmdShowSandboxingStatus(), out);
    auto t1 = std::chrono::steady_clock::now();

    CHECK_EQUAL(static_cast<int>(SshResult::SUCCESS), static_cast<int>(res));
    bool unsupported = (out.find("Parse error") != std::string::npos);
    double rttMs = std::chrono::duration<double, std::milli>(t1 - t0).count();
    bool passed = (res == SshResult::SUCCESS) && unsupported && (rttMs <= 2500.0);

    ZyxelBenchmark::record({
        "LiveFirewallE7", "DryFireSandboxing", ZyxelSecurityCmd::cmdShowSandboxingStatus(),
        rttMs, 0, out.size(), unsupported ? 1U : 0U, passed,
        passed ? "PASS (DRY-FIRE)" : "FAIL"
    });

    CHECK_TRUE(unsupported);
    CHECK_TRUE(rttMs <= 2500.0);
}

TEST(LiveFirewallE7, DryFireAppInvalid) {
    ZyxelDriver &driver = ZyxelDriver::getInstance();
    if (!driver.isConnected()) return;

    auto t0 = std::chrono::steady_clock::now();
    std::string out;
    SshResult res = driver.getSshClient().executeCommand(ZyxelSecurityCmd::cmdInvalidAppDryFire(), out);
    auto t1 = std::chrono::steady_clock::now();

    CHECK_EQUAL(static_cast<int>(SshResult::SUCCESS), static_cast<int>(res));
    bool rejected = (out.find("Parse error") != std::string::npos);
    double rttMs = std::chrono::duration<double, std::milli>(t1 - t0).count();
    bool passed = (res == SshResult::SUCCESS) && rejected && (rttMs <= 2500.0);

    ZyxelBenchmark::record({
        "LiveFirewallE7", "DryFireAppInvalid", ZyxelSecurityCmd::cmdInvalidAppDryFire(),
        rttMs, 0, out.size(), rejected ? 1U : 0U, passed,
        passed ? "PASS (DRY-FIRE)" : "FAIL"
    });

    CHECK_TRUE(rejected);
    CHECK_TRUE(rttMs <= 2500.0);
}

TEST(LiveFirewallE7, DryFireAntiVirusInvalid) {
    ZyxelDriver &driver = ZyxelDriver::getInstance();
    if (!driver.isConnected()) return;

    auto t0 = std::chrono::steady_clock::now();
    std::string out;
    SshResult res = driver.getSshClient().executeCommand(ZyxelSecurityCmd::cmdInvalidAntiVirusDryFire(), out);
    auto t1 = std::chrono::steady_clock::now();

    CHECK_EQUAL(static_cast<int>(SshResult::SUCCESS), static_cast<int>(res));
    bool rejected = (out.find("Parse error") != std::string::npos);
    double rttMs = std::chrono::duration<double, std::milli>(t1 - t0).count();
    bool passed = (res == SshResult::SUCCESS) && rejected && (rttMs <= 2500.0);

    ZyxelBenchmark::record({
        "LiveFirewallE7", "DryFireAntiVirusInvalid", ZyxelSecurityCmd::cmdInvalidAntiVirusDryFire(),
        rttMs, 0, out.size(), rejected ? 1U : 0U, passed,
        passed ? "PASS (DRY-FIRE)" : "FAIL"
    });

    CHECK_TRUE(rejected);
    CHECK_TRUE(rttMs <= 2500.0);
}

TEST(LiveFirewallE7, DryFireSslInspectionInvalid) {
    ZyxelDriver &driver = ZyxelDriver::getInstance();
    if (!driver.isConnected()) return;

    auto t0 = std::chrono::steady_clock::now();
    std::string out;
    SshResult res = driver.getSshClient().executeCommand(ZyxelSecurityCmd::cmdInvalidSslInspectionDryFire(), out);
    auto t1 = std::chrono::steady_clock::now();

    CHECK_EQUAL(static_cast<int>(SshResult::SUCCESS), static_cast<int>(res));
    bool rejected = (out.find("Parse error") != std::string::npos);
    double rttMs = std::chrono::duration<double, std::milli>(t1 - t0).count();
    bool passed = (res == SshResult::SUCCESS) && rejected && (rttMs <= 2500.0);

    ZyxelBenchmark::record({
        "LiveFirewallE7", "DryFireSslInspectionInvalid", ZyxelSecurityCmd::cmdInvalidSslInspectionDryFire(),
        rttMs, 0, out.size(), rejected ? 1U : 0U, passed,
        passed ? "PASS (DRY-FIRE)" : "FAIL"
    });

    CHECK_TRUE(rejected);
    CHECK_TRUE(rttMs <= 2500.0);
}

TEST(LiveFirewallE7, DryFireBwmOutOfRange) {
    ZyxelDriver &driver = ZyxelDriver::getInstance();
    if (!driver.isConnected()) return;

    auto t0 = std::chrono::steady_clock::now();
    std::string out;
    SshResult res = driver.getSshClient().executeCommand(ZyxelSecurityCmd::cmdInvalidBwmDryFire(), out);
    auto t1 = std::chrono::steady_clock::now();

    CHECK_EQUAL(static_cast<int>(SshResult::SUCCESS), static_cast<int>(res));
    bool rejected = (out.find("Parse error") != std::string::npos || out.find("out of range") != std::string::npos);
    double rttMs = std::chrono::duration<double, std::milli>(t1 - t0).count();
    bool passed = (res == SshResult::SUCCESS) && rejected && (rttMs <= 2500.0);

    ZyxelBenchmark::record({
        "LiveFirewallE7", "DryFireBwmOutOfRange", ZyxelSecurityCmd::cmdInvalidBwmDryFire(),
        rttMs, 0, out.size(), rejected ? 1U : 0U, passed,
        passed ? "PASS (DRY-FIRE)" : "FAIL"
    });

    CHECK_TRUE(rejected);
    CHECK_TRUE(rttMs <= 2500.0);
}

TEST(LiveFirewallE7, WriteBwmAppendRollback) {
    ZyxelDriver &driver = ZyxelDriver::getInstance();
    if (!driver.isConnected()) return;
    ZyxelSshClient &ssh = driver.getSshClient();

    auto t0 = std::chrono::steady_clock::now();
    std::string out;

    // 1. Append BWM rule
    ssh.executeCommand("configure terminal", out);
    ssh.executeCommand(ZyxelSecurityCmd::cmdBwmAppend(), out);
    ssh.executeCommand("description NETMON_QA_BWM_E7", out);
    ssh.executeCommand("exit", out);
    ssh.executeCommand("exit", out);
    ssh.unwindToRootPrompt();

    // 2. Verify creation
    ssh.executeCommand(ZyxelSecurityCmd::cmdShowBwmAll(), out);
    bool created = (out.find("NETMON_QA_BWM_E7") != std::string::npos);

    // 3. Rollback: delete rule 2
    ssh.executeCommand("configure terminal", out);
    SshResult resDel = ssh.executeCommand(ZyxelSecurityCmd::cmdBwmDelete(2), out);
    ssh.executeCommand("exit", out);
    ssh.unwindToRootPrompt();

    CHECK_EQUAL(static_cast<int>(SshResult::SUCCESS), static_cast<int>(resDel));

    // 4. Verify clean deletion
    ssh.executeCommand(ZyxelSecurityCmd::cmdShowBwmAll(), out);
    bool deleted = (out.find("NETMON_QA_BWM_E7") == std::string::npos);
    auto t1 = std::chrono::steady_clock::now();

    double rttMs = std::chrono::duration<double, std::milli>(t1 - t0).count();
    bool passed = created && deleted && (rttMs <= 6000.0);

    ZyxelBenchmark::record({
        "LiveFirewallE7", "WriteBwmAppendRollback", "bwm append NETMON_QA_BWM_E7",
        rttMs, 0, out.size(), 1, passed,
        passed ? "PASS (ROLLBACK)" : "FAIL"
    });

    CHECK_TRUE(created);
    CHECK_TRUE(deleted);
    CHECK_TRUE(rttMs <= 6000.0);
}

TEST(LiveFirewallE7, WriteSslInspectionProfileRollback) {
    ZyxelDriver &driver = ZyxelDriver::getInstance();
    if (!driver.isConnected()) return;
    ZyxelSshClient &ssh = driver.getSshClient();

    auto t0 = std::chrono::steady_clock::now();
    std::string out;

    // 1. Create SSL Inspection profile
    ssh.executeCommand("configure terminal", out);
    ssh.executeCommand(ZyxelSecurityCmd::cmdSslInspectionProfile("NETMON_QA_SSL_E7"), out);
    ssh.executeCommand("exit", out);
    ssh.executeCommand("exit", out);
    ssh.unwindToRootPrompt();

    // 2. Verify creation
    ssh.executeCommand(ZyxelSecurityCmd::cmdShowSslInspectionProfile(), out);
    bool created = (out.find("profile name: NETMON_QA_SSL_E7") != std::string::npos);

    // 3. Rollback
    ssh.executeCommand("configure terminal", out);
    SshResult resDel = ssh.executeCommand(ZyxelSecurityCmd::cmdNoSslInspectionProfile("NETMON_QA_SSL_E7"), out);
    ssh.executeCommand("exit", out);
    ssh.unwindToRootPrompt();

    CHECK_EQUAL(static_cast<int>(SshResult::SUCCESS), static_cast<int>(resDel));

    // 4. Verify clean deletion
    ssh.executeCommand(ZyxelSecurityCmd::cmdShowSslInspectionProfile(), out);
    bool deleted = (out.find("profile name: NETMON_QA_SSL_E7") == std::string::npos);
    auto t1 = std::chrono::steady_clock::now();

    double rttMs = std::chrono::duration<double, std::milli>(t1 - t0).count();
    bool passed = created && deleted && (rttMs <= 6000.0);

    ZyxelBenchmark::record({
        "LiveFirewallE7", "WriteSslInspectionProfileRollback", "ssl-inspection profile NETMON_QA_SSL_E7",
        rttMs, 0, out.size(), 1, passed,
        passed ? "PASS (ROLLBACK)" : "FAIL"
    });

    CHECK_TRUE(created);
    CHECK_TRUE(deleted);
    CHECK_TRUE(rttMs <= 6000.0);
}

TEST(LiveFirewallE7, WriteAntiVirusProfileRollback) {
    ZyxelDriver &driver = ZyxelDriver::getInstance();
    if (!driver.isConnected()) return;
    ZyxelSshClient &ssh = driver.getSshClient();

    auto t0 = std::chrono::steady_clock::now();
    std::string out;

    // 1. Create Anti-Virus profile
    ssh.executeCommand("configure terminal", out);
    ssh.executeCommand(ZyxelSecurityCmd::cmdAntiVirusProfile("NETMON_QA_AV_E7"), out);
    ssh.executeCommand("exit", out);
    ssh.executeCommand("exit", out);
    ssh.unwindToRootPrompt();

    // 2. Verify creation
    ssh.executeCommand(ZyxelSecurityCmd::cmdShowAntiVirusProfile(), out);
    bool created = (out.find("name: NETMON_QA_AV_E7") != std::string::npos);

    // 3. Rollback
    ssh.executeCommand("configure terminal", out);
    SshResult resDel = ssh.executeCommand(ZyxelSecurityCmd::cmdNoAntiVirusProfile("NETMON_QA_AV_E7"), out);
    ssh.executeCommand("exit", out);
    ssh.unwindToRootPrompt();

    CHECK_EQUAL(static_cast<int>(SshResult::SUCCESS), static_cast<int>(resDel));

    // 4. Verify clean deletion
    ssh.executeCommand(ZyxelSecurityCmd::cmdShowAntiVirusProfile(), out);
    bool deleted = (out.find("name: NETMON_QA_AV_E7") == std::string::npos);
    auto t1 = std::chrono::steady_clock::now();

    double rttMs = std::chrono::duration<double, std::milli>(t1 - t0).count();
    bool passed = created && deleted && (rttMs <= 6000.0);

    ZyxelBenchmark::record({
        "LiveFirewallE7", "WriteAntiVirusProfileRollback", "anti-virus NETMON_QA_AV_E7",
        rttMs, 0, out.size(), 1, passed,
        passed ? "PASS (ROLLBACK)" : "FAIL"
    });

    CHECK_TRUE(created);
    CHECK_TRUE(deleted);
    CHECK_TRUE(rttMs <= 6000.0);
}

// ============================================================================
// CppUTest Diagnostic Test Group: LiveFirewallE8 (Envelope 8: VPN - Ch 33..35)
// ============================================================================

TEST_GROUP(LiveFirewallE8) {
    void setup() override {
        ZyxelDriver &driver = ZyxelDriver::getInstance();
        if (driver.isConnected()) {
            ZyxelSshClient &ssh = driver.getSshClient();
            ssh.unwindToRootPrompt();
            std::string out;
            ssh.executeCommand("configure terminal", out);
            ssh.executeCommand("no isakmp policy NETMON_QA_IKE_E8", out);
            ssh.executeCommand("no sslvpn application NETMON_QA_SSLVPN_E8", out);
            ssh.executeCommand("exit", out);
            ssh.unwindToRootPrompt();
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(25));
    }

    void teardown() override {
        ZyxelDriver &driver = ZyxelDriver::getInstance();
        if (driver.isConnected()) {
            ZyxelSshClient &ssh = driver.getSshClient();
            ssh.unwindToRootPrompt();
            std::string out;
            ssh.executeCommand("configure terminal", out);
            ssh.executeCommand("no isakmp policy NETMON_QA_IKE_E8", out);
            ssh.executeCommand("no sslvpn application NETMON_QA_SSLVPN_E8", out);
            ssh.executeCommand("exit", out);
            ssh.unwindToRootPrompt();
        }
    }
};

TEST(LiveFirewallE8, ReadCryptoBoostTcp) {
    ZyxelDriver &driver = ZyxelDriver::getInstance();
    if (!driver.isConnected()) return;

    auto t0 = std::chrono::steady_clock::now();
    std::string out;
    SshResult res = driver.getSshClient().executeCommand(ZyxelVpnCmd::cmdShowCryptoBoostTcp(), out);
    auto t1 = std::chrono::steady_clock::now();

    CHECK_EQUAL(static_cast<int>(SshResult::SUCCESS), static_cast<int>(res));

    auto p0 = std::chrono::steady_clock::now();
    bool boostTcp = false;
    bool parsed = ZyxelVpnCmd::parseCryptoBoostTcp(out, boostTcp);
    auto p1 = std::chrono::steady_clock::now();

    double rttMs = std::chrono::duration<double, std::milli>(t1 - t0).count();
    uint64_t parseUs = std::chrono::duration_cast<std::chrono::microseconds>(p1 - p0).count();
    bool passed = (res == SshResult::SUCCESS) && parsed && (rttMs <= 1500.0);

    ZyxelBenchmark::record({
        "LiveFirewallE8", "ReadCryptoBoostTcp", ZyxelVpnCmd::cmdShowCryptoBoostTcp(),
        rttMs, parseUs, out.size(), 1, passed,
        passed ? "PASS (SLA)" : "FAIL"
    });

    CHECK_TRUE(parsed);
    CHECK_TRUE(rttMs <= 1500.0);
}

TEST(LiveFirewallE8, ReadCryptoIgnoreDfBit) {
    ZyxelDriver &driver = ZyxelDriver::getInstance();
    if (!driver.isConnected()) return;

    auto t0 = std::chrono::steady_clock::now();
    std::string out;
    SshResult res = driver.getSshClient().executeCommand(ZyxelVpnCmd::cmdShowCryptoIgnoreDfBit(), out);
    auto t1 = std::chrono::steady_clock::now();

    CHECK_EQUAL(static_cast<int>(SshResult::SUCCESS), static_cast<int>(res));

    double rttMs = std::chrono::duration<double, std::milli>(t1 - t0).count();
    bool passed = (res == SshResult::SUCCESS) && (rttMs <= 1500.0);

    ZyxelBenchmark::record({
        "LiveFirewallE8", "ReadCryptoIgnoreDfBit", ZyxelVpnCmd::cmdShowCryptoIgnoreDfBit(),
        rttMs, 0, out.size(), 1, passed,
        passed ? "PASS (SLA)" : "FAIL"
    });

    CHECK_TRUE(rttMs <= 1500.0);
}

TEST(LiveFirewallE8, ReadCryptoMap) {
    ZyxelDriver &driver = ZyxelDriver::getInstance();
    if (!driver.isConnected()) return;

    auto t0 = std::chrono::steady_clock::now();
    std::string out;
    SshResult res = driver.getSshClient().executeCommand(ZyxelVpnCmd::cmdShowCryptoMap(), out);
    auto t1 = std::chrono::steady_clock::now();

    CHECK_EQUAL(static_cast<int>(SshResult::SUCCESS), static_cast<int>(res));

    double rttMs = std::chrono::duration<double, std::milli>(t1 - t0).count();
    bool passed = (res == SshResult::SUCCESS) && (rttMs <= 1500.0);

    ZyxelBenchmark::record({
        "LiveFirewallE8", "ReadCryptoMap", ZyxelVpnCmd::cmdShowCryptoMap(),
        rttMs, 0, out.size(), 1, passed,
        passed ? "PASS (SLA)" : "FAIL"
    });

    CHECK_TRUE(rttMs <= 1500.0);
}

TEST(LiveFirewallE8, ReadCryptoMap6) {
    ZyxelDriver &driver = ZyxelDriver::getInstance();
    if (!driver.isConnected()) return;

    auto t0 = std::chrono::steady_clock::now();
    std::string out;
    SshResult res = driver.getSshClient().executeCommand(ZyxelVpnCmd::cmdShowCryptoMap6(), out);
    auto t1 = std::chrono::steady_clock::now();

    CHECK_EQUAL(static_cast<int>(SshResult::SUCCESS), static_cast<int>(res));

    double rttMs = std::chrono::duration<double, std::milli>(t1 - t0).count();
    bool passed = (res == SshResult::SUCCESS) && (rttMs <= 1500.0);

    ZyxelBenchmark::record({
        "LiveFirewallE8", "ReadCryptoMap6", ZyxelVpnCmd::cmdShowCryptoMap6(),
        rttMs, 0, out.size(), 1, passed,
        passed ? "PASS (SLA)" : "FAIL"
    });

    CHECK_TRUE(rttMs <= 1500.0);
}

TEST(LiveFirewallE8, ReadCryptoMapConnCheck) {
    ZyxelDriver &driver = ZyxelDriver::getInstance();
    if (!driver.isConnected()) return;

    auto t0 = std::chrono::steady_clock::now();
    std::string out;
    SshResult res = driver.getSshClient().executeCommand(ZyxelVpnCmd::cmdShowCryptoMapConnCheck(), out);
    auto t1 = std::chrono::steady_clock::now();

    CHECK_EQUAL(static_cast<int>(SshResult::SUCCESS), static_cast<int>(res));

    double rttMs = std::chrono::duration<double, std::milli>(t1 - t0).count();
    bool passed = (res == SshResult::SUCCESS) && (rttMs <= 1500.0);

    ZyxelBenchmark::record({
        "LiveFirewallE8", "ReadCryptoMapConnCheck", ZyxelVpnCmd::cmdShowCryptoMapConnCheck(),
        rttMs, 0, out.size(), 1, passed,
        passed ? "PASS (SLA)" : "FAIL"
    });

    CHECK_TRUE(rttMs <= 1500.0);
}

TEST(LiveFirewallE8, ReadIkev2Policy) {
    ZyxelDriver &driver = ZyxelDriver::getInstance();
    if (!driver.isConnected()) return;

    auto t0 = std::chrono::steady_clock::now();
    std::string out;
    SshResult res = driver.getSshClient().executeCommand(ZyxelVpnCmd::cmdShowIkev2Policy(), out);
    auto t1 = std::chrono::steady_clock::now();

    CHECK_EQUAL(static_cast<int>(SshResult::SUCCESS), static_cast<int>(res));

    double rttMs = std::chrono::duration<double, std::milli>(t1 - t0).count();
    bool passed = (res == SshResult::SUCCESS) && (rttMs <= 1500.0);

    ZyxelBenchmark::record({
        "LiveFirewallE8", "ReadIkev2Policy", ZyxelVpnCmd::cmdShowIkev2Policy(),
        rttMs, 0, out.size(), 1, passed,
        passed ? "PASS (SLA)" : "FAIL"
    });

    CHECK_TRUE(rttMs <= 1500.0);
}

TEST(LiveFirewallE8, ReadIsakmpPolicy) {
    ZyxelDriver &driver = ZyxelDriver::getInstance();
    if (!driver.isConnected()) return;

    auto t0 = std::chrono::steady_clock::now();
    std::string out;
    SshResult res = driver.getSshClient().executeCommand(ZyxelVpnCmd::cmdShowIsakmpPolicy(), out);
    auto t1 = std::chrono::steady_clock::now();

    CHECK_EQUAL(static_cast<int>(SshResult::SUCCESS), static_cast<int>(res));

    double rttMs = std::chrono::duration<double, std::milli>(t1 - t0).count();
    bool passed = (res == SshResult::SUCCESS) && (rttMs <= 1500.0);

    ZyxelBenchmark::record({
        "LiveFirewallE8", "ReadIsakmpPolicy", ZyxelVpnCmd::cmdShowIsakmpPolicy(),
        rttMs, 0, out.size(), 1, passed,
        passed ? "PASS (SLA)" : "FAIL"
    });

    CHECK_TRUE(rttMs <= 1500.0);
}

TEST(LiveFirewallE8, ReadIsakmpSa) {
    ZyxelDriver &driver = ZyxelDriver::getInstance();
    if (!driver.isConnected()) return;

    auto t0 = std::chrono::steady_clock::now();
    std::string out;
    SshResult res = driver.getSshClient().executeCommand(ZyxelVpnCmd::cmdShowIsakmpSa(), out);
    auto t1 = std::chrono::steady_clock::now();

    CHECK_EQUAL(static_cast<int>(SshResult::SUCCESS), static_cast<int>(res));

    double rttMs = std::chrono::duration<double, std::milli>(t1 - t0).count();
    bool passed = (res == SshResult::SUCCESS) && (rttMs <= 1500.0);

    ZyxelBenchmark::record({
        "LiveFirewallE8", "ReadIsakmpSa", ZyxelVpnCmd::cmdShowIsakmpSa(),
        rttMs, 0, out.size(), 1, passed,
        passed ? "PASS (SLA)" : "FAIL"
    });

    CHECK_TRUE(rttMs <= 1500.0);
}

TEST(LiveFirewallE8, ReadSaCounter) {
    ZyxelDriver &driver = ZyxelDriver::getInstance();
    if (!driver.isConnected()) return;

    auto t0 = std::chrono::steady_clock::now();
    std::string out;
    SshResult res = driver.getSshClient().executeCommand(ZyxelVpnCmd::cmdShowSaCounter(), out);
    auto t1 = std::chrono::steady_clock::now();

    CHECK_EQUAL(static_cast<int>(SshResult::SUCCESS), static_cast<int>(res));

    auto p0 = std::chrono::steady_clock::now();
    int vpnStatus = -1;
    bool parsed = ZyxelVpnCmd::parseSaCounter(out, vpnStatus);
    auto p1 = std::chrono::steady_clock::now();

    double rttMs = std::chrono::duration<double, std::milli>(t1 - t0).count();
    uint64_t parseUs = std::chrono::duration_cast<std::chrono::microseconds>(p1 - p0).count();
    bool passed = (res == SshResult::SUCCESS) && parsed && (rttMs <= 1500.0);

    ZyxelBenchmark::record({
        "LiveFirewallE8", "ReadSaCounter", ZyxelVpnCmd::cmdShowSaCounter(),
        rttMs, parseUs, out.size(), 1, passed,
        passed ? "PASS (SLA)" : "FAIL"
    });

    CHECK_TRUE(parsed);
    CHECK_TRUE(rttMs <= 1500.0);
}

TEST(LiveFirewallE8, ReadSaMonitor) {
    ZyxelDriver &driver = ZyxelDriver::getInstance();
    if (!driver.isConnected()) return;

    auto t0 = std::chrono::steady_clock::now();
    std::string out;
    SshResult res = driver.getSshClient().executeCommand(ZyxelVpnCmd::cmdShowSaMonitor(), out);
    auto t1 = std::chrono::steady_clock::now();

    CHECK_EQUAL(static_cast<int>(SshResult::SUCCESS), static_cast<int>(res));

    double rttMs = std::chrono::duration<double, std::milli>(t1 - t0).count();
    bool passed = (res == SshResult::SUCCESS) && (rttMs <= 1500.0);

    ZyxelBenchmark::record({
        "LiveFirewallE8", "ReadSaMonitor", ZyxelVpnCmd::cmdShowSaMonitor(),
        rttMs, 0, out.size(), 1, passed,
        passed ? "PASS (SLA)" : "FAIL"
    });

    CHECK_TRUE(rttMs <= 1500.0);
}

TEST(LiveFirewallE8, ReadVcpAllowedCryptoMap) {
    ZyxelDriver &driver = ZyxelDriver::getInstance();
    if (!driver.isConnected()) return;

    auto t0 = std::chrono::steady_clock::now();
    std::string out;
    SshResult res = driver.getSshClient().executeCommand(ZyxelVpnCmd::cmdShowVcpAllowedCryptoMap(), out);
    auto t1 = std::chrono::steady_clock::now();

    CHECK_EQUAL(static_cast<int>(SshResult::SUCCESS), static_cast<int>(res));

    double rttMs = std::chrono::duration<double, std::milli>(t1 - t0).count();
    bool passed = (res == SshResult::SUCCESS) && (rttMs <= 1500.0);

    ZyxelBenchmark::record({
        "LiveFirewallE8", "ReadVcpAllowedCryptoMap", ZyxelVpnCmd::cmdShowVcpAllowedCryptoMap(),
        rttMs, 0, out.size(), 1, passed,
        passed ? "PASS (SLA)" : "FAIL"
    });

    CHECK_TRUE(rttMs <= 1500.0);
}

TEST(LiveFirewallE8, ReadVcpAllowedCryptoMap6) {
    ZyxelDriver &driver = ZyxelDriver::getInstance();
    if (!driver.isConnected()) return;

    auto t0 = std::chrono::steady_clock::now();
    std::string out;
    SshResult res = driver.getSshClient().executeCommand(ZyxelVpnCmd::cmdShowVcpAllowedCryptoMap6(), out);
    auto t1 = std::chrono::steady_clock::now();

    CHECK_EQUAL(static_cast<int>(SshResult::SUCCESS), static_cast<int>(res));

    double rttMs = std::chrono::duration<double, std::milli>(t1 - t0).count();
    bool passed = (res == SshResult::SUCCESS) && (rttMs <= 1500.0);

    ZyxelBenchmark::record({
        "LiveFirewallE8", "ReadVcpAllowedCryptoMap6", ZyxelVpnCmd::cmdShowVcpAllowedCryptoMap6(),
        rttMs, 0, out.size(), 1, passed,
        passed ? "PASS (SLA)" : "FAIL"
    });

    CHECK_TRUE(rttMs <= 1500.0);
}

TEST(LiveFirewallE8, ReadVcpAllowedUsers) {
    ZyxelDriver &driver = ZyxelDriver::getInstance();
    if (!driver.isConnected()) return;

    auto t0 = std::chrono::steady_clock::now();
    std::string out;
    SshResult res = driver.getSshClient().executeCommand(ZyxelVpnCmd::cmdShowVcpAllowedUsers(), out);
    auto t1 = std::chrono::steady_clock::now();

    CHECK_EQUAL(static_cast<int>(SshResult::SUCCESS), static_cast<int>(res));

    auto p0 = std::chrono::steady_clock::now();
    std::vector<ZyxelVcpUser> users;
    bool parsed = ZyxelVpnCmd::parseVcpAllowedUsers(out, users);
    auto p1 = std::chrono::steady_clock::now();

    double rttMs = std::chrono::duration<double, std::milli>(t1 - t0).count();
    uint64_t parseUs = std::chrono::duration_cast<std::chrono::microseconds>(p1 - p0).count();
    bool passed = (res == SshResult::SUCCESS) && parsed && (rttMs <= 1500.0);

    ZyxelBenchmark::record({
        "LiveFirewallE8", "ReadVcpAllowedUsers", ZyxelVpnCmd::cmdShowVcpAllowedUsers(),
        rttMs, parseUs, out.size(), static_cast<uint32_t>(users.size()), passed,
        passed ? "PASS (SLA)" : "FAIL"
    });

    CHECK_TRUE(parsed);
    CHECK_TRUE(rttMs <= 1500.0);
}

TEST(LiveFirewallE8, ReadVpnConcentrator) {
    ZyxelDriver &driver = ZyxelDriver::getInstance();
    if (!driver.isConnected()) return;

    auto t0 = std::chrono::steady_clock::now();
    std::string out;
    SshResult res = driver.getSshClient().executeCommand(ZyxelVpnCmd::cmdShowVpnConcentrator(), out);
    auto t1 = std::chrono::steady_clock::now();

    CHECK_EQUAL(static_cast<int>(SshResult::SUCCESS), static_cast<int>(res));

    double rttMs = std::chrono::duration<double, std::milli>(t1 - t0).count();
    bool passed = (res == SshResult::SUCCESS) && (rttMs <= 1500.0);

    ZyxelBenchmark::record({
        "LiveFirewallE8", "ReadVpnConcentrator", ZyxelVpnCmd::cmdShowVpnConcentrator(),
        rttMs, 0, out.size(), 1, passed,
        passed ? "PASS (SLA)" : "FAIL"
    });

    CHECK_TRUE(rttMs <= 1500.0);
}

TEST(LiveFirewallE8, ReadVpnConcentrator6) {
    ZyxelDriver &driver = ZyxelDriver::getInstance();
    if (!driver.isConnected()) return;

    auto t0 = std::chrono::steady_clock::now();
    std::string out;
    SshResult res = driver.getSshClient().executeCommand(ZyxelVpnCmd::cmdShowVpnConcentrator6(), out);
    auto t1 = std::chrono::steady_clock::now();

    CHECK_EQUAL(static_cast<int>(SshResult::SUCCESS), static_cast<int>(res));

    double rttMs = std::chrono::duration<double, std::milli>(t1 - t0).count();
    bool passed = (res == SshResult::SUCCESS) && (rttMs <= 1500.0);

    ZyxelBenchmark::record({
        "LiveFirewallE8", "ReadVpnConcentrator6", ZyxelVpnCmd::cmdShowVpnConcentrator6(),
        rttMs, 0, out.size(), 1, passed,
        passed ? "PASS (SLA)" : "FAIL"
    });

    CHECK_TRUE(rttMs <= 1500.0);
}

TEST(LiveFirewallE8, ReadVpnConfigurationProvisionActivation) {
    ZyxelDriver &driver = ZyxelDriver::getInstance();
    if (!driver.isConnected()) return;

    auto t0 = std::chrono::steady_clock::now();
    std::string out;
    SshResult res = driver.getSshClient().executeCommand(ZyxelVpnCmd::cmdShowVpnConfigurationProvisionActivation(), out);
    auto t1 = std::chrono::steady_clock::now();

    CHECK_EQUAL(static_cast<int>(SshResult::SUCCESS), static_cast<int>(res));

    auto p0 = std::chrono::steady_clock::now();
    bool active = false;
    bool parsed = ZyxelVpnCmd::parseVpnConfigurationProvisionActivation(out, active);
    auto p1 = std::chrono::steady_clock::now();

    double rttMs = std::chrono::duration<double, std::milli>(t1 - t0).count();
    uint64_t parseUs = std::chrono::duration_cast<std::chrono::microseconds>(p1 - p0).count();
    bool passed = (res == SshResult::SUCCESS) && parsed && (rttMs <= 1500.0);

    ZyxelBenchmark::record({
        "LiveFirewallE8", "ReadVpnConfigurationProvisionActivation", ZyxelVpnCmd::cmdShowVpnConfigurationProvisionActivation(),
        rttMs, parseUs, out.size(), 1, passed,
        passed ? "PASS (SLA)" : "FAIL"
    });

    CHECK_TRUE(parsed);
    CHECK_TRUE(rttMs <= 1500.0);
}

TEST(LiveFirewallE8, ReadVpnConfigurationProvisionAuthentication) {
    ZyxelDriver &driver = ZyxelDriver::getInstance();
    if (!driver.isConnected()) return;

    auto t0 = std::chrono::steady_clock::now();
    std::string out;
    SshResult res = driver.getSshClient().executeCommand(ZyxelVpnCmd::cmdShowVpnConfigurationProvisionAuthentication(), out);
    auto t1 = std::chrono::steady_clock::now();

    CHECK_EQUAL(static_cast<int>(SshResult::SUCCESS), static_cast<int>(res));

    double rttMs = std::chrono::duration<double, std::milli>(t1 - t0).count();
    bool passed = (res == SshResult::SUCCESS) && (rttMs <= 1500.0);

    ZyxelBenchmark::record({
        "LiveFirewallE8", "ReadVpnConfigurationProvisionAuthentication", ZyxelVpnCmd::cmdShowVpnConfigurationProvisionAuthentication(),
        rttMs, 0, out.size(), 1, passed,
        passed ? "PASS (SLA)" : "FAIL"
    });

    CHECK_TRUE(rttMs <= 1500.0);
}

TEST(LiveFirewallE8, ReadVpnConfigurationProvisionIosfilter) {
    ZyxelDriver &driver = ZyxelDriver::getInstance();
    if (!driver.isConnected()) return;

    auto t0 = std::chrono::steady_clock::now();
    std::string out;
    SshResult res = driver.getSshClient().executeCommand(ZyxelVpnCmd::cmdShowVpnConfigurationProvisionIosfilter(), out);
    auto t1 = std::chrono::steady_clock::now();

    CHECK_EQUAL(static_cast<int>(SshResult::SUCCESS), static_cast<int>(res));

    double rttMs = std::chrono::duration<double, std::milli>(t1 - t0).count();
    bool passed = (res == SshResult::SUCCESS) && (rttMs <= 1500.0);

    ZyxelBenchmark::record({
        "LiveFirewallE8", "ReadVpnConfigurationProvisionIosfilter", ZyxelVpnCmd::cmdShowVpnConfigurationProvisionIosfilter(),
        rttMs, 0, out.size(), 1, passed,
        passed ? "PASS (SLA)" : "FAIL"
    });

    CHECK_TRUE(rttMs <= 1500.0);
}

TEST(LiveFirewallE8, ReadVpnConfigurationProvisionPort) {
    ZyxelDriver &driver = ZyxelDriver::getInstance();
    if (!driver.isConnected()) return;

    auto t0 = std::chrono::steady_clock::now();
    std::string out;
    SshResult res = driver.getSshClient().executeCommand(ZyxelVpnCmd::cmdShowVpnConfigurationProvisionPort(), out);
    auto t1 = std::chrono::steady_clock::now();

    CHECK_EQUAL(static_cast<int>(SshResult::SUCCESS), static_cast<int>(res));

    auto p0 = std::chrono::steady_clock::now();
    int port = 0;
    bool parsed = ZyxelVpnCmd::parseVpnConfigurationProvisionPort(out, port);
    auto p1 = std::chrono::steady_clock::now();

    double rttMs = std::chrono::duration<double, std::milli>(t1 - t0).count();
    uint64_t parseUs = std::chrono::duration_cast<std::chrono::microseconds>(p1 - p0).count();
    bool passed = (res == SshResult::SUCCESS) && parsed && (port == 443) && (rttMs <= 1500.0);

    ZyxelBenchmark::record({
        "LiveFirewallE8", "ReadVpnConfigurationProvisionPort", ZyxelVpnCmd::cmdShowVpnConfigurationProvisionPort(),
        rttMs, parseUs, out.size(), 1, passed,
        passed ? "PASS (SLA)" : "FAIL"
    });

    CHECK_TRUE(parsed);
    LONGS_EQUAL(443, port);
    CHECK_TRUE(rttMs <= 1500.0);
}

TEST(LiveFirewallE8, ReadVpnConfigurationProvisionRules) {
    ZyxelDriver &driver = ZyxelDriver::getInstance();
    if (!driver.isConnected()) return;

    auto t0 = std::chrono::steady_clock::now();
    std::string out;
    SshResult res = driver.getSshClient().executeCommand(ZyxelVpnCmd::cmdShowVpnConfigurationProvisionRules(), out);
    auto t1 = std::chrono::steady_clock::now();

    CHECK_EQUAL(static_cast<int>(SshResult::SUCCESS), static_cast<int>(res));

    double rttMs = std::chrono::duration<double, std::milli>(t1 - t0).count();
    bool passed = (res == SshResult::SUCCESS) && (rttMs <= 1500.0);

    ZyxelBenchmark::record({
        "LiveFirewallE8", "ReadVpnConfigurationProvisionRules", ZyxelVpnCmd::cmdShowVpnConfigurationProvisionRules(),
        rttMs, 0, out.size(), 1, passed,
        passed ? "PASS (SLA)" : "FAIL"
    });

    CHECK_TRUE(rttMs <= 1500.0);
}

TEST(LiveFirewallE8, ReadVpnCounters) {
    ZyxelDriver &driver = ZyxelDriver::getInstance();
    if (!driver.isConnected()) return;

    auto t0 = std::chrono::steady_clock::now();
    std::string out;
    SshResult res = driver.getSshClient().executeCommand(ZyxelVpnCmd::cmdShowVpnCounters(), out);
    auto t1 = std::chrono::steady_clock::now();

    CHECK_EQUAL(static_cast<int>(SshResult::SUCCESS), static_cast<int>(res));

    auto p0 = std::chrono::steady_clock::now();
    ZyxelVpnCounters counters;
    bool parsed = ZyxelVpnCmd::parseVpnCounters(out, counters);
    auto p1 = std::chrono::steady_clock::now();

    double rttMs = std::chrono::duration<double, std::milli>(t1 - t0).count();
    uint64_t parseUs = std::chrono::duration_cast<std::chrono::microseconds>(p1 - p0).count();
    bool passed = (res == SshResult::SUCCESS) && parsed && (rttMs <= 1500.0);

    ZyxelBenchmark::record({
        "LiveFirewallE8", "ReadVpnCounters", ZyxelVpnCmd::cmdShowVpnCounters(),
        rttMs, parseUs, out.size(), 1, passed,
        passed ? "PASS (SLA)" : "FAIL"
    });

    CHECK_TRUE(parsed);
    CHECK_TRUE(rttMs <= 1500.0);
}

TEST(LiveFirewallE8, ReadVpnServiceStatus) {
    ZyxelDriver &driver = ZyxelDriver::getInstance();
    if (!driver.isConnected()) return;

    auto t0 = std::chrono::steady_clock::now();
    std::string out;
    SshResult res = driver.getSshClient().executeCommand(ZyxelVpnCmd::cmdShowVpnServiceStatus(), out);
    auto t1 = std::chrono::steady_clock::now();

    CHECK_EQUAL(static_cast<int>(SshResult::SUCCESS), static_cast<int>(res));

    auto p0 = std::chrono::steady_clock::now();
    bool active = false;
    bool autoDisable = false;
    bool parsed = ZyxelVpnCmd::parseVpnServiceStatus(out, active, autoDisable);
    auto p1 = std::chrono::steady_clock::now();

    double rttMs = std::chrono::duration<double, std::milli>(t1 - t0).count();
    uint64_t parseUs = std::chrono::duration_cast<std::chrono::microseconds>(p1 - p0).count();
    bool passed = (res == SshResult::SUCCESS) && parsed && (rttMs <= 1500.0);

    ZyxelBenchmark::record({
        "LiveFirewallE8", "ReadVpnServiceStatus", ZyxelVpnCmd::cmdShowVpnServiceStatus(),
        rttMs, parseUs, out.size(), 1, passed,
        passed ? "PASS (SLA)" : "FAIL"
    });

    CHECK_TRUE(parsed);
    CHECK_TRUE(rttMs <= 1500.0);
}

TEST(LiveFirewallE8, ReadSslvpnLoginPort) {
    ZyxelDriver &driver = ZyxelDriver::getInstance();
    if (!driver.isConnected()) return;

    auto t0 = std::chrono::steady_clock::now();
    std::string out;
    SshResult res = driver.getSshClient().executeCommand(ZyxelVpnCmd::cmdShowSslvpnLoginPort(), out);
    auto t1 = std::chrono::steady_clock::now();

    CHECK_EQUAL(static_cast<int>(SshResult::SUCCESS), static_cast<int>(res));

    auto p0 = std::chrono::steady_clock::now();
    int port = 0;
    bool parsed = ZyxelVpnCmd::parseSslvpnLoginPort(out, port);
    auto p1 = std::chrono::steady_clock::now();

    double rttMs = std::chrono::duration<double, std::milli>(t1 - t0).count();
    uint64_t parseUs = std::chrono::duration_cast<std::chrono::microseconds>(p1 - p0).count();
    bool passed = (res == SshResult::SUCCESS) && parsed && (port == 443) && (rttMs <= 1500.0);

    ZyxelBenchmark::record({
        "LiveFirewallE8", "ReadSslvpnLoginPort", ZyxelVpnCmd::cmdShowSslvpnLoginPort(),
        rttMs, parseUs, out.size(), 1, passed,
        passed ? "PASS (SLA)" : "FAIL"
    });

    CHECK_TRUE(parsed);
    LONGS_EQUAL(443, port);
    CHECK_TRUE(rttMs <= 1500.0);
}

TEST(LiveFirewallE8, ReadSslvpnPolicy) {
    ZyxelDriver &driver = ZyxelDriver::getInstance();
    if (!driver.isConnected()) return;

    auto t0 = std::chrono::steady_clock::now();
    std::string out;
    SshResult res = driver.getSshClient().executeCommand(ZyxelVpnCmd::cmdShowSslvpnPolicy(), out);
    auto t1 = std::chrono::steady_clock::now();

    CHECK_EQUAL(static_cast<int>(SshResult::SUCCESS), static_cast<int>(res));

    double rttMs = std::chrono::duration<double, std::milli>(t1 - t0).count();
    bool passed = (res == SshResult::SUCCESS) && (rttMs <= 1500.0);

    ZyxelBenchmark::record({
        "LiveFirewallE8", "ReadSslvpnPolicy", ZyxelVpnCmd::cmdShowSslvpnPolicy(),
        rttMs, 0, out.size(), 1, passed,
        passed ? "PASS (SLA)" : "FAIL"
    });

    CHECK_TRUE(rttMs <= 1500.0);
}

TEST(LiveFirewallE8, ReadSslvpnApplication) {
    ZyxelDriver &driver = ZyxelDriver::getInstance();
    if (!driver.isConnected()) return;

    auto t0 = std::chrono::steady_clock::now();
    std::string out;
    SshResult res = driver.getSshClient().executeCommand(ZyxelVpnCmd::cmdShowSslvpnApplication(), out);
    auto t1 = std::chrono::steady_clock::now();

    CHECK_EQUAL(static_cast<int>(SshResult::SUCCESS), static_cast<int>(res));

    double rttMs = std::chrono::duration<double, std::milli>(t1 - t0).count();
    bool passed = (res == SshResult::SUCCESS) && (rttMs <= 1500.0);

    ZyxelBenchmark::record({
        "LiveFirewallE8", "ReadSslvpnApplication", ZyxelVpnCmd::cmdShowSslvpnApplication(),
        rttMs, 0, out.size(), 1, passed,
        passed ? "PASS (SLA)" : "FAIL"
    });

    CHECK_TRUE(rttMs <= 1500.0);
}

TEST(LiveFirewallE8, ReadSslvpnMonitor) {
    ZyxelDriver &driver = ZyxelDriver::getInstance();
    if (!driver.isConnected()) return;

    auto t0 = std::chrono::steady_clock::now();
    std::string out;
    SshResult res = driver.getSshClient().executeCommand(ZyxelVpnCmd::cmdShowSslvpnMonitor(), out);
    auto t1 = std::chrono::steady_clock::now();

    CHECK_EQUAL(static_cast<int>(SshResult::SUCCESS), static_cast<int>(res));

    double rttMs = std::chrono::duration<double, std::milli>(t1 - t0).count();
    bool passed = (res == SshResult::SUCCESS) && (rttMs <= 1500.0);

    ZyxelBenchmark::record({
        "LiveFirewallE8", "ReadSslvpnMonitor", ZyxelVpnCmd::cmdShowSslvpnMonitor(),
        rttMs, 0, out.size(), 1, passed,
        passed ? "PASS (SLA)" : "FAIL"
    });

    CHECK_TRUE(rttMs <= 1500.0);
}

TEST(LiveFirewallE8, ReadWorkspaceApplication) {
    ZyxelDriver &driver = ZyxelDriver::getInstance();
    if (!driver.isConnected()) return;

    auto t0 = std::chrono::steady_clock::now();
    std::string out;
    SshResult res = driver.getSshClient().executeCommand(ZyxelVpnCmd::cmdShowWorkspaceApplication(), out);
    auto t1 = std::chrono::steady_clock::now();

    CHECK_EQUAL(static_cast<int>(SshResult::SUCCESS), static_cast<int>(res));

    double rttMs = std::chrono::duration<double, std::milli>(t1 - t0).count();
    bool passed = (res == SshResult::SUCCESS) && (rttMs <= 1500.0);

    ZyxelBenchmark::record({
        "LiveFirewallE8", "ReadWorkspaceApplication", ZyxelVpnCmd::cmdShowWorkspaceApplication(),
        rttMs, 0, out.size(), 1, passed,
        passed ? "PASS (SLA)" : "FAIL"
    });

    CHECK_TRUE(rttMs <= 1500.0);
}

TEST(LiveFirewallE8, ReadWorkspaceCifs) {
    ZyxelDriver &driver = ZyxelDriver::getInstance();
    if (!driver.isConnected()) return;

    auto t0 = std::chrono::steady_clock::now();
    std::string out;
    SshResult res = driver.getSshClient().executeCommand(ZyxelVpnCmd::cmdShowWorkspaceCifs(), out);
    auto t1 = std::chrono::steady_clock::now();

    CHECK_EQUAL(static_cast<int>(SshResult::SUCCESS), static_cast<int>(res));

    double rttMs = std::chrono::duration<double, std::milli>(t1 - t0).count();
    bool passed = (res == SshResult::SUCCESS) && (rttMs <= 1500.0);

    ZyxelBenchmark::record({
        "LiveFirewallE8", "ReadWorkspaceCifs", ZyxelVpnCmd::cmdShowWorkspaceCifs(),
        rttMs, 0, out.size(), 1, passed,
        passed ? "PASS (SLA)" : "FAIL"
    });

    CHECK_TRUE(rttMs <= 1500.0);
}

TEST(LiveFirewallE8, ReadAccountL2tp) {
    ZyxelDriver &driver = ZyxelDriver::getInstance();
    if (!driver.isConnected()) return;

    auto t0 = std::chrono::steady_clock::now();
    std::string out;
    SshResult res = driver.getSshClient().executeCommand(ZyxelVpnCmd::cmdShowAccountL2tp(), out);
    auto t1 = std::chrono::steady_clock::now();

    CHECK_EQUAL(static_cast<int>(SshResult::SUCCESS), static_cast<int>(res));

    double rttMs = std::chrono::duration<double, std::milli>(t1 - t0).count();
    bool passed = (res == SshResult::SUCCESS) && (rttMs <= 1500.0);

    ZyxelBenchmark::record({
        "LiveFirewallE8", "ReadAccountL2tp", ZyxelVpnCmd::cmdShowAccountL2tp(),
        rttMs, 0, out.size(), 1, passed,
        passed ? "PASS (SLA)" : "FAIL"
    });

    CHECK_TRUE(rttMs <= 1500.0);
}

TEST(LiveFirewallE8, ReadInterfacePpp) {
    ZyxelDriver &driver = ZyxelDriver::getInstance();
    if (!driver.isConnected()) return;

    auto t0 = std::chrono::steady_clock::now();
    std::string out;
    SshResult res = driver.getSshClient().executeCommand(ZyxelVpnCmd::cmdShowInterfacePpp(), out);
    auto t1 = std::chrono::steady_clock::now();

    CHECK_EQUAL(static_cast<int>(SshResult::SUCCESS), static_cast<int>(res));

    double rttMs = std::chrono::duration<double, std::milli>(t1 - t0).count();
    bool passed = (res == SshResult::SUCCESS) && (rttMs <= 1500.0);

    ZyxelBenchmark::record({
        "LiveFirewallE8", "ReadInterfacePpp", ZyxelVpnCmd::cmdShowInterfacePpp(),
        rttMs, 0, out.size(), 1, passed,
        passed ? "PASS (SLA)" : "FAIL"
    });

    CHECK_TRUE(rttMs <= 1500.0);
}

TEST(LiveFirewallE8, ReadL2tpOverIpsec) {
    ZyxelDriver &driver = ZyxelDriver::getInstance();
    if (!driver.isConnected()) return;

    auto t0 = std::chrono::steady_clock::now();
    std::string out;
    SshResult res = driver.getSshClient().executeCommand(ZyxelVpnCmd::cmdShowL2tpOverIpsec(), out);
    auto t1 = std::chrono::steady_clock::now();

    CHECK_EQUAL(static_cast<int>(SshResult::SUCCESS), static_cast<int>(res));

    auto p0 = std::chrono::steady_clock::now();
    ZyxelL2tpStatus l2tp;
    bool parsed = ZyxelVpnCmd::parseL2tpOverIpsec(out, l2tp);
    auto p1 = std::chrono::steady_clock::now();

    double rttMs = std::chrono::duration<double, std::milli>(t1 - t0).count();
    uint64_t parseUs = std::chrono::duration_cast<std::chrono::microseconds>(p1 - p0).count();
    bool passed = (res == SshResult::SUCCESS) && parsed && (rttMs <= 1500.0);

    ZyxelBenchmark::record({
        "LiveFirewallE8", "ReadL2tpOverIpsec", ZyxelVpnCmd::cmdShowL2tpOverIpsec(),
        rttMs, parseUs, out.size(), 1, passed,
        passed ? "PASS (SLA)" : "FAIL"
    });

    CHECK_TRUE(parsed);
    CHECK_TRUE(rttMs <= 1500.0);
}

TEST(LiveFirewallE8, ReadL2tpOverIpsecSession) {
    ZyxelDriver &driver = ZyxelDriver::getInstance();
    if (!driver.isConnected()) return;

    auto t0 = std::chrono::steady_clock::now();
    std::string out;
    SshResult res = driver.getSshClient().executeCommand(ZyxelVpnCmd::cmdShowL2tpOverIpsecSession(), out);
    auto t1 = std::chrono::steady_clock::now();

    CHECK_EQUAL(static_cast<int>(SshResult::SUCCESS), static_cast<int>(res));

    double rttMs = std::chrono::duration<double, std::milli>(t1 - t0).count();
    bool passed = (res == SshResult::SUCCESS) && (rttMs <= 1500.0);

    ZyxelBenchmark::record({
        "LiveFirewallE8", "ReadL2tpOverIpsecSession", ZyxelVpnCmd::cmdShowL2tpOverIpsecSession(),
        rttMs, 0, out.size(), 1, passed,
        passed ? "PASS (SLA)" : "FAIL"
    });

    CHECK_TRUE(rttMs <= 1500.0);
}

TEST(LiveFirewallE8, DryFireInvalidIpsecCommand) {
    ZyxelDriver &driver = ZyxelDriver::getInstance();
    if (!driver.isConnected()) return;

    auto t0 = std::chrono::steady_clock::now();
    std::string out;
    SshResult res = driver.getSshClient().executeCommand(ZyxelVpnCmd::cmdInvalidIpsecDryFire(), out);
    auto t1 = std::chrono::steady_clock::now();

    bool rejected = (res == SshResult::ERR_SYNTAX || out.find("Parse error") != std::string::npos);
    double rttMs = std::chrono::duration<double, std::milli>(t1 - t0).count();
    bool passed = rejected && rttMs <= 2500.0;

    ZyxelBenchmark::record({
        "LiveFirewallE8", "DryFireInvalidIpsecCommand", ZyxelVpnCmd::cmdInvalidIpsecDryFire(),
        rttMs, 0, out.size(), rejected ? 1U : 0U, passed,
        passed ? "PASS (REJECT)" : "FAIL"
    });

    CHECK_TRUE(rejected);
    CHECK_TRUE(rttMs <= 2500.0);
}

TEST(LiveFirewallE8, DryFireInvalidSslvpnCommand) {
    ZyxelDriver &driver = ZyxelDriver::getInstance();
    if (!driver.isConnected()) return;

    auto t0 = std::chrono::steady_clock::now();
    std::string out;
    SshResult res = driver.getSshClient().executeCommand(ZyxelVpnCmd::cmdInvalidSslvpnDryFire(), out);
    auto t1 = std::chrono::steady_clock::now();

    bool rejected = (res == SshResult::ERR_SYNTAX || out.find("Parse error") != std::string::npos);
    double rttMs = std::chrono::duration<double, std::milli>(t1 - t0).count();
    bool passed = rejected && rttMs <= 2500.0;

    ZyxelBenchmark::record({
        "LiveFirewallE8", "DryFireInvalidSslvpnCommand", ZyxelVpnCmd::cmdInvalidSslvpnDryFire(),
        rttMs, 0, out.size(), rejected ? 1U : 0U, passed,
        passed ? "PASS (REJECT)" : "FAIL"
    });

    CHECK_TRUE(rejected);
    CHECK_TRUE(rttMs <= 2500.0);
}

TEST(LiveFirewallE8, WriteIsakmpPolicyRollback) {
    ZyxelDriver &driver = ZyxelDriver::getInstance();
    if (!driver.isConnected()) return;
    ZyxelSshClient &ssh = driver.getSshClient();

    auto t0 = std::chrono::steady_clock::now();
    std::string out;

    // 1. Create IKE policy
    ssh.executeCommand("configure terminal", out);
    ssh.executeCommand(ZyxelVpnCmd::cmdIsakmpPolicy("NETMON_QA_IKE_E8"), out);
    ssh.executeCommand("exit", out);
    ssh.executeCommand("exit", out);
    ssh.unwindToRootPrompt();

    // 2. Verify creation
    ssh.executeCommand(ZyxelVpnCmd::cmdShowIsakmpPolicy(), out);
    bool created = (out.find("NETMON_QA_IKE_E8") != std::string::npos);

    // 3. Rollback
    ssh.executeCommand("configure terminal", out);
    SshResult resDel = ssh.executeCommand(ZyxelVpnCmd::cmdNoIsakmpPolicy("NETMON_QA_IKE_E8"), out);
    ssh.executeCommand("exit", out);
    ssh.unwindToRootPrompt();

    CHECK_EQUAL(static_cast<int>(SshResult::SUCCESS), static_cast<int>(resDel));

    // 4. Verify clean deletion
    ssh.executeCommand(ZyxelVpnCmd::cmdShowIsakmpPolicy(), out);
    bool deleted = (out.find("NETMON_QA_IKE_E8") == std::string::npos);
    auto t1 = std::chrono::steady_clock::now();

    double rttMs = std::chrono::duration<double, std::milli>(t1 - t0).count();
    bool passed = created && deleted && (rttMs <= 6000.0);

    ZyxelBenchmark::record({
        "LiveFirewallE8", "WriteIsakmpPolicyRollback", "isakmp policy NETMON_QA_IKE_E8",
        rttMs, 0, out.size(), 1, passed,
        passed ? "PASS (ROLLBACK)" : "FAIL"
    });

    CHECK_TRUE(created);
    CHECK_TRUE(deleted);
    CHECK_TRUE(rttMs <= 6000.0);
}

TEST(LiveFirewallE8, WriteSslvpnApplicationRollback) {
    ZyxelDriver &driver = ZyxelDriver::getInstance();
    if (!driver.isConnected()) return;
    ZyxelSshClient &ssh = driver.getSshClient();

    auto t0 = std::chrono::steady_clock::now();
    std::string out;

    // 1. Create SSL VPN application
    ssh.executeCommand("configure terminal", out);
    ssh.executeCommand(ZyxelVpnCmd::cmdSslvpnApplication("NETMON_QA_SSLVPN_E8"), out);
    ssh.executeCommand("exit", out);
    ssh.executeCommand("exit", out);
    ssh.unwindToRootPrompt();

    // 2. Verify creation
    ssh.executeCommand(ZyxelVpnCmd::cmdShowSslvpnApplication(), out);
    bool created = (out.find("NETMON_QA_SSLVPN_E8") != std::string::npos);

    // 3. Rollback
    ssh.executeCommand("configure terminal", out);
    SshResult resDel = ssh.executeCommand(ZyxelVpnCmd::cmdNoSslvpnApplication("NETMON_QA_SSLVPN_E8"), out);
    ssh.executeCommand("exit", out);
    ssh.unwindToRootPrompt();

    CHECK_EQUAL(static_cast<int>(SshResult::SUCCESS), static_cast<int>(resDel));

    // 4. Verify clean deletion
    ssh.executeCommand(ZyxelVpnCmd::cmdShowSslvpnApplication(), out);
    bool deleted = (out.find("NETMON_QA_SSLVPN_E8") == std::string::npos);
    auto t1 = std::chrono::steady_clock::now();

    double rttMs = std::chrono::duration<double, std::milli>(t1 - t0).count();
    bool passed = created && deleted && (rttMs <= 6000.0);

    ZyxelBenchmark::record({
        "LiveFirewallE8", "WriteSslvpnApplicationRollback", "sslvpn application NETMON_QA_SSLVPN_E8",
        rttMs, 0, out.size(), 1, passed,
        passed ? "PASS (ROLLBACK)" : "FAIL"
    });

    CHECK_TRUE(created);
    CHECK_TRUE(deleted);
    CHECK_TRUE(rttMs <= 6000.0);
}

// ============================================================================
// CppUTest Diagnostic Test Group: LiveFirewallE9 (Envelope 9: Auth & Certs)
// ============================================================================

TEST_GROUP(LiveFirewallE9) {
    void setup() override {
        ZyxelDriver &driver = ZyxelDriver::getInstance();
        if (driver.isConnected()) {
            ZyxelSshClient &ssh = driver.getSshClient();
            ssh.unwindToRootPrompt();
            std::string out;
            ssh.executeCommand("configure terminal", out);
            ssh.executeCommand("no groupname NETMON_QA_GRP_E9", out);
            ssh.executeCommand("no aaa group server radius NETMON_QA_AAA_E9", out);
            ssh.executeCommand("exit", out);
            ssh.unwindToRootPrompt();
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(25));
    }

    void teardown() override {
        ZyxelDriver &driver = ZyxelDriver::getInstance();
        if (driver.isConnected()) {
            ZyxelSshClient &ssh = driver.getSshClient();
            ssh.unwindToRootPrompt();
            std::string out;
            ssh.executeCommand("configure terminal", out);
            ssh.executeCommand("no groupname NETMON_QA_GRP_E9", out);
            ssh.executeCommand("no aaa group server radius NETMON_QA_AAA_E9", out);
            ssh.executeCommand("exit", out);
            ssh.unwindToRootPrompt();
        }
    }
};

TEST(LiveFirewallE9, ReadCnmAgentConfiguration) {
    ZyxelDriver &driver = ZyxelDriver::getInstance();
    if (!driver.isConnected()) return;

    auto t0 = std::chrono::steady_clock::now();
    std::string out;
    SshResult res = driver.getSshClient().executeCommand(ZyxelAuthCmd::cmdShowCnmAgentConfiguration(), out);
    auto t1 = std::chrono::steady_clock::now();

    CHECK_EQUAL(static_cast<int>(SshResult::SUCCESS), static_cast<int>(res));

    auto p0 = std::chrono::steady_clock::now();
    bool active = false;
    bool parsed = ZyxelAuthCmd::parseCnmConfiguration(out, active);
    auto p1 = std::chrono::steady_clock::now();

    double rttMs = std::chrono::duration<double, std::milli>(t1 - t0).count();
    uint64_t parseUs = std::chrono::duration_cast<std::chrono::microseconds>(p1 - p0).count();
    bool passed = (res == SshResult::SUCCESS) && parsed && (rttMs <= 1500.0);

    ZyxelBenchmark::record({
        "LiveFirewallE9", "ReadCnmAgentConfiguration", ZyxelAuthCmd::cmdShowCnmAgentConfiguration(),
        rttMs, parseUs, out.size(), 1, passed,
        passed ? "PASS (SLA)" : "FAIL"
    });

    CHECK_TRUE(parsed);
    CHECK_TRUE(rttMs <= 1500.0);
}

TEST(LiveFirewallE9, ReadMonitorMode) {
    ZyxelDriver &driver = ZyxelDriver::getInstance();
    if (!driver.isConnected()) return;

    auto t0 = std::chrono::steady_clock::now();
    std::string out;
    SshResult res = driver.getSshClient().executeCommand(ZyxelAuthCmd::cmdShowMonitorMode(), out);
    auto t1 = std::chrono::steady_clock::now();

    CHECK_EQUAL(static_cast<int>(SshResult::SUCCESS), static_cast<int>(res));

    auto p0 = std::chrono::steady_clock::now();
    ZyxelCnmStatus st;
    bool parsed = ZyxelAuthCmd::parseMonitorMode(out, st);
    auto p1 = std::chrono::steady_clock::now();

    double rttMs = std::chrono::duration<double, std::milli>(t1 - t0).count();
    uint64_t parseUs = std::chrono::duration_cast<std::chrono::microseconds>(p1 - p0).count();
    bool passed = (res == SshResult::SUCCESS) && parsed && (rttMs <= 1500.0);

    ZyxelBenchmark::record({
        "LiveFirewallE9", "ReadMonitorMode", ZyxelAuthCmd::cmdShowMonitorMode(),
        rttMs, parseUs, out.size(), 1, passed,
        passed ? "PASS (SLA)" : "FAIL"
    });

    CHECK_TRUE(parsed);
    CHECK_TRUE(rttMs <= 1500.0);
}

TEST(LiveFirewallE9, ReadSecuReporterStatus) {
    ZyxelDriver &driver = ZyxelDriver::getInstance();
    if (!driver.isConnected()) return;

    auto t0 = std::chrono::steady_clock::now();
    std::string out;
    SshResult res = driver.getSshClient().executeCommand(ZyxelAuthCmd::cmdShowSecuReporterStatus(), out);
    auto t1 = std::chrono::steady_clock::now();

    CHECK_EQUAL(static_cast<int>(SshResult::SUCCESS), static_cast<int>(res));

    auto p0 = std::chrono::steady_clock::now();
    ZyxelSecuReporterStatus st;
    bool parsed = ZyxelAuthCmd::parseSecuReporterStatus(out, st);
    auto p1 = std::chrono::steady_clock::now();

    double rttMs = std::chrono::duration<double, std::milli>(t1 - t0).count();
    uint64_t parseUs = std::chrono::duration_cast<std::chrono::microseconds>(p1 - p0).count();
    bool passed = (res == SshResult::SUCCESS) && parsed && (rttMs <= 1500.0);

    ZyxelBenchmark::record({
        "LiveFirewallE9", "ReadSecuReporterStatus", ZyxelAuthCmd::cmdShowSecuReporterStatus(),
        rttMs, parseUs, out.size(), 1, passed,
        passed ? "PASS (SLA)" : "FAIL"
    });

    CHECK_TRUE(parsed);
    CHECK_TRUE(rttMs <= 1500.0);
}

TEST(LiveFirewallE9, ReadSecumanagerStatus) {
    ZyxelDriver &driver = ZyxelDriver::getInstance();
    if (!driver.isConnected()) return;

    auto t0 = std::chrono::steady_clock::now();
    std::string out;
    SshResult res = driver.getSshClient().executeCommand(ZyxelAuthCmd::cmdShowSecumanagerStatus(), out);
    auto t1 = std::chrono::steady_clock::now();

    CHECK_EQUAL(static_cast<int>(SshResult::SUCCESS), static_cast<int>(res));

    double rttMs = std::chrono::duration<double, std::milli>(t1 - t0).count();
    bool passed = (res == SshResult::SUCCESS) && (rttMs <= 1500.0);

    ZyxelBenchmark::record({
        "LiveFirewallE9", "ReadSecumanagerStatus", ZyxelAuthCmd::cmdShowSecumanagerStatus(),
        rttMs, 0, out.size(), 1, passed,
        passed ? "PASS (SLA)" : "FAIL"
    });

    CHECK_TRUE(rttMs <= 1500.0);
}

TEST(LiveFirewallE9, ReadWebAuthActivation) {
    ZyxelDriver &driver = ZyxelDriver::getInstance();
    if (!driver.isConnected()) return;

    auto t0 = std::chrono::steady_clock::now();
    std::string out;
    SshResult res = driver.getSshClient().executeCommand(ZyxelAuthCmd::cmdShowWebAuthActivation(), out);
    auto t1 = std::chrono::steady_clock::now();

    CHECK_EQUAL(static_cast<int>(SshResult::SUCCESS), static_cast<int>(res));

    auto p0 = std::chrono::steady_clock::now();
    bool active = true;
    bool parsed = ZyxelAuthCmd::parseWebAuthActivation(out, active);
    auto p1 = std::chrono::steady_clock::now();

    double rttMs = std::chrono::duration<double, std::milli>(t1 - t0).count();
    uint64_t parseUs = std::chrono::duration_cast<std::chrono::microseconds>(p1 - p0).count();
    bool passed = (res == SshResult::SUCCESS) && parsed && (rttMs <= 1500.0);

    ZyxelBenchmark::record({
        "LiveFirewallE9", "ReadWebAuthActivation", ZyxelAuthCmd::cmdShowWebAuthActivation(),
        rttMs, parseUs, out.size(), 1, passed,
        passed ? "PASS (SLA)" : "FAIL"
    });

    CHECK_TRUE(parsed);
    CHECK_TRUE(rttMs <= 1500.0);
}

TEST(LiveFirewallE9, ReadWebAuthDefaultRule) {
    ZyxelDriver &driver = ZyxelDriver::getInstance();
    if (!driver.isConnected()) return;

    auto t0 = std::chrono::steady_clock::now();
    std::string out;
    SshResult res = driver.getSshClient().executeCommand(ZyxelAuthCmd::cmdShowWebAuthDefaultRule(), out);
    auto t1 = std::chrono::steady_clock::now();

    CHECK_EQUAL(static_cast<int>(SshResult::SUCCESS), static_cast<int>(res));

    double rttMs = std::chrono::duration<double, std::milli>(t1 - t0).count();
    bool passed = (res == SshResult::SUCCESS) && (rttMs <= 1500.0);

    ZyxelBenchmark::record({
        "LiveFirewallE9", "ReadWebAuthDefaultRule", ZyxelAuthCmd::cmdShowWebAuthDefaultRule(),
        rttMs, 0, out.size(), 1, passed,
        passed ? "PASS (SLA)" : "FAIL"
    });

    CHECK_TRUE(rttMs <= 1500.0);
}

TEST(LiveFirewallE9, ReadWebAuthExceptionalService) {
    ZyxelDriver &driver = ZyxelDriver::getInstance();
    if (!driver.isConnected()) return;

    auto t0 = std::chrono::steady_clock::now();
    std::string out;
    SshResult res = driver.getSshClient().executeCommand(ZyxelAuthCmd::cmdShowWebAuthExceptionalService(), out);
    auto t1 = std::chrono::steady_clock::now();

    CHECK_EQUAL(static_cast<int>(SshResult::SUCCESS), static_cast<int>(res));

    double rttMs = std::chrono::duration<double, std::milli>(t1 - t0).count();
    bool passed = (res == SshResult::SUCCESS) && (rttMs <= 1500.0);

    ZyxelBenchmark::record({
        "LiveFirewallE9", "ReadWebAuthExceptionalService", ZyxelAuthCmd::cmdShowWebAuthExceptionalService(),
        rttMs, 0, out.size(), 1, passed,
        passed ? "PASS (SLA)" : "FAIL"
    });

    CHECK_TRUE(rttMs <= 1500.0);
}

TEST(LiveFirewallE9, ReadWebAuthMethod) {
    ZyxelDriver &driver = ZyxelDriver::getInstance();
    if (!driver.isConnected()) return;

    auto t0 = std::chrono::steady_clock::now();
    std::string out;
    SshResult res = driver.getSshClient().executeCommand(ZyxelAuthCmd::cmdShowWebAuthMethod(), out);
    auto t1 = std::chrono::steady_clock::now();

    CHECK_EQUAL(static_cast<int>(SshResult::SUCCESS), static_cast<int>(res));

    double rttMs = std::chrono::duration<double, std::milli>(t1 - t0).count();
    bool passed = (res == SshResult::SUCCESS) && (rttMs <= 1500.0);

    ZyxelBenchmark::record({
        "LiveFirewallE9", "ReadWebAuthMethod", ZyxelAuthCmd::cmdShowWebAuthMethod(),
        rttMs, 0, out.size(), 1, passed,
        passed ? "PASS (SLA)" : "FAIL"
    });

    CHECK_TRUE(rttMs <= 1500.0);
}

TEST(LiveFirewallE9, ReadWebAuthPolicyAll) {
    ZyxelDriver &driver = ZyxelDriver::getInstance();
    if (!driver.isConnected()) return;

    auto t0 = std::chrono::steady_clock::now();
    std::string out;
    SshResult res = driver.getSshClient().executeCommand(ZyxelAuthCmd::cmdShowWebAuthPolicy(), out);
    auto t1 = std::chrono::steady_clock::now();

    CHECK_EQUAL(static_cast<int>(SshResult::SUCCESS), static_cast<int>(res));

    double rttMs = std::chrono::duration<double, std::milli>(t1 - t0).count();
    bool passed = (res == SshResult::SUCCESS) && (rttMs <= 1500.0);

    ZyxelBenchmark::record({
        "LiveFirewallE9", "ReadWebAuthPolicyAll", ZyxelAuthCmd::cmdShowWebAuthPolicy(),
        rttMs, 0, out.size(), 1, passed,
        passed ? "PASS (SLA)" : "FAIL"
    });

    CHECK_TRUE(rttMs <= 1500.0);
}

TEST(LiveFirewallE9, ReadWebAuthPortalStatus) {
    ZyxelDriver &driver = ZyxelDriver::getInstance();
    if (!driver.isConnected()) return;

    auto t0 = std::chrono::steady_clock::now();
    std::string out;
    SshResult res = driver.getSshClient().executeCommand(ZyxelAuthCmd::cmdShowWebAuthPortalStatus(), out);
    auto t1 = std::chrono::steady_clock::now();

    CHECK_EQUAL(static_cast<int>(SshResult::SUCCESS), static_cast<int>(res));

    auto p0 = std::chrono::steady_clock::now();
    std::string logoutIp;
    bool sessionPage = false;
    bool parsed = ZyxelAuthCmd::parseWebAuthPortalStatus(out, logoutIp, sessionPage);
    auto p1 = std::chrono::steady_clock::now();

    double rttMs = std::chrono::duration<double, std::milli>(t1 - t0).count();
    uint64_t parseUs = std::chrono::duration_cast<std::chrono::microseconds>(p1 - p0).count();
    bool passed = (res == SshResult::SUCCESS) && parsed && (logoutIp == "6.6.6.6") && (rttMs <= 1500.0);

    ZyxelBenchmark::record({
        "LiveFirewallE9", "ReadWebAuthPortalStatus", ZyxelAuthCmd::cmdShowWebAuthPortalStatus(),
        rttMs, parseUs, out.size(), 1, passed,
        passed ? "PASS (SLA)" : "FAIL"
    });

    CHECK_TRUE(parsed);
    STRCMP_EQUAL("6.6.6.6", logoutIp.c_str());
    CHECK_TRUE(rttMs <= 1500.0);
}

TEST(LiveFirewallE9, ReadWebAuthRedirectFqdn) {
    ZyxelDriver &driver = ZyxelDriver::getInstance();
    if (!driver.isConnected()) return;

    auto t0 = std::chrono::steady_clock::now();
    std::string out;
    SshResult res = driver.getSshClient().executeCommand(ZyxelAuthCmd::cmdShowWebAuthRedirectFqdn(), out);
    auto t1 = std::chrono::steady_clock::now();

    CHECK_EQUAL(static_cast<int>(SshResult::SUCCESS), static_cast<int>(res));

    double rttMs = std::chrono::duration<double, std::milli>(t1 - t0).count();
    bool passed = (res == SshResult::SUCCESS) && (rttMs <= 1500.0);

    ZyxelBenchmark::record({
        "LiveFirewallE9", "ReadWebAuthRedirectFqdn", ZyxelAuthCmd::cmdShowWebAuthRedirectFqdn(),
        rttMs, 0, out.size(), 1, passed,
        passed ? "PASS (SLA)" : "FAIL"
    });

    CHECK_TRUE(rttMs <= 1500.0);
}

TEST(LiveFirewallE9, ReadWebAuthStatus) {
    ZyxelDriver &driver = ZyxelDriver::getInstance();
    if (!driver.isConnected()) return;

    auto t0 = std::chrono::steady_clock::now();
    std::string out;
    SshResult res = driver.getSshClient().executeCommand(ZyxelAuthCmd::cmdShowWebAuthStatus(), out);
    auto t1 = std::chrono::steady_clock::now();

    CHECK_EQUAL(static_cast<int>(SshResult::SUCCESS), static_cast<int>(res));

    double rttMs = std::chrono::duration<double, std::milli>(t1 - t0).count();
    bool passed = (res == SshResult::SUCCESS) && (rttMs <= 1500.0);

    ZyxelBenchmark::record({
        "LiveFirewallE9", "ReadWebAuthStatus", ZyxelAuthCmd::cmdShowWebAuthStatus(),
        rttMs, 0, out.size(), 1, passed,
        passed ? "PASS (SLA)" : "FAIL"
    });

    CHECK_TRUE(rttMs <= 1500.0);
}

TEST(LiveFirewallE9, ReadSsoAgent) {
    ZyxelDriver &driver = ZyxelDriver::getInstance();
    if (!driver.isConnected()) return;

    auto t0 = std::chrono::steady_clock::now();
    std::string out;
    SshResult res = driver.getSshClient().executeCommand(ZyxelAuthCmd::cmdShowSsoAgent(), out);
    auto t1 = std::chrono::steady_clock::now();

    CHECK_EQUAL(static_cast<int>(SshResult::SUCCESS), static_cast<int>(res));

    double rttMs = std::chrono::duration<double, std::milli>(t1 - t0).count();
    bool passed = (res == SshResult::SUCCESS) && (rttMs <= 1500.0);

    ZyxelBenchmark::record({
        "LiveFirewallE9", "ReadSsoAgent", ZyxelAuthCmd::cmdShowSsoAgent(),
        rttMs, 0, out.size(), 1, passed,
        passed ? "PASS (SLA)" : "FAIL"
    });

    CHECK_TRUE(rttMs <= 1500.0);
}

TEST(LiveFirewallE9, ReadSsoPort) {
    ZyxelDriver &driver = ZyxelDriver::getInstance();
    if (!driver.isConnected()) return;

    auto t0 = std::chrono::steady_clock::now();
    std::string out;
    SshResult res = driver.getSshClient().executeCommand(ZyxelAuthCmd::cmdShowSsoPort(), out);
    auto t1 = std::chrono::steady_clock::now();

    CHECK_EQUAL(static_cast<int>(SshResult::SUCCESS), static_cast<int>(res));

    auto p0 = std::chrono::steady_clock::now();
    int port = 0;
    bool parsed = ZyxelAuthCmd::parseSsoPort(out, port);
    auto p1 = std::chrono::steady_clock::now();

    double rttMs = std::chrono::duration<double, std::milli>(t1 - t0).count();
    uint64_t parseUs = std::chrono::duration_cast<std::chrono::microseconds>(p1 - p0).count();
    bool passed = (res == SshResult::SUCCESS) && parsed && (port == 2158) && (rttMs <= 1500.0);

    ZyxelBenchmark::record({
        "LiveFirewallE9", "ReadSsoPort", ZyxelAuthCmd::cmdShowSsoPort(),
        rttMs, parseUs, out.size(), 1, passed,
        passed ? "PASS (SLA)" : "FAIL"
    });

    CHECK_TRUE(parsed);
    LONGS_EQUAL(2158, port);
    CHECK_TRUE(rttMs <= 1500.0);
}

TEST(LiveFirewallE9, ReadAdvertisementActivation) {
    ZyxelDriver &driver = ZyxelDriver::getInstance();
    if (!driver.isConnected()) return;

    auto t0 = std::chrono::steady_clock::now();
    std::string out;
    SshResult res = driver.getSshClient().executeCommand(ZyxelAuthCmd::cmdShowAdvertisementActivation(), out);
    auto t1 = std::chrono::steady_clock::now();

    CHECK_EQUAL(static_cast<int>(SshResult::SUCCESS), static_cast<int>(res));

    double rttMs = std::chrono::duration<double, std::milli>(t1 - t0).count();
    bool passed = (res == SshResult::SUCCESS) && (rttMs <= 1500.0);

    ZyxelBenchmark::record({
        "LiveFirewallE9", "ReadAdvertisementActivation", ZyxelAuthCmd::cmdShowAdvertisementActivation(),
        rttMs, 0, out.size(), 1, passed,
        passed ? "PASS (SLA)" : "FAIL"
    });

    CHECK_TRUE(rttMs <= 1500.0);
}

TEST(LiveFirewallE9, ReadBillingStatus) {
    ZyxelDriver &driver = ZyxelDriver::getInstance();
    if (!driver.isConnected()) return;

    auto t0 = std::chrono::steady_clock::now();
    std::string out;
    SshResult res = driver.getSshClient().executeCommand(ZyxelAuthCmd::cmdShowBillingStatus(), out);
    auto t1 = std::chrono::steady_clock::now();

    CHECK_EQUAL(static_cast<int>(SshResult::SUCCESS), static_cast<int>(res));

    auto p0 = std::chrono::steady_clock::now();
    std::string acctMethod;
    int accumTimeout = 0;
    bool parsed = ZyxelAuthCmd::parseBillingStatus(out, acctMethod, accumTimeout);
    auto p1 = std::chrono::steady_clock::now();

    double rttMs = std::chrono::duration<double, std::milli>(t1 - t0).count();
    uint64_t parseUs = std::chrono::duration_cast<std::chrono::microseconds>(p1 - p0).count();
    bool passed = (res == SshResult::SUCCESS) && parsed && (rttMs <= 1500.0);

    ZyxelBenchmark::record({
        "LiveFirewallE9", "ReadBillingStatus", ZyxelAuthCmd::cmdShowBillingStatus(),
        rttMs, parseUs, out.size(), 1, passed,
        passed ? "PASS (SLA)" : "FAIL"
    });

    CHECK_TRUE(parsed);
    CHECK_TRUE(rttMs <= 1500.0);
}

TEST(LiveFirewallE9, ReadFreeTimeStatus) {
    ZyxelDriver &driver = ZyxelDriver::getInstance();
    if (!driver.isConnected()) return;

    auto t0 = std::chrono::steady_clock::now();
    std::string out;
    SshResult res = driver.getSshClient().executeCommand(ZyxelAuthCmd::cmdShowFreeTimeStatus(), out);
    auto t1 = std::chrono::steady_clock::now();

    CHECK_EQUAL(static_cast<int>(SshResult::SUCCESS), static_cast<int>(res));

    auto p0 = std::chrono::steady_clock::now();
    bool active = true;
    std::string period;
    bool parsed = ZyxelAuthCmd::parseFreeTimeStatus(out, active, period);
    auto p1 = std::chrono::steady_clock::now();

    double rttMs = std::chrono::duration<double, std::milli>(t1 - t0).count();
    uint64_t parseUs = std::chrono::duration_cast<std::chrono::microseconds>(p1 - p0).count();
    bool passed = (res == SshResult::SUCCESS) && parsed && (rttMs <= 1500.0);

    ZyxelBenchmark::record({
        "LiveFirewallE9", "ReadFreeTimeStatus", ZyxelAuthCmd::cmdShowFreeTimeStatus(),
        rttMs, parseUs, out.size(), 1, passed,
        passed ? "PASS (SLA)" : "FAIL"
    });

    CHECK_TRUE(parsed);
    CHECK_TRUE(rttMs <= 1500.0);
}

TEST(LiveFirewallE9, ReadIpIpnpActivation) {
    ZyxelDriver &driver = ZyxelDriver::getInstance();
    if (!driver.isConnected()) return;

    auto t0 = std::chrono::steady_clock::now();
    std::string out;
    SshResult res = driver.getSshClient().executeCommand(ZyxelAuthCmd::cmdShowIpIpnpActivation(), out);
    auto t1 = std::chrono::steady_clock::now();

    CHECK_EQUAL(static_cast<int>(SshResult::SUCCESS), static_cast<int>(res));

    double rttMs = std::chrono::duration<double, std::milli>(t1 - t0).count();
    bool passed = (res == SshResult::SUCCESS) && (rttMs <= 1500.0);

    ZyxelBenchmark::record({
        "LiveFirewallE9", "ReadIpIpnpActivation", ZyxelAuthCmd::cmdShowIpIpnpActivation(),
        rttMs, 0, out.size(), 1, passed,
        passed ? "PASS (SLA)" : "FAIL"
    });

    CHECK_TRUE(rttMs <= 1500.0);
}

TEST(LiveFirewallE9, ReadPaymentServiceActivation) {
    ZyxelDriver &driver = ZyxelDriver::getInstance();
    if (!driver.isConnected()) return;

    auto t0 = std::chrono::steady_clock::now();
    std::string out;
    SshResult res = driver.getSshClient().executeCommand(ZyxelAuthCmd::cmdShowPaymentServiceActivation(), out);
    auto t1 = std::chrono::steady_clock::now();

    CHECK_EQUAL(static_cast<int>(SshResult::SUCCESS), static_cast<int>(res));

    double rttMs = std::chrono::duration<double, std::milli>(t1 - t0).count();
    bool passed = (res == SshResult::SUCCESS) && (rttMs <= 1500.0);

    ZyxelBenchmark::record({
        "LiveFirewallE9", "ReadPaymentServiceActivation", ZyxelAuthCmd::cmdShowPaymentServiceActivation(),
        rttMs, 0, out.size(), 1, passed,
        passed ? "PASS (SLA)" : "FAIL"
    });

    CHECK_TRUE(rttMs <= 1500.0);
}

TEST(LiveFirewallE9, ReadUsername) {
    ZyxelDriver &driver = ZyxelDriver::getInstance();
    if (!driver.isConnected()) return;

    auto t0 = std::chrono::steady_clock::now();
    std::string out;
    SshResult res = driver.getSshClient().executeCommand(ZyxelAuthCmd::cmdShowUsername(), out);
    auto t1 = std::chrono::steady_clock::now();

    CHECK_EQUAL(static_cast<int>(SshResult::SUCCESS), static_cast<int>(res));

    auto p0 = std::chrono::steady_clock::now();
    std::vector<ZyxelUserEntry> users;
    bool parsed = ZyxelAuthCmd::parseUsers(out, users);
    auto p1 = std::chrono::steady_clock::now();

    double rttMs = std::chrono::duration<double, std::milli>(t1 - t0).count();
    uint64_t parseUs = std::chrono::duration_cast<std::chrono::microseconds>(p1 - p0).count();
    bool passed = (res == SshResult::SUCCESS) && parsed && (rttMs <= 1500.0);

    ZyxelBenchmark::record({
        "LiveFirewallE9", "ReadUsername", ZyxelAuthCmd::cmdShowUsername(),
        rttMs, parseUs, out.size(), static_cast<uint32_t>(users.size()), passed,
        passed ? "PASS (SLA)" : "FAIL"
    });

    CHECK_TRUE(parsed);
    CHECK_TRUE(rttMs <= 1500.0);
}

TEST(LiveFirewallE9, ReadGroupname) {
    ZyxelDriver &driver = ZyxelDriver::getInstance();
    if (!driver.isConnected()) return;

    auto t0 = std::chrono::steady_clock::now();
    std::string out;
    SshResult res = driver.getSshClient().executeCommand(ZyxelAuthCmd::cmdShowGroupname(), out);
    auto t1 = std::chrono::steady_clock::now();

    CHECK_EQUAL(static_cast<int>(SshResult::SUCCESS), static_cast<int>(res));

    double rttMs = std::chrono::duration<double, std::milli>(t1 - t0).count();
    bool passed = (res == SshResult::SUCCESS) && (rttMs <= 1500.0);

    ZyxelBenchmark::record({
        "LiveFirewallE9", "ReadGroupname", ZyxelAuthCmd::cmdShowGroupname(),
        rttMs, 0, out.size(), 1, passed,
        passed ? "PASS (SLA)" : "FAIL"
    });

    CHECK_TRUE(rttMs <= 1500.0);
}

TEST(LiveFirewallE9, ReadLockoutUsers) {
    ZyxelDriver &driver = ZyxelDriver::getInstance();
    if (!driver.isConnected()) return;

    auto t0 = std::chrono::steady_clock::now();
    std::string out;
    SshResult res = driver.getSshClient().executeCommand(ZyxelAuthCmd::cmdShowLockoutUsers(), out);
    auto t1 = std::chrono::steady_clock::now();

    CHECK_EQUAL(static_cast<int>(SshResult::SUCCESS), static_cast<int>(res));

    double rttMs = std::chrono::duration<double, std::milli>(t1 - t0).count();
    bool passed = (res == SshResult::SUCCESS) && (rttMs <= 1500.0);

    ZyxelBenchmark::record({
        "LiveFirewallE9", "ReadLockoutUsers", ZyxelAuthCmd::cmdShowLockoutUsers(),
        rttMs, 0, out.size(), 1, passed,
        passed ? "PASS (SLA)" : "FAIL"
    });

    CHECK_TRUE(rttMs <= 1500.0);
}

TEST(LiveFirewallE9, ReadPasswordComplexityVerifyStatus) {
    ZyxelDriver &driver = ZyxelDriver::getInstance();
    if (!driver.isConnected()) return;

    auto t0 = std::chrono::steady_clock::now();
    std::string out;
    SshResult res = driver.getSshClient().executeCommand(ZyxelAuthCmd::cmdShowPasswordComplexityVerifyStatus(), out);
    auto t1 = std::chrono::steady_clock::now();

    CHECK_EQUAL(static_cast<int>(SshResult::SUCCESS), static_cast<int>(res));

    double rttMs = std::chrono::duration<double, std::milli>(t1 - t0).count();
    bool passed = (res == SshResult::SUCCESS) && (rttMs <= 1500.0);

    ZyxelBenchmark::record({
        "LiveFirewallE9", "ReadPasswordComplexityVerifyStatus", ZyxelAuthCmd::cmdShowPasswordComplexityVerifyStatus(),
        rttMs, 0, out.size(), 1, passed,
        passed ? "PASS (SLA)" : "FAIL"
    });

    CHECK_TRUE(rttMs <= 1500.0);
}

TEST(LiveFirewallE9, ReadPwdExpiryAll) {
    ZyxelDriver &driver = ZyxelDriver::getInstance();
    if (!driver.isConnected()) return;

    auto t0 = std::chrono::steady_clock::now();
    std::string out;
    SshResult res = driver.getSshClient().executeCommand(ZyxelAuthCmd::cmdShowPwdExpiry(), out);
    auto t1 = std::chrono::steady_clock::now();

    CHECK_EQUAL(static_cast<int>(SshResult::SUCCESS), static_cast<int>(res));

    double rttMs = std::chrono::duration<double, std::milli>(t1 - t0).count();
    bool passed = (res == SshResult::SUCCESS) && (rttMs <= 1500.0);

    ZyxelBenchmark::record({
        "LiveFirewallE9", "ReadPwdExpiryAll", ZyxelAuthCmd::cmdShowPwdExpiry(),
        rttMs, 0, out.size(), 1, passed,
        passed ? "PASS (SLA)" : "FAIL"
    });

    CHECK_TRUE(rttMs <= 1500.0);
}

TEST(LiveFirewallE9, ReadUsersDefaultSettingAll) {
    ZyxelDriver &driver = ZyxelDriver::getInstance();
    if (!driver.isConnected()) return;

    auto t0 = std::chrono::steady_clock::now();
    std::string out;
    SshResult res = driver.getSshClient().executeCommand(ZyxelAuthCmd::cmdShowUsersDefaultSetting(), out);
    auto t1 = std::chrono::steady_clock::now();

    CHECK_EQUAL(static_cast<int>(SshResult::SUCCESS), static_cast<int>(res));

    double rttMs = std::chrono::duration<double, std::milli>(t1 - t0).count();
    bool passed = (res == SshResult::SUCCESS) && (rttMs <= 1500.0);

    ZyxelBenchmark::record({
        "LiveFirewallE9", "ReadUsersDefaultSettingAll", ZyxelAuthCmd::cmdShowUsersDefaultSetting(),
        rttMs, 0, out.size(), 1, passed,
        passed ? "PASS (SLA)" : "FAIL"
    });

    CHECK_TRUE(rttMs <= 1500.0);
}

TEST(LiveFirewallE9, ReadUsersIdleDetectionSettings) {
    ZyxelDriver &driver = ZyxelDriver::getInstance();
    if (!driver.isConnected()) return;

    auto t0 = std::chrono::steady_clock::now();
    std::string out;
    SshResult res = driver.getSshClient().executeCommand(ZyxelAuthCmd::cmdShowUsersIdleDetectionSettings(), out);
    auto t1 = std::chrono::steady_clock::now();

    CHECK_EQUAL(static_cast<int>(SshResult::SUCCESS), static_cast<int>(res));

    double rttMs = std::chrono::duration<double, std::milli>(t1 - t0).count();
    bool passed = (res == SshResult::SUCCESS) && (rttMs <= 1500.0);

    ZyxelBenchmark::record({
        "LiveFirewallE9", "ReadUsersIdleDetectionSettings", ZyxelAuthCmd::cmdShowUsersIdleDetectionSettings(),
        rttMs, 0, out.size(), 1, passed,
        passed ? "PASS (SLA)" : "FAIL"
    });

    CHECK_TRUE(rttMs <= 1500.0);
}

TEST(LiveFirewallE9, ReadUsersRetrySettings) {
    ZyxelDriver &driver = ZyxelDriver::getInstance();
    if (!driver.isConnected()) return;

    auto t0 = std::chrono::steady_clock::now();
    std::string out;
    SshResult res = driver.getSshClient().executeCommand(ZyxelAuthCmd::cmdShowUsersRetrySettings(), out);
    auto t1 = std::chrono::steady_clock::now();

    CHECK_EQUAL(static_cast<int>(SshResult::SUCCESS), static_cast<int>(res));

    auto p0 = std::chrono::steady_clock::now();
    ZyxelUserRetrySettings st;
    bool parsed = ZyxelAuthCmd::parseUsersRetrySettings(out, st);
    auto p1 = std::chrono::steady_clock::now();

    double rttMs = std::chrono::duration<double, std::milli>(t1 - t0).count();
    uint64_t parseUs = std::chrono::duration_cast<std::chrono::microseconds>(p1 - p0).count();
    bool passed = (res == SshResult::SUCCESS) && parsed && (st.maxRetryCount == 5) && (st.lockoutPeriod == 30) && (rttMs <= 1500.0);

    ZyxelBenchmark::record({
        "LiveFirewallE9", "ReadUsersRetrySettings", ZyxelAuthCmd::cmdShowUsersRetrySettings(),
        rttMs, parseUs, out.size(), 1, passed,
        passed ? "PASS (SLA)" : "FAIL"
    });

    CHECK_TRUE(parsed);
    LONGS_EQUAL(5, st.maxRetryCount);
    LONGS_EQUAL(30, st.lockoutPeriod);
    CHECK_TRUE(rttMs <= 1500.0);
}

TEST(LiveFirewallE9, ReadUsersSimultaneousLogonSettings) {
    ZyxelDriver &driver = ZyxelDriver::getInstance();
    if (!driver.isConnected()) return;

    auto t0 = std::chrono::steady_clock::now();
    std::string out;
    SshResult res = driver.getSshClient().executeCommand(ZyxelAuthCmd::cmdShowUsersSimultaneousLogonSettings(), out);
    auto t1 = std::chrono::steady_clock::now();

    CHECK_EQUAL(static_cast<int>(SshResult::SUCCESS), static_cast<int>(res));

    double rttMs = std::chrono::duration<double, std::milli>(t1 - t0).count();
    bool passed = (res == SshResult::SUCCESS) && (rttMs <= 1500.0);

    ZyxelBenchmark::record({
        "LiveFirewallE9", "ReadUsersSimultaneousLogonSettings", ZyxelAuthCmd::cmdShowUsersSimultaneousLogonSettings(),
        rttMs, 0, out.size(), 1, passed,
        passed ? "PASS (SLA)" : "FAIL"
    });

    CHECK_TRUE(rttMs <= 1500.0);
}

TEST(LiveFirewallE9, ReadUsersUpdateLeaseSettings) {
    ZyxelDriver &driver = ZyxelDriver::getInstance();
    if (!driver.isConnected()) return;

    auto t0 = std::chrono::steady_clock::now();
    std::string out;
    SshResult res = driver.getSshClient().executeCommand(ZyxelAuthCmd::cmdShowUsersUpdateLeaseSettings(), out);
    auto t1 = std::chrono::steady_clock::now();

    CHECK_EQUAL(static_cast<int>(SshResult::SUCCESS), static_cast<int>(res));

    double rttMs = std::chrono::duration<double, std::milli>(t1 - t0).count();
    bool passed = (res == SshResult::SUCCESS) && (rttMs <= 1500.0);

    ZyxelBenchmark::record({
        "LiveFirewallE9", "ReadUsersUpdateLeaseSettings", ZyxelAuthCmd::cmdShowUsersUpdateLeaseSettings(),
        rttMs, 0, out.size(), 1, passed,
        passed ? "PASS (SLA)" : "FAIL"
    });

    CHECK_TRUE(rttMs <= 1500.0);
}

TEST(LiveFirewallE9, ReadUsersAll) {
    ZyxelDriver &driver = ZyxelDriver::getInstance();
    if (!driver.isConnected()) return;

    auto t0 = std::chrono::steady_clock::now();
    std::string out;
    SshResult res = driver.getSshClient().executeCommand(ZyxelAuthCmd::cmdShowUsers(), out);
    auto t1 = std::chrono::steady_clock::now();

    CHECK_EQUAL(static_cast<int>(SshResult::SUCCESS), static_cast<int>(res));

    double rttMs = std::chrono::duration<double, std::milli>(t1 - t0).count();
    bool passed = (res == SshResult::SUCCESS) && (rttMs <= 1500.0);

    ZyxelBenchmark::record({
        "LiveFirewallE9", "ReadUsersAll", ZyxelAuthCmd::cmdShowUsers(),
        rttMs, 0, out.size(), 1, passed,
        passed ? "PASS (SLA)" : "FAIL"
    });

    CHECK_TRUE(rttMs <= 1500.0);
}

TEST(LiveFirewallE9, ReadAaaAuthenticationDefault) {
    ZyxelDriver &driver = ZyxelDriver::getInstance();
    if (!driver.isConnected()) return;

    auto t0 = std::chrono::steady_clock::now();
    std::string out;
    SshResult res = driver.getSshClient().executeCommand(ZyxelAuthCmd::cmdShowAaaAuthentication(), out);
    auto t1 = std::chrono::steady_clock::now();

    CHECK_EQUAL(static_cast<int>(SshResult::SUCCESS), static_cast<int>(res));

    double rttMs = std::chrono::duration<double, std::milli>(t1 - t0).count();
    bool passed = (res == SshResult::SUCCESS) && (rttMs <= 1500.0);

    ZyxelBenchmark::record({
        "LiveFirewallE9", "ReadAaaAuthenticationDefault", ZyxelAuthCmd::cmdShowAaaAuthentication(),
        rttMs, 0, out.size(), 1, passed,
        passed ? "PASS (SLA)" : "FAIL"
    });

    CHECK_TRUE(rttMs <= 1500.0);
}

TEST(LiveFirewallE9, ReadCaCategoryLocal) {
    ZyxelDriver &driver = ZyxelDriver::getInstance();
    if (!driver.isConnected()) return;

    auto t0 = std::chrono::steady_clock::now();
    std::string out;
    SshResult res = driver.getSshClient().executeCommand(ZyxelAuthCmd::cmdShowCaCategoryLocal(), out);
    auto t1 = std::chrono::steady_clock::now();

    CHECK_EQUAL(static_cast<int>(SshResult::SUCCESS), static_cast<int>(res));

    auto p0 = std::chrono::steady_clock::now();
    std::vector<ZyxelCaCertInfo> certs;
    bool parsed = ZyxelAuthCmd::parseCaCertInfo(out, certs);
    auto p1 = std::chrono::steady_clock::now();

    double rttMs = std::chrono::duration<double, std::milli>(t1 - t0).count();
    uint64_t parseUs = std::chrono::duration_cast<std::chrono::microseconds>(p1 - p0).count();
    bool passed = (res == SshResult::SUCCESS) && parsed && (rttMs <= 1500.0);

    ZyxelBenchmark::record({
        "LiveFirewallE9", "ReadCaCategoryLocal", ZyxelAuthCmd::cmdShowCaCategoryLocal(),
        rttMs, parseUs, out.size(), static_cast<uint32_t>(certs.size()), passed,
        passed ? "PASS (SLA)" : "FAIL"
    });

    CHECK_TRUE(parsed);
    CHECK_TRUE(rttMs <= 1500.0);
}

TEST(LiveFirewallE9, ReadCaCategoryRemote) {
    ZyxelDriver &driver = ZyxelDriver::getInstance();
    if (!driver.isConnected()) return;

    auto t0 = std::chrono::steady_clock::now();
    std::string out;
    SshResult res = driver.getSshClient().executeCommand(ZyxelAuthCmd::cmdShowCaCategoryRemote(), out);
    auto t1 = std::chrono::steady_clock::now();

    CHECK_EQUAL(static_cast<int>(SshResult::SUCCESS), static_cast<int>(res));

    auto p0 = std::chrono::steady_clock::now();
    std::vector<ZyxelCaCertInfo> certs;
    bool parsed = ZyxelAuthCmd::parseCaCertInfo(out, certs);
    auto p1 = std::chrono::steady_clock::now();

    double rttMs = std::chrono::duration<double, std::milli>(t1 - t0).count();
    uint64_t parseUs = std::chrono::duration_cast<std::chrono::microseconds>(p1 - p0).count();
    bool passed = (res == SshResult::SUCCESS) && parsed && (rttMs <= 1500.0);

    ZyxelBenchmark::record({
        "LiveFirewallE9", "ReadCaCategoryRemote", ZyxelAuthCmd::cmdShowCaCategoryRemote(),
        rttMs, parseUs, out.size(), static_cast<uint32_t>(certs.size()), passed,
        passed ? "PASS (SLA)" : "FAIL"
    });

    CHECK_TRUE(parsed);
    CHECK_TRUE(rttMs <= 1500.0);
}

TEST(LiveFirewallE9, ReadCaSpaceusage) {
    ZyxelDriver &driver = ZyxelDriver::getInstance();
    if (!driver.isConnected()) return;

    auto t0 = std::chrono::steady_clock::now();
    std::string out;
    SshResult res = driver.getSshClient().executeCommand(ZyxelAuthCmd::cmdShowCaSpaceusage(), out);
    auto t1 = std::chrono::steady_clock::now();

    CHECK_EQUAL(static_cast<int>(SshResult::SUCCESS), static_cast<int>(res));

    auto p0 = std::chrono::steady_clock::now();
    ZyxelCaSpaceUsage usage;
    bool parsed = ZyxelAuthCmd::parseCaSpaceUsage(out, usage);
    auto p1 = std::chrono::steady_clock::now();

    double rttMs = std::chrono::duration<double, std::milli>(t1 - t0).count();
    uint64_t parseUs = std::chrono::duration_cast<std::chrono::microseconds>(p1 - p0).count();
    bool passed = (res == SshResult::SUCCESS) && parsed && (usage.total == 262144) && (rttMs <= 1500.0);

    ZyxelBenchmark::record({
        "LiveFirewallE9", "ReadCaSpaceusage", ZyxelAuthCmd::cmdShowCaSpaceusage(),
        rttMs, parseUs, out.size(), 1, passed,
        passed ? "PASS (SLA)" : "FAIL"
    });

    CHECK_TRUE(parsed);
    LONGS_EQUAL(262144, usage.total);
    CHECK_TRUE(rttMs <= 1500.0);
}

TEST(LiveFirewallE9, DryFireInvalidDynamicGuest) {
    ZyxelDriver &driver = ZyxelDriver::getInstance();
    if (!driver.isConnected()) return;

    auto t0 = std::chrono::steady_clock::now();
    std::string out;
    SshResult res = driver.getSshClient().executeCommand(ZyxelAuthCmd::cmdInvalidDynamicGuestDryFire(), out);
    auto t1 = std::chrono::steady_clock::now();

    bool rejected = (res == SshResult::ERR_SYNTAX || out.find("Parse error") != std::string::npos);
    double rttMs = std::chrono::duration<double, std::milli>(t1 - t0).count();
    bool passed = rejected && rttMs <= 2500.0;

    ZyxelBenchmark::record({
        "LiveFirewallE9", "DryFireInvalidDynamicGuest", ZyxelAuthCmd::cmdInvalidDynamicGuestDryFire(),
        rttMs, 0, out.size(), rejected ? 1U : 0U, passed,
        passed ? "PASS (REJECT)" : "FAIL"
    });

    CHECK_TRUE(rejected);
    CHECK_TRUE(rttMs <= 2500.0);
}

TEST(LiveFirewallE9, DryFireInvalidAaaServer) {
    ZyxelDriver &driver = ZyxelDriver::getInstance();
    if (!driver.isConnected()) return;

    auto t0 = std::chrono::steady_clock::now();
    std::string out;
    SshResult res = driver.getSshClient().executeCommand(ZyxelAuthCmd::cmdInvalidAaaServerDryFire(), out);
    auto t1 = std::chrono::steady_clock::now();

    bool rejected = (res == SshResult::ERR_SYNTAX || out.find("Parse error") != std::string::npos);
    double rttMs = std::chrono::duration<double, std::milli>(t1 - t0).count();
    bool passed = rejected && rttMs <= 2500.0;

    ZyxelBenchmark::record({
        "LiveFirewallE9", "DryFireInvalidAaaServer", ZyxelAuthCmd::cmdInvalidAaaServerDryFire(),
        rttMs, 0, out.size(), rejected ? 1U : 0U, passed,
        passed ? "PASS (REJECT)" : "FAIL"
    });

    CHECK_TRUE(rejected);
    CHECK_TRUE(rttMs <= 2500.0);
}

TEST(LiveFirewallE9, DryFireInvalidAuthMethod) {
    ZyxelDriver &driver = ZyxelDriver::getInstance();
    if (!driver.isConnected()) return;

    auto t0 = std::chrono::steady_clock::now();
    std::string out;
    SshResult res = driver.getSshClient().executeCommand(ZyxelAuthCmd::cmdInvalidAuthMethodDryFire(), out);
    auto t1 = std::chrono::steady_clock::now();

    bool rejected = (res == SshResult::ERR_SYNTAX || out.find("Parse error") != std::string::npos);
    double rttMs = std::chrono::duration<double, std::milli>(t1 - t0).count();
    bool passed = rejected && rttMs <= 2500.0;

    ZyxelBenchmark::record({
        "LiveFirewallE9", "DryFireInvalidAuthMethod", ZyxelAuthCmd::cmdInvalidAuthMethodDryFire(),
        rttMs, 0, out.size(), rejected ? 1U : 0U, passed,
        passed ? "PASS (REJECT)" : "FAIL"
    });

    CHECK_TRUE(rejected);
    CHECK_TRUE(rttMs <= 2500.0);
}

TEST(LiveFirewallE9, WriteGroupnameRollback) {
    ZyxelDriver &driver = ZyxelDriver::getInstance();
    if (!driver.isConnected()) return;
    ZyxelSshClient &ssh = driver.getSshClient();

    auto t0 = std::chrono::steady_clock::now();
    std::string out;

    // 1. Create group
    ssh.executeCommand("configure terminal", out);
    ssh.executeCommand(ZyxelAuthCmd::cmdGroupname("NETMON_QA_GRP_E9"), out);
    ssh.executeCommand("exit", out);
    ssh.executeCommand("exit", out);
    ssh.unwindToRootPrompt();

    // 2. Verify creation
    ssh.executeCommand(ZyxelAuthCmd::cmdShowGroupname("NETMON_QA_GRP_E9"), out);
    bool created = (out.find("Group: NETMON_QA_GRP_E9") != std::string::npos);

    // 3. Rollback
    ssh.executeCommand("configure terminal", out);
    SshResult resDel = ssh.executeCommand(ZyxelAuthCmd::cmdNoGroupname("NETMON_QA_GRP_E9"), out);
    ssh.executeCommand("exit", out);
    ssh.unwindToRootPrompt();

    CHECK_EQUAL(static_cast<int>(SshResult::SUCCESS), static_cast<int>(resDel));

    // 4. Verify clean deletion
    ssh.executeCommand(ZyxelAuthCmd::cmdShowGroupname("NETMON_QA_GRP_E9"), out);
    bool deleted = (out.find("User group does not exist") != std::string::npos);
    auto t1 = std::chrono::steady_clock::now();

    double rttMs = std::chrono::duration<double, std::milli>(t1 - t0).count();
    bool passed = created && deleted && (rttMs <= 6000.0);

    ZyxelBenchmark::record({
        "LiveFirewallE9", "WriteGroupnameRollback", "groupname NETMON_QA_GRP_E9",
        rttMs, 0, out.size(), 1, passed,
        passed ? "PASS (ROLLBACK)" : "FAIL"
    });

    CHECK_TRUE(created);
    CHECK_TRUE(deleted);
    CHECK_TRUE(rttMs <= 6000.0);
}

TEST(LiveFirewallE9, WriteAaaGroupServerRadiusRollback) {
    ZyxelDriver &driver = ZyxelDriver::getInstance();
    if (!driver.isConnected()) return;
    ZyxelSshClient &ssh = driver.getSshClient();

    auto t0 = std::chrono::steady_clock::now();
    std::string out;

    // 1. Create AAA group server
    ssh.executeCommand("configure terminal", out);
    ssh.executeCommand(ZyxelAuthCmd::cmdAaaGroupServerRadius("NETMON_QA_AAA_E9"), out);
    ssh.executeCommand("exit", out);
    ssh.executeCommand("exit", out);
    ssh.unwindToRootPrompt();

    // 2. Verify creation
    ssh.executeCommand(ZyxelAuthCmd::cmdShowAaaGroupServerRadius("NETMON_QA_AAA_E9"), out);
    bool created = (out.find("group attribute") != std::string::npos);

    // 3. Rollback
    ssh.executeCommand("configure terminal", out);
    SshResult resDel = ssh.executeCommand(ZyxelAuthCmd::cmdNoAaaGroupServerRadius("NETMON_QA_AAA_E9"), out);
    ssh.executeCommand("exit", out);
    ssh.unwindToRootPrompt();

    CHECK_EQUAL(static_cast<int>(SshResult::SUCCESS), static_cast<int>(resDel));

    // 4. Verify clean deletion
    ssh.executeCommand(ZyxelAuthCmd::cmdShowAaaGroupServerRadius("NETMON_QA_AAA_E9"), out);
    bool deleted = (out.find("group attribute") == std::string::npos);
    auto t1 = std::chrono::steady_clock::now();

    double rttMs = std::chrono::duration<double, std::milli>(t1 - t0).count();
    bool passed = created && deleted && (rttMs <= 6000.0);

    ZyxelBenchmark::record({
        "LiveFirewallE9", "WriteAaaGroupServerRadiusRollback", "aaa group server radius NETMON_QA_AAA_E9",
        rttMs, 0, out.size(), 1, passed,
        passed ? "PASS (ROLLBACK)" : "FAIL"
    });

    CHECK_TRUE(created);
    CHECK_TRUE(deleted);
    CHECK_TRUE(rttMs <= 6000.0);
}

// ============================================================================
// Envelope 10: Wireless LAN & Lifecycle (Ch 6..14, 73)
// ============================================================================

TEST_GROUP(LiveFirewallE10) {
    void setup() override {
        ZyxelDriver &driver = ZyxelDriver::getInstance();
        if (driver.isConnected()) {
            ZyxelSshClient &ssh = driver.getSshClient();
            ssh.unwindToRootPrompt();
            std::string out;
            ssh.executeCommand("configure terminal", out);
            ssh.executeCommand("no wlan-ssid-profile NETMON_QA_SSID_E10", out);
            ssh.executeCommand("no wlan-security-profile NETMON_QA_SEC_E10", out);
            ssh.executeCommand("exit", out);
            ssh.unwindToRootPrompt();
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(25));
    }

    void teardown() override {
        ZyxelDriver &driver = ZyxelDriver::getInstance();
        if (driver.isConnected()) {
            ZyxelSshClient &ssh = driver.getSshClient();
            ssh.unwindToRootPrompt();
            std::string out;
            ssh.executeCommand("configure terminal", out);
            ssh.executeCommand("no wlan-ssid-profile NETMON_QA_SSID_E10", out);
            ssh.executeCommand("no wlan-security-profile NETMON_QA_SEC_E10", out);
            ssh.executeCommand("exit", out);
            ssh.unwindToRootPrompt();
        }
    }
};

TEST(LiveFirewallE10, ReadCapwapApAll) {
    ZyxelDriver &driver = ZyxelDriver::getInstance();
    if (!driver.isConnected()) return;

    auto t0 = std::chrono::steady_clock::now();
    std::string out;
    SshResult res = driver.getSshClient().executeCommand(ZyxelWlanCmd::cmdShowCapwapApAll(), out);
    auto t1 = std::chrono::steady_clock::now();

    CHECK_EQUAL(static_cast<int>(SshResult::SUCCESS), static_cast<int>(res));
    double rttMs = std::chrono::duration<double, std::milli>(t1 - t0).count();
    bool passed = (res == SshResult::SUCCESS) && (rttMs <= 1500.0);

    ZyxelBenchmark::record({
        "LiveFirewallE10", "ReadCapwapApAll", ZyxelWlanCmd::cmdShowCapwapApAll(),
        rttMs, 0, out.size(), 1, passed,
        passed ? "PASS (SLA)" : "FAIL"
    });

    CHECK_TRUE(rttMs <= 1500.0);
}

TEST(LiveFirewallE10, ReadCapwapApConfigStatus) {
    ZyxelDriver &driver = ZyxelDriver::getInstance();
    if (!driver.isConnected()) return;

    auto t0 = std::chrono::steady_clock::now();
    std::string out;
    SshResult res = driver.getSshClient().executeCommand(ZyxelWlanCmd::cmdShowCapwapApConfigStatus(), out);
    auto t1 = std::chrono::steady_clock::now();

    CHECK_EQUAL(static_cast<int>(SshResult::SUCCESS), static_cast<int>(res));
    double rttMs = std::chrono::duration<double, std::milli>(t1 - t0).count();
    bool passed = (res == SshResult::SUCCESS) && (rttMs <= 1500.0);

    ZyxelBenchmark::record({
        "LiveFirewallE10", "ReadCapwapApConfigStatus", ZyxelWlanCmd::cmdShowCapwapApConfigStatus(),
        rttMs, 0, out.size(), 1, passed,
        passed ? "PASS (SLA)" : "FAIL"
    });

    CHECK_TRUE(rttMs <= 1500.0);
}

TEST(LiveFirewallE10, ReadCapwapApStatistics) {
    ZyxelDriver &driver = ZyxelDriver::getInstance();
    if (!driver.isConnected()) return;

    auto t0 = std::chrono::steady_clock::now();
    std::string out;
    SshResult res = driver.getSshClient().executeCommand(ZyxelWlanCmd::cmdShowCapwapApStatistics(), out);
    auto t1 = std::chrono::steady_clock::now();

    CHECK_EQUAL(static_cast<int>(SshResult::SUCCESS), static_cast<int>(res));
    double rttMs = std::chrono::duration<double, std::milli>(t1 - t0).count();
    bool passed = (res == SshResult::SUCCESS) && (rttMs <= 1500.0);

    ZyxelBenchmark::record({
        "LiveFirewallE10", "ReadCapwapApStatistics", ZyxelWlanCmd::cmdShowCapwapApStatistics(),
        rttMs, 0, out.size(), 1, passed,
        passed ? "PASS (SLA)" : "FAIL"
    });

    CHECK_TRUE(rttMs <= 1500.0);
}

TEST(LiveFirewallE10, ReadCapwapApFallback) {
    ZyxelDriver &driver = ZyxelDriver::getInstance();
    if (!driver.isConnected()) return;

    auto t0 = std::chrono::steady_clock::now();
    std::string out;
    SshResult res = driver.getSshClient().executeCommand(ZyxelWlanCmd::cmdShowCapwapApFallback(), out);
    auto t1 = std::chrono::steady_clock::now();

    CHECK_EQUAL(static_cast<int>(SshResult::SUCCESS), static_cast<int>(res));

    auto p0 = std::chrono::steady_clock::now();
    ZyxelCapwapFallback fb;
    bool parsed = ZyxelWlanCmd::parseCapwapFallback(out, fb);
    auto p1 = std::chrono::steady_clock::now();

    double rttMs = std::chrono::duration<double, std::milli>(t1 - t0).count();
    uint64_t parseUs = std::chrono::duration_cast<std::chrono::microseconds>(p1 - p0).count();
    bool passed = (res == SshResult::SUCCESS) && parsed && (rttMs <= 1500.0);

    ZyxelBenchmark::record({
        "LiveFirewallE10", "ReadCapwapApFallback", ZyxelWlanCmd::cmdShowCapwapApFallback(),
        rttMs, parseUs, out.size(), 1, passed,
        passed ? "PASS (SLA)" : "FAIL"
    });

    CHECK_TRUE(parsed);
    CHECK_TRUE(rttMs <= 1500.0);
}

TEST(LiveFirewallE10, ReadCapwapApFallbackInterval) {
    ZyxelDriver &driver = ZyxelDriver::getInstance();
    if (!driver.isConnected()) return;

    auto t0 = std::chrono::steady_clock::now();
    std::string out;
    SshResult res = driver.getSshClient().executeCommand(ZyxelWlanCmd::cmdShowCapwapApFallbackInterval(), out);
    auto t1 = std::chrono::steady_clock::now();

    CHECK_EQUAL(static_cast<int>(SshResult::SUCCESS), static_cast<int>(res));
    double rttMs = std::chrono::duration<double, std::milli>(t1 - t0).count();
    bool passed = (res == SshResult::SUCCESS) && (out.find("Fallback Interval:") != std::string::npos) && (rttMs <= 1500.0);

    ZyxelBenchmark::record({
        "LiveFirewallE10", "ReadCapwapApFallbackInterval", ZyxelWlanCmd::cmdShowCapwapApFallbackInterval(),
        rttMs, 0, out.size(), 1, passed,
        passed ? "PASS (SLA)" : "FAIL"
    });

    CHECK_TRUE(passed);
    CHECK_TRUE(rttMs <= 1500.0);
}

TEST(LiveFirewallE10, ReadCapwapApIdleTimeout) {
    ZyxelDriver &driver = ZyxelDriver::getInstance();
    if (!driver.isConnected()) return;

    auto t0 = std::chrono::steady_clock::now();
    std::string out;
    SshResult res = driver.getSshClient().executeCommand(ZyxelWlanCmd::cmdShowCapwapApIdleTimeout(), out);
    auto t1 = std::chrono::steady_clock::now();

    CHECK_EQUAL(static_cast<int>(SshResult::SUCCESS), static_cast<int>(res));
    double rttMs = std::chrono::duration<double, std::milli>(t1 - t0).count();
    bool passed = (res == SshResult::SUCCESS) && (out.find("Idle timeout:") != std::string::npos) && (rttMs <= 1500.0);

    ZyxelBenchmark::record({
        "LiveFirewallE10", "ReadCapwapApIdleTimeout", ZyxelWlanCmd::cmdShowCapwapApIdleTimeout(),
        rttMs, 0, out.size(), 1, passed,
        passed ? "PASS (SLA)" : "FAIL"
    });

    CHECK_TRUE(passed);
    CHECK_TRUE(rttMs <= 1500.0);
}

TEST(LiveFirewallE10, ReadCapwapApWaitList) {
    ZyxelDriver &driver = ZyxelDriver::getInstance();
    if (!driver.isConnected()) return;

    auto t0 = std::chrono::steady_clock::now();
    std::string out;
    SshResult res = driver.getSshClient().executeCommand(ZyxelWlanCmd::cmdShowCapwapApWaitList(), out);
    auto t1 = std::chrono::steady_clock::now();

    CHECK_EQUAL(static_cast<int>(SshResult::SUCCESS), static_cast<int>(res));
    double rttMs = std::chrono::duration<double, std::milli>(t1 - t0).count();
    bool passed = (res == SshResult::SUCCESS) && (rttMs <= 1500.0);

    ZyxelBenchmark::record({
        "LiveFirewallE10", "ReadCapwapApWaitList", ZyxelWlanCmd::cmdShowCapwapApWaitList(),
        rttMs, 0, out.size(), 1, passed,
        passed ? "PASS (SLA)" : "FAIL"
    });

    CHECK_TRUE(rttMs <= 1500.0);
}

TEST(LiveFirewallE10, ReadCapwapManualAdd) {
    ZyxelDriver &driver = ZyxelDriver::getInstance();
    if (!driver.isConnected()) return;

    auto t0 = std::chrono::steady_clock::now();
    std::string out;
    SshResult res = driver.getSshClient().executeCommand(ZyxelWlanCmd::cmdShowCapwapManualAdd(), out);
    auto t1 = std::chrono::steady_clock::now();

    CHECK_EQUAL(static_cast<int>(SshResult::SUCCESS), static_cast<int>(res));
    double rttMs = std::chrono::duration<double, std::milli>(t1 - t0).count();
    bool passed = (res == SshResult::SUCCESS) && (out.find("Manual add:") != std::string::npos) && (rttMs <= 1500.0);

    ZyxelBenchmark::record({
        "LiveFirewallE10", "ReadCapwapManualAdd", ZyxelWlanCmd::cmdShowCapwapManualAdd(),
        rttMs, 0, out.size(), 1, passed,
        passed ? "PASS (SLA)" : "FAIL"
    });

    CHECK_TRUE(passed);
    CHECK_TRUE(rttMs <= 1500.0);
}

TEST(LiveFirewallE10, ReadCapwapStationAll) {
    ZyxelDriver &driver = ZyxelDriver::getInstance();
    if (!driver.isConnected()) return;

    auto t0 = std::chrono::steady_clock::now();
    std::string out;
    SshResult res = driver.getSshClient().executeCommand(ZyxelWlanCmd::cmdShowCapwapStationAll(), out);
    auto t1 = std::chrono::steady_clock::now();

    CHECK_EQUAL(static_cast<int>(SshResult::SUCCESS), static_cast<int>(res));
    double rttMs = std::chrono::duration<double, std::milli>(t1 - t0).count();
    bool passed = (res == SshResult::SUCCESS) && (rttMs <= 1500.0);

    ZyxelBenchmark::record({
        "LiveFirewallE10", "ReadCapwapStationAll", ZyxelWlanCmd::cmdShowCapwapStationAll(),
        rttMs, 0, out.size(), 1, passed,
        passed ? "PASS (SLA)" : "FAIL"
    });

    CHECK_TRUE(rttMs <= 1500.0);
}

TEST(LiveFirewallE10, ReadCountryCodeList) {
    ZyxelDriver &driver = ZyxelDriver::getInstance();
    if (!driver.isConnected()) return;

    auto t0 = std::chrono::steady_clock::now();
    std::string out;
    SshResult res = driver.getSshClient().executeCommand(ZyxelWlanCmd::cmdShowCountryCodeList(), out);
    auto t1 = std::chrono::steady_clock::now();

    CHECK_EQUAL(static_cast<int>(SshResult::SUCCESS), static_cast<int>(res));
    double rttMs = std::chrono::duration<double, std::milli>(t1 - t0).count();
    bool passed = (res == SshResult::SUCCESS) && (out.find("Country Code") != std::string::npos) && (rttMs <= 1500.0);

    ZyxelBenchmark::record({
        "LiveFirewallE10", "ReadCountryCodeList", ZyxelWlanCmd::cmdShowCountryCodeList(),
        rttMs, 0, out.size(), 1, passed,
        passed ? "PASS (SLA)" : "FAIL"
    });

    CHECK_TRUE(passed);
    CHECK_TRUE(rttMs <= 1500.0);
}

TEST(LiveFirewallE10, ReadDefaultCountryCode) {
    ZyxelDriver &driver = ZyxelDriver::getInstance();
    if (!driver.isConnected()) return;

    auto t0 = std::chrono::steady_clock::now();
    std::string out;
    SshResult res = driver.getSshClient().executeCommand(ZyxelWlanCmd::cmdShowDefaultCountryCode(), out);
    auto t1 = std::chrono::steady_clock::now();

    CHECK_EQUAL(static_cast<int>(SshResult::SUCCESS), static_cast<int>(res));
    double rttMs = std::chrono::duration<double, std::milli>(t1 - t0).count();
    bool passed = (res == SshResult::SUCCESS) && (out.find("AC Country Code:") != std::string::npos) && (rttMs <= 1500.0);

    ZyxelBenchmark::record({
        "LiveFirewallE10", "ReadDefaultCountryCode", ZyxelWlanCmd::cmdShowDefaultCountryCode(),
        rttMs, 0, out.size(), 1, passed,
        passed ? "PASS (SLA)" : "FAIL"
    });

    CHECK_TRUE(passed);
    CHECK_TRUE(rttMs <= 1500.0);
}

TEST(LiveFirewallE10, ReadVpnPolicyPool) {
    ZyxelDriver &driver = ZyxelDriver::getInstance();
    if (!driver.isConnected()) return;

    auto t0 = std::chrono::steady_clock::now();
    std::string out;
    SshResult res = driver.getSshClient().executeCommand(ZyxelWlanCmd::cmdShowVpnPolicyPool(), out);
    auto t1 = std::chrono::steady_clock::now();

    CHECK_EQUAL(static_cast<int>(SshResult::SUCCESS), static_cast<int>(res));
    double rttMs = std::chrono::duration<double, std::milli>(t1 - t0).count();
    bool passed = (res == SshResult::SUCCESS) && (out.find("start:") != std::string::npos) && (rttMs <= 1500.0);

    ZyxelBenchmark::record({
        "LiveFirewallE10", "ReadVpnPolicyPool", ZyxelWlanCmd::cmdShowVpnPolicyPool(),
        rttMs, 0, out.size(), 1, passed,
        passed ? "PASS (SLA)" : "FAIL"
    });

    CHECK_TRUE(passed);
    CHECK_TRUE(rttMs <= 1500.0);
}

TEST(LiveFirewallE10, ReadApGroupFirstPriority) {
    ZyxelDriver &driver = ZyxelDriver::getInstance();
    if (!driver.isConnected()) return;

    auto t0 = std::chrono::steady_clock::now();
    std::string out;
    SshResult res = driver.getSshClient().executeCommand(ZyxelWlanCmd::cmdShowApGroupFirstPriority(), out);
    auto t1 = std::chrono::steady_clock::now();

    CHECK_EQUAL(static_cast<int>(SshResult::SUCCESS), static_cast<int>(res));
    double rttMs = std::chrono::duration<double, std::milli>(t1 - t0).count();
    bool passed = (res == SshResult::SUCCESS) && (out.find("Profile Name:") != std::string::npos) && (rttMs <= 1500.0);

    ZyxelBenchmark::record({
        "LiveFirewallE10", "ReadApGroupFirstPriority", ZyxelWlanCmd::cmdShowApGroupFirstPriority(),
        rttMs, 0, out.size(), 1, passed,
        passed ? "PASS (SLA)" : "FAIL"
    });

    CHECK_TRUE(passed);
    CHECK_TRUE(rttMs <= 1500.0);
}

TEST(LiveFirewallE10, ReadApGroupProfileAll) {
    ZyxelDriver &driver = ZyxelDriver::getInstance();
    if (!driver.isConnected()) return;

    auto t0 = std::chrono::steady_clock::now();
    std::string out;
    SshResult res = driver.getSshClient().executeCommand(ZyxelWlanCmd::cmdShowApGroupProfileAll(), out);
    auto t1 = std::chrono::steady_clock::now();

    CHECK_EQUAL(static_cast<int>(SshResult::SUCCESS), static_cast<int>(res));
    double rttMs = std::chrono::duration<double, std::milli>(t1 - t0).count();
    bool passed = (res == SshResult::SUCCESS) && (rttMs <= 1500.0);

    ZyxelBenchmark::record({
        "LiveFirewallE10", "ReadApGroupProfileAll", ZyxelWlanCmd::cmdShowApGroupProfileAll(),
        rttMs, 0, out.size(), 1, passed,
        passed ? "PASS (SLA)" : "FAIL"
    });

    CHECK_TRUE(rttMs <= 1500.0);
}

TEST(LiveFirewallE10, ReadApGroupProfileRuleCount) {
    ZyxelDriver &driver = ZyxelDriver::getInstance();
    if (!driver.isConnected()) return;

    auto t0 = std::chrono::steady_clock::now();
    std::string out;
    SshResult res = driver.getSshClient().executeCommand(ZyxelWlanCmd::cmdShowApGroupProfileRuleCount(), out);
    auto t1 = std::chrono::steady_clock::now();

    CHECK_EQUAL(static_cast<int>(SshResult::SUCCESS), static_cast<int>(res));
    double rttMs = std::chrono::duration<double, std::milli>(t1 - t0).count();
    bool passed = (res == SshResult::SUCCESS) && (out.find("ap group profile count") != std::string::npos || out.find("profile count") != std::string::npos) && (rttMs <= 1500.0);

    ZyxelBenchmark::record({
        "LiveFirewallE10", "ReadApGroupProfileRuleCount", ZyxelWlanCmd::cmdShowApGroupProfileRuleCount(),
        rttMs, 0, out.size(), 1, passed,
        passed ? "PASS (SLA)" : "FAIL"
    });

    CHECK_TRUE(passed);
    CHECK_TRUE(rttMs <= 1500.0);
}

TEST(LiveFirewallE10, ReadWlanMacfilterProfileAll) {
    ZyxelDriver &driver = ZyxelDriver::getInstance();
    if (!driver.isConnected()) return;

    auto t0 = std::chrono::steady_clock::now();
    std::string out;
    SshResult res = driver.getSshClient().executeCommand(ZyxelWlanCmd::cmdShowWlanMacfilterProfileAll(), out);
    auto t1 = std::chrono::steady_clock::now();

    CHECK_EQUAL(static_cast<int>(SshResult::SUCCESS), static_cast<int>(res));
    double rttMs = std::chrono::duration<double, std::milli>(t1 - t0).count();
    bool passed = (res == SshResult::SUCCESS) && (rttMs <= 1500.0);

    ZyxelBenchmark::record({
        "LiveFirewallE10", "ReadWlanMacfilterProfileAll", ZyxelWlanCmd::cmdShowWlanMacfilterProfileAll(),
        rttMs, 0, out.size(), 1, passed,
        passed ? "PASS (SLA)" : "FAIL"
    });

    CHECK_TRUE(rttMs <= 1500.0);
}

TEST(LiveFirewallE10, ReadWlanMonitorProfileAll) {
    ZyxelDriver &driver = ZyxelDriver::getInstance();
    if (!driver.isConnected()) return;

    auto t0 = std::chrono::steady_clock::now();
    std::string out;
    SshResult res = driver.getSshClient().executeCommand(ZyxelWlanCmd::cmdShowWlanMonitorProfileAll(), out);
    auto t1 = std::chrono::steady_clock::now();

    CHECK_EQUAL(static_cast<int>(SshResult::SUCCESS), static_cast<int>(res));
    double rttMs = std::chrono::duration<double, std::milli>(t1 - t0).count();
    bool passed = (res == SshResult::SUCCESS) && (out.find("monitor profile:") != std::string::npos) && (rttMs <= 1500.0);

    ZyxelBenchmark::record({
        "LiveFirewallE10", "ReadWlanMonitorProfileAll", ZyxelWlanCmd::cmdShowWlanMonitorProfileAll(),
        rttMs, 0, out.size(), 1, passed,
        passed ? "PASS (SLA)" : "FAIL"
    });

    CHECK_TRUE(passed);
    CHECK_TRUE(rttMs <= 1500.0);
}

TEST(LiveFirewallE10, ReadWlanRadioProfileAll) {
    ZyxelDriver &driver = ZyxelDriver::getInstance();
    if (!driver.isConnected()) return;

    auto t0 = std::chrono::steady_clock::now();
    std::string out;
    SshResult res = driver.getSshClient().executeCommand(ZyxelWlanCmd::cmdShowWlanRadioProfileAll(), out);
    auto t1 = std::chrono::steady_clock::now();

    CHECK_EQUAL(static_cast<int>(SshResult::SUCCESS), static_cast<int>(res));
    double rttMs = std::chrono::duration<double, std::milli>(t1 - t0).count();
    bool passed = (res == SshResult::SUCCESS) && (out.find("ap profile:") != std::string::npos) && (rttMs <= 1500.0);

    ZyxelBenchmark::record({
        "LiveFirewallE10", "ReadWlanRadioProfileAll", ZyxelWlanCmd::cmdShowWlanRadioProfileAll(),
        rttMs, 0, out.size(), 1, passed,
        passed ? "PASS (SLA)" : "FAIL"
    });

    CHECK_TRUE(passed);
    CHECK_TRUE(rttMs <= 1500.0);
}

TEST(LiveFirewallE10, ReadWlanSecurityProfileAll) {
    ZyxelDriver &driver = ZyxelDriver::getInstance();
    if (!driver.isConnected()) return;

    auto t0 = std::chrono::steady_clock::now();
    std::string out;
    SshResult res = driver.getSshClient().executeCommand(ZyxelWlanCmd::cmdShowWlanSecurityProfileAll(), out);
    auto t1 = std::chrono::steady_clock::now();

    CHECK_EQUAL(static_cast<int>(SshResult::SUCCESS), static_cast<int>(res));
    double rttMs = std::chrono::duration<double, std::milli>(t1 - t0).count();
    bool passed = (res == SshResult::SUCCESS) && (out.find("security profile:") != std::string::npos) && (rttMs <= 1500.0);

    ZyxelBenchmark::record({
        "LiveFirewallE10", "ReadWlanSecurityProfileAll", ZyxelWlanCmd::cmdShowWlanSecurityProfileAll(),
        rttMs, 0, out.size(), 1, passed,
        passed ? "PASS (SLA)" : "FAIL"
    });

    CHECK_TRUE(passed);
    CHECK_TRUE(rttMs <= 1500.0);
}

TEST(LiveFirewallE10, ReadWlanSsidProfileAll) {
    ZyxelDriver &driver = ZyxelDriver::getInstance();
    if (!driver.isConnected()) return;

    auto t0 = std::chrono::steady_clock::now();
    std::string out;
    SshResult res = driver.getSshClient().executeCommand(ZyxelWlanCmd::cmdShowWlanSsidProfileAll(), out);
    auto t1 = std::chrono::steady_clock::now();

    CHECK_EQUAL(static_cast<int>(SshResult::SUCCESS), static_cast<int>(res));
    double rttMs = std::chrono::duration<double, std::milli>(t1 - t0).count();
    bool passed = (res == SshResult::SUCCESS) && (out.find("ssid profile:") != std::string::npos) && (rttMs <= 1500.0);

    ZyxelBenchmark::record({
        "LiveFirewallE10", "ReadWlanSsidProfileAll", ZyxelWlanCmd::cmdShowWlanSsidProfileAll(),
        rttMs, 0, out.size(), 1, passed,
        passed ? "PASS (SLA)" : "FAIL"
    });

    CHECK_TRUE(passed);
    CHECK_TRUE(rttMs <= 1500.0);
}

TEST(LiveFirewallE10, ReadZymeshApInfo) {
    ZyxelDriver &driver = ZyxelDriver::getInstance();
    if (!driver.isConnected()) return;

    auto t0 = std::chrono::steady_clock::now();
    std::string out;
    SshResult res = driver.getSshClient().executeCommand(ZyxelWlanCmd::cmdShowZymeshApInfo(), out);
    auto t1 = std::chrono::steady_clock::now();

    CHECK_EQUAL(static_cast<int>(SshResult::SUCCESS), static_cast<int>(res));

    auto p0 = std::chrono::steady_clock::now();
    ZyxelZyMeshInfo info;
    bool parsed = ZyxelWlanCmd::parseZyMeshInfo(out, info);
    auto p1 = std::chrono::steady_clock::now();

    double rttMs = std::chrono::duration<double, std::milli>(t1 - t0).count();
    uint64_t parseUs = std::chrono::duration_cast<std::chrono::microseconds>(p1 - p0).count();
    bool passed = (res == SshResult::SUCCESS) && parsed && (rttMs <= 1500.0);

    ZyxelBenchmark::record({
        "LiveFirewallE10", "ReadZymeshApInfo", ZyxelWlanCmd::cmdShowZymeshApInfo(),
        rttMs, parseUs, out.size(), 1, passed,
        passed ? "PASS (SLA)" : "FAIL"
    });

    CHECK_TRUE(parsed);
    CHECK_TRUE(rttMs <= 1500.0);
}

TEST(LiveFirewallE10, ReadZymeshProvisionGroup) {
    ZyxelDriver &driver = ZyxelDriver::getInstance();
    if (!driver.isConnected()) return;

    auto t0 = std::chrono::steady_clock::now();
    std::string out;
    SshResult res = driver.getSshClient().executeCommand(ZyxelWlanCmd::cmdShowZymeshProvisionGroup(), out);
    auto t1 = std::chrono::steady_clock::now();

    CHECK_EQUAL(static_cast<int>(SshResult::SUCCESS), static_cast<int>(res));
    double rttMs = std::chrono::duration<double, std::milli>(t1 - t0).count();
    bool passed = (res == SshResult::SUCCESS) && (out.find("Provision Group MAC:") != std::string::npos) && (rttMs <= 1500.0);

    ZyxelBenchmark::record({
        "LiveFirewallE10", "ReadZymeshProvisionGroup", ZyxelWlanCmd::cmdShowZymeshProvisionGroup(),
        rttMs, 0, out.size(), 1, passed,
        passed ? "PASS (SLA)" : "FAIL"
    });

    CHECK_TRUE(passed);
    CHECK_TRUE(rttMs <= 1500.0);
}

TEST(LiveFirewallE10, ReadZymeshProfileAll) {
    ZyxelDriver &driver = ZyxelDriver::getInstance();
    if (!driver.isConnected()) return;

    auto t0 = std::chrono::steady_clock::now();
    std::string out;
    SshResult res = driver.getSshClient().executeCommand(ZyxelWlanCmd::cmdShowZymeshProfileAll(), out);
    auto t1 = std::chrono::steady_clock::now();

    CHECK_EQUAL(static_cast<int>(SshResult::SUCCESS), static_cast<int>(res));
    double rttMs = std::chrono::duration<double, std::milli>(t1 - t0).count();
    bool passed = (res == SshResult::SUCCESS) && (out.find("ZyMesh profile:") != std::string::npos) && (rttMs <= 1500.0);

    ZyxelBenchmark::record({
        "LiveFirewallE10", "ReadZymeshProfileAll", ZyxelWlanCmd::cmdShowZymeshProfileAll(),
        rttMs, 0, out.size(), 1, passed,
        passed ? "PASS (SLA)" : "FAIL"
    });

    CHECK_TRUE(passed);
    CHECK_TRUE(rttMs <= 1500.0);
}

TEST(LiveFirewallE10, ReadRogueApContainmentConfig) {
    ZyxelDriver &driver = ZyxelDriver::getInstance();
    if (!driver.isConnected()) return;

    auto t0 = std::chrono::steady_clock::now();
    std::string out;
    SshResult res = driver.getSshClient().executeCommand(ZyxelWlanCmd::cmdShowRogueApContainmentConfig(), out);
    auto t1 = std::chrono::steady_clock::now();

    CHECK_EQUAL(static_cast<int>(SshResult::SUCCESS), static_cast<int>(res));
    double rttMs = std::chrono::duration<double, std::milli>(t1 - t0).count();
    bool passed = (res == SshResult::SUCCESS) && (out.find("rapc activate:") != std::string::npos) && (rttMs <= 1500.0);

    ZyxelBenchmark::record({
        "LiveFirewallE10", "ReadRogueApContainmentConfig", ZyxelWlanCmd::cmdShowRogueApContainmentConfig(),
        rttMs, 0, out.size(), 1, passed,
        passed ? "PASS (SLA)" : "FAIL"
    });

    CHECK_TRUE(passed);
    CHECK_TRUE(rttMs <= 1500.0);
}

TEST(LiveFirewallE10, ReadRogueApContainmentList) {
    ZyxelDriver &driver = ZyxelDriver::getInstance();
    if (!driver.isConnected()) return;

    auto t0 = std::chrono::steady_clock::now();
    std::string out;
    SshResult res = driver.getSshClient().executeCommand(ZyxelWlanCmd::cmdShowRogueApContainmentList(), out);
    auto t1 = std::chrono::steady_clock::now();

    CHECK_EQUAL(static_cast<int>(SshResult::SUCCESS), static_cast<int>(res));
    double rttMs = std::chrono::duration<double, std::milli>(t1 - t0).count();
    bool passed = (res == SshResult::SUCCESS) && (out.find("no.   mac") != std::string::npos) && (rttMs <= 1500.0);

    ZyxelBenchmark::record({
        "LiveFirewallE10", "ReadRogueApContainmentList", ZyxelWlanCmd::cmdShowRogueApContainmentList(),
        rttMs, 0, out.size(), 1, passed,
        passed ? "PASS (SLA)" : "FAIL"
    });

    CHECK_TRUE(passed);
    CHECK_TRUE(rttMs <= 1500.0);
}

TEST(LiveFirewallE10, ReadRogueApDetectionInfo) {
    ZyxelDriver &driver = ZyxelDriver::getInstance();
    if (!driver.isConnected()) return;

    auto t0 = std::chrono::steady_clock::now();
    std::string out;
    SshResult res = driver.getSshClient().executeCommand(ZyxelWlanCmd::cmdShowRogueApDetectionInfo(), out);
    auto t1 = std::chrono::steady_clock::now();

    CHECK_EQUAL(static_cast<int>(SshResult::SUCCESS), static_cast<int>(res));

    auto p0 = std::chrono::steady_clock::now();
    ZyxelRogueApInfo info;
    bool parsed = ZyxelWlanCmd::parseRogueApInfo(out, info);
    auto p1 = std::chrono::steady_clock::now();

    double rttMs = std::chrono::duration<double, std::milli>(t1 - t0).count();
    uint64_t parseUs = std::chrono::duration_cast<std::chrono::microseconds>(p1 - p0).count();
    bool passed = (res == SshResult::SUCCESS) && parsed && (rttMs <= 1500.0);

    ZyxelBenchmark::record({
        "LiveFirewallE10", "ReadRogueApDetectionInfo", ZyxelWlanCmd::cmdShowRogueApDetectionInfo(),
        rttMs, parseUs, out.size(), 1, passed,
        passed ? "PASS (SLA)" : "FAIL"
    });

    CHECK_TRUE(parsed);
    CHECK_TRUE(rttMs <= 1500.0);
}

TEST(LiveFirewallE10, ReadRogueApDetectionListAll) {
    ZyxelDriver &driver = ZyxelDriver::getInstance();
    if (!driver.isConnected()) return;

    auto t0 = std::chrono::steady_clock::now();
    std::string out;
    SshResult res = driver.getSshClient().executeCommand(ZyxelWlanCmd::cmdShowRogueApDetectionListAll(), out);
    auto t1 = std::chrono::steady_clock::now();

    CHECK_EQUAL(static_cast<int>(SshResult::SUCCESS), static_cast<int>(res));
    double rttMs = std::chrono::duration<double, std::milli>(t1 - t0).count();
    bool passed = (res == SshResult::SUCCESS) && (out.find("role") != std::string::npos || out.find("no.") != std::string::npos) && (rttMs <= 1500.0);

    ZyxelBenchmark::record({
        "LiveFirewallE10", "ReadRogueApDetectionListAll", ZyxelWlanCmd::cmdShowRogueApDetectionListAll(),
        rttMs, 0, out.size(), 1, passed,
        passed ? "PASS (SLA)" : "FAIL"
    });

    CHECK_TRUE(passed);
    CHECK_TRUE(rttMs <= 1500.0);
}

TEST(LiveFirewallE10, ReadRogueApDetectionMonitoring) {
    ZyxelDriver &driver = ZyxelDriver::getInstance();
    if (!driver.isConnected()) return;

    auto t0 = std::chrono::steady_clock::now();
    std::string out;
    SshResult res = driver.getSshClient().executeCommand(ZyxelWlanCmd::cmdShowRogueApDetectionMonitoring(), out);
    auto t1 = std::chrono::steady_clock::now();

    CHECK_EQUAL(static_cast<int>(SshResult::SUCCESS), static_cast<int>(res));
    double rttMs = std::chrono::duration<double, std::milli>(t1 - t0).count();
    bool passed = (res == SshResult::SUCCESS) && (out.find("Role") != std::string::npos) && (rttMs <= 1500.0);

    ZyxelBenchmark::record({
        "LiveFirewallE10", "ReadRogueApDetectionMonitoring", ZyxelWlanCmd::cmdShowRogueApDetectionMonitoring(),
        rttMs, 0, out.size(), 1, passed,
        passed ? "PASS (SLA)" : "FAIL"
    });

    CHECK_TRUE(passed);
    CHECK_TRUE(rttMs <= 1500.0);
}

TEST(LiveFirewallE10, ReadRogueApDetectionStatus) {
    ZyxelDriver &driver = ZyxelDriver::getInstance();
    if (!driver.isConnected()) return;

    auto t0 = std::chrono::steady_clock::now();
    std::string out;
    SshResult res = driver.getSshClient().executeCommand(ZyxelWlanCmd::cmdShowRogueApDetectionStatus(), out);
    auto t1 = std::chrono::steady_clock::now();

    CHECK_EQUAL(static_cast<int>(SshResult::SUCCESS), static_cast<int>(res));

    auto p0 = std::chrono::steady_clock::now();
    ZyxelRogueApStatus status;
    bool parsed = ZyxelWlanCmd::parseRogueApStatus(out, status);
    auto p1 = std::chrono::steady_clock::now();

    double rttMs = std::chrono::duration<double, std::milli>(t1 - t0).count();
    uint64_t parseUs = std::chrono::duration_cast<std::chrono::microseconds>(p1 - p0).count();
    bool passed = (res == SshResult::SUCCESS) && parsed && (rttMs <= 1500.0);

    ZyxelBenchmark::record({
        "LiveFirewallE10", "ReadRogueApDetectionStatus", ZyxelWlanCmd::cmdShowRogueApDetectionStatus(),
        rttMs, parseUs, out.size(), 1, passed,
        passed ? "PASS (SLA)" : "FAIL"
    });

    CHECK_TRUE(parsed);
    CHECK_TRUE(rttMs <= 1500.0);
}

TEST(LiveFirewallE10, ReadWirelessHealthAction) {
    ZyxelDriver &driver = ZyxelDriver::getInstance();
    if (!driver.isConnected()) return;

    auto t0 = std::chrono::steady_clock::now();
    std::string out;
    SshResult res = driver.getSshClient().executeCommand(ZyxelWlanCmd::cmdShowWirelessHealthAction(), out);
    auto t1 = std::chrono::steady_clock::now();

    CHECK_EQUAL(static_cast<int>(SshResult::SUCCESS), static_cast<int>(res));
    double rttMs = std::chrono::duration<double, std::milli>(t1 - t0).count();
    bool passed = (res == SshResult::SUCCESS) && (out.find("aggressiveness:") != std::string::npos) && (rttMs <= 1500.0);

    ZyxelBenchmark::record({
        "LiveFirewallE10", "ReadWirelessHealthAction", ZyxelWlanCmd::cmdShowWirelessHealthAction(),
        rttMs, 0, out.size(), 1, passed,
        passed ? "PASS (SLA)" : "FAIL"
    });

    CHECK_TRUE(passed);
    CHECK_TRUE(rttMs <= 1500.0);
}

TEST(LiveFirewallE10, ReadFrameCaptureConfig) {
    ZyxelDriver &driver = ZyxelDriver::getInstance();
    if (!driver.isConnected()) return;

    auto t0 = std::chrono::steady_clock::now();
    std::string out;
    SshResult res = driver.getSshClient().executeCommand(ZyxelWlanCmd::cmdShowFrameCaptureConfig(), out);
    auto t1 = std::chrono::steady_clock::now();

    CHECK_EQUAL(static_cast<int>(SshResult::SUCCESS), static_cast<int>(res));

    auto p0 = std::chrono::steady_clock::now();
    ZyxelFrameCaptureConfig config;
    bool parsed = ZyxelWlanCmd::parseFrameCaptureConfig(out, config);
    auto p1 = std::chrono::steady_clock::now();

    double rttMs = std::chrono::duration<double, std::milli>(t1 - t0).count();
    uint64_t parseUs = std::chrono::duration_cast<std::chrono::microseconds>(p1 - p0).count();
    bool passed = (res == SshResult::SUCCESS) && parsed && (rttMs <= 1500.0);

    ZyxelBenchmark::record({
        "LiveFirewallE10", "ReadFrameCaptureConfig", ZyxelWlanCmd::cmdShowFrameCaptureConfig(),
        rttMs, parseUs, out.size(), 1, passed,
        passed ? "PASS (SLA)" : "FAIL"
    });

    CHECK_TRUE(parsed);
    CHECK_TRUE(rttMs <= 1500.0);
}

TEST(LiveFirewallE10, ReadFrameCaptureStatus) {
    ZyxelDriver &driver = ZyxelDriver::getInstance();
    if (!driver.isConnected()) return;

    auto t0 = std::chrono::steady_clock::now();
    std::string out;
    SshResult res = driver.getSshClient().executeCommand(ZyxelWlanCmd::cmdShowFrameCaptureStatus(), out);
    auto t1 = std::chrono::steady_clock::now();

    CHECK_EQUAL(static_cast<int>(SshResult::SUCCESS), static_cast<int>(res));
    double rttMs = std::chrono::duration<double, std::milli>(t1 - t0).count();
    bool passed = (res == SshResult::SUCCESS) && (out.find("capture status:") != std::string::npos) && (rttMs <= 1500.0);

    ZyxelBenchmark::record({
        "LiveFirewallE10", "ReadFrameCaptureStatus", ZyxelWlanCmd::cmdShowFrameCaptureStatus(),
        rttMs, 0, out.size(), 1, passed,
        passed ? "PASS (SLA)" : "FAIL"
    });

    CHECK_TRUE(passed);
    CHECK_TRUE(rttMs <= 1500.0);
}

TEST(LiveFirewallE10, ReadAutoHealingConfig) {
    ZyxelDriver &driver = ZyxelDriver::getInstance();
    if (!driver.isConnected()) return;

    auto t0 = std::chrono::steady_clock::now();
    std::string out;
    SshResult res = driver.getSshClient().executeCommand(ZyxelWlanCmd::cmdShowAutoHealingConfig(), out);
    auto t1 = std::chrono::steady_clock::now();

    CHECK_EQUAL(static_cast<int>(SshResult::SUCCESS), static_cast<int>(res));

    auto p0 = std::chrono::steady_clock::now();
    ZyxelAutoHealingConfig config;
    bool parsed = ZyxelWlanCmd::parseAutoHealingConfig(out, config);
    auto p1 = std::chrono::steady_clock::now();

    double rttMs = std::chrono::duration<double, std::milli>(t1 - t0).count();
    uint64_t parseUs = std::chrono::duration_cast<std::chrono::microseconds>(p1 - p0).count();
    bool passed = (res == SshResult::SUCCESS) && parsed && (rttMs <= 1500.0);

    ZyxelBenchmark::record({
        "LiveFirewallE10", "ReadAutoHealingConfig", ZyxelWlanCmd::cmdShowAutoHealingConfig(),
        rttMs, parseUs, out.size(), 1, passed,
        passed ? "PASS (SLA)" : "FAIL"
    });

    CHECK_TRUE(parsed);
    CHECK_TRUE(rttMs <= 1500.0);
}

TEST(LiveFirewallE10, ReadCapwapApAcIp) {
    ZyxelDriver &driver = ZyxelDriver::getInstance();
    if (!driver.isConnected()) return;

    auto t0 = std::chrono::steady_clock::now();
    std::string out;
    SshResult res = driver.getSshClient().executeCommand(ZyxelWlanCmd::cmdShowCapwapApAcIp(), out);
    auto t1 = std::chrono::steady_clock::now();

    CHECK_EQUAL(static_cast<int>(SshResult::SUCCESS), static_cast<int>(res));
    double rttMs = std::chrono::duration<double, std::milli>(t1 - t0).count();
    bool passed = (res == SshResult::SUCCESS) && (out.find("AC IP:") != std::string::npos) && (rttMs <= 1500.0);

    ZyxelBenchmark::record({
        "LiveFirewallE10", "ReadCapwapApAcIp", ZyxelWlanCmd::cmdShowCapwapApAcIp(),
        rttMs, 0, out.size(), 1, passed,
        passed ? "PASS (SLA)" : "FAIL"
    });

    CHECK_TRUE(passed);
    CHECK_TRUE(rttMs <= 1500.0);
}

TEST(LiveFirewallE10, ReadCapwapApInfo) {
    ZyxelDriver &driver = ZyxelDriver::getInstance();
    if (!driver.isConnected()) return;

    auto t0 = std::chrono::steady_clock::now();
    std::string out;
    SshResult res = driver.getSshClient().executeCommand(ZyxelWlanCmd::cmdShowCapwapApInfo(), out);
    auto t1 = std::chrono::steady_clock::now();

    CHECK_EQUAL(static_cast<int>(SshResult::SUCCESS), static_cast<int>(res));

    auto p0 = std::chrono::steady_clock::now();
    ZyxelCapwapApInfo info;
    bool parsed = ZyxelWlanCmd::parseCapwapApInfo(out, info);
    auto p1 = std::chrono::steady_clock::now();

    double rttMs = std::chrono::duration<double, std::milli>(t1 - t0).count();
    uint64_t parseUs = std::chrono::duration_cast<std::chrono::microseconds>(p1 - p0).count();
    bool passed = (res == SshResult::SUCCESS) && parsed && (rttMs <= 1500.0);

    ZyxelBenchmark::record({
        "LiveFirewallE10", "ReadCapwapApInfo", ZyxelWlanCmd::cmdShowCapwapApInfo(),
        rttMs, parseUs, out.size(), 1, passed,
        passed ? "PASS (SLA)" : "FAIL"
    });

    CHECK_TRUE(parsed);
    CHECK_TRUE(rttMs <= 1500.0);
}

TEST(LiveFirewallE10, DryFireCapwapApAllConfig) {
    ZyxelDriver &driver = ZyxelDriver::getInstance();
    if (!driver.isConnected()) return;

    auto t0 = std::chrono::steady_clock::now();
    std::string out;
    SshResult res = driver.getSshClient().executeCommand("show capwap ap all config", out);
    auto t1 = std::chrono::steady_clock::now();

    bool rejected = (res == SshResult::ERR_SYNTAX || out.find("Parse error") != std::string::npos || out.find("ERROR") != std::string::npos);
    double rttMs = std::chrono::duration<double, std::milli>(t1 - t0).count();
    bool passed = rejected && (rttMs <= 2500.0);

    ZyxelBenchmark::record({
        "LiveFirewallE10", "DryFireCapwapApAllConfig", "show capwap ap all config",
        rttMs, 0, out.size(), rejected ? 1U : 0U, passed,
        passed ? "PASS (REJECT)" : "FAIL"
    });

    CHECK_TRUE(rejected);
    CHECK_TRUE(rttMs <= 2500.0);
}

TEST(LiveFirewallE10, DryFireApInfoTopAlert) {
    ZyxelDriver &driver = ZyxelDriver::getInstance();
    if (!driver.isConnected()) return;

    auto t0 = std::chrono::steady_clock::now();
    std::string out;
    SshResult res = driver.getSshClient().executeCommand(ZyxelWlanCmd::cmdShowApInfoTopAlert("all"), out);
    auto t1 = std::chrono::steady_clock::now();

    bool rejected = (res == SshResult::ERR_SYNTAX || out.find("Parse error") != std::string::npos || out.find("ERROR") != std::string::npos);
    double rttMs = std::chrono::duration<double, std::milli>(t1 - t0).count();
    bool passed = rejected && (rttMs <= 2500.0);

    ZyxelBenchmark::record({
        "LiveFirewallE10", "DryFireApInfoTopAlert", ZyxelWlanCmd::cmdShowApInfoTopAlert("all"),
        rttMs, 0, out.size(), rejected ? 1U : 0U, passed,
        passed ? "PASS (REJECT)" : "FAIL"
    });

    CHECK_TRUE(rejected);
    CHECK_TRUE(rttMs <= 2500.0);
}

TEST(LiveFirewallE10, DryFireCapwapApDiscoveryType) {
    ZyxelDriver &driver = ZyxelDriver::getInstance();
    if (!driver.isConnected()) return;

    auto t0 = std::chrono::steady_clock::now();
    std::string out;
    SshResult res = driver.getSshClient().executeCommand(ZyxelWlanCmd::cmdShowCapwapApDiscoveryType(), out);
    auto t1 = std::chrono::steady_clock::now();

    bool rejected = (res == SshResult::ERR_SYNTAX || out.find("Parse error") != std::string::npos || out.find("ERROR") != std::string::npos);
    double rttMs = std::chrono::duration<double, std::milli>(t1 - t0).count();
    bool passed = rejected && (rttMs <= 2500.0);

    ZyxelBenchmark::record({
        "LiveFirewallE10", "DryFireCapwapApDiscoveryType", ZyxelWlanCmd::cmdShowCapwapApDiscoveryType(),
        rttMs, 0, out.size(), rejected ? 1U : 0U, passed,
        passed ? "PASS (REJECT)" : "FAIL"
    });

    CHECK_TRUE(rejected);
    CHECK_TRUE(rttMs <= 2500.0);
}

TEST(LiveFirewallE10, WriteWlanSsidProfileRollback) {
    ZyxelDriver &driver = ZyxelDriver::getInstance();
    if (!driver.isConnected()) return;
    ZyxelSshClient &ssh = driver.getSshClient();

    auto t0 = std::chrono::steady_clock::now();
    std::string out;

    // 1. Create SSID profile
    ssh.executeCommand("configure terminal", out);
    ssh.executeCommand(ZyxelWlanCmd::cmdWlanSsidProfile("NETMON_QA_SSID_E10"), out);
    ssh.executeCommand("ssid NETMON_TEST_SSID", out);
    ssh.executeCommand("exit", out);
    ssh.executeCommand("exit", out);
    ssh.unwindToRootPrompt();

    // 2. Verify creation
    ssh.executeCommand("show wlan-ssid-profile NETMON_QA_SSID_E10", out);
    bool created = (out.find("ssid profile: NETMON_QA_SSID_E10") != std::string::npos ||
                    out.find("NETMON_TEST_SSID") != std::string::npos);

    // 3. Rollback
    ssh.executeCommand("configure terminal", out);
    SshResult resDel = ssh.executeCommand(ZyxelWlanCmd::cmdNoWlanSsidProfile("NETMON_QA_SSID_E10"), out);
    ssh.executeCommand("exit", out);
    ssh.unwindToRootPrompt();

    CHECK_EQUAL(static_cast<int>(SshResult::SUCCESS), static_cast<int>(resDel));

    // 4. Verify clean deletion
    ssh.executeCommand("show wlan-ssid-profile NETMON_QA_SSID_E10", out);
    bool deleted = (out.find("ssid profile: NETMON_QA_SSID_E10") == std::string::npos);
    auto t1 = std::chrono::steady_clock::now();

    double rttMs = std::chrono::duration<double, std::milli>(t1 - t0).count();
    bool passed = created && deleted && (rttMs <= 6000.0);

    ZyxelBenchmark::record({
        "LiveFirewallE10", "WriteWlanSsidProfileRollback", "wlan-ssid-profile NETMON_QA_SSID_E10",
        rttMs, 0, out.size(), 1, passed,
        passed ? "PASS (ROLLBACK)" : "FAIL"
    });

    CHECK_TRUE(created);
    CHECK_TRUE(deleted);
    CHECK_TRUE(rttMs <= 6000.0);
}

TEST(LiveFirewallE10, WriteWlanSecurityProfileRollback) {
    ZyxelDriver &driver = ZyxelDriver::getInstance();
    if (!driver.isConnected()) return;
    ZyxelSshClient &ssh = driver.getSshClient();

    auto t0 = std::chrono::steady_clock::now();
    std::string out;

    // 1. Create Security profile
    ssh.executeCommand("configure terminal", out);
    ssh.executeCommand(ZyxelWlanCmd::cmdWlanSecurityProfile("NETMON_QA_SEC_E10"), out);
    ssh.executeCommand("exit", out);
    ssh.executeCommand("exit", out);
    ssh.unwindToRootPrompt();

    // 2. Verify creation
    ssh.executeCommand("show wlan-security-profile NETMON_QA_SEC_E10", out);
    bool created = (out.find("security profile: NETMON_QA_SEC_E10") != std::string::npos);

    // 3. Rollback
    ssh.executeCommand("configure terminal", out);
    SshResult resDel = ssh.executeCommand(ZyxelWlanCmd::cmdNoWlanSecurityProfile("NETMON_QA_SEC_E10"), out);
    ssh.executeCommand("exit", out);
    ssh.unwindToRootPrompt();

    CHECK_EQUAL(static_cast<int>(SshResult::SUCCESS), static_cast<int>(resDel));

    // 4. Verify clean deletion
    ssh.executeCommand("show wlan-security-profile NETMON_QA_SEC_E10", out);
    bool deleted = (out.find("security profile: NETMON_QA_SEC_E10") == std::string::npos);
    auto t1 = std::chrono::steady_clock::now();

    double rttMs = std::chrono::duration<double, std::milli>(t1 - t0).count();
    bool passed = created && deleted && (rttMs <= 6000.0);

    ZyxelBenchmark::record({
        "LiveFirewallE10", "WriteWlanSecurityProfileRollback", "wlan-security-profile NETMON_QA_SEC_E10",
        rttMs, 0, out.size(), 1, passed,
        passed ? "PASS (ROLLBACK)" : "FAIL"
    });

    CHECK_TRUE(created);
    CHECK_TRUE(deleted);
    CHECK_TRUE(rttMs <= 6000.0);
}

// ============================================================================
// Public Entry Point: runLiveFirewallDiagnostic
// ============================================================================

int __attribute__((weak)) runLiveFirewallDiagnostic(std::ostream &os,
                                                      const std::string &mode,
                                                      const std::string &filter) {
    ZyxelDriver &driver = ZyxelDriver::getInstance();
    if (!driver.isConfigured()) {
        os << "Error: Firewall driver is not configured in netmon.conf.\n";
        return -1;
    }

    std::string pw;
    if (!AuthManager::getInstance().getRouterPassword(pw) || pw.empty()) {
        os << "Error: No firewall password found in encrypted vault. Use 'firewall set-password' first.\n";
        return -2;
    }

    if (!driver.isConnected()) {
        os << "Error: Firewall driver is not connected to remote host.\n";
        return -3;
    }

    // Acquire diagnostic lock: isolates channel, suspends keepalive, rejects clearance requests
    ZyxelDiagnosticGuard guard(driver);
    if (!guard.isAcquired()) {
        os << "Error: Firewall diagnostic is already active.\n";
        return -4;
    }

    os << "Acquired diagnostic channel lock. Keepalive suspended; client clearance requests busy.\n";
    os << "Executing live firewall diagnostic suite (mode=" << mode << ")...\n\n";

    ZyxelBenchmark::clear();

    auto mapEnvelope = [](const std::string &token) -> std::string {
        std::string lower = token;
        std::transform(lower.begin(), lower.end(), lower.begin(), ::tolower);
        if (lower == "e1") return "LiveFirewallE1";
        if (lower == "e2") return "LiveFirewallE2";
        if (lower == "e3") return "LiveFirewallE3";
        if (lower == "e4") return "LiveFirewallE4";
        if (lower == "e5") return "LiveFirewallE5";
        if (lower == "e6") return "LiveFirewallE6";
        if (lower == "e7") return "LiveFirewallE7";
        if (lower == "e8") return "LiveFirewallE8";
        if (lower == "e9") return "LiveFirewallE9";
        if (lower == "e10") return "LiveFirewallE10";
        return "";
    };

    std::vector<std::string> args = { "netmon_diag", "-v" };

    std::string envFromFilter = mapEnvelope(filter);
    std::string envFromMode = mapEnvelope(mode);

    if (!envFromFilter.empty()) {
        args.push_back("-sg");
        args.push_back(envFromFilter);
    } else if (!envFromMode.empty()) {
        args.push_back("-sg");
        args.push_back(envFromMode);
    } else if (mode == "read") {
        args.push_back("-sg");
        args.push_back("LiveFirewallReadDiag");
    } else if (mode == "write") {
        args.push_back("-sg");
        args.push_back("LiveFirewallWriteDiag");
    }
    // For "all" (and not an envelope token), omit -sg so CppUTest executes all registered groups

    if (!filter.empty() && envFromFilter.empty()) {
        args.push_back("-sn");
        args.push_back(filter);
    }

    std::vector<const char *> argv;
    for (const auto &arg : args) {
        argv.push_back(arg.c_str());
    }

    int testResult = CommandLineTestRunner::RunAllTests(static_cast<int>(argv.size()), argv.data());

    std::string targetHost = "gateway";
    if (!Config::getInstance().getSnmpTargets().empty()) {
        targetHost = Config::getInstance().getSnmpTargets()[0].ip;
    }
    os << "\n";
    ZyxelBenchmark::renderScorecard(os, targetHost, "USG FLEX 200");

    return testResult;
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
