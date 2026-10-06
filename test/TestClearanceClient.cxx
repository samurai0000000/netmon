/*
 * TestClearanceClient.cxx
 *
 * Copyright (C) 2026, Charles Chiou
 */

#include <CppUTest/TestHarness.h>
#include <string>
#include <vector>
#include <fstream>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <signal.h>
#include <poll.h>
#include <chrono>

TEST_GROUP(ClearanceClientTest) {
    void cleanup() {
        char sock[128];
        snprintf(sock, sizeof(sock), "/tmp/netmon-ai-%d.sock", (int)getpid());
        int fd = socket(AF_UNIX, SOCK_STREAM, 0);
        if (fd >= 0) {
            struct sockaddr_un addr;
            memset(&addr, 0, sizeof(addr));
            addr.sun_family = AF_UNIX;
            snprintf(addr.sun_path, sizeof(addr.sun_path), "%s", sock);
            if (connect(fd, (struct sockaddr *)&addr, sizeof(addr)) == 0) {
                uint32_t lenBe = htonl(5);
                ssize_t w = write(fd, &lenBe, 4);
                (void)w;
                w = write(fd, "CLOSE", 5);
                (void)w;
            }
            close(fd);
        }
        unlink(sock);
    }

    void setup() {
        cleanup();
    }

    void teardown() {
        cleanup();
    }
};

static void writeAll(int fd, const void *data, size_t len) {
    const char *p = static_cast<const char *>(data);
    size_t rem = len;
    while (rem > 0) {
        ssize_t n = write(fd, p, rem);
        if (n <= 0) break;
        p += n;
        rem -= n;
    }
}

static std::string runCommandCapture(const std::string &cmd, int &exitCode) {
    char buf[512];
    std::string result;
    FILE *pipe = popen((cmd + " 2>&1").c_str(), "r");
    if (!pipe) {
        exitCode = -1;
        return "";
    }
    while (fgets(buf, sizeof(buf), pipe) != NULL) {
        result += buf;
    }
    int status = pclose(pipe);
    if (WIFEXITED(status)) {
        exitCode = WEXITSTATUS(status);
    } else {
        exitCode = -1;
    }
    return result;
}

static std::string getClientBin() {
    if (access("./netmon-ai-client", X_OK) == 0) return "./netmon-ai-client";
    if (access("./build/netmon-ai-client", X_OK) == 0) return "./build/netmon-ai-client";
    if (access("../build/netmon-ai-client", X_OK) == 0) return "../build/netmon-ai-client";
    return "/tmp/netmon-ai-client-test";
}

TEST(ClearanceClientTest, SubcommandArgumentCountValidation) {
    int code = 0;
    std::string out;
    std::string bin = getClientBin();

    // Zero subcommands
    out = runCommandCapture(bin, code);
    LONGS_EQUAL(1, code);
    CHECK_TRUE(out.find("Usage:") != std::string::npos);

    // Unknown subcommand
    out = runCommandCapture(bin + " foo", code);
    LONGS_EQUAL(1, code);
    CHECK_TRUE(out.find("Unknown subcommand 'foo'") != std::string::npos);

    // Request with no args
    out = runCommandCapture(bin + " request", code);
    LONGS_EQUAL(1, code);
    CHECK_TRUE(out.find("requires a security level: R, RW, or RWP") != std::string::npos);

    // Request with unknown level
    out = runCommandCapture(bin + " request foo", code);
    LONGS_EQUAL(1, code);
    CHECK_TRUE(out.find("Unknown security level 'foo'") != std::string::npos);

    // Request with trailing args
    out = runCommandCapture(bin + " request R extra", code);
    LONGS_EQUAL(1, code);
    CHECK_TRUE(out.find("accepts no additional arguments after the security level") != std::string::npos);

    // Status with trailing args
    out = runCommandCapture(bin + " status extra", code);
    LONGS_EQUAL(1, code);
    CHECK_TRUE(out.find("accepts no additional arguments") != std::string::npos);

    // Cancel with trailing args
    out = runCommandCapture(bin + " cancel extra", code);
    LONGS_EQUAL(1, code);
    CHECK_TRUE(out.find("accepts no additional arguments") != std::string::npos);

    // Close with trailing args
    out = runCommandCapture(bin + " close extra", code);
    LONGS_EQUAL(1, code);
    CHECK_TRUE(out.find("accepts no additional arguments") != std::string::npos);

    // Do without any commands
    out = runCommandCapture(bin + " do", code);
    LONGS_EQUAL(1, code);
    CHECK_TRUE(out.find("requires at least one command argument") != std::string::npos);
}

TEST(ClearanceClientTest, MockServerSessionLifecycleAndReuse) {
    // Spin up a mock TCP clearance server on a loopback port
    int serverFd = socket(AF_INET, SOCK_STREAM, 0);
    CHECK_TRUE(serverFd >= 0);

    int opt = 1;
    setsockopt(serverFd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    struct sockaddr_in sin;
    memset(&sin, 0, sizeof(sin));
    sin.sin_family = AF_INET;
    sin.sin_port = htons(0); // ephemeral port
    sin.sin_addr.s_addr = htonl(INADDR_LOOPBACK);

    CHECK_EQUAL(0, bind(serverFd, (struct sockaddr *)&sin, sizeof(sin)));
    CHECK_EQUAL(0, listen(serverFd, 5));

    socklen_t sinLen = sizeof(sin);
    CHECK_EQUAL(0, getsockname(serverFd, (struct sockaddr *)&sin, &sinLen));
    int testPort = ntohs(sin.sin_port);

    // Run background child to handle connection
    pid_t helperPid = fork();
    if (helperPid == 0) {
        alarm(10);
        // Child server mock
        struct sockaddr_in peer;
        socklen_t peerLen = sizeof(peer);
        int clientSock = accept(serverFd, (struct sockaddr *)&peer, &peerLen);
        if (clientSock < 0) _exit(1);

        // Receive HELLO 1
        uint32_t lenBe = 0;
        if (read(clientSock, &lenBe, 4) != 4) _exit(2);
        uint32_t len = ntohl(lenBe);
        std::vector<char> buf(len + 1, 0);
        if (read(clientSock, buf.data(), len) != (ssize_t)len) _exit(3);

        // Send HELLO 1 response
        std::string helloRsp = "HELLO 1";
        uint32_t rspLenBe = htonl((uint32_t)helloRsp.size());
        writeAll(clientSock, &rspLenBe, 4);
        writeAll(clientSock, helloRsp.data(), helloRsp.size());

        // Now handle requests
        while (1) {
            uint32_t reqLenBe = 0;
            ssize_t n = read(clientSock, &reqLenBe, 4);
            if (n <= 0) break;
            uint32_t reqLen = ntohl(reqLenBe);
            std::vector<char> reqBuf(reqLen + 1, 0);
            if (read(clientSock, reqBuf.data(), reqLen) != (ssize_t)reqLen) break;
            std::string req(reqBuf.data());

            if (req.rfind("REQUEST", 0) == 0) {
                if (req.find("platform=") == std::string::npos) {
                    _exit(4);
                }
                std::string rsp = "GRANTED 300";
                uint32_t rLen = htonl((uint32_t)rsp.size());
                writeAll(clientSock, &rLen, 4);
                writeAll(clientSock, rsp.data(), rsp.size());
            } else if (req.rfind("DO ", 0) == 0) {
                std::string cmd = req.substr(3);
                std::string rsp;
                if (cmd == "bad command") {
                    rsp = "RSP UNCLASSIFIED - Invalid";
                } else {
                    rsp = "RSP OK Router# output for " + cmd;
                }
                uint32_t rLen = htonl((uint32_t)rsp.size());
                writeAll(clientSock, &rLen, 4);
                writeAll(clientSock, rsp.data(), rsp.size());
            } else if (req == "CLOSE") {
                break;
            }
        }
        close(clientSock);
        close(serverFd);
        _exit(0);
    }

    std::string envEndpoint = "127.0.0.1:" + std::to_string(testPort);
    setenv("NETMON_LISTEN", envEndpoint.c_str(), 1);
    std::string bin = "NETMON_LISTEN=" + envEndpoint + " " + getClientBin();

    // 1. Initial request: spawns daemon and receives GRANTED 300
    int code = 0;
    std::string out = runCommandCapture(bin + " request RW", code);
    LONGS_EQUAL(0, code);
    CHECK_TRUE(out.find("GRANTED 300") != std::string::npos);

    // 2. Second request immediately returns ALREADY_GRANTED without resending REQUEST
    out = runCommandCapture(bin + " request RW", code);
    LONGS_EQUAL(0, code);
    CHECK_TRUE(out.find("ALREADY_GRANTED") != std::string::npos);

    // 3. Status check
    out = runCommandCapture(bin + " status", code);
    LONGS_EQUAL(0, code);
    CHECK_TRUE(out.find("STATUS") != std::string::npos);
    CHECK_TRUE(out.find("granted=yes") != std::string::npos);

    // 4. Multi-argument DO: sends multiple quoted commands in sequence
    out = runCommandCapture(bin + " do \"show version\" \"show zone\"", code);
    LONGS_EQUAL(0, code);
    CHECK_TRUE(out.find("show version") != std::string::npos);
    CHECK_TRUE(out.find("show zone") != std::string::npos);

    // 5. Multi-argument DO stops sequence on non-OK command
    out = runCommandCapture(bin + " do \"show version\" \"bad command\" \"show zone\"", code);
    LONGS_EQUAL(1, code);
    CHECK_TRUE(out.find("UNCLASSIFIED") != std::string::npos);

    // 6. Close session
    out = runCommandCapture(bin + " close", code);
    LONGS_EQUAL(0, code);
    CHECK_TRUE(out.find("CLOSED") != std::string::npos);

    // Wait for helper server child
    int helperStatus = 0;
    waitpid(helperPid, &helperStatus, 0);
    CHECK_TRUE(WIFEXITED(helperStatus));
    LONGS_EQUAL(0, WEXITSTATUS(helperStatus));
    close(serverFd);
}

TEST(ClearanceClientTest, UngrantedDoFailsFast) {
    int serverFd = socket(AF_INET, SOCK_STREAM, 0);
    CHECK_TRUE(serverFd >= 0);
    int opt = 1;
    setsockopt(serverFd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    struct sockaddr_in sin;
    memset(&sin, 0, sizeof(sin));
    sin.sin_family = AF_INET;
    sin.sin_port = htons(0);
    sin.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
    CHECK_EQUAL(0, bind(serverFd, (struct sockaddr *)&sin, sizeof(sin)));
    CHECK_EQUAL(0, listen(serverFd, 5));

    socklen_t sinLen = sizeof(sin);
    CHECK_EQUAL(0, getsockname(serverFd, (struct sockaddr *)&sin, &sinLen));
    int testPort = ntohs(sin.sin_port);

    pid_t helperPid = fork();
    if (helperPid == 0) {
        alarm(10);
        struct sockaddr_in peer;
        socklen_t peerLen = sizeof(peer);
        int clientSock = accept(serverFd, (struct sockaddr *)&peer, &peerLen);
        if (clientSock < 0) _exit(1);

        uint32_t lenBe = 0;
        if (read(clientSock, &lenBe, 4) != 4) _exit(2);
        uint32_t len = ntohl(lenBe);
        std::vector<char> buf(len + 1, 0);
        if (read(clientSock, buf.data(), len) != (ssize_t)len) _exit(3);

        std::string helloRsp = "HELLO 1";
        uint32_t rspLenBe = htonl((uint32_t)helloRsp.size());
        writeAll(clientSock, &rspLenBe, 4);
        writeAll(clientSock, helloRsp.data(), helloRsp.size());

        while (1) {
            uint32_t reqLenBe = 0;
            if (read(clientSock, &reqLenBe, 4) != 4) break;
            uint32_t reqLen = ntohl(reqLenBe);
            std::vector<char> reqBuf(reqLen + 1, 0);
            if (read(clientSock, reqBuf.data(), reqLen) != (ssize_t)reqLen) break;
            std::string req(reqBuf.data());
            if (req == "CLOSE") break;
        }
        close(clientSock);
        close(serverFd);
        _exit(0);
    }

    std::string envEndpoint = "127.0.0.1:" + std::to_string(testPort);
    std::string bin = "NETMON_LISTEN=" + envEndpoint + " " + getClientBin();

    int code = 0;
    std::string out = runCommandCapture(bin + " do \"show version\"", code);
    LONGS_EQUAL(1, code);
    CHECK_TRUE(out.find("Session has not been granted clearance") != std::string::npos);

    runCommandCapture(bin + " close", code);
    int helperStatus = 0;
    waitpid(helperPid, &helperStatus, 0);
    close(serverFd);
}

TEST(ClearanceClientTest, BenchmarkFirewallRulePipelining) {
    int serverFd = socket(AF_INET, SOCK_STREAM, 0);
    CHECK_TRUE(serverFd >= 0);
    int opt = 1;
    setsockopt(serverFd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    struct sockaddr_in sin;
    memset(&sin, 0, sizeof(sin));
    sin.sin_family = AF_INET;
    sin.sin_port = htons(0);
    sin.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
    CHECK_EQUAL(0, bind(serverFd, (struct sockaddr *)&sin, sizeof(sin)));
    CHECK_EQUAL(0, listen(serverFd, 5));

    socklen_t sinLen = sizeof(sin);
    CHECK_EQUAL(0, getsockname(serverFd, (struct sockaddr *)&sin, &sinLen));
    int testPort = ntohs(sin.sin_port);

    pid_t helperPid = fork();
    if (helperPid == 0) {
        alarm(10);
        struct sockaddr_in peer;
        socklen_t peerLen = sizeof(peer);
        int clientSock = accept(serverFd, (struct sockaddr *)&peer, &peerLen);
        if (clientSock < 0) _exit(1);

        uint32_t lenBe = 0;
        if (read(clientSock, &lenBe, 4) != 4) _exit(2);
        uint32_t len = ntohl(lenBe);
        std::vector<char> buf(len + 1, 0);
        if (read(clientSock, buf.data(), len) != (ssize_t)len) _exit(3);

        std::string helloRsp = "HELLO 1";
        uint32_t rspLenBe = htonl((uint32_t)helloRsp.size());
        writeAll(clientSock, &rspLenBe, 4);
        writeAll(clientSock, helloRsp.data(), helloRsp.size());

        while (1) {
            uint32_t reqLenBe = 0;
            if (read(clientSock, &reqLenBe, 4) != 4) break;
            uint32_t reqLen = ntohl(reqLenBe);
            std::vector<char> reqBuf(reqLen + 1, 0);
            if (read(clientSock, reqBuf.data(), reqLen) != (ssize_t)reqLen) break;
            std::string req(reqBuf.data());

            if (req.rfind("REQUEST", 0) == 0) {
                std::string rsp = "GRANTED 300";
                uint32_t rLen = htonl((uint32_t)rsp.size());
                writeAll(clientSock, &rLen, 4);
                writeAll(clientSock, rsp.data(), rsp.size());
            } else if (req.rfind("DO ", 0) == 0) {
                std::string rsp = "RSP OK Router# rule data";
                uint32_t rLen = htonl((uint32_t)rsp.size());
                writeAll(clientSock, &rLen, 4);
                writeAll(clientSock, rsp.data(), rsp.size());
            } else if (req == "CLOSE") {
                break;
            }
        }
        close(clientSock);
        close(serverFd);
        _exit(0);
    }

    std::string envEndpoint = "127.0.0.1:" + std::to_string(testPort);
    std::string bin = "NETMON_LISTEN=" + envEndpoint + " " + getClientBin();

    int code = 0;
    std::string out = runCommandCapture(bin + " request R", code);
    LONGS_EQUAL(0, code);

    // Build command with 25 pipelined rules
    std::string doCmd = bin + " do";
    for (int i = 1; i <= 25; ++i) {
        doCmd += " \"show secure-policy " + std::to_string(i) + "\"";
    }

    auto tStart = std::chrono::steady_clock::now();
    out = runCommandCapture(doCmd, code);
    auto tEnd = std::chrono::steady_clock::now();
    auto elapsedMs = std::chrono::duration_cast<std::chrono::milliseconds>(tEnd - tStart).count();

    LONGS_EQUAL(0, code);
    // Hard ceiling assertion: 25 pipelined commands over loopback IPC must complete in under 2000ms
    CHECK_TRUE(elapsedMs < 2000);

    runCommandCapture(bin + " close", code);
    int helperStatus = 0;
    waitpid(helperPid, &helperStatus, 0);
    close(serverFd);
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
