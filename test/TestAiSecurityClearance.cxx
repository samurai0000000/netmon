/*
 * TestAiSecurityClearance.cxx
 *
 * Copyright (C) 2026, Charles Chiou
 */

#include "AiSecurityClearance.hxx"
#include "RecordingRouter.hxx"
#include "AimonGatewayClient.hxx"
#include "Config.hxx"
#include "UnixAuth.hxx"
#include "NetMonShell.hxx"
#include <openssl/sha.h>
#include <nlohmann/json.hpp>
#include <CppUTest/TestHarness.h>
#include <string>
#include <vector>
#include <fstream>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <arpa/inet.h>

TEST_GROUP(AiSecurityClassifierTest) {
    void setup() {
        RecordingRouter::getInstance().reset();
    }

    void teardown() {
        RecordingRouter::getInstance().reset();
    }
};

TEST(AiSecurityClassifierTest, Level3ShapesNominal) {
    std::vector<std::string> l3Cmds = {
        "show version",
        "show cpu status",
        "show mem status",
        "show conn status",
        "ping 192.168.1.1 count 5",
        "traceroute 192.168.1.1",
        "show interface summary all",
        "show interface ge1",
        "show ip route-settings",
        "show zone",
        "show arp-table",
        "show address-object",
        "show address-object LAN_HOST",
        "show object-group address",
        "show object-group address TRUSTED_HOSTS",
        "show object-group service",
        "show object-group service HTTP_SERVICES",
        "show service-object",
        "show service-object HTTP_80",
        "show secure-policy",
        "show secure-policy 10",
        "show secure-policy DENY_RULE",
        "show ip virtual-server",
        "show ip virtual-server WEB_SRV",
        "show app statistics summary",
        "show idp statistics summary"
    };

    for (const auto &cmd : l3Cmds) {
        int timeoutMs = 0;
        std::string method;
        LineClassification cls = AiSecurityClassifier::classify(cmd, "#", timeoutMs, method);
        CHECK_EQUAL(static_cast<int>(LineClassification::LEVEL3), static_cast<int>(cls));
        CHECK_FALSE(method.empty());
        CHECK_TRUE(timeoutMs > 0);
    }
}

TEST(AiSecurityClassifierTest, Level3TenEnvelopesExpanded) {
    std::vector<std::string> expandedCmds = {
        "show ip route",
        "show interfaces status",
        "show interfaces detail",
        "show anti-spam statistics",
        "show anti-spam profile",
        "show anti-virus status",
        "show content-filter status",
        "show crypto ike sa",
        "show crypto ipsec sa",
        "show vpn-monitor",
        "show ssl-vpn status",
        "show user status",
        "show user-group",
        "show aaa-server",
        "show logging status",
        "show environment",
        "show clock",
        "show registration status",
        "show running-config",
        "show bwm status",
        "show bridge status",
        "show vlan status"
    };

    for (const auto &cmd : expandedCmds) {
        int timeoutMs = 0;
        std::string method;
        LineClassification cls = AiSecurityClassifier::classify(cmd, "#", timeoutMs, method);
        CHECK_EQUAL(static_cast<int>(LineClassification::LEVEL3), static_cast<int>(cls));
        CHECK_FALSE(method.empty());
        CHECK_TRUE(timeoutMs > 0);
    }
}

TEST(AiSecurityClassifierTest, Level3Timeouts) {
    int timeoutMs = 0;
    std::string method;

    // Ping timeout: 10000 + (count * 2000)
    AiSecurityClassifier::classify("ping 10.0.0.1 count 5", "#", timeoutMs, method);
    LONGS_EQUAL(20000, timeoutMs);

    // Traceroute timeout: 60000
    AiSecurityClassifier::classify("traceroute 10.0.0.1", "#", timeoutMs, method);
    LONGS_EQUAL(60000, timeoutMs);

    // Default timeout: 5000
    AiSecurityClassifier::classify("show version", "#", timeoutMs, method);
    LONGS_EQUAL(5000, timeoutMs);

    // Targeted single rule lookup: 5000
    AiSecurityClassifier::classify("show secure-policy 1", "#", timeoutMs, method);
    LONGS_EQUAL(5000, timeoutMs);

    // Full table and DPI telemetry dump timeouts: 15000
    AiSecurityClassifier::classify("show secure-policy", "#", timeoutMs, method);
    LONGS_EQUAL(15000, timeoutMs);

    AiSecurityClassifier::classify("show service-object", "#", timeoutMs, method);
    LONGS_EQUAL(15000, timeoutMs);

    AiSecurityClassifier::classify("show ip virtual-server", "#", timeoutMs, method);
    LONGS_EQUAL(15000, timeoutMs);

    AiSecurityClassifier::classify("show app statistics summary", "#", timeoutMs, method);
    LONGS_EQUAL(15000, timeoutMs);

    AiSecurityClassifier::classify("show idp statistics summary", "#", timeoutMs, method);
    LONGS_EQUAL(15000, timeoutMs);
}

TEST(AiSecurityClassifierTest, Level3ShapesIllegalArgs) {
    int timeoutMs = 0;
    std::string method;

    // Invalid IPv4 in ping
    CHECK_EQUAL(static_cast<int>(LineClassification::UNCLASSIFIED),
                static_cast<int>(AiSecurityClassifier::classify("ping 999.168.1.1 count 5", "#", timeoutMs, method)));

    // Ping count out of range (1-20 accepted)
    CHECK_EQUAL(static_cast<int>(LineClassification::UNCLASSIFIED),
                static_cast<int>(AiSecurityClassifier::classify("ping 10.0.0.1 count 0", "#", timeoutMs, method)));
    CHECK_EQUAL(static_cast<int>(LineClassification::UNCLASSIFIED),
                static_cast<int>(AiSecurityClassifier::classify("ping 10.0.0.1 count 25", "#", timeoutMs, method)));

    // Invalid IPv4 in traceroute
    CHECK_EQUAL(static_cast<int>(LineClassification::UNCLASSIFIED),
                static_cast<int>(AiSecurityClassifier::classify("traceroute not_an_ip", "#", timeoutMs, method)));

    // Name with illegal characters or too long (>31 chars)
    CHECK_EQUAL(static_cast<int>(LineClassification::UNCLASSIFIED),
                static_cast<int>(AiSecurityClassifier::classify("show interface bad@interface", "#", timeoutMs, method)));
    CHECK_EQUAL(static_cast<int>(LineClassification::UNCLASSIFIED),
                static_cast<int>(AiSecurityClassifier::classify("show interface a_very_long_interface_name_that_exceeds_thirty_one_chars", "#", timeoutMs, method)));
}

TEST(AiSecurityClassifierTest, AddressGroupShowUnclassified) {
    int timeoutMs = 0;
    std::string method;
    CHECK_EQUAL(static_cast<int>(LineClassification::UNCLASSIFIED),
                static_cast<int>(AiSecurityClassifier::classify("show address-group", "#", timeoutMs, method)));
    CHECK_EQUAL(static_cast<int>(LineClassification::UNCLASSIFIED),
                static_cast<int>(AiSecurityClassifier::classify("show address-group TRUSTED_HOSTS", "#", timeoutMs, method)));
}

TEST(AiSecurityClassifierTest, Level2RootShapesNominal) {
    std::vector<std::string> l2Cmds = {
        "ip route 10.0.0.0 255.255.255.0 192.168.1.1 10",
        "ip route 10.0.0.0 24 192.168.1.1 1",
        "no ip route 10.0.0.0 255.255.255.0 192.168.1.1",
        "address-object HOST_A 192.168.1.10",
        "address-object RANGE_A 192.168.1.10-192.168.1.20",
        "address-object SUBNET_A 192.168.1.0/24",
        "address-object SUBNET_B 192.168.1.0/255.255.255.0",
        "no address-object HOST_A",
        "address-group GRP_A HOST_A",
        "no address-group GRP_A HOST_A",
        "service-object HTTP_TCP tcp eq 80",
        "service-object DNS_UDP udp eq 53",
        "service-object PING_ICMP icmp eq 8",
        "no service-object HTTP_TCP",
        "secure-policy insert 1",
        "no secure-policy 5",
        "no secure-policy name BLOCK_RULE",
        "ip virtual-server VS_1 interface ge1 original-ip 1.2.3.4 map-to 192.168.1.50 map-type port original-service HTTP mapped-service HTTP_LOCAL activate",
        "ip virtual-server VS_2 map-type port original-service HTTP mapped-service HTTP_LOCAL deactivate"
    };

    for (const auto &cmd : l2Cmds) {
        int timeoutMs = 0;
        std::string method;
        LineClassification cls = AiSecurityClassifier::classify(cmd, "#", timeoutMs, method);
        CHECK_EQUAL(static_cast<int>(LineClassification::LEVEL2_ROOT), static_cast<int>(cls));
        CHECK_FALSE(method.empty());
    }
}

TEST(AiSecurityClassifierTest, DiagnosticCleanupNeedsWriteClearance) {
    const char *cleanup[] = {
        "no wlan-security-profile NETMON_QA_SEC_E10",
        "no wlan-ssid-profile NETMON_QA_SSID_E10",
        "no groupname NETMON_QA_GRP_E9",
        "no sslvpn application NETMON_QA_SSLVPN_E8",
        "no isakmp policy NETMON_QA_IKE_E8",
        "no anti-virus NETMON_QA_AV_E7",
        "no ssl-inspection profile NETMON_QA_SSL_E7",
        "no ip ddns profile NETMON_QA_DDNS",
        "no object-group address6 NETMON_QA_GRP6",
        "no aaa group server radius NETMON_QA_AAA_E9"
    };

    for (const char *command : cleanup) {
        int timeoutMs = 0;
        std::string method;
        CHECK_EQUAL(
            static_cast<int>(LineClassification::LEVEL2_ROOT),
            static_cast<int>(AiSecurityClassifier::classify(
                command, "Router#", timeoutMs, method)));
        STRCMP_EQUAL("cmdDeleteNamedObject", method.c_str());
    }

    int timeoutMs = 0;
    std::string method;
    CHECK_EQUAL(
        static_cast<int>(LineClassification::UNCLASSIFIED),
        static_cast<int>(AiSecurityClassifier::classify(
            "no sslvpn application bad-name", "Router#", timeoutMs, method)));
}

TEST(AiSecurityClassifierTest, Level2RootShapesIllegalArgs) {
    int timeoutMs = 0;
    std::string method;

    // Route metric > 127
    CHECK_EQUAL(static_cast<int>(LineClassification::UNCLASSIFIED),
                static_cast<int>(AiSecurityClassifier::classify("ip route 10.0.0.0 24 192.168.1.1 200", "#", timeoutMs, method)));

    // Address object with invalid range (start not IPv4)
    CHECK_EQUAL(static_cast<int>(LineClassification::UNCLASSIFIED),
                static_cast<int>(AiSecurityClassifier::classify("address-object RANGE_A badip-192.168.1.20", "#", timeoutMs, method)));

    // Service object with invalid proto
    CHECK_EQUAL(static_cast<int>(LineClassification::UNCLASSIFIED),
                static_cast<int>(AiSecurityClassifier::classify("service-object HTTP_TCP sctp eq 80", "#", timeoutMs, method)));

    // Service object with port 0 or > 65535
    CHECK_EQUAL(static_cast<int>(LineClassification::UNCLASSIFIED),
                static_cast<int>(AiSecurityClassifier::classify("service-object HTTP_TCP tcp eq 0", "#", timeoutMs, method)));
    CHECK_EQUAL(static_cast<int>(LineClassification::UNCLASSIFIED),
                static_cast<int>(AiSecurityClassifier::classify("service-object HTTP_TCP tcp eq 70000", "#", timeoutMs, method)));

    // Secure policy insert with invalid position (>9999 or 0)
    CHECK_EQUAL(static_cast<int>(LineClassification::UNCLASSIFIED),
                static_cast<int>(AiSecurityClassifier::classify("secure-policy insert 0", "#", timeoutMs, method)));
    CHECK_EQUAL(static_cast<int>(LineClassification::UNCLASSIFIED),
                static_cast<int>(AiSecurityClassifier::classify("secure-policy insert 10000", "#", timeoutMs, method)));

    CHECK_EQUAL(static_cast<int>(LineClassification::UNCLASSIFIED),
                static_cast<int>(AiSecurityClassifier::classify("no secure-policy BLOCK_RULE", "Router#", timeoutMs, method)));
}

TEST(AiSecurityClassifierTest, Level2SubmodeFragments) {
    int timeoutMs = 0;
    std::string method;
    std::string submodePrompt = "Router(secure-policy)#";

    std::vector<std::string> submodeCmds = {
        "description Allow HTTP inbound",
        "name RULE_HTTP",
        "from WAN",
        "to LAN",
        "sourceip any",
        "destinationip WEB_SERVER",
        "service HTTP",
        "action allow",
        "action deny",
        "activate",
        "no activate"
    };

    // In submode prompt, all must be accepted as LEVEL2_SUBMODE
    for (const auto &cmd : submodeCmds) {
        LineClassification cls = AiSecurityClassifier::classify(cmd, submodePrompt, timeoutMs, method);
        CHECK_EQUAL(static_cast<int>(LineClassification::LEVEL2_SUBMODE), static_cast<int>(cls));
        CHECK_FALSE(method.empty());
    }

    // At root prompt, submode fragments MUST be UNCLASSIFIED
    for (const auto &cmd : submodeCmds) {
        LineClassification cls = AiSecurityClassifier::classify(cmd, "Router#", timeoutMs, method);
        CHECK_EQUAL(static_cast<int>(LineClassification::UNCLASSIFIED), static_cast<int>(cls));
    }

    CHECK_EQUAL(static_cast<int>(LineClassification::UNCLASSIFIED),
                static_cast<int>(AiSecurityClassifier::classify("deactivate", submodePrompt, timeoutMs, method)));
    CHECK_EQUAL(static_cast<int>(LineClassification::UNCLASSIFIED),
                static_cast<int>(AiSecurityClassifier::classify("exit", submodePrompt, timeoutMs, method)));
    CHECK_EQUAL(static_cast<int>(LineClassification::UNCLASSIFIED),
                static_cast<int>(AiSecurityClassifier::classify("name RULE_HTTP", submodePrompt, timeoutMs, method, false)));
    CHECK_EQUAL(static_cast<int>(LineClassification::LEVEL2_SUBMODE),
                static_cast<int>(AiSecurityClassifier::classify("no activate", submodePrompt, timeoutMs, method, false)));

    CHECK_TRUE(AiSecurityClassifier::isSecurePolicySubmode("Router(secure-policy)#"));
    CHECK_FALSE(AiSecurityClassifier::isSecurePolicySubmode("Router(config-secure-policy)#"));
    CHECK_TRUE(AiSecurityClassifier::isRootPrompt("Router#"));
    CHECK_FALSE(AiSecurityClassifier::isRootPrompt("Router>"));
    CHECK_FALSE(AiSecurityClassifier::isRootPrompt("Router(config)#"));
    CHECK_FALSE(AiSecurityClassifier::isRootPrompt(""));
    CHECK_FALSE(AiSecurityClassifier::isRootPrompt("#"));
}

TEST(AiSecurityClassifierTest, Level1DeviceManagementForbidden) {
    int timeoutMs = 0;
    std::string method;

    LineClassification clsWrite = AiSecurityClassifier::classify("write", "#", timeoutMs, method);
    CHECK_EQUAL(static_cast<int>(LineClassification::LEVEL1), static_cast<int>(clsWrite));

    LineClassification clsReboot = AiSecurityClassifier::classify("reboot", "#", timeoutMs, method);
    CHECK_EQUAL(static_cast<int>(LineClassification::LEVEL1), static_cast<int>(clsReboot));
}

TEST(AiSecurityClassifierTest, ChainingAndControlCharactersRejected) {
    int timeoutMs = 0;
    std::string method;

    std::vector<std::string> badCmds = {
        "show version; reboot",
        "show version && write",
        "show version | grep foo",
        "show version `reboot`",
        "show version $(reboot)",
        "show version\nreboot",
        "show version\r\nwrite",
        "show\tversion",
        "?",
        "show ?",
        "router ospf ?"
    };

    for (const auto &cmd : badCmds) {
        LineClassification cls = AiSecurityClassifier::classify(cmd, "#", timeoutMs, method);
        CHECK_EQUAL(static_cast<int>(LineClassification::UNCLASSIFIED), static_cast<int>(cls));
    }
}

TEST_GROUP(AiSecurityClearanceManagerTest) {
    void setup() {
        RecordingRouter::getInstance().reset();
        AiSecurityClearanceManager::getInstance().resetForTesting();
    }

    void teardown() {
        RecordingRouter::getInstance().reset();
        AiSecurityClearanceManager::getInstance().resetForTesting();
    }
};

TEST(AiSecurityClearanceManagerTest, Level3RejectedWithoutApproval) {
    auto &mgr = AiSecurityClearanceManager::getInstance();
    uint64_t connId = mgr.registerConnectionForTesting(100, "192.0.2.10", 55555);

    // Handshake
    std::string rspHello = mgr.handleClientMessage(connId, "HELLO 1");
    STRCMP_EQUAL("HELLO 1", rspHello.c_str());

    // Execute Level 3 without grant -> Strictly rejected with NOT_AUTHORIZED
    std::string rsp = mgr.handleClientMessage(connId, "DO show version");
    CHECK_TRUE(rsp.rfind("RSP NOT_AUTHORIZED", 0) == 0);
}

TEST(AiSecurityClearanceManagerTest, Level3AllowedWithApproval) {
    auto &mgr = AiSecurityClearanceManager::getInstance();
    uint64_t connId = mgr.registerConnectionForTesting(100, "192.0.2.10", 55555);

    mgr.handleClientMessage(connId, "REQUEST tier=write");
    CHECK_TRUE(mgr.consoleApprove(connId));

    // Execute Level 3 with grant -> OK
    std::string rsp = mgr.handleClientMessage(connId, "DO show version");
    CHECK_TRUE(rsp.rfind("RSP OK", 0) == 0);
}

TEST(AiSecurityClearanceManagerTest, RepeatedRequestDrawsNoPrompt) {
    auto &mgr = AiSecurityClearanceManager::getInstance();
    uint64_t conn1 = mgr.registerConnectionForTesting(101, "192.0.2.10", 55556);
    uint64_t conn2 = mgr.registerConnectionForTesting(102, "192.0.2.11", 55557);

    std::string rsp1 = mgr.handleClientMessage(conn1, "REQUEST tier=write");
    CHECK_TRUE(rsp1.rfind("WAITING", 0) == 0);

    // Repeated request on same connection returns WAITING without duplicate pending prompt
    std::string rsp1Repeat = mgr.handleClientMessage(conn1, "REQUEST tier=write");
    CHECK_TRUE(rsp1Repeat.rfind("WAITING", 0) == 0);

    // Request from another connection returns BUSY while first is pending
    std::string rsp2 = mgr.handleClientMessage(conn2, "REQUEST tier=write");
    STRCMP_EQUAL("BUSY", rsp2.c_str());
}

TEST(AiSecurityClearanceManagerTest, Level2RejectedWithoutApproval) {
    auto &mgr = AiSecurityClearanceManager::getInstance();
    uint64_t connId = mgr.registerConnectionForTesting(101, "192.0.2.10", 55556);

    std::string rsp = mgr.handleClientMessage(connId, "DO ip route 10.0.0.0 24 192.168.1.1 1");
    CHECK_TRUE(rsp.rfind("RSP NOT_AUTHORIZED", 0) == 0);
}

TEST(AiSecurityClearanceManagerTest, Level2ApprovedAndExecuted) {
    auto &mgr = AiSecurityClearanceManager::getInstance();
    uint64_t connId = mgr.registerConnectionForTesting(102, "192.0.2.10", 55557);

    // Request clearance
    std::string rspReq = mgr.handleClientMessage(connId, "REQUEST tier=write");
    CHECK_TRUE(rspReq.rfind("WAITING", 0) == 0);

    // Console operator inspects and approves
    uint64_t pendingId = 0;
    std::string peerIp;
    uint16_t peerPort = 0;
    CHECK_TRUE(mgr.hasPendingApproval(pendingId, peerIp, peerPort));
    LONGS_EQUAL(connId, pendingId);
    STRCMP_EQUAL("192.0.2.10", peerIp.c_str());
    LONGS_EQUAL(55557, peerPort);

    CHECK_TRUE(mgr.consoleApprove(pendingId));

    // Request notification was processed, socket now elevated
    std::string rsp = mgr.handleClientMessage(connId, "DO ip route 10.0.0.0 24 192.168.1.1 1");
    CHECK_TRUE(rsp.rfind("RSP OK", 0) == 0);

    auto calls = RecordingRouter::getInstance().getClearanceCalls();
    LONGS_EQUAL(1, calls.size());
    STRCMP_EQUAL("ip route 10.0.0.0 24 192.168.1.1 1", calls[0].command.c_str());
}

TEST(AiSecurityClearanceManagerTest, DiagnosticCleanupRequiresWriteGrant) {
    auto &mgr = AiSecurityClearanceManager::getInstance();
    uint64_t connId =
        mgr.registerConnectionForTesting(103, "192.0.2.10", 55558);
    const char *command = "no ip ddns profile NETMON_QA_DDNS";

    mgr.setConnectionLevelForTesting(
        connId, ClientClearanceState::LEVEL3, 0);
    std::string readRsp =
        mgr.handleClientMessage(connId, std::string("DO ") + command);
    CHECK_TRUE(readRsp.rfind("RSP NOT_AUTHORIZED", 0) == 0);

    mgr.setConnectionLevelForTesting(
        connId, ClientClearanceState::LEVEL2, 300);
    std::string writeRsp =
        mgr.handleClientMessage(connId, std::string("DO ") + command);
    CHECK_TRUE(writeRsp.rfind("RSP OK", 0) == 0);

    auto calls = RecordingRouter::getInstance().getClearanceCalls();
    LONGS_EQUAL(1, calls.size());
    STRCMP_EQUAL(command, calls[0].command.c_str());
}

TEST(AiSecurityClearanceManagerTest, DeniedRequestRefusesAllCommands) {
    auto &mgr = AiSecurityClearanceManager::getInstance();
    uint64_t connId = mgr.registerConnectionForTesting(103, "192.0.2.10", 55558);

    mgr.handleClientMessage(connId, "REQUEST tier=write");
    CHECK_TRUE(mgr.consoleDeny(connId));

    // Still cannot run Level 2
    std::string rsp = mgr.handleClientMessage(connId, "DO ip route 10.0.0.0 24 192.168.1.1 1");
    CHECK_TRUE(rsp.rfind("RSP NOT_AUTHORIZED", 0) == 0);

    // Cannot run Level 3 either
    std::string rspL3 = mgr.handleClientMessage(connId, "DO show version");
    CHECK_TRUE(rspL3.rfind("RSP NOT_AUTHORIZED", 0) == 0);
}

TEST(AiSecurityClearanceManagerTest, GrantPreservedAcrossCommands) {
    auto &mgr = AiSecurityClearanceManager::getInstance();
    uint64_t connId = mgr.registerConnectionForTesting(104, "192.0.2.10", 55559);

    mgr.setConnectionLevelForTesting(connId, ClientClearanceState::LEVEL2, 300);

    std::string rsp1 = mgr.handleClientMessage(connId, "DO show version");
    CHECK_TRUE(rsp1.rfind("RSP OK", 0) == 0);

    std::string rsp2 = mgr.handleClientMessage(connId, "DO show zone");
    CHECK_TRUE(rsp2.rfind("RSP OK", 0) == 0);
}

TEST(AiSecurityClearanceManagerTest, Level1AlwaysRefused) {
    auto &mgr = AiSecurityClearanceManager::getInstance();
    uint64_t connId = mgr.registerConnectionForTesting(105, "192.0.2.10", 55560);
    mgr.setConnectionLevelForTesting(connId, ClientClearanceState::LEVEL2, 300);

    std::string rspWrite = mgr.handleClientMessage(connId, "DO write");
    CHECK_TRUE(rspWrite.rfind("RSP FORBIDDEN", 0) == 0);

    std::string rspReboot = mgr.handleClientMessage(connId, "DO reboot");
    CHECK_TRUE(rspReboot.rfind("RSP FORBIDDEN", 0) == 0);
}

TEST(AiSecurityClearanceManagerTest, SessionOwnerLockingAndUnwind) {
    auto &mgr = AiSecurityClearanceManager::getInstance();
    uint64_t conn1 = mgr.registerConnectionForTesting(106, "192.0.2.10", 55561);
    uint64_t conn2 = mgr.registerConnectionForTesting(107, "192.0.2.20", 55562);

    mgr.setConnectionLevelForTesting(conn1, ClientClearanceState::LEVEL2, 300);
    mgr.setConnectionLevelForTesting(conn2, ClientClearanceState::LEVEL2, 300);

    // Conn1 enters submode
    RecordingRouter::getInstance().setNextClearanceResponse(
        SshResult::SUCCESS, "Submode entered", "Router(secure-policy)#");

    std::string rsp1 = mgr.handleClientMessage(conn1, "DO secure-policy insert 1");
    CHECK_TRUE(rsp1.rfind("RSP OK", 0) == 0);
    LONGS_EQUAL(conn1, mgr.getSessionOwner());

    // Conn2 attempts Level 3 while Conn1 owns prompt -> BUSY
    std::string rsp2 = mgr.handleClientMessage(conn2, "DO show version");
    STRCMP_EQUAL("BUSY", rsp2.c_str());

    // Conn1 executes submode fragment
    RecordingRouter::getInstance().setNextClearanceResponse(
        SshResult::SUCCESS, "Action set", "Router(secure-policy)#");
    std::string rspSub = mgr.handleClientMessage(conn1, "DO action allow");
    CHECK_TRUE(rspSub.rfind("RSP OK", 0) == 0);

    // Conn1 returns to the root prompt
    RecordingRouter::getInstance().setNextClearanceResponse(
        SshResult::SUCCESS, "Exited", "Router#");
    std::string rspExit = mgr.handleClientMessage(conn1, "DO activate");
    CHECK_TRUE(rspExit.rfind("RSP OK", 0) == 0);

    // Session owner released
    LONGS_EQUAL(0, mgr.getSessionOwner());

    // Conn2 can now execute
    std::string rsp2Ok = mgr.handleClientMessage(conn2, "DO show version");
    CHECK_TRUE(rsp2Ok.rfind("RSP OK", 0) == 0);
}

TEST(AiSecurityClearanceManagerTest, DisconnectUnwindsSubmode) {
    auto &mgr = AiSecurityClearanceManager::getInstance();
    uint64_t conn1 = mgr.registerConnectionForTesting(108, "192.0.2.10", 55563);
    mgr.setConnectionLevelForTesting(conn1, ClientClearanceState::LEVEL2, 300);

    // Conn1 enters submode
    RecordingRouter::getInstance().setNextClearanceResponse(
        SshResult::SUCCESS, "Submode entered", "Router(secure-policy)#");
    mgr.handleClientMessage(conn1, "DO secure-policy insert 1");
    LONGS_EQUAL(conn1, mgr.getSessionOwner());

    // Socket disconnects while in submode
    mgr.handleSocketClosed(conn1);

    // Release the owner without sending exit
    LONGS_EQUAL(0, mgr.getSessionOwner());
    LONGS_EQUAL(0, RecordingRouter::getInstance().getUnwindCalls());
}

TEST_GROUP(AiSecurityGatewayTest) {
    void setup() {
        RecordingRouter::getInstance().reset();
        AiSecurityClearanceManager::getInstance().resetForTesting();
    }

    void teardown() {
        RecordingRouter::getInstance().reset();
        AiSecurityClearanceManager::getInstance().resetForTesting();
    }
};

TEST(AiSecurityGatewayTest, RegistrationIncludesClearanceClient) {
    nlohmann::json reg = AimonGatewayClient::getRegistrationJson(3884);
    CHECK_TRUE(reg.contains("params"));
    CHECK_TRUE(reg["params"].contains("tools"));
    const auto &tools = reg["params"]["tools"];
    bool found = false;
    for (const auto &t : tools) {
        if (t.value("name", "") == "netmon_get_clearance_client") {
            found = true;
            std::string desc = t.value("description", "");
            CHECK_TRUE(desc.find("zyxel-clearance.md") != std::string::npos);
            CHECK_TRUE(desc.find("router-clearance") != std::string::npos);
            break;
        }
    }
    CHECK_TRUE(found);
}

TEST(AiSecurityGatewayTest, ToolNetmonGetClearanceClientPayload) {
    std::string req = "{\"jsonrpc\":\"2.0\",\"id\":1001,\"method\":\"tools/call\",\"params\":{\"name\":\"netmon_get_clearance_client\",\"arguments\":{}}}";
    std::string respStr = AimonGatewayClient::getInstance().dispatchRequest(req);
    CHECK_FALSE(respStr.empty());

    nlohmann::json resp = nlohmann::json::parse(respStr);
    CHECK_TRUE(resp.contains("result"));
    CHECK_TRUE(resp["result"].contains("content"));
    const auto &content = resp["result"]["content"];
    CHECK_TRUE(content.is_array());
    CHECK_FALSE(content.empty());

    std::string text = content[0].value("text", "");
    CHECK_FALSE(text.empty());

    nlohmann::json toolRes = nlohmann::json::parse(text);
    LONGS_EQUAL(1, toolRes.value("protocol_version", 0));
    STRCMP_EQUAL("netmon-ai-client.c", toolRes.value("filename", "").c_str());
    CHECK_TRUE(toolRes.contains("compiler_argv"));
    CHECK_TRUE(toolRes.contains("listen"));
    STRCMP_EQUAL(".agents/skills/netmon/references/zyxel-clearance.md", toolRes.value("skill", "").c_str());
    CHECK_TRUE(toolRes.contains("commands"));
    CHECK_TRUE(toolRes.contains("procedure"));
    CHECK_TRUE(toolRes.contains("source"));
    CHECK_TRUE(toolRes.contains("sha256"));

    // Verify SHA-256 matches actual source
    std::string source = toolRes.value("source", "");
    std::string sha256Expected = toolRes.value("sha256", "");
    unsigned char hash[SHA256_DIGEST_LENGTH];
    SHA256(reinterpret_cast<const unsigned char*>(source.data()), source.size(), hash);
    char hexBuf[SHA256_DIGEST_LENGTH * 2 + 1];
    for (int i = 0; i < SHA256_DIGEST_LENGTH; ++i) {
        sprintf(hexBuf + i * 2, "%02x", hash[i]);
    }
    STRCMP_EQUAL(hexBuf, sha256Expected.c_str());
}

TEST(AiSecurityGatewayTest, FirewallBlockAndUnblockRefuseMutation) {
    std::string reqBlock = "{\"jsonrpc\":\"2.0\",\"id\":1002,\"method\":\"tools/call\",\"params\":{\"name\":\"firewall_block_ip\",\"arguments\":{\"ip\":\"1.2.3.4\",\"reason\":\"test\"}}}";
    std::string respBlockStr = AimonGatewayClient::getInstance().dispatchRequest(reqBlock);
    nlohmann::json respBlock = nlohmann::json::parse(respBlockStr);
    std::string textBlock = respBlock["result"]["content"][0].value("text", "");
    CHECK_TRUE(textBlock.find("This tool no longer changes the firewall.") != std::string::npos);
    CHECK_TRUE(textBlock.find("zyxel-clearance.md") != std::string::npos);
    CHECK_TRUE(textBlock.find("netmon_get_clearance_client") != std::string::npos);

    std::string reqUnblock = "{\"jsonrpc\":\"2.0\",\"id\":1003,\"method\":\"tools/call\",\"params\":{\"name\":\"firewall_unblock_ip\",\"arguments\":{\"ip\":\"1.2.3.4\"}}}";
    std::string respUnblockStr = AimonGatewayClient::getInstance().dispatchRequest(reqUnblock);
    nlohmann::json respUnblock = nlohmann::json::parse(respUnblockStr);
    std::string textUnblock = respUnblock["result"]["content"][0].value("text", "");
    CHECK_TRUE(textUnblock.find("This tool no longer changes the firewall.") != std::string::npos);
    CHECK_TRUE(textUnblock.find("zyxel-clearance.md") != std::string::npos);
    CHECK_TRUE(textUnblock.find("netmon_get_clearance_client") != std::string::npos);

    // Verify router driver was never called
    LONGS_EQUAL(0, RecordingRouter::getInstance().getCallCount());
}

TEST_GROUP(UnixAuthTest) {
    void setup() {
        UnixAuth::resetForTesting();
    }

    void teardown() {
        UnixAuth::resetForTesting();
    }
};

TEST(UnixAuthTest, GetCurrentUsernameReturnsNonEmpty) {
    std::string user = UnixAuth::getCurrentUsername();
    CHECK_FALSE(user.empty());
}

TEST(UnixAuthTest, MockAuthSuccessAndFailure) {
    UnixAuth::setAuthVerifierForTesting([](const std::string &u, const std::string &p) {
        return (u == "samurai" && p == "correct_pass_123");
    });

    CHECK_TRUE(UnixAuth::authenticateUser("samurai", "correct_pass_123"));
    CHECK_FALSE(UnixAuth::authenticateUser("samurai", "wrong_pass"));
    CHECK_FALSE(UnixAuth::authenticateUser("intruder", "correct_pass_123"));
}

TEST(UnixAuthTest, EmptyCredentialsRejected) {
    CHECK_FALSE(UnixAuth::authenticateUser("", ""));
    CHECK_FALSE(UnixAuth::authenticateUser("samurai", ""));
    CHECK_FALSE(UnixAuth::authenticateUser("", "pass"));
}

TEST_GROUP(ConfigAuditFileTest) {
    void setup() {
        Config::getInstance().resetForTesting();
    }

    void teardown() {
        Config::getInstance().resetForTesting();
    }
};

TEST(ConfigAuditFileTest, DefaultAuditFilePath) {
    STRCMP_EQUAL("~/.config/netmon/audit.log", Config::getInstance().getAuditFile().c_str());
}

TEST(ConfigAuditFileTest, SetAuditFilePath) {
    Config::getInstance().setAuditFile("test_data/custom_audit.log");
    STRCMP_EQUAL("test_data/custom_audit.log", Config::getInstance().getAuditFile().c_str());
}

TEST_GROUP(AiClearanceAuditPersistenceTest) {
    const std::string testLogPath = "test_data/test_clearance_audit.log";

    void setup() {
        RecordingRouter::getInstance().reset();
        AiSecurityClearanceManager::getInstance().resetForTesting();
        Config::getInstance().resetForTesting();
        Config::getInstance().setAuditFile(testLogPath);
        unlink(testLogPath.c_str());
    }

    void teardown() {
        unlink(testLogPath.c_str());
        AiSecurityClearanceManager::getInstance().resetForTesting();
        Config::getInstance().resetForTesting();
        RecordingRouter::getInstance().reset();
    }
};

TEST(AiClearanceAuditPersistenceTest, DoCommandsPersistStructuredJsonRecordsToDisk) {
    auto &mgr = AiSecurityClearanceManager::getInstance();
    uint64_t connId = mgr.registerConnectionForTesting(200, "192.0.2.99", 43210);

    // Handshake and grant
    mgr.handleClientMessage(connId, "HELLO 1");
    mgr.setConnectionLevelForTesting(connId, ClientClearanceState::LEVEL2, 300);

    // 1. Authorized read command
    std::string rspShow = mgr.handleClientMessage(connId, "DO show version");
    CHECK_TRUE(rspShow.rfind("RSP OK", 0) == 0);

    // 2. Forbidden Level 1 command
    std::string rspReboot = mgr.handleClientMessage(connId, "DO reboot");
    CHECK_TRUE(rspReboot.rfind("RSP FORBIDDEN", 0) == 0);

    // 3. Unclassified smuggled command
    std::string rspSmuggle = mgr.handleClientMessage(connId, "DO show version; reboot");
    CHECK_TRUE(rspSmuggle.rfind("RSP UNCLASSIFIED", 0) == 0);

    // Verify file exists
    std::ifstream infile(testLogPath);
    CHECK_TRUE(infile.is_open());

    // Verify permissions: 0600
    struct stat st;
    CHECK_EQUAL(0, stat(testLogPath.c_str(), &st));
    CHECK_EQUAL(0600, st.st_mode & 0777);

    // Read lines
    std::vector<std::string> lines;
    std::string line;
    while (std::getline(infile, line)) {
        if (!line.empty()) {
            lines.push_back(line);
        }
    }
    infile.close();

    LONGS_EQUAL(3, lines.size());

    // Check line 1: show version
    nlohmann::json obj1 = nlohmann::json::parse(lines[0]);
    STRCMP_EQUAL("show version", obj1.value("command", "").c_str());
    STRCMP_EQUAL("OK", obj1.value("decision", "").c_str());
    STRCMP_EQUAL("cmdShowVersion", obj1.value("matched_method", "").c_str());
    LONGS_EQUAL(3, obj1.value("required_level", 0));
    LONGS_EQUAL(connId, obj1.value("connection_id", 0));
    STRCMP_EQUAL("192.0.2.99", obj1.value("peer_address", "").c_str());
    LONGS_EQUAL(43210, obj1.value("peer_port", 0));
    LONGS_EQUAL(0, obj1.value("ssh_result", -1));

    // Check line 2: reboot
    nlohmann::json obj2 = nlohmann::json::parse(lines[1]);
    STRCMP_EQUAL("reboot", obj2.value("command", "").c_str());
    STRCMP_EQUAL("FORBIDDEN", obj2.value("decision", "").c_str());
    LONGS_EQUAL(1, obj2.value("required_level", 0));

    // Check line 3: smuggled command
    nlohmann::json obj3 = nlohmann::json::parse(lines[2]);
    STRCMP_EQUAL("show version; reboot", obj3.value("command", "").c_str());
    STRCMP_EQUAL("UNCLASSIFIED", obj3.value("decision", "").c_str());
    STRCMP_EQUAL("UNCLASSIFIED", obj3.value("matched_method", "").c_str());
}

TEST(AiSecurityClearanceManagerTest, FormatAndParseConnIdUniversal) {
    STRCMP_EQUAL("conn-0001", AiSecurityClearanceManager::formatConnId(1).c_str());
    STRCMP_EQUAL("conn-0003", AiSecurityClearanceManager::formatConnId(3).c_str());
    STRCMP_EQUAL("conn-0042", AiSecurityClearanceManager::formatConnId(42).c_str());
    STRCMP_EQUAL("conn-1234", AiSecurityClearanceManager::formatConnId(1234).c_str());
    STRCMP_EQUAL("conn-99999", AiSecurityClearanceManager::formatConnId(99999).c_str());

    LONGS_EQUAL(3, AiSecurityClearanceManager::parseConnId("conn-0003"));
    LONGS_EQUAL(3, AiSecurityClearanceManager::parseConnId("conn-3"));
    LONGS_EQUAL(0, AiSecurityClearanceManager::parseConnId("3")); // Bare integer rejected
    LONGS_EQUAL(0, AiSecurityClearanceManager::parseConnId("2")); // Bare integer rejected
    LONGS_EQUAL(42, AiSecurityClearanceManager::parseConnId("conn-0042"));
    LONGS_EQUAL(0, AiSecurityClearanceManager::parseConnId("invalid"));
    LONGS_EQUAL(0, AiSecurityClearanceManager::parseConnId("conn-"));
    LONGS_EQUAL(0, AiSecurityClearanceManager::parseConnId("conn-0"));
    LONGS_EQUAL(0, AiSecurityClearanceManager::parseConnId(""));
}

TEST(AiSecurityClearanceManagerTest, ShellRejectsBareIntegerAndAcceptsAuthList) {
    LONGS_EQUAL(1, NetMonShell::getInstance().executeCommand("grant 3"));
    CHECK_FALSE(NetMonShell::getInstance().isExecutingCommand());
    LONGS_EQUAL(1, NetMonShell::getInstance().executeCommand("grant 2 120"));
    CHECK_FALSE(NetMonShell::getInstance().isExecutingCommand());
    LONGS_EQUAL(1, NetMonShell::getInstance().executeCommand("deny 3"));
    CHECK_FALSE(NetMonShell::getInstance().isExecutingCommand());
    LONGS_EQUAL(1, NetMonShell::getInstance().executeCommand("deny 2"));
    CHECK_FALSE(NetMonShell::getInstance().isExecutingCommand());
    LONGS_EQUAL(0, NetMonShell::getInstance().executeCommand("auth list"));
    CHECK_FALSE(NetMonShell::getInstance().isExecutingCommand());
    LONGS_EQUAL(1, NetMonShell::getInstance().executeCommand("auth set-password"));
    CHECK_FALSE(NetMonShell::getInstance().isExecutingCommand());
}

TEST(AiSecurityClearanceManagerTest, CustomDurationGrantAndRemainingTimer) {
    auto &mgr = AiSecurityClearanceManager::getInstance();
    mgr.resetForTesting();

    int fds[2];
    CHECK_EQUAL(0, socketpair(AF_UNIX, SOCK_STREAM, 0, fds));

    uint64_t connId = mgr.registerConnectionForTesting(fds[0], "192.0.2.99", 54321);
    STRCMP_EQUAL("WAITING 1", mgr.handleClientMessage(connId, "REQUEST tier=write").c_str());

    // Grant with custom 120s duration
    CHECK_TRUE(mgr.consoleApprove(connId, 120));

    // Verify socket received GRANTED 1 120
    uint32_t netLen = 0;
    CHECK_EQUAL(sizeof(netLen), recv(fds[1], &netLen, sizeof(netLen), 0));
    uint32_t len = ntohl(netLen);
    std::vector<char> buf(len + 1, 0);
    CHECK_EQUAL(len, recv(fds[1], buf.data(), len, 0));
    STRCMP_EQUAL("GRANTED 1 120", buf.data());

    auto clearances = mgr.getActiveClearances();
    LONGS_EQUAL(1, clearances.size());
    LONGS_EQUAL(connId, clearances[0].connectionId);
    STRCMP_EQUAL("conn-0001", clearances[0].formattedConnId.c_str());
    STRCMP_EQUAL("192.0.2.99", clearances[0].peerAddress.c_str());
    LONGS_EQUAL(54321, clearances[0].peerPort);
    CHECK_TRUE(clearances[0].clearanceState == ClientClearanceState::LEVEL2);
    CHECK_TRUE(clearances[0].remainingSeconds <= 120 && clearances[0].remainingSeconds >= 115);

    close(fds[0]);
    close(fds[1]);
    mgr.resetForTesting();
}

TEST(AiSecurityClearanceManagerTest, RequestWithMetadataStoresAndListsInActiveClearances) {
    auto &mgr = AiSecurityClearanceManager::getInstance();
    mgr.resetForTesting();

    int fds[2];
    CHECK_EQUAL(0, socketpair(AF_UNIX, SOCK_STREAM, 0, fds));

    uint64_t connId = mgr.registerConnectionForTesting(fds[0], "192.0.2.39", 56488);
    std::string req = "REQUEST tier=\"RW\" platform=\"Antigravity IDE (test-client)\" model=\"Gemini-2.0\" session=\"test-123\"";
    STRCMP_EQUAL("WAITING 1", mgr.handleClientMessage(connId, req).c_str());

    auto clearances = mgr.getActiveClearances();
    LONGS_EQUAL(1, clearances.size());
    LONGS_EQUAL(connId, clearances[0].connectionId);
    STRCMP_EQUAL("conn-0001", clearances[0].formattedConnId.c_str());
    CHECK_TRUE(clearances[0].clearanceState == ClientClearanceState::PENDING);
    STRCMP_EQUAL("tier=\"RW\" platform=\"Antigravity IDE (test-client)\" model=\"Gemini-2.0\" session=\"test-123\"",
                 clearances[0].metadata.c_str());

    close(fds[0]);
    close(fds[1]);
    mgr.resetForTesting();
}

TEST(AiSecurityClearanceManagerTest, ConsoleDenyLockoutRejectsRepeatRequests) {
    auto &mgr = AiSecurityClearanceManager::getInstance();
    mgr.resetForTesting();

    int fds[2];
    CHECK_EQUAL(0, socketpair(AF_UNIX, SOCK_STREAM, 0, fds));

    uint64_t connId = mgr.registerConnectionForTesting(fds[0], "192.0.2.42", 51234);
    STRCMP_EQUAL("WAITING 1", mgr.handleClientMessage(connId, "REQUEST tier=write").c_str());

    uint64_t pendingId = 0;
    std::string peerIp;
    uint16_t peerPort = 0;
    CHECK_TRUE(mgr.hasPendingApproval(pendingId, peerIp, peerPort));
    LONGS_EQUAL(connId, pendingId);

    // Operator denies with 60-second lockout
    CHECK_TRUE(mgr.consoleDeny(connId, 60));

    // Verify socket received framed DENIED 1
    uint32_t netLen = 0;
    CHECK_EQUAL(sizeof(netLen), recv(fds[1], &netLen, sizeof(netLen), 0));
    uint32_t len = ntohl(netLen);
    std::vector<char> buf(len + 1, 0);
    CHECK_EQUAL(len, recv(fds[1], buf.data(), len, 0));
    STRCMP_EQUAL("DENIED 1", buf.data());

    // Pending state cleared immediately
    CHECK_FALSE(mgr.hasPendingApproval(pendingId, peerIp, peerPort));

    // getActiveClearances reports DENIED with active remainingSeconds
    auto clearances = mgr.getActiveClearances();
    LONGS_EQUAL(1, clearances.size());
    LONGS_EQUAL(connId, clearances[0].connectionId);
    STRCMP_EQUAL("conn-0001", clearances[0].formattedConnId.c_str());
    CHECK_TRUE(clearances[0].clearanceState == ClientClearanceState::DENIED);
    CHECK_TRUE(clearances[0].remainingSeconds <= 60 && clearances[0].remainingSeconds >= 55);

    // Shell auth list displays cleanly without crash or leak
    LONGS_EQUAL(0, NetMonShell::getInstance().executeCommand("auth list"));
    CHECK_FALSE(NetMonShell::getInstance().isExecutingCommand());

    // Client repeats REQUEST during lockout: rejected immediately, does NOT re-prompt console
    STRCMP_EQUAL("DENIED 1", mgr.handleClientMessage(connId, "REQUEST tier=write").c_str());
    CHECK_FALSE(mgr.hasPendingApproval(pendingId, peerIp, peerPort));

    // Simulate lockout expiry
    mgr.setConnectionLevelForTesting(connId, ClientClearanceState::DENIED, 0);
    clearances = mgr.getActiveClearances();
    LONGS_EQUAL(1, clearances.size());
    CHECK_TRUE(clearances[0].clearanceState == ClientClearanceState::NONE);
    LONGS_EQUAL(0, clearances[0].remainingSeconds);

    // Client repeats REQUEST after expiry: allowed to re-prompt console
    STRCMP_EQUAL("WAITING 1", mgr.handleClientMessage(connId, "REQUEST tier=write").c_str());
    CHECK_TRUE(mgr.hasPendingApproval(pendingId, peerIp, peerPort));
    LONGS_EQUAL(connId, pendingId);

    close(fds[0]);
    close(fds[1]);
    mgr.resetForTesting();
}

TEST(AiSecurityClearanceManagerTest, GracefulDowngradeToReadOnGrantExpiry) {
    auto &mgr = AiSecurityClearanceManager::getInstance();
    uint64_t connId = mgr.registerConnectionForTesting(105, "192.0.2.88", 55588);

    // Initial state is blocked per Envelope 3
    std::string rspInit = mgr.handleClientMessage(connId, "DO show version");
    CHECK_TRUE(rspInit.rfind("RSP NOT_AUTHORIZED", 0) == 0);

    // Request elevation to READ/WRITE
    mgr.handleClientMessage(connId, "REQUEST tier=write");
    CHECK_TRUE(mgr.consoleApprove(connId, 1, ClearanceTier::READ_WRITE));

    // While granted, reads and mutations are authorized
    std::string rspMut = mgr.handleClientMessage(connId, "DO secure-policy insert 1");
    CHECK_FALSE(rspMut.rfind("RSP NOT_AUTHORIZED", 0) == 0);

    // Sleep past 1s grant deadline
    std::this_thread::sleep_for(std::chrono::milliseconds(1100));

    // After grant expiry: mutation is blocked
    std::string rspMutExpired = mgr.handleClientMessage(connId, "DO secure-policy insert 1");
    CHECK_TRUE(rspMutExpired.rfind("RSP NOT_AUTHORIZED", 0) == 0);

    // After grant expiry: read is allowed (gracefully downgraded to READ)
    std::string rspReadAfterExpiry = mgr.handleClientMessage(connId, "DO show version");
    CHECK_TRUE(rspReadAfterExpiry.rfind("RSP OK", 0) == 0);

    mgr.resetForTesting();
}

TEST(AiSecurityClearanceManagerTest, ReadGrantHasIndefiniteTimeout) {
    auto &mgr = AiSecurityClearanceManager::getInstance();
    uint64_t connId = mgr.registerConnectionForTesting(106, "192.0.2.89", 55589);

    mgr.handleClientMessage(connId, "REQUEST tier=read");
    CHECK_TRUE(mgr.consoleApprove(connId, 0, ClearanceTier::READ));

    auto clearances = mgr.getActiveClearances();
    LONGS_EQUAL(1, clearances.size());
    CHECK_TRUE(clearances[0].clearanceState == ClientClearanceState::READ);
    LONGS_EQUAL(0, clearances[0].remainingSeconds);

    std::string rsp = mgr.handleClientMessage(connId, "DO show version");
    CHECK_TRUE(rsp.rfind("RSP OK", 0) == 0);

    mgr.resetForTesting();
}

TEST(AiSecurityClearanceManagerTest, ReadGrantFiniteDurationExpiresToNone) {
    auto &mgr = AiSecurityClearanceManager::getInstance();
    uint64_t connId = mgr.registerConnectionForTesting(107, "192.0.2.90", 55590);

    // Initial state rejected
    std::string rspInit = mgr.handleClientMessage(connId, "DO show version");
    CHECK_TRUE(rspInit.rfind("RSP NOT_AUTHORIZED", 0) == 0);

    // Request and grant READ for 1 second
    mgr.handleClientMessage(connId, "REQUEST tier=read");
    CHECK_TRUE(mgr.consoleApprove(connId, 1, ClearanceTier::READ));

    // While granted: read succeeds
    std::string rspActive = mgr.handleClientMessage(connId, "DO show version");
    CHECK_TRUE(rspActive.rfind("RSP OK", 0) == 0);

    // Sleep past 1s deadline
    std::this_thread::sleep_for(std::chrono::milliseconds(1100));

    // After finite READ expiry: drops back to NONE and read is rejected
    std::string rspExpired = mgr.handleClientMessage(connId, "DO show version");
    CHECK_TRUE(rspExpired.rfind("RSP NOT_AUTHORIZED", 0) == 0);

    mgr.resetForTesting();
}

TEST(AiSecurityClearanceManagerTest, ClearanceTierToStringValues) {
    STRCMP_EQUAL("NONE", clearanceTierToString(ClearanceTier::NONE).c_str());
    STRCMP_EQUAL("READ", clearanceTierToString(ClearanceTier::READ).c_str());
    STRCMP_EQUAL("READ/WRITE", clearanceTierToString(ClearanceTier::READ_WRITE).c_str());
    STRCMP_EQUAL("READ/WRITE + PASSWORD ACCESS!!!", clearanceTierToString(ClearanceTier::READ_WRITE_PASSWORD).c_str());
}

TEST(AiSecurityClearanceManagerTest, AimonGatewayClientFirewallGetMetrics) {
    auto reg = AimonGatewayClient::getRegistrationJson(3884);
    bool foundMetricsTool = false;
    for (const auto &tool : reg["params"]["tools"]) {
        if (tool["name"] == "firewall_get_metrics") {
            foundMetricsTool = true;
            break;
        }
    }
    CHECK_TRUE(foundMetricsTool);

    auto &client = AimonGatewayClient::getInstance();
    client.setRouterDriver(std::shared_ptr<RouterDriver>(&RecordingRouter::getInstance(), [](RouterDriver *) {}));

    std::string req = "{\"jsonrpc\":\"2.0\",\"id\":99,\"method\":\"tools/call\",\"params\":{\"name\":\"firewall_get_metrics\",\"arguments\":{}}}";
    std::string rsp = client.dispatchRequest(req);
    CHECK_FALSE(rsp.empty());

    nlohmann::json parsedRsp = nlohmann::json::parse(rsp);
    CHECK_EQUAL(99, parsedRsp["id"].get<int>());
    CHECK_TRUE(parsedRsp.contains("result"));
    CHECK_TRUE(parsedRsp["result"]["content"][0]["text"].get<std::string>().find("RecordingRouter") != std::string::npos);

    client.setRouterDriver(nullptr);
}

TEST(AiSecurityClearanceManagerTest, BareRequestRejectedWithSyntaxError) {
    auto &mgr = AiSecurityClearanceManager::getInstance();
    mgr.resetForTesting();

    uint64_t connId = mgr.registerConnectionForTesting(201, "192.0.2.101", 54001);

    // Bare REQUEST with no tier attribute must be rejected immediately with SYNTAX error
    std::string rsp = mgr.handleClientMessage(connId, "REQUEST");
    STRCMP_EQUAL("RSP SYNTAX - Missing or invalid security tier (must be R, RW, or RWP)", rsp.c_str());

    // Connection remains ungranted and not pending
    uint64_t pendingId = 0;
    std::string peerIp;
    uint16_t peerPort = 0;
    CHECK_FALSE(mgr.hasPendingApproval(pendingId, peerIp, peerPort));

    auto clearances = mgr.getActiveClearances();
    LONGS_EQUAL(1, clearances.size());
    CHECK_TRUE(clearances[0].clearanceState == ClientClearanceState::NONE);

    // Any DO command is rejected
    std::string rspDo = mgr.handleClientMessage(connId, "DO show version");
    CHECK_TRUE(rspDo.rfind("RSP NOT_AUTHORIZED", 0) == 0);

    mgr.resetForTesting();
}

TEST(AiSecurityClearanceManagerTest, InvalidTierRejectedWithSyntaxError) {
    auto &mgr = AiSecurityClearanceManager::getInstance();
    mgr.resetForTesting();

    uint64_t connId = mgr.registerConnectionForTesting(202, "192.0.2.102", 54002);

    // Unknown tier string rejected
    std::string rspUnknown = mgr.handleClientMessage(connId, "REQUEST tier=superuser");
    STRCMP_EQUAL("RSP SYNTAX - Missing or invalid security tier (must be R, RW, or RWP)", rspUnknown.c_str());

    // Empty tier attribute rejected
    std::string rspEmpty = mgr.handleClientMessage(connId, "REQUEST tier=\"\"");
    STRCMP_EQUAL("RSP SYNTAX - Missing or invalid security tier (must be R, RW, or RWP)", rspEmpty.c_str());

    // Trailing/arbitrary words without tier rejected
    std::string rspBogus = mgr.handleClientMessage(connId, "REQUEST platform=\"Linux\" model=\"Test\"");
    STRCMP_EQUAL("RSP SYNTAX - Missing or invalid security tier (must be R, RW, or RWP)", rspBogus.c_str());

    uint64_t pendingId = 0;
    std::string peerIp;
    uint16_t peerPort = 0;
    CHECK_FALSE(mgr.hasPendingApproval(pendingId, peerIp, peerPort));

    mgr.resetForTesting();
}

TEST(AiSecurityClearanceManagerTest, AllThreeTiersAcceptedWithProperRequestedTier) {
    auto &mgr = AiSecurityClearanceManager::getInstance();
    mgr.resetForTesting();

    // 1. Tier R (Read)
    uint64_t conn1 = mgr.registerConnectionForTesting(203, "192.0.2.103", 54003);
    std::string rsp1 = mgr.handleClientMessage(conn1, "REQUEST tier=R platform=\"Test\" model=\"Gemini\"");
    STRCMP_EQUAL("WAITING 1", rsp1.c_str());

    uint64_t pendingId = 0;
    std::string peerIp;
    uint16_t peerPort = 0;
    ClearanceTier reqTier = ClearanceTier::NONE;
    CHECK_TRUE(mgr.hasPendingApproval(pendingId, peerIp, peerPort, &reqTier));
    LONGS_EQUAL(conn1, pendingId);
    CHECK_TRUE(reqTier == ClearanceTier::READ);

    CHECK_TRUE(mgr.consoleApprove(pendingId, 0, reqTier));
    auto cl1 = mgr.getActiveClearances();
    LONGS_EQUAL(1, cl1.size());
    CHECK_TRUE(cl1[0].clearanceState == ClientClearanceState::READ);
    LONGS_EQUAL(0, cl1[0].remainingSeconds);

    mgr.resetForTesting();

    // 2. Tier RW (Read/Write)
    uint64_t conn2 = mgr.registerConnectionForTesting(204, "192.0.2.104", 54004);
    std::string rsp2 = mgr.handleClientMessage(conn2, "REQUEST tier=\"RW\"");
    STRCMP_EQUAL("WAITING 1", rsp2.c_str());

    reqTier = ClearanceTier::NONE;
    CHECK_TRUE(mgr.hasPendingApproval(pendingId, peerIp, peerPort, &reqTier));
    LONGS_EQUAL(conn2, pendingId);
    CHECK_TRUE(reqTier == ClearanceTier::READ_WRITE);

    CHECK_TRUE(mgr.consoleApprove(pendingId, 300, reqTier));
    auto cl2 = mgr.getActiveClearances();
    LONGS_EQUAL(1, cl2.size());
    CHECK_TRUE(cl2[0].clearanceState == ClientClearanceState::LEVEL2);

    mgr.resetForTesting();

    // 3. Tier RWP (Read/Write + Password)
    uint64_t conn3 = mgr.registerConnectionForTesting(205, "192.0.2.105", 54005);
    std::string rsp3 = mgr.handleClientMessage(conn3, "REQUEST tier=\"RWP\"");
    STRCMP_EQUAL("WAITING 1", rsp3.c_str());

    reqTier = ClearanceTier::NONE;
    CHECK_TRUE(mgr.hasPendingApproval(pendingId, peerIp, peerPort, &reqTier));
    LONGS_EQUAL(conn3, pendingId);
    CHECK_TRUE(reqTier == ClearanceTier::READ_WRITE_PASSWORD);

    CHECK_TRUE(mgr.consoleApprove(pendingId, 300, reqTier));
    auto cl3 = mgr.getActiveClearances();
    LONGS_EQUAL(1, cl3.size());
    CHECK_TRUE(cl3[0].clearanceState == ClientClearanceState::LEVEL2);

    mgr.resetForTesting();
}

TEST(AiSecurityClearanceManagerTest, ShellGrantCommandDefaultsToRequestedTier) {
    auto &mgr = AiSecurityClearanceManager::getInstance();
    mgr.resetForTesting();

    uint64_t connId = mgr.registerConnectionForTesting(206, "192.0.2.106", 54006);

    // Client requests READ
    STRCMP_EQUAL("WAITING 1", mgr.handleClientMessage(connId, "REQUEST tier=read").c_str());

    // Operator executes 'grant conn-0001' with NO tier specified
    // Must prompt password, verify it, approve as READ, and default to indefinite
    NetMonShell::getInstance().setPasswordReaderForTesting([](const std::string &) { return "test_pass_123"; });
    UnixAuth::setAuthVerifierForTesting([](const std::string &, const std::string &p) { return p == "test_pass_123"; });

    LONGS_EQUAL(0, NetMonShell::getInstance().executeCommand("grant conn-0001"));

    auto cl = mgr.getActiveClearances();
    LONGS_EQUAL(1, cl.size());
    CHECK_TRUE(cl[0].clearanceState == ClientClearanceState::READ);
    LONGS_EQUAL(0, cl[0].remainingSeconds);

    // Can execute read commands
    std::string rspRead = mgr.handleClientMessage(connId, "DO show version");
    CHECK_TRUE(rspRead.rfind("RSP OK", 0) == 0);

    // Mutation commands remain blocked
    std::string rspMut = mgr.handleClientMessage(connId, "DO secure-policy insert 1");
    CHECK_TRUE(rspMut.rfind("RSP NOT_AUTHORIZED", 0) == 0);

    UnixAuth::resetForTesting();
    NetMonShell::getInstance().resetForTesting();
    mgr.resetForTesting();
}

TEST(AiSecurityClearanceManagerTest, ShellGrantRejectsInvalidPasswordForAllTiers) {
    auto &mgr = AiSecurityClearanceManager::getInstance();
    mgr.resetForTesting();
    NetMonShell::getInstance().resetForTesting();

    uint64_t connId = mgr.registerConnectionForTesting(207, "192.0.2.107", 54007);
    STRCMP_EQUAL("WAITING 1", mgr.handleClientMessage(connId, "REQUEST tier=read").c_str());

    // Wrong password -> grant must be rejected and clearance denied
    NetMonShell::getInstance().setPasswordReaderForTesting([](const std::string &) { return "wrong_password"; });
    UnixAuth::setAuthVerifierForTesting([](const std::string &, const std::string &p) { return p == "correct_pass"; });

    LONGS_EQUAL(1, NetMonShell::getInstance().executeCommand("grant conn-0001"));

    auto cl = mgr.getActiveClearances();
    LONGS_EQUAL(1, cl.size());
    CHECK_TRUE(cl[0].clearanceState == ClientClearanceState::DENIED);

    // Socket remains denied/unauthorized
    std::string rsp = mgr.handleClientMessage(connId, "DO show version");
    CHECK_TRUE(rsp.rfind("RSP NOT_AUTHORIZED", 0) == 0);

    UnixAuth::resetForTesting();
    NetMonShell::getInstance().resetForTesting();
    mgr.resetForTesting();
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


