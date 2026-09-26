/*
 * TestApprovalQueue.cxx
 *
 * Copyright (C) 2026, Charles Chiou
 */

#include <unistd.h>
#include <thread>
#include <future>
#include <string>
#include <vector>
#include <nlohmann/json.hpp>
#include "SecurityCheckpoint.hxx"
#include "SnmpDatabase.hxx"
#include "RecordingRouter.hxx"
#include "AimonGatewayClient.hxx"

#include <CppUTest/TestHarness.h>

TEST_GROUP(ApprovalQueue) {
    void setup() {
        unlink("test_data/test_approval_queue.db");
        unlink("test_data/test_approval_queue.db-shm");
        unlink("test_data/test_approval_queue.db-wal");
        unlink("test_data/test_audit.log");

        SnmpDatabase::getInstance().close();
        SnmpDatabase::getInstance().open("test_data/test_approval_queue.db");

        SecurityCheckpoint::getInstance().resetForTesting();
        SecurityCheckpoint::getInstance().setAuditFilePath("test_data/test_audit.log");

        RecordingRouter::getInstance().reset();
    }

    void teardown() {
        SecurityCheckpoint::getInstance().resetForTesting();
        SnmpDatabase::getInstance().close();
        unlink("test_data/test_approval_queue.db");
        unlink("test_data/test_approval_queue.db-shm");
        unlink("test_data/test_approval_queue.db-wal");
        unlink("test_data/test_audit.log");

        RecordingRouter::getInstance().reset();
    }
};

TEST(ApprovalQueue, DefaultModeIsRequireApproval) {
    CHECK_EQUAL(static_cast<int>(PolicyMode::RequireApproval),
                static_cast<int>(SecurityCheckpoint::getInstance().getPolicyMode()));
    CHECK_EQUAL(std::string("require_approval"),
                SecurityCheckpoint::policyModeToString(SecurityCheckpoint::getInstance().getPolicyMode()));
}

TEST(ApprovalQueue, DisabledDoesNotCallRouter) {
    SecurityCheckpoint::getInstance().setPolicyMode(PolicyMode::Disabled);
    nlohmann::json payload = {{"ip", "192.168.1.150"}, {"reason", "test block"}};
    nlohmann::json res = SecurityCheckpoint::getInstance().handleAgentMutation("firewall_block_ip", "ai_agent", payload);

    CHECK_EQUAL(std::string("disabled"), res.value("status", ""));
    CHECK_EQUAL(0, RecordingRouter::getInstance().getCallCount());
    CHECK_EQUAL(0, SecurityCheckpoint::getInstance().getPendingTickets().size());
}

TEST(ApprovalQueue, DryRunDoesNotCallRouter) {
    SecurityCheckpoint::getInstance().setPolicyMode(PolicyMode::DryRun);
    nlohmann::json payload = {{"ip", "192.168.1.151"}, {"reason", "test dry run"}};
    nlohmann::json res = SecurityCheckpoint::getInstance().handleAgentMutation("firewall_block_ip", "ai_agent", payload);

    CHECK_EQUAL(std::string("dry_run"), res.value("status", ""));
    CHECK_TRUE(res.value("simulated", false));
    CHECK_EQUAL(0, RecordingRouter::getInstance().getCallCount());
    CHECK_EQUAL(0, SecurityCheckpoint::getInstance().getPendingTickets().size());
}

TEST(ApprovalQueue, EnqueueStoresCanonicalPayload) {
    SecurityCheckpoint::getInstance().setPolicyMode(PolicyMode::RequireApproval);
    nlohmann::json payload = {{"ip", "192.168.1.152"}, {"reason", "port scanner"}, {"extra", 42}};
    nlohmann::json res = SecurityCheckpoint::getInstance().handleAgentMutation("firewall_block_ip", "ai_agent", payload);

    CHECK_EQUAL(std::string("pending"), res.value("status", ""));
    CHECK_TRUE(res.contains("ticket_id"));
    int64_t ticketId = res["ticket_id"];
    CHECK(ticketId > 0);
    CHECK_EQUAL(0, RecordingRouter::getInstance().getCallCount());

    PendingAction act;
    CHECK_TRUE(SecurityCheckpoint::getInstance().getTicket(ticketId, act));
    CHECK_EQUAL(std::string("firewall_block_ip"), act.tool);
    CHECK_EQUAL(std::string("ai_agent"), act.requester);
    CHECK_EQUAL(std::string("pending"), act.status);

    nlohmann::json storedPayload = nlohmann::json::parse(act.payload);
    CHECK_EQUAL(std::string("192.168.1.152"), storedPayload.value("ip", ""));
    CHECK_EQUAL(std::string("port scanner"), storedPayload.value("reason", ""));
    CHECK_EQUAL(42, storedPayload.value("extra", 0));
}

TEST(ApprovalQueue, ApproveReplaysStoredPayload) {
    SecurityCheckpoint::getInstance().setPolicyMode(PolicyMode::RequireApproval);
    nlohmann::json payload = {{"ip", "192.168.1.153"}, {"reason", "malicious payload"}};
    nlohmann::json res = SecurityCheckpoint::getInstance().handleAgentMutation("firewall_block_ip", "ai_agent", payload);
    int64_t ticketId = res["ticket_id"];

    std::string outErr;
    CHECK_TRUE(SecurityCheckpoint::getInstance().approve(ticketId, outErr));
    CHECK_EQUAL(1, RecordingRouter::getInstance().getCallCount());

    auto calls = RecordingRouter::getInstance().getCalls();
    CHECK_EQUAL(std::string("blockIp"), calls[0].method);
    CHECK_EQUAL(std::string("192.168.1.153"), calls[0].ip);
    CHECK_EQUAL(std::string("malicious payload"), calls[0].reason);

    PendingAction act;
    CHECK_TRUE(SecurityCheckpoint::getInstance().getTicket(ticketId, act));
    CHECK_EQUAL(std::string("approved"), act.status);
}

TEST(ApprovalQueue, DenyDoesNotCallRouter) {
    SecurityCheckpoint::getInstance().setPolicyMode(PolicyMode::RequireApproval);
    nlohmann::json payload = {{"ip", "192.168.1.154"}, {"reason", "suspicious"}};
    nlohmann::json res = SecurityCheckpoint::getInstance().handleAgentMutation("firewall_block_ip", "ai_agent", payload);
    int64_t ticketId = res["ticket_id"];

    CHECK_TRUE(SecurityCheckpoint::getInstance().deny(ticketId, "Operator rejected"));
    CHECK_EQUAL(0, RecordingRouter::getInstance().getCallCount());

    PendingAction act;
    CHECK_TRUE(SecurityCheckpoint::getInstance().getTicket(ticketId, act));
    CHECK_EQUAL(std::string("denied"), act.status);

    std::string outErr;
    CHECK_FALSE(SecurityCheckpoint::getInstance().approve(ticketId, outErr));
    CHECK_EQUAL(0, RecordingRouter::getInstance().getCallCount());
}

TEST(ApprovalQueue, ExpiredTicketCannotBeApproved) {
    int64_t ticketId = SecurityCheckpoint::getInstance().enqueuePendingAction(
        "firewall_block_ip", "ai_agent",
        {{"ip", "192.168.1.155"}, {"reason", "expiring"}}, -10);
    CHECK(ticketId > 0);

    std::string outErr;
    CHECK_FALSE(SecurityCheckpoint::getInstance().approve(ticketId, outErr));
    CHECK_EQUAL(0, RecordingRouter::getInstance().getCallCount());
}

TEST(ApprovalQueue, ProtectedAddressSurvivesApproval) {
    int64_t ticketId = SecurityCheckpoint::getInstance().enqueuePendingAction(
        "firewall_block_ip", "ai_agent",
        {{"ip", "192.168.1.200"}, {"reason", "test protected"}}, 3600);
    CHECK(ticketId > 0);

    SecurityCheckpoint::getInstance().addProtectedIp("192.168.1.200");

    std::string outErr;
    CHECK_FALSE(SecurityCheckpoint::getInstance().approve(ticketId, outErr));
    CHECK_EQUAL(0, RecordingRouter::getInstance().getCallCount());
    CHECK(outErr.find("protected") != std::string::npos);
}

TEST(ApprovalQueue, RouterFailureReturnsPending) {
    RecordingRouter::getInstance().setBlockIpSuccess(false);

    nlohmann::json payload = {{"ip", "192.168.1.156"}, {"reason", "router fault"}};
    nlohmann::json res = SecurityCheckpoint::getInstance().handleAgentMutation("firewall_block_ip", "ai_agent", payload);
    int64_t ticketId = res["ticket_id"];

    std::string outErr;
    CHECK_FALSE(SecurityCheckpoint::getInstance().approve(ticketId, outErr));
    CHECK_EQUAL(1, RecordingRouter::getInstance().getCallCount());

    PendingAction act;
    CHECK_TRUE(SecurityCheckpoint::getInstance().getTicket(ticketId, act));
    CHECK_EQUAL(std::string("pending"), act.status);
}

TEST(ApprovalQueue, CrashWhileExecutingBecomesInterrupted) {
    int64_t ticketId = SecurityCheckpoint::getInstance().enqueuePendingAction(
        "firewall_block_ip", "ai_agent",
        {{"ip", "192.168.1.157"}, {"reason", "crash test"}}, 3600);
    CHECK(ticketId > 0);

    // Simulate crash mid-flight by forcibly setting executing in DB
    sqlite3_exec(SnmpDatabase::getInstance().getHandle(),
                 "UPDATE pending_actions SET status = 'executing';",
                 nullptr, nullptr, nullptr);

    SecurityCheckpoint::getInstance().recoverOnStartup();

    PendingAction act;
    CHECK_TRUE(SecurityCheckpoint::getInstance().getTicket(ticketId, act));
    CHECK_EQUAL(std::string("interrupted"), act.status);

    std::string outErr;
    CHECK_FALSE(SecurityCheckpoint::getInstance().approve(ticketId, outErr));
    CHECK_EQUAL(0, RecordingRouter::getInstance().getCallCount());
}

TEST(ApprovalQueue, ReconcileAppliedDoesNotCallRouter) {
    int64_t ticketId = SecurityCheckpoint::getInstance().enqueuePendingAction(
        "firewall_block_ip", "ai_agent",
        {{"ip", "192.168.1.157"}, {"reason", "reconcile applied test"}}, 3600);
    CHECK(ticketId > 0);

    sqlite3_exec(SnmpDatabase::getInstance().getHandle(),
                 "UPDATE pending_actions SET status = 'interrupted';",
                 nullptr, nullptr, nullptr);

    std::string outErr;
    CHECK_TRUE(SecurityCheckpoint::getInstance().reconcile(ticketId, "applied", outErr));
    CHECK_EQUAL(0, RecordingRouter::getInstance().getCallCount());

    PendingAction act;
    CHECK_TRUE(SecurityCheckpoint::getInstance().getTicket(ticketId, act));
    CHECK_EQUAL(std::string("approved"), act.status);
}

TEST(ApprovalQueue, ReconcileRetryThenApproveCallsRouterOnce) {
    int64_t ticketId = SecurityCheckpoint::getInstance().enqueuePendingAction(
        "firewall_block_ip", "ai_agent",
        {{"ip", "192.168.1.158"}, {"reason", "retry me"}}, 3600);
    CHECK(ticketId > 0);

    sqlite3_exec(SnmpDatabase::getInstance().getHandle(),
                 "UPDATE pending_actions SET status = 'interrupted';",
                 nullptr, nullptr, nullptr);

    std::string outErr;
    CHECK_TRUE(SecurityCheckpoint::getInstance().reconcile(ticketId, "retry", outErr));
    CHECK_EQUAL(0, RecordingRouter::getInstance().getCallCount());

    PendingAction act;
    CHECK_TRUE(SecurityCheckpoint::getInstance().getTicket(ticketId, act));
    CHECK_EQUAL(std::string("pending"), act.status);

    CHECK_TRUE(SecurityCheckpoint::getInstance().approve(ticketId, outErr));
    CHECK_EQUAL(1, RecordingRouter::getInstance().getCallCount());

    CHECK_TRUE(SecurityCheckpoint::getInstance().getTicket(ticketId, act));
    CHECK_EQUAL(std::string("approved"), act.status);
}

TEST(ApprovalQueue, ConcurrentApproveAffectsOneRow) {
    int64_t ticketId = SecurityCheckpoint::getInstance().enqueuePendingAction(
        "firewall_block_ip", "ai_agent",
        {{"ip", "192.168.1.159"}, {"reason", "concurrency test"}}, 3600);
    CHECK(ticketId > 0);

    bool res1 = false;
    bool res2 = false;
    std::string err1, err2;

    std::thread t1([ticketId, &res1, &err1]() {
        res1 = SecurityCheckpoint::getInstance().approve(ticketId, err1);
    });
    std::thread t2([ticketId, &res2, &err2]() {
        res2 = SecurityCheckpoint::getInstance().approve(ticketId, err2);
    });

    t1.join();
    t2.join();

    // Exactly one caller must win
    CHECK_TRUE((res1 && !res2) || (!res1 && res2));
    CHECK_EQUAL(1, RecordingRouter::getInstance().getCallCount());
}

TEST(ApprovalQueue, RestartReloadsPayload) {
    int64_t ticketId = SecurityCheckpoint::getInstance().enqueuePendingAction(
        "firewall_block_ip", "ai_agent",
        {{"ip", "192.168.1.160"}, {"reason", "persisted across restart"}}, 3600);
    CHECK(ticketId > 0);

    SnmpDatabase::getInstance().close();
    CHECK_TRUE(SnmpDatabase::getInstance().open("test_data/test_approval_queue.db"));
    SecurityCheckpoint::getInstance().recoverOnStartup();

    PendingAction act;
    CHECK_TRUE(SecurityCheckpoint::getInstance().getTicket(ticketId, act));
    CHECK_EQUAL(std::string("pending"), act.status);

    nlohmann::json pl = nlohmann::json::parse(act.payload);
    CHECK_EQUAL(std::string("192.168.1.160"), pl.value("ip", ""));
    CHECK_EQUAL(std::string("persisted across restart"), pl.value("reason", ""));
}

TEST(ApprovalQueue, AgentInterfaceHasNoApproveMethod) {
    nlohmann::json reg = AimonGatewayClient::getRegistrationJson(3884);
    auto tools = reg["params"]["tools"];
    for (const auto &tool : tools) {
        std::string name = tool["name"];
        CHECK(name != "approve");
        CHECK(name != "deny");
        CHECK(name != "reconcile");
        CHECK(name != "policy");
        CHECK(name != "set_policy");
        CHECK(name != "auth");
        CHECK(name != "login");
    }

    std::string line = "{\"jsonrpc\":\"2.0\",\"id\":99,\"method\":\"tools/call\",\"params\":{\"name\":\"approve\",\"arguments\":{\"ticket_id\":1}}}";
    std::string resp = AimonGatewayClient::getInstance().dispatchRequest(line);
    nlohmann::json respJson = nlohmann::json::parse(resp);
    CHECK_TRUE(respJson.contains("error"));
    CHECK_EQUAL(-32601, respJson["error"]["code"].get<int>());
}

TEST_GROUP(Integration_ApprovalRestart) {
    void setup() {
        unlink("test_data/test_second_instance.db");
        unlink("test_data/test_second_instance.db-shm");
        unlink("test_data/test_second_instance.db-wal");
        unlink("test_data/test_second_instance_audit.log");

        SnmpDatabase::getInstance().close();
        SnmpDatabase::getInstance().open("test_data/test_second_instance.db");

        SecurityCheckpoint::getInstance().resetForTesting();
        SecurityCheckpoint::getInstance().setAuditFilePath("test_data/test_second_instance_audit.log");

        RecordingRouter::getInstance().reset();
    }

    void teardown() {
        SecurityCheckpoint::getInstance().resetForTesting();
        SnmpDatabase::getInstance().close();
        unlink("test_data/test_second_instance.db");
        unlink("test_data/test_second_instance.db-shm");
        unlink("test_data/test_second_instance.db-wal");
        unlink("test_data/test_second_instance_audit.log");

        RecordingRouter::getInstance().reset();
    }
};

TEST(Integration_ApprovalRestart, SecondInstanceApprovesStoredPayload) {
    int64_t ticketId = SecurityCheckpoint::getInstance().enqueuePendingAction(
        "firewall_block_ip", "ai_agent",
        {{"ip", "192.168.1.161"}, {"reason", "second instance test"}}, 3600);
    CHECK(ticketId > 0);

    // Simulate process 1 exit
    SnmpDatabase::getInstance().close();

    // Simulate process 2 start
    CHECK_TRUE(SnmpDatabase::getInstance().open("test_data/test_second_instance.db"));
    SecurityCheckpoint::getInstance().recoverOnStartup();

    std::string outErr;
    CHECK_TRUE(SecurityCheckpoint::getInstance().approve(ticketId, outErr));
    CHECK_EQUAL(1, RecordingRouter::getInstance().getCallCount());

    auto calls = RecordingRouter::getInstance().getCalls();
    CHECK_EQUAL(std::string("192.168.1.161"), calls[0].ip);

    PendingAction act;
    CHECK_TRUE(SecurityCheckpoint::getInstance().getTicket(ticketId, act));
    CHECK_EQUAL(std::string("approved"), act.status);
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
