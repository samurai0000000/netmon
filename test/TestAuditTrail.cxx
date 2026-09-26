/*
 * TestAuditTrail.cxx
 *
 * Copyright (C) 2026, Charles Chiou
 */

#include <unistd.h>
#include <fstream>
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

#include "SecurityCheckpoint.hxx"
#include "SnmpDatabase.hxx"
#include "RecordingRouter.hxx"
#include "AuthManager.hxx"
#include "WebServer.hxx"
#include "NetMonShell.hxx"

#include <CppUTest/TestHarness.h>

namespace fs = std::filesystem;

TEST_GROUP(AuditChain) {
    void setup() {
        unlink("test_data/test_audit_chain.db");
        unlink("test_data/test_audit_chain.db-shm");
        unlink("test_data/test_audit_chain.db-wal");
        unlink("test_data/test_audit_chain.log");
        unlink("test_data/test_audit_chain.log.tmp");

        SnmpDatabase::getInstance().close();
        SnmpDatabase::getInstance().open("test_data/test_audit_chain.db");

        SecurityCheckpoint::getInstance().resetForTesting();
        SecurityCheckpoint::getInstance().setAuditFilePath("test_data/test_audit_chain.log");

        RecordingRouter::getInstance().reset();
    }

    void teardown() {
        SecurityCheckpoint::getInstance().resetForTesting();
        SnmpDatabase::getInstance().close();

        unlink("test_data/test_audit_chain.db");
        unlink("test_data/test_audit_chain.db-shm");
        unlink("test_data/test_audit_chain.db-wal");
        unlink("test_data/test_audit_chain.log");
        unlink("test_data/test_audit_chain.log.tmp");

        RecordingRouter::getInstance().reset();
    }
};

TEST(AuditChain, ApproveWritesSqliteAndOutbox) {
    int64_t id = SecurityCheckpoint::getInstance().enqueuePendingAction(
        "firewall_block_ip", "ai_agent",
        {{"ip", "192.168.1.180"}, {"reason", "audit approve"}}, 3600);
    CHECK(id > 0);

    std::string outErr;
    CHECK_TRUE(SecurityCheckpoint::getInstance().approve(id, outErr));

    PendingAction act;
    CHECK_TRUE(SecurityCheckpoint::getInstance().getTicket(id, act));
    CHECK_EQUAL(std::string("approved"), act.status);

    AuditOutboxRecord r;
    CHECK_TRUE(SnmpDatabase::getInstance().getLastAuditOutbox(r));
    CHECK_EQUAL(id, r.actionId);
    CHECK_EQUAL(std::string("approved"), r.decision);
    CHECK_EQUAL(std::string("firewall_block_ip"), r.tool);
    CHECK_EQUAL(std::string("ai_agent"), r.requester);
}

TEST(AuditChain, DenyWritesSqliteAndOutbox) {
    int64_t id = SecurityCheckpoint::getInstance().enqueuePendingAction(
        "firewall_block_ip", "ai_agent",
        {{"ip", "192.168.1.181"}, {"reason", "audit deny"}}, 3600);
    CHECK(id > 0);

    CHECK_TRUE(SecurityCheckpoint::getInstance().deny(id, "Rejected by policy"));

    PendingAction act;
    CHECK_TRUE(SecurityCheckpoint::getInstance().getTicket(id, act));
    CHECK_EQUAL(std::string("denied"), act.status);

    AuditOutboxRecord r;
    CHECK_TRUE(SnmpDatabase::getInstance().getLastAuditOutbox(r));
    CHECK_EQUAL(id, r.actionId);
    CHECK_EQUAL(std::string("denied"), r.decision);
    CHECK_EQUAL(std::string("Rejected by policy"), r.reason);
}

TEST(AuditChain, FirstRecordHasEmptyPreviousHash) {
    int64_t id = SecurityCheckpoint::getInstance().enqueuePendingAction(
        "firewall_block_ip", "ai_agent",
        {{"ip", "192.168.1.182"}}, 3600);
    CHECK(id > 0);

    std::string outErr;
    CHECK_TRUE(SecurityCheckpoint::getInstance().approve(id, outErr));

    AuditOutboxRecord r;
    CHECK_TRUE(SnmpDatabase::getInstance().getLastAuditOutbox(r));
    CHECK_EQUAL(1, r.sequence);
    CHECK_EQUAL(std::string(""), r.prevHash);
}

TEST(AuditChain, SecondRecordLinksToFirst) {
    int64_t id1 = SecurityCheckpoint::getInstance().enqueuePendingAction(
        "firewall_block_ip", "ai_agent",
        {{"ip", "192.168.1.183"}}, 3600);
    CHECK(id1 > 0);

    std::string outErr;
    CHECK_TRUE(SecurityCheckpoint::getInstance().approve(id1, outErr));

    AuditOutboxRecord r1;
    CHECK_TRUE(SnmpDatabase::getInstance().getLastAuditOutbox(r1));
    CHECK_EQUAL(1, r1.sequence);
    std::string expectedHash1 = SecurityCheckpoint::computeRecordHash(r1);

    int64_t id2 = SecurityCheckpoint::getInstance().enqueuePendingAction(
        "firewall_block_ip", "ai_agent",
        {{"ip", "192.168.1.184"}}, 3600);
    CHECK(id2 > 0);

    CHECK_TRUE(SecurityCheckpoint::getInstance().deny(id2, "Second action denied"));

    AuditOutboxRecord r2;
    CHECK_TRUE(SnmpDatabase::getInstance().getLastAuditOutbox(r2));
    CHECK_EQUAL(2, r2.sequence);
    CHECK_EQUAL(expectedHash1, r2.prevHash);
}

TEST(AuditChain, CrashBeforeProjectionIsRepaired) {
    int64_t id = SecurityCheckpoint::getInstance().enqueuePendingAction(
        "firewall_block_ip", "ai_agent",
        {{"ip", "192.168.1.185"}}, 3600);
    CHECK(id > 0);

    std::string outErr;
    CHECK_TRUE(SecurityCheckpoint::getInstance().approve(id, outErr));

    // Simulate crash before projection was written: delete the file
    unlink("test_data/test_audit_chain.log");

    SecurityCheckpoint::getInstance().recoverOnStartup();

    std::ifstream f("test_data/test_audit_chain.log");
    CHECK_TRUE(f.good());
    std::string line;
    CHECK_TRUE(std::getline(f, line));
    nlohmann::json obj = nlohmann::json::parse(line);
    CHECK_EQUAL(1, obj.value("sequence", 0));
    CHECK_EQUAL(std::string("approved"), obj.value("decision", ""));
}

TEST(AuditChain, MismatchedFileIsRebuiltFromSqlite) {
    int64_t id = SecurityCheckpoint::getInstance().enqueuePendingAction(
        "firewall_block_ip", "ai_agent",
        {{"ip", "192.168.1.186"}}, 3600);
    CHECK(id > 0);

    std::string outErr;
    CHECK_TRUE(SecurityCheckpoint::getInstance().approve(id, outErr));

    // Deliberately corrupt the file hash
    {
        std::ofstream corrupt("test_data/test_audit_chain.log", std::ios::trunc);
        corrupt << "{\"sequence\":1,\"timestamp\":1000,\"prev_hash\":\"\",\"record_hash\":\"TAMPERED_FAKE_HASH\","
                << "\"action_id\":1,\"tool\":\"firewall_block_ip\",\"requester\":\"ai_agent\",\"payload\":\"{}\","
                << "\"decision\":\"approved\",\"reason\":\"tampered\"}\n";
    }

    SecurityCheckpoint::getInstance().recoverOnStartup();

    std::ifstream repaired("test_data/test_audit_chain.log");
    std::string line;
    CHECK_TRUE(std::getline(repaired, line));
    nlohmann::json obj = nlohmann::json::parse(line);

    AuditOutboxRecord r;
    CHECK_TRUE(SnmpDatabase::getInstance().getLastAuditOutbox(r));
    CHECK_EQUAL(SecurityCheckpoint::computeRecordHash(r), obj.value("record_hash", ""));
}

TEST_GROUP(Integration_OperatorAccess) {
    std::string testVaultDir = "test_data/op_access_vault";

    void setup() {
        WebServer::getInstance().stop();
        WebServer::getInstance().join();

        NetMonShell::getInstance().resetForTesting();
        AuthManager::getInstance().resetForTesting();
        if (fs::exists(testVaultDir)) {
            fs::remove_all(testVaultDir);
        }
        fs::create_directories(testVaultDir);
        AuthManager::getInstance().setVaultDir(testVaultDir);
        AuthManager::getInstance().setPassword("AdminPass2026!", "", false);
    }

    void teardown() {
        WebServer::getInstance().stop();
        WebServer::getInstance().join();

        NetMonShell::getInstance().resetForTesting();
        AuthManager::getInstance().resetForTesting();
        if (fs::exists(testVaultDir)) {
            fs::remove_all(testVaultDir);
        }
    }
};

TEST(Integration_OperatorAccess, MissingBearerIsRejected) {
    CHECK_TRUE(WebServer::getInstance().start("127.0.0.1", 3884, 3886));
    std::this_thread::sleep_for(std::chrono::milliseconds(50));

    httplib::Client cli("127.0.0.1", 3886);
    cli.set_connection_timeout(1, 0);
    cli.set_read_timeout(2, 0);

    auto res1 = cli.Get("/api/admin/pending");
    CHECK_TRUE(res1 != nullptr);
    CHECK_EQUAL(401, res1->status);

    auto res2 = cli.Get("/api/admin/audit");
    CHECK_TRUE(res2 != nullptr);
    CHECK_EQUAL(401, res2->status);

    auto res3 = cli.Post("/api/admin/policy", "{\"policy\":\"live\"}", "application/json");
    CHECK_TRUE(res3 != nullptr);
    CHECK_EQUAL(401, res3->status);

    cli.stop();
    WebServer::getInstance().stop();
    WebServer::getInstance().join();
}

TEST(Integration_OperatorAccess, CliPasswordChangeRequiresCurrentPassword) {
    int retBad = NetMonShell::getInstance().executeCommand("auth set-password WrongPassword NewPassword123!");
    CHECK(retBad != 0);

    std::string tokenOld = AuthManager::getInstance().login("AdminPass2026!");
    CHECK_FALSE(tokenOld.empty());

    int retGood = NetMonShell::getInstance().executeCommand("auth set-password AdminPass2026! NewPassword123!");
    CHECK_EQUAL(0, retGood);

    CHECK_TRUE(AuthManager::getInstance().login("AdminPass2026!").empty());
    CHECK_FALSE(AuthManager::getInstance().login("NewPassword123!").empty());
}

TEST(Integration_OperatorAccess, CliPasswordChangeIncrementsGeneration) {
    std::string sessionToken = AuthManager::getInstance().login("AdminPass2026!");
    CHECK_FALSE(sessionToken.empty());
    CHECK_TRUE(AuthManager::getInstance().validateSession(sessionToken));

    int genBefore = AuthManager::getInstance().getDiskGeneration();
    CHECK_TRUE(genBefore >= 0);

    int ret = NetMonShell::getInstance().executeCommand("auth set-password AdminPass2026! BrandNewSecret999!");
    CHECK_EQUAL(0, ret);

    int genAfter = AuthManager::getInstance().getDiskGeneration();
    CHECK_TRUE(genAfter >= 0);
    CHECK_EQUAL(genBefore + 1, genAfter);

    // Old session token issued under previous generation must now fail validation
    CHECK_FALSE(AuthManager::getInstance().validateSession(sessionToken));
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
