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

    // 2. Insert rule 1 using ZyxelFirewallCmd::cmdInsertFastDeny
    ssh.executeCommand("configure terminal", out);
    std::vector<std::string> cmds = ZyxelFirewallCmd::cmdInsertFastDeny(1, "NETMON_QA_RULE", "NETMON_QA_RULE_HOST", "Live diag QA test");
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
    ssh.executeCommand(ZyxelFirewallCmd::cmdDeleteRule("NETMON_QA_RULE"), out);
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

    std::vector<std::string> args = { "netmon_diag", "-v" };
    if (mode == "read") {
        args.push_back("-sg");
        args.push_back("LiveFirewallReadDiag");
    } else if (mode == "write") {
        args.push_back("-sg");
        args.push_back("LiveFirewallWriteDiag");
    }
    // For "all", omit -sg so CppUTest executes both LiveFirewallReadDiag and LiveFirewallWriteDiag

    if (!filter.empty()) {
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
