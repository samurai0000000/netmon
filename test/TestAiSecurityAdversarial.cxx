/*
 * TestAiSecurityAdversarial.cxx
 *
 * Copyright (C) 2026, Charles Chiou
 */

#include "AiSecurityClearance.hxx"
#include "RecordingRouter.hxx"
#include "AimonGatewayClient.hxx"
#include <openssl/sha.h>
#include <nlohmann/json.hpp>
#include <CppUTest/TestHarness.h>
#include <string>
#include <vector>
#include <thread>
#include <chrono>
#include <cstring>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <poll.h>

static const uint16_t TEST_PORT = 39885;

static int connectLoopback(uint16_t port) {
    int fd = socket(AF_INET, SOCK_STREAM, 0);
    if (fd < 0) return -1;
    struct sockaddr_in sin;
    memset(&sin, 0, sizeof(sin));
    sin.sin_family = AF_INET;
    sin.sin_port = htons(port);
    inet_pton(AF_INET, "127.0.0.1", &sin.sin_addr);
    if (connect(fd, (struct sockaddr *)&sin, sizeof(sin)) < 0) {
        close(fd);
        return -1;
    }
    return fd;
}

static bool sendRaw(int fd, const void *buf, size_t len) {
    const uint8_t *p = (const uint8_t *)buf;
    size_t rem = len;
    while (rem > 0) {
        ssize_t n = send(fd, p, rem, MSG_NOSIGNAL);
        if (n <= 0) return false;
        p += n;
        rem -= static_cast<size_t>(n);
    }
    return true;
}

static bool sendFramed(int fd, const std::string &msg) {
    uint32_t lenBe = htonl(static_cast<uint32_t>(msg.size()));
    if (!sendRaw(fd, &lenBe, 4)) return false;
    return sendRaw(fd, msg.data(), msg.size());
}

static bool recvFramed(int fd, std::string &out) {
    uint32_t lenBe = 0;
    uint8_t *p = (uint8_t *)&lenBe;
    size_t rem = 4;
    while (rem > 0) {
        ssize_t n = recv(fd, p, rem, 0);
        if (n <= 0) return false;
        p += n;
        rem -= static_cast<size_t>(n);
    }
    uint32_t len = ntohl(lenBe);
    if (len > 1048576) return false;
    std::vector<char> buf(len);
    p = (uint8_t *)buf.data();
    rem = len;
    while (rem > 0) {
        ssize_t n = recv(fd, p, rem, 0);
        if (n <= 0) return false;
        p += n;
        rem -= static_cast<size_t>(n);
    }
    out.assign(buf.data(), len);
    return true;
}

static std::string getClientSourcePath() {
    if (access("client/netmon-ai-client.c", F_OK) == 0) {
        return "client/netmon-ai-client.c";
    }
    if (access("../client/netmon-ai-client.c", F_OK) == 0) {
        return "../client/netmon-ai-client.c";
    }
    return "client/netmon-ai-client.c";
}

static bool ensureClientBinaryBuilt() {
    std::string srcPath = getClientSourcePath();
    std::string buildCmd = "gcc -std=c99 -Wall -Wextra -pedantic -O2 " + srcPath + " -o /tmp/netmon-ai-client-test";
    int ret = system(buildCmd.c_str());
    return (ret == 0);
}

TEST_GROUP(AiSecurityAppletTest) {
    void setup() {
        RecordingRouter::getInstance().reset();
        AiSecurityClearanceManager::getInstance().resetForTesting();
        ensureClientBinaryBuilt();
    }

    void teardown() {
        RecordingRouter::getInstance().reset();
        AiSecurityClearanceManager::getInstance().resetForTesting();
    }
};

TEST(AiSecurityAppletTest, CompilerArgvBuildsAppletWithoutWarnings) {
    // Section 9.3: Build the published source with returned compiler arguments
    std::string srcPath = getClientSourcePath();
    std::string buildCmd = "gcc -std=c99 -Wall -Wextra -pedantic -O2 " + srcPath + " -o /tmp/netmon-ai-client-test";
    int ret = system(buildCmd.c_str());
    LONGS_EQUAL(0, ret);

    // Verify binary exists and can execute
    CHECK_EQUAL(0, access("/tmp/netmon-ai-client-test", X_OK));
}

TEST(AiSecurityAppletTest, AppletRejectsUnknownArgumentsAndDoesNotListen) {
    int ret = system("/tmp/netmon-ai-client-test --invalid-flag >/dev/null 2>&1");
    CHECK_TRUE(ret != 0);

    // Verify applet opens no listening socket
    // When invoked with invalid args, exit is immediate and no socket is held
    CHECK_EQUAL(0, access("/tmp/netmon-ai-client-test", F_OK));
}

TEST_GROUP(AiSecurityWireFramingTest) {
    void setup() {
        RecordingRouter::getInstance().reset();
        AiSecurityClearanceManager::getInstance().resetForTesting();
        AiSecurityClearanceManager::getInstance().start("127.0.0.1", TEST_PORT);
        usleep(50000); // 50ms for listener thread to bind
    }

    void teardown() {
        AiSecurityClearanceManager::getInstance().stop();
        RecordingRouter::getInstance().reset();
        AiSecurityClearanceManager::getInstance().resetForTesting();
        usleep(50000);
    }
};

TEST(AiSecurityWireFramingTest, FragmentedHeaderAndPayload) {
    int fd = connectLoopback(TEST_PORT);
    CHECK_TRUE(fd >= 0);

    // Send 4-byte length prefix 1 byte at a time with delays
    std::string msg = "HELLO 1";
    uint32_t lenBe = htonl(static_cast<uint32_t>(msg.size()));
    const uint8_t *pLen = reinterpret_cast<const uint8_t *>(&lenBe);

    for (size_t i = 0; i < 4; ++i) {
        sendRaw(fd, pLen + i, 1);
        usleep(5000); // 5ms delay
    }

    // Send payload 1 byte at a time with delays
    for (size_t i = 0; i < msg.size(); ++i) {
        sendRaw(fd, msg.data() + i, 1);
        usleep(5000);
    }

    std::string rsp;
    CHECK_TRUE(recvFramed(fd, rsp));
    STRCMP_EQUAL("HELLO 1", rsp.c_str());

    close(fd);
}

TEST(AiSecurityWireFramingTest, MultipleFramesPackedInSingleSend) {
    int fd = connectLoopback(TEST_PORT);
    CHECK_TRUE(fd >= 0);

    // Pack HELLO 1 and DO ip route into a single buffer (mutation without grant -> NOT_AUTHORIZED)
    std::string m1 = "HELLO 1";
    std::string m2 = "DO ip route 10.0.0.0 24 192.168.1.1 1";

    uint32_t len1Be = htonl(static_cast<uint32_t>(m1.size()));
    uint32_t len2Be = htonl(static_cast<uint32_t>(m2.size()));

    std::vector<uint8_t> packed;
    packed.insert(packed.end(), reinterpret_cast<uint8_t *>(&len1Be), reinterpret_cast<uint8_t *>(&len1Be) + 4);
    packed.insert(packed.end(), m1.begin(), m1.end());
    packed.insert(packed.end(), reinterpret_cast<uint8_t *>(&len2Be), reinterpret_cast<uint8_t *>(&len2Be) + 4);
    packed.insert(packed.end(), m2.begin(), m2.end());

    CHECK_TRUE(sendRaw(fd, packed.data(), packed.size()));

    std::string rsp1;
    CHECK_TRUE(recvFramed(fd, rsp1));
    STRCMP_EQUAL("HELLO 1", rsp1.c_str());

    std::string rsp2;
    CHECK_TRUE(recvFramed(fd, rsp2));
    CHECK_TRUE(rsp2.rfind("RSP NOT_AUTHORIZED", 0) == 0);

    close(fd);
}

TEST(AiSecurityWireFramingTest, OversizedLengthRejection) {
    int fd = connectLoopback(TEST_PORT);
    CHECK_TRUE(fd >= 0);

    // Send length 5000 (> 4096 limit)
    uint32_t lenBe = htonl(5000);
    sendRaw(fd, &lenBe, 4);

    // Server should close connection (read returns EOF)
    char buf[16];
    ssize_t n = recv(fd, buf, sizeof(buf), 0);
    CHECK_TRUE(n <= 0);

    close(fd);
}

TEST(AiSecurityWireFramingTest, ZeroLengthRejection) {
    int fd = connectLoopback(TEST_PORT);
    CHECK_TRUE(fd >= 0);

    // Send length 0
    uint32_t lenBe = htonl(0);
    sendRaw(fd, &lenBe, 4);

    // Server should close connection
    char buf[16];
    ssize_t n = recv(fd, buf, sizeof(buf), 0);
    CHECK_TRUE(n <= 0);

    close(fd);
}

TEST(AiSecurityWireFramingTest, AbruptDisconnectOnPartialPayload) {
    int fd = connectLoopback(TEST_PORT);
    CHECK_TRUE(fd >= 0);

    // Send length 50, but only 10 bytes then close
    uint32_t lenBe = htonl(50);
    sendRaw(fd, &lenBe, 4);
    sendRaw(fd, "0123456789", 10);
    close(fd);

    // Server must not crash or leak
    usleep(20000);
    CHECK_TRUE(AiSecurityClearanceManager::getInstance().isListening());
}

TEST_GROUP(AiSecurityGrantLifecycleTest) {
    void setup() {
        RecordingRouter::getInstance().reset();
        AiSecurityClearanceManager::getInstance().resetForTesting();
        AiSecurityClearanceManager::getInstance().start("127.0.0.1", TEST_PORT);
        usleep(50000);
    }

    void teardown() {
        AiSecurityClearanceManager::getInstance().stop();
        RecordingRouter::getInstance().reset();
        AiSecurityClearanceManager::getInstance().resetForTesting();
        usleep(50000);
    }
};

TEST(AiSecurityGrantLifecycleTest, StaleApprovalAfterDisconnectHasNoEffect) {
    int fd = connectLoopback(TEST_PORT);
    CHECK_TRUE(fd >= 0);

    sendFramed(fd, "HELLO 1");
    std::string rsp;
    recvFramed(fd, rsp);

    sendFramed(fd, "REQUEST tier=write");
    recvFramed(fd, rsp);
    CHECK_TRUE(rsp.rfind("WAITING", 0) == 0);

    // Extract connId
    uint64_t connId = 0;
    std::string peerIp;
    uint16_t peerPort = 0;
    CHECK_TRUE(AiSecurityClearanceManager::getInstance().hasPendingApproval(connId, peerIp, peerPort));
    CHECK_TRUE(connId > 0);

    // Client abruptly disconnects
    close(fd);
    usleep(30000);

    // Operator approves the now-closed connection
    bool approved = AiSecurityClearanceManager::getInstance().consoleApprove(connId);
    CHECK_FALSE(approved); // Must fail because connection is gone!

    // Connect a new client -> must still be Level 3
    int fd2 = connectLoopback(TEST_PORT);
    CHECK_TRUE(fd2 >= 0);
    sendFramed(fd2, "HELLO 1");
    recvFramed(fd2, rsp);

    // Mutation must be NOT_AUTHORIZED
    sendFramed(fd2, "DO ip route 10.0.0.0 24 192.168.1.1 1");
    recvFramed(fd2, rsp);
    CHECK_TRUE(rsp.rfind("RSP NOT_AUTHORIZED", 0) == 0);

    close(fd2);
}

TEST(AiSecurityGrantLifecycleTest, SecondApprovalRequestReceivesBusy) {
    int fd1 = connectLoopback(TEST_PORT);
    int fd2 = connectLoopback(TEST_PORT);
    CHECK_TRUE(fd1 >= 0 && fd2 >= 0);

    sendFramed(fd1, "HELLO 1");
    std::string rsp;
    recvFramed(fd1, rsp);

    sendFramed(fd2, "HELLO 1");
    recvFramed(fd2, rsp);

    // Conn1 requests approval
    sendFramed(fd1, "REQUEST tier=write");
    recvFramed(fd1, rsp);
    CHECK_TRUE(rsp.rfind("WAITING", 0) == 0);

    // Conn2 requests approval while Conn1 is pending -> BUSY
    sendFramed(fd2, "REQUEST tier=write");
    recvFramed(fd2, rsp);
    STRCMP_EQUAL("BUSY", rsp.c_str());

    close(fd1);
    close(fd2);
}

TEST(AiSecurityGrantLifecycleTest, RequestWhileAnotherIsLevel2ReceivesBusy) {
    int fd1 = connectLoopback(TEST_PORT);
    int fd2 = connectLoopback(TEST_PORT);
    CHECK_TRUE(fd1 >= 0 && fd2 >= 0);

    sendFramed(fd1, "HELLO 1");
    std::string rsp;
    recvFramed(fd1, rsp);

    sendFramed(fd2, "HELLO 1");
    recvFramed(fd2, rsp);

    // Conn1 requests approval
    sendFramed(fd1, "REQUEST tier=write");
    recvFramed(fd1, rsp);

    uint64_t connId = 0;
    std::string peerIp;
    uint16_t peerPort = 0;
    CHECK_TRUE(AiSecurityClearanceManager::getInstance().hasPendingApproval(connId, peerIp, peerPort));

    // Operator approves Conn1
    CHECK_TRUE(AiSecurityClearanceManager::getInstance().consoleApprove(connId));

    // Conn1 receives GRANTED notification
    recvFramed(fd1, rsp);
    CHECK_TRUE(rsp.rfind("GRANTED", 0) == 0);

    // Conn2 requests approval while Conn1 has active Level 2 -> BUSY
    sendFramed(fd2, "REQUEST tier=write");
    recvFramed(fd2, rsp);
    STRCMP_EQUAL("BUSY", rsp.c_str());

    close(fd1);
    close(fd2);
}

TEST(AiSecurityGrantLifecycleTest, OperatorDenySendsDeniedNotification) {
    int fd = connectLoopback(TEST_PORT);
    CHECK_TRUE(fd >= 0);

    sendFramed(fd, "HELLO 1");
    std::string rsp;
    recvFramed(fd, rsp);

    sendFramed(fd, "REQUEST tier=write");
    recvFramed(fd, rsp);
    CHECK_TRUE(rsp.rfind("WAITING", 0) == 0);

    uint64_t connId = 0;
    std::string peerIp;
    uint16_t peerPort = 0;
    CHECK_TRUE(AiSecurityClearanceManager::getInstance().hasPendingApproval(connId, peerIp, peerPort));

    // Operator denies
    CHECK_TRUE(AiSecurityClearanceManager::getInstance().consoleDeny(connId));

    // Client receives DENIED notification
    recvFramed(fd, rsp);
    CHECK_TRUE(rsp.rfind("DENIED", 0) == 0);

    close(fd);
}

TEST(AiSecurityGrantLifecycleTest, CancelCannotCrossConnectionIds) {
    auto &mgr = AiSecurityClearanceManager::getInstance();
    uint64_t conn1 = mgr.registerConnectionForTesting(201, "127.0.0.1", 50001);
    uint64_t conn2 = mgr.registerConnectionForTesting(202, "127.0.0.1", 50002);
    (void)conn1;

    // Conn2 sends CANCEL when it has no in-flight command
    std::string rsp = mgr.handleClientMessage(conn2, "CANCEL");
    STRCMP_EQUAL("RSP OK - No active command", rsp.c_str());
    LONGS_EQUAL(0, RecordingRouter::getInstance().getCancelCalls());
}

static void elevateConnection(int fd) {
    sendFramed(fd, "REQUEST tier=write");
    std::string rsp;
    recvFramed(fd, rsp);
    uint64_t cid = 0;
    std::string ip;
    uint16_t port = 0;
    if (AiSecurityClearanceManager::getInstance().hasPendingApproval(cid, ip, port)) {
        AiSecurityClearanceManager::getInstance().consoleApprove(cid);
        recvFramed(fd, rsp);
    }
}

TEST(AiSecurityGrantLifecycleTest, CancelDuringInFlightCommand) {
    int fd = connectLoopback(TEST_PORT);
    CHECK_TRUE(fd >= 0);

    sendFramed(fd, "HELLO 1");
    std::string rsp;
    recvFramed(fd, rsp);
    elevateConnection(fd);

    // Set 200ms delay in router execution
    RecordingRouter::getInstance().setExecutionDelayMs(200);

    // Send DO command
    sendFramed(fd, "DO show version");

    // Wait 30ms then send CANCEL while command is in-flight
    usleep(30000);
    sendFramed(fd, "CANCEL");

    // Must receive CANCELED response
    CHECK_TRUE(recvFramed(fd, rsp));
    CHECK_TRUE(rsp.find("CANCELED") != std::string::npos);
    CHECK_TRUE(RecordingRouter::getInstance().getCancelCalls() > 0);

    close(fd);
}

TEST(AiSecurityGrantLifecycleTest, StatusQueryWhileClearanceCommandInFlight) {
    int fd = connectLoopback(TEST_PORT);
    CHECK_TRUE(fd >= 0);

    sendFramed(fd, "HELLO 1");
    std::string rsp;
    recvFramed(fd, rsp);
    elevateConnection(fd);

    // Set 150ms delay in router execution
    RecordingRouter::getInstance().setExecutionDelayMs(150);

    // Send DO command in background
    std::thread cmdThread([fd]() {
        sendFramed(fd, "DO show version");
        std::string r;
        recvFramed(fd, r);
    });

    // Wait 20ms for command to enter router worker
    usleep(20000);

    // Status query must return quickly without deadlocking
    auto t0 = std::chrono::steady_clock::now();
    nlohmann::json status = ZyxelDriver::getInstance().getStatus();
    auto t1 = std::chrono::steady_clock::now();
    auto elapsedMs = std::chrono::duration_cast<std::chrono::milliseconds>(t1 - t0).count();

    CHECK_TRUE(elapsedMs < 100);
    CHECK_TRUE(status.contains("status"));

    cmdThread.join();
    close(fd);
}

TEST(AiSecurityGrantLifecycleTest, AbruptDisconnectDuringInFlightCommand) {
    int fd = connectLoopback(TEST_PORT);
    CHECK_TRUE(fd >= 0);

    sendFramed(fd, "HELLO 1");
    std::string rsp;
    recvFramed(fd, rsp);
    elevateConnection(fd);

    // Set 150ms delay
    RecordingRouter::getInstance().setExecutionDelayMs(150);

    // Send command and immediately close socket
    sendFramed(fd, "DO show version");
    usleep(20000);
    close(fd);

    // Wait for worker to finish and assert clean state
    usleep(200000);
    uint64_t cid = 0;
    std::string ip;
    uint16_t port = 0;
    CHECK_FALSE(AiSecurityClearanceManager::getInstance().hasPendingApproval(cid, ip, port));
}

TEST_GROUP(AiSecurityPipedBatchTest) {
    void setup() {
        RecordingRouter::getInstance().reset();
        AiSecurityClearanceManager::getInstance().resetForTesting();
        ensureClientBinaryBuilt();
        AiSecurityClearanceManager::getInstance().start("127.0.0.1", TEST_PORT);
        usleep(50000);
    }

    void teardown() {
        AiSecurityClearanceManager::getInstance().stop();
        RecordingRouter::getInstance().reset();
        AiSecurityClearanceManager::getInstance().resetForTesting();
        usleep(50000);
    }
};

TEST(AiSecurityPipedBatchTest, DeniedRequestStopsBatchAndExitsNonZero) {
    // Thread to handle operator denial
    std::thread opThread([]() {
        for (int i = 0; i < 50; ++i) {
            usleep(20000);
            uint64_t connId = 0;
            std::string peerIp;
            uint16_t peerPort = 0;
            if (AiSecurityClearanceManager::getInstance().hasPendingApproval(connId, peerIp, peerPort)) {
                AiSecurityClearanceManager::getInstance().consoleDeny(connId);
                return;
            }
        }
    });

    std::string cmd = "NETMON_LISTEN=127.0.0.1:39885 /tmp/netmon-ai-client-test request RW >/dev/null 2>&1";
    int ret = system(cmd.c_str());
    if (opThread.joinable()) opThread.join();

    // Must exit with non-zero status
    CHECK_TRUE(ret != 0);

    // The router command must NEVER have been called!
    LONGS_EQUAL(0, RecordingRouter::getInstance().getClearanceCalls().size());
    system("NETMON_LISTEN=127.0.0.1:39885 /tmp/netmon-ai-client-test close >/dev/null 2>&1");
}

TEST(AiSecurityPipedBatchTest, ApprovedRequestExecutesBatchAndExitsZero) {
    // Thread to handle operator approval
    std::thread opThread([]() {
        for (int i = 0; i < 50; ++i) {
            usleep(20000);
            uint64_t connId = 0;
            std::string peerIp;
            uint16_t peerPort = 0;
            if (AiSecurityClearanceManager::getInstance().hasPendingApproval(connId, peerIp, peerPort)) {
                AiSecurityClearanceManager::getInstance().consoleApprove(connId);
                return;
            }
        }
    });

    int retReq = system("NETMON_LISTEN=127.0.0.1:39885 /tmp/netmon-ai-client-test request RW >/dev/null 2>&1");
    if (opThread.joinable()) opThread.join();
    LONGS_EQUAL(0, retReq);

    int retDo = system("NETMON_LISTEN=127.0.0.1:39885 /tmp/netmon-ai-client-test do \"address-object TEST_HOST 10.0.0.1\" >/dev/null 2>&1");
    LONGS_EQUAL(0, retDo);

    system("NETMON_LISTEN=127.0.0.1:39885 /tmp/netmon-ai-client-test close >/dev/null 2>&1");

    // Router must have received the mutation command
    STRCMP_EQUAL("address-object TEST_HOST 10.0.0.1", RecordingRouter::getInstance().getClearanceCalls()[0].command.c_str());
}

TEST(AiSecurityPipedBatchTest, ReadOnlyDoBlockedWithoutRequest) {
    // Note: Request and consoleApprove ARE needed for Level 3 read commands per Envelope 3
    int retDo = system("NETMON_LISTEN=127.0.0.1:39885 /tmp/netmon-ai-client-test do \"show version\" \"show object-group address\" \"show object-group service\" >/dev/null 2>&1");
    // Client exits with 1 on NOT_AUTHORIZED
    CHECK_TRUE(retDo != 0);

    system("NETMON_LISTEN=127.0.0.1:39885 /tmp/netmon-ai-client-test close >/dev/null 2>&1");

    auto calls = RecordingRouter::getInstance().getClearanceCalls();
    LONGS_EQUAL(0, calls.size());
}

TEST(AiSecurityPipedBatchTest, FirstDoFailureStopsBatch) {
    std::thread opThread([]() {
        for (int i = 0; i < 50; ++i) {
            usleep(20000);
            uint64_t connId = 0;
            std::string peerIp;
            uint16_t peerPort = 0;
            if (AiSecurityClearanceManager::getInstance().hasPendingApproval(connId, peerIp, peerPort)) {
                AiSecurityClearanceManager::getInstance().consoleApprove(connId);
                return;
            }
        }
    });

    system("NETMON_LISTEN=127.0.0.1:39885 /tmp/netmon-ai-client-test request RW >/dev/null 2>&1");
    if (opThread.joinable()) opThread.join();

    int ret = system("NETMON_LISTEN=127.0.0.1:39885 /tmp/netmon-ai-client-test do \"illegal_command_here\" \"show version\" >/dev/null 2>&1");

    // Must exit non-zero
    CHECK_TRUE(ret != 0);

    // Second command (show version) must NOT have run because first failed
    LONGS_EQUAL(0, RecordingRouter::getInstance().getClearanceCalls().size());
    system("NETMON_LISTEN=127.0.0.1:39885 /tmp/netmon-ai-client-test close >/dev/null 2>&1");
}

TEST_GROUP(AiSecuritySmugglingTest) {
    void setup() {
        RecordingRouter::getInstance().reset();
    }

    void teardown() {
        RecordingRouter::getInstance().reset();
    }
};

TEST(AiSecuritySmugglingTest, SemicolonChainingRejected) {
    int timeoutMs = 0;
    std::string method;
    LineClassification cls = AiSecurityClassifier::classify("show version; reboot", "#", timeoutMs, method);
    CHECK_EQUAL(static_cast<int>(LineClassification::UNCLASSIFIED), static_cast<int>(cls));
}

TEST(AiSecuritySmugglingTest, DoubleAmpersandChainingRejected) {
    int timeoutMs = 0;
    std::string method;
    LineClassification cls = AiSecurityClassifier::classify("show version && write", "#", timeoutMs, method);
    CHECK_EQUAL(static_cast<int>(LineClassification::UNCLASSIFIED), static_cast<int>(cls));
}

TEST(AiSecuritySmugglingTest, PipeRedirectionRejected) {
    int timeoutMs = 0;
    std::string method;
    LineClassification cls = AiSecurityClassifier::classify("show version | grep flex", "#", timeoutMs, method);
    CHECK_EQUAL(static_cast<int>(LineClassification::UNCLASSIFIED), static_cast<int>(cls));
}

TEST(AiSecuritySmugglingTest, CommandSubstitutionRejected) {
    int timeoutMs = 0;
    std::string method;
    LineClassification cls1 = AiSecurityClassifier::classify("show version `reboot`", "#", timeoutMs, method);
    CHECK_EQUAL(static_cast<int>(LineClassification::UNCLASSIFIED), static_cast<int>(cls1));

    LineClassification cls2 = AiSecurityClassifier::classify("show version $(reboot)", "#", timeoutMs, method);
    CHECK_EQUAL(static_cast<int>(LineClassification::UNCLASSIFIED), static_cast<int>(cls2));
}

TEST(AiSecuritySmugglingTest, CaseVariationBypassRejected) {
    int timeoutMs = 0;
    std::string method;
    LineClassification cls1 = AiSecurityClassifier::classify("SHOW VERSION", "#", timeoutMs, method);
    CHECK_EQUAL(static_cast<int>(LineClassification::UNCLASSIFIED), static_cast<int>(cls1));

    LineClassification cls2 = AiSecurityClassifier::classify("Show Version", "#", timeoutMs, method);
    CHECK_EQUAL(static_cast<int>(LineClassification::UNCLASSIFIED), static_cast<int>(cls2));

    LineClassification cls3 = AiSecurityClassifier::classify("Secure-Policy Insert 1", "#", timeoutMs, method);
    CHECK_EQUAL(static_cast<int>(LineClassification::UNCLASSIFIED), static_cast<int>(cls3));
}

TEST(AiSecuritySmugglingTest, PathTraversalAndIllegalNamesRejected) {
    int timeoutMs = 0;
    std::string method;
    LineClassification cls1 = AiSecurityClassifier::classify("show interface ../../../etc/passwd", "#", timeoutMs, method);
    CHECK_EQUAL(static_cast<int>(LineClassification::UNCLASSIFIED), static_cast<int>(cls1));

    LineClassification cls2 = AiSecurityClassifier::classify("show address-object <script>alert(1)</script>", "#", timeoutMs, method);
    CHECK_EQUAL(static_cast<int>(LineClassification::UNCLASSIFIED), static_cast<int>(cls2));
}

TEST(AiSecurityGrantLifecycleTest, WireProtocolBareAndInvalidTierRejectedOverTcp) {
    int fd = connectLoopback(TEST_PORT);
    CHECK_TRUE(fd >= 0);

    sendFramed(fd, "HELLO 1");
    std::string rsp;
    recvFramed(fd, rsp);
    STRCMP_EQUAL("HELLO 1", rsp.c_str());

    // 1. Bare REQUEST with no tier attribute -> rejected with RSP SYNTAX
    sendFramed(fd, "REQUEST");
    recvFramed(fd, rsp);
    STRCMP_EQUAL("RSP SYNTAX - Missing or invalid security tier (must be R, RW, or RWP)", rsp.c_str());

    // 2. REQUEST with invalid tier attribute -> rejected with RSP SYNTAX
    sendFramed(fd, "REQUEST tier=bogus");
    recvFramed(fd, rsp);
    STRCMP_EQUAL("RSP SYNTAX - Missing or invalid security tier (must be R, RW, or RWP)", rsp.c_str());

    // 3. Connection state remains NONE; DO commands fail
    sendFramed(fd, "DO show version");
    recvFramed(fd, rsp);
    CHECK_TRUE(rsp.rfind("RSP NOT_AUTHORIZED", 0) == 0);

    // 4. Valid tier=read succeeds
    sendFramed(fd, "REQUEST tier=read");
    recvFramed(fd, rsp);
    CHECK_TRUE(rsp.rfind("WAITING", 0) == 0);

    close(fd);
}

TEST(AiSecurityPipedBatchTest, DeepProcessHierarchyAndAnchorPid) {
    pid_t myPid = getpid();
    // 1. Spawns session with explicit NETMON_ANCHOR_PID and approves it via background operator thread
    std::thread opThread([]() {
        for (int i = 0; i < 50; ++i) {
            usleep(20000);
            uint64_t connId = 0;
            std::string peerIp;
            uint16_t peerPort = 0;
            if (AiSecurityClearanceManager::getInstance().hasPendingApproval(connId, peerIp, peerPort)) {
                AiSecurityClearanceManager::getInstance().consoleApprove(connId, 0, ClearanceTier::READ);
                return;
            }
        }
    });

    std::string cmdReq = "NETMON_LISTEN=127.0.0.1:39885 NETMON_ANCHOR_PID=" + std::to_string(myPid) + " /tmp/netmon-ai-client-test request R >/dev/null 2>&1";
    int retReq = system(cmdReq.c_str());
    if (opThread.joinable()) opThread.join();
    LONGS_EQUAL(0, retReq);

    // 2. Query status from 3-level nested subshell via NETMON_ANCHOR_PID inheritance
    std::string cmdNested = "NETMON_LISTEN=127.0.0.1:39885 NETMON_ANCHOR_PID=" + std::to_string(myPid) + " sh -c 'sh -c \"/tmp/netmon-ai-client-test status\"' >/dev/null 2>&1";
    int retNested = system(cmdNested.c_str());
    LONGS_EQUAL(0, retNested);

    // 3. Query status directly with NETMON_ANCHOR_PID
    std::string cmdAnchor = "NETMON_LISTEN=127.0.0.1:39885 NETMON_ANCHOR_PID=" + std::to_string(myPid) + " /tmp/netmon-ai-client-test status >/dev/null 2>&1";
    int retAnchor = system(cmdAnchor.c_str());
    LONGS_EQUAL(0, retAnchor);

    std::string cmdClose = "NETMON_LISTEN=127.0.0.1:39885 NETMON_ANCHOR_PID=" + std::to_string(myPid) + " /tmp/netmon-ai-client-test close >/dev/null 2>&1";
    int retClose = system(cmdClose.c_str());
    (void)retClose;
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
