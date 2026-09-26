/*
 * TestSyslogIngest.cxx
 *
 * Copyright (C) 2026, Charles Chiou
 */

#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <cstring>
#include <string>
#include <vector>
#include <filesystem>
#include <chrono>
#include <thread>
#include <nlohmann/json.hpp>

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-parameter"
#pragma GCC diagnostic ignored "-Wshadow"
#include <httplib.h>
#pragma GCC diagnostic pop

#include "SyslogServer.hxx"
#include "SnmpDatabase.hxx"
#include "SecurityCheckpoint.hxx"
#include "WebServer.hxx"
#include "AuthManager.hxx"
#include "Config.hxx"
#include "NetMonShell.hxx"
#include "RecordingRouter.hxx"

#include <CppUTest/TestHarness.h>

namespace fs = std::filesystem;
using json = nlohmann::json;

static void sendUdpDatagram(const std::string &host, int port, const std::string &msg) {
    int s = socket(AF_INET, SOCK_DGRAM, 0);
    if (s < 0) return;
    struct sockaddr_in dst;
    std::memset(&dst, 0, sizeof(dst));
    dst.sin_family = AF_INET;
    dst.sin_port = htons(port);
    inet_pton(AF_INET, host.c_str(), &dst.sin_addr);
    sendto(s, msg.data(), msg.size(), 0, reinterpret_cast<const struct sockaddr *>(&dst), sizeof(dst));
    close(s);
}

TEST_GROUP(SyslogParser) {
    void setup() {
    }

    void teardown() {
    }
};

TEST(SyslogParser, AcceptsRfc3164FromRouter) {
    SyslogEvent ev;
    std::string raw = "<134>Feb 28 12:00:00 myrouter myproc[123]: session closed for user admin";
    CHECK_TRUE(SyslogParser::parse(raw, ev));
    CHECK_EQUAL(16, ev.facility);
    CHECK_EQUAL(6, ev.severity);
    CHECK_EQUAL("myproc[123]", ev.tag);
    CHECK_EQUAL("session closed for user admin", ev.message);
    CHECK_TRUE(ev.timestamp > 0);
}

TEST(SyslogParser, AcceptsRfc5424FromRouter) {
    SyslogEvent ev;
    std::string raw = "<165>1 2026-09-26T22:14:15.003Z myrouter myproc 1234 ID47 - session closed for user admin";
    CHECK_TRUE(SyslogParser::parse(raw, ev));
    CHECK_EQUAL(20, ev.facility);
    CHECK_EQUAL(5, ev.severity);
    CHECK_EQUAL("myproc[1234]", ev.tag);
    CHECK_EQUAL("session closed for user admin", ev.message);
    CHECK_TRUE(ev.timestamp > 0);
}

TEST(SyslogParser, DropsEmptyDatagram) {
    SyslogEvent ev;
    CHECK_FALSE(SyslogParser::parse("", ev));
    CHECK_FALSE(SyslogParser::parse(nullptr, 0, ev));
}

TEST(SyslogParser, DropsOversizedDatagram) {
    SyslogEvent ev;
    std::string huge(3000, 'A');
    CHECK_FALSE(SyslogParser::parse(huge, ev));

    std::string borderline = "<14>" + std::string(SyslogParser::MAX_DATAGRAM_SIZE + 10, 'x');
    CHECK_FALSE(SyslogParser::parse(borderline, ev));
}

TEST(SyslogParser, DropsMalformedPriority) {
    SyslogEvent ev;
    CHECK_FALSE(SyslogParser::parse("<>message", ev));
    CHECK_FALSE(SyslogParser::parse("<abc>message", ev));
    CHECK_FALSE(SyslogParser::parse("<999>message", ev));
    CHECK_FALSE(SyslogParser::parse("message without priority", ev));
    CHECK_FALSE(SyslogParser::parse("<14incomplete", ev));
}

TEST_GROUP(Integration_SyslogFilter) {
    std::string testVaultDir = "test_data/syslog_vault";

    void setup() {
        WebServer::getInstance().stop();
        WebServer::getInstance().join();
        SyslogServer::getInstance().resetForTesting();

        unlink("test_data/test_syslog.db");
        unlink("test_data/test_syslog.db-shm");
        unlink("test_data/test_syslog.db-wal");
        unlink("test_data/test_syslog.log");
        unlink("test_data/test_syslog.log.tmp");

        SnmpDatabase::getInstance().close();
        SnmpDatabase::getInstance().open("test_data/test_syslog.db");

        SecurityCheckpoint::getInstance().resetForTesting();
        SecurityCheckpoint::getInstance().setAuditFilePath("test_data/test_syslog.log");

        NetMonShell::getInstance().resetForTesting();
        AuthManager::getInstance().resetForTesting();
        if (fs::exists(testVaultDir)) {
            fs::remove_all(testVaultDir);
        }
        fs::create_directories(testVaultDir);
        AuthManager::getInstance().setVaultDir(testVaultDir);
        AuthManager::getInstance().setPassword("SyslogAdminPass2026!", "", false);

        RecordingRouter::getInstance().reset();
    }

    void teardown() {
        WebServer::getInstance().stop();
        WebServer::getInstance().join();
        SyslogServer::getInstance().resetForTesting();

        SecurityCheckpoint::getInstance().resetForTesting();
        SnmpDatabase::getInstance().close();

        NetMonShell::getInstance().resetForTesting();
        AuthManager::getInstance().resetForTesting();
        if (fs::exists(testVaultDir)) {
            fs::remove_all(testVaultDir);
        }

        unlink("test_data/test_syslog.db");
        unlink("test_data/test_syslog.db-shm");
        unlink("test_data/test_syslog.db-wal");
        unlink("test_data/test_syslog.log");
        unlink("test_data/test_syslog.log.tmp");

        RecordingRouter::getInstance().reset();
    }
};

TEST(Integration_SyslogFilter, DropsNonRouterSource) {
    CHECK_TRUE(SyslogServer::getInstance().start("127.0.0.1", 15140));
    std::this_thread::sleep_for(std::chrono::milliseconds(30));

    // Configure router address to 192.168.1.1 (spoofed/non-router relative to 127.0.0.1 sender)
    SyslogServer::getInstance().setRouterAddress("192.168.1.1");

    sendUdpDatagram("127.0.0.1", SyslogServer::getInstance().getPort(),
                    "<134>Feb 28 12:00:00 spoofed zyxel: unauthorized packet");
    std::this_thread::sleep_for(std::chrono::milliseconds(60));

    auto events = SnmpDatabase::getInstance().getSyslogEvents();
    CHECK_EQUAL(0, events.size());

    // Now configure router address to 127.0.0.1 (matching sender)
    SyslogServer::getInstance().setRouterAddress("127.0.0.1");

    sendUdpDatagram("127.0.0.1", SyslogServer::getInstance().getPort(),
                    "<134>Feb 28 12:00:00 validrouter zyxel: legitimate packet");
    std::this_thread::sleep_for(std::chrono::milliseconds(60));

    events = SnmpDatabase::getInstance().getSyslogEvents();
    CHECK_EQUAL(1, events.size());
    CHECK_EQUAL("legitimate packet", events[0].message);

    SyslogServer::getInstance().stop();
    SyslogServer::getInstance().join();
}

TEST(Integration_SyslogFilter, TimeAdjacentDoesNotChangeAuditStatus) {
    // Create an action and approve it
    PendingAction pa;
    pa.createdAt = std::time(nullptr);
    pa.expiresAt = pa.createdAt + 600;
    pa.requester = "aimon";
    pa.tool = "firewall_block_ip";
    pa.payload = "{\"ip\":\"192.168.1.100\",\"reason\":\"test\"}";
    pa.status = "pending";
    int64_t actionId = SnmpDatabase::getInstance().insertPendingAction(pa);
    CHECK_TRUE(actionId > 0);

    std::string auditOut;
    bool ok = SecurityCheckpoint::getInstance().approve(actionId, auditOut);
    if (!ok) {
        std::cerr << "approve failed with error: " << auditOut << std::endl;
    }
    CHECK_TRUE(ok);

    // Action is now approved in SQLite
    PendingAction checkAction;
    CHECK_TRUE(SnmpDatabase::getInstance().getPendingAction(actionId, checkAction));
    CHECK_EQUAL("approved", checkAction.status);

    AuditOutboxRecord r;
    CHECK_TRUE(SnmpDatabase::getInstance().getLastAuditOutbox(r));
    CHECK_EQUAL("approved", r.decision);
    int64_t auditSeq = r.sequence;

    // Ingest a syslog event within 10 seconds of the audit event
    SyslogEvent ev;
    ev.timestamp = r.timestamp;
    ev.sourceIp = "192.168.1.1";
    ev.facility = 1;
    ev.severity = 5;
    ev.tag = "kernel";
    ev.message = "DROP IN=wan0 OUT= SRC=192.168.1.100";
    ev.raw = "<13>Feb 28 12:00:00 router kernel: DROP IN=wan0 OUT= SRC=192.168.1.100";

    int64_t evId = SnmpDatabase::getInstance().insertSyslogEvent(ev);
    CHECK_TRUE(evId > 0);
    CHECK_TRUE(ev.timeAdjacent);
    CHECK_EQUAL(auditSeq, ev.adjacentAuditSeq);

    // Audit status in SQLite and ticket status remain unchanged
    PendingAction checkActionAfter;
    CHECK_TRUE(SnmpDatabase::getInstance().getPendingAction(actionId, checkActionAfter));
    CHECK_EQUAL("approved", checkActionAfter.status);

    AuditOutboxRecord rAfter;
    CHECK_TRUE(SnmpDatabase::getInstance().getLastAuditOutbox(rAfter));
    CHECK_EQUAL("approved", rAfter.decision);
    CHECK_EQUAL(auditSeq, rAfter.sequence);
}

TEST(Integration_SyslogFilter, AuthenticatedLoopbackReadReturnsLabel) {
    CHECK_TRUE(WebServer::getInstance().start("127.0.0.1", 3884, 3886));
    std::this_thread::sleep_for(std::chrono::milliseconds(50));

    httplib::Client cli("127.0.0.1", 3886);
    cli.set_connection_timeout(1, 0);
    cli.set_read_timeout(2, 0);

    // Unauthenticated read must be rejected
    auto resUnauth = cli.Get("/api/admin/syslog");
    CHECK_TRUE(resUnauth != nullptr);
    CHECK_EQUAL(401, resUnauth->status);

    // Authenticate
    auto resLogin = cli.Post("/api/auth/login", "{\"password\":\"SyslogAdminPass2026!\"}", "application/json");
    CHECK_TRUE(resLogin != nullptr);
    CHECK_EQUAL(200, resLogin->status);
    json loginJson = json::parse(resLogin->body);
    std::string token = loginJson.value("token", "");
    CHECK_FALSE(token.empty());

    httplib::Headers headers = {
        {"Authorization", "Bearer " + token}
    };

    // Insert an audit record
    AuditOutboxRecord auditRec;
    auditRec.timestamp = std::time(nullptr);
    auditRec.prevHash = "";
    auditRec.tool = "block_ip";
    auditRec.requester = "operator";
    auditRec.payload = "{\"ip\":\"192.168.1.200\"}";
    auditRec.decision = "approved";
    auditRec.reason = "CLI approved";
    int64_t aSeq = SnmpDatabase::getInstance().insertAuditOutbox(auditRec);
    CHECK_TRUE(aSeq > 0);

    // Insert a time-adjacent syslog event
    SyslogEvent ev;
    ev.timestamp = auditRec.timestamp;
    ev.sourceIp = "192.168.1.1";
    ev.facility = 4;
    ev.severity = 6;
    ev.tag = "firewall";
    ev.message = "RULE_MATCH block 192.168.1.200";
    ev.raw = "<38>Feb 28 12:00:00 firewall: RULE_MATCH block 192.168.1.200";
    int64_t evId = SnmpDatabase::getInstance().insertSyslogEvent(ev);
    CHECK_TRUE(evId > 0);

    auto resAuth = cli.Get("/api/admin/syslog", headers);
    CHECK_TRUE(resAuth != nullptr);
    CHECK_EQUAL(200, resAuth->status);

    json resJson = json::parse(resAuth->body);
    CHECK_EQUAL("ok", resJson.value("status", ""));
    CHECK_TRUE(resJson["events"].is_array());
    CHECK_TRUE(!resJson["events"].empty());

    bool foundAdj = false;
    for (const auto &item : resJson["events"]) {
        if (item.value("id", 0) == evId) {
            CHECK_TRUE(item.value("time_adjacent", false));
            CHECK_EQUAL(aSeq, item.value("adjacent_audit_seq", 0));
            foundAdj = true;
        }
    }
    CHECK_TRUE(foundAdj);

    cli.stop();
    WebServer::getInstance().stop();
    WebServer::getInstance().join();
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
