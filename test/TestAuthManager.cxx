/*
 * TestAuthManager.cxx
 *
 * Copyright (C) 2026, Charles Chiou
 */

#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <filesystem>
#include <chrono>
#include <thread>
#include <sys/wait.h>
#include <unistd.h>

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-parameter"
#pragma GCC diagnostic ignored "-Wshadow"
#include <httplib.h>
#pragma GCC diagnostic pop

#include <nlohmann/json.hpp>

#include "AuthManager.hxx"
#include "WebServer.hxx"
#include "Config.hxx"

#include <CppUTest/TestHarness.h>

namespace fs = std::filesystem;
using json = nlohmann::json;

TEST_GROUP(AuthManager) {
    std::string testVaultDir = "test_data/auth_unit_test";

    void setup() {
        AuthManager::getInstance().resetForTesting();
        if (fs::exists(testVaultDir)) {
            fs::remove_all(testVaultDir);
        }
        fs::create_directories(testVaultDir);
        AuthManager::getInstance().setVaultDir(testVaultDir);
    }

    void teardown() {
        AuthManager::getInstance().resetForTesting();
        if (fs::exists(testVaultDir)) {
            fs::remove_all(testVaultDir);
        }
    }
};

TEST(AuthManager, RejectsEmptyPassword) {
    CHECK_FALSE(AuthManager::getInstance().setPassword("", "", false));
    std::string token = AuthManager::getInstance().login("");
    CHECK_TRUE(token.empty());
}

TEST(AuthManager, AcceptsCorrectPassword) {
    CHECK_TRUE(AuthManager::getInstance().setPassword("StrongPass123!", "", false));
    std::string token = AuthManager::getInstance().login("StrongPass123!");
    CHECK_FALSE(token.empty());
    CHECK_TRUE(AuthManager::getInstance().validateSession(token));
}

TEST(AuthManager, RejectsWrongPassword) {
    CHECK_TRUE(AuthManager::getInstance().setPassword("CorrectSecret", "", false));
    std::string token = AuthManager::getInstance().login("WrongSecret");
    CHECK_TRUE(token.empty());
}

TEST(AuthManager, SessionTokenIs32Bytes) {
    CHECK_TRUE(AuthManager::getInstance().setPassword("PassForTokenCheck", "", false));
    std::string token = AuthManager::getInstance().login("PassForTokenCheck");
    CHECK_FALSE(token.empty());
    LONGS_EQUAL(32, token.size());
}

TEST(AuthManager, IdleSessionExpires) {
    CHECK_TRUE(AuthManager::getInstance().setPassword("PassForExpiry", "", false));
    std::string token = AuthManager::getInstance().login("PassForExpiry");
    CHECK_FALSE(token.empty());
    CHECK_TRUE(AuthManager::getInstance().validateSession(token));

    // Age the session past the 15-minute (900 second) threshold
    time_t expiredTime = time(nullptr) - 905;
    AuthManager::getInstance().setSessionTimestampForTesting(token, expiredTime);

    CHECK_FALSE(AuthManager::getInstance().validateSession(token));
}

TEST(AuthManager, LoginReadsVaultFromDisk) {
    CHECK_TRUE(AuthManager::getInstance().setPassword("PassV1", "", false));
    std::string token1 = AuthManager::getInstance().login("PassV1");
    CHECK_FALSE(token1.empty());

    // Modify the vault on disk with new password
    CHECK_TRUE(AuthManager::getInstance().setPassword("PassV2", "", false));

    // Old password must fail and new password must succeed immediately
    std::string tokenOld = AuthManager::getInstance().login("PassV1");
    CHECK_TRUE(tokenOld.empty());

    std::string tokenNew = AuthManager::getInstance().login("PassV2");
    CHECK_FALSE(tokenNew.empty());
}

TEST(AuthManager, MissingVaultFailsClosed) {
    std::string nonExistent = "test_data/non_existent_vault_dir";
    if (fs::exists(nonExistent)) {
        fs::remove_all(nonExistent);
    }
    AuthManager::getInstance().setVaultDir(nonExistent);

    std::string token = AuthManager::getInstance().login("AnyPassword");
    CHECK_TRUE(token.empty());
    CHECK_FALSE(AuthManager::getInstance().validateSession("fake_session_token_32_bytes_len"));

    AuthManager::getInstance().setVaultDir(testVaultDir);
}

TEST(AuthManager, SecondProcessGenerationRevokesLiveSession) {
    CHECK_TRUE(AuthManager::getInstance().setPassword("ParentPass1", "", false));
    std::string token = AuthManager::getInstance().login("ParentPass1");
    CHECK_FALSE(token.empty());
    CHECK_TRUE(AuthManager::getInstance().validateSession(token));

    // Fork a child process to change the password and bump generation on disk
    pid_t pid = fork();
    if (pid == 0) {
        bool ok = AuthManager::getInstance().setPassword("ChildPass2", "", false);
        _exit(ok ? 0 : 1);
    }

    int status = 0;
    waitpid(pid, &status, 0);
    CHECK_TRUE(WIFEXITED(status));
    CHECK_EQUAL(0, WEXITSTATUS(status));

    // The daemon/parent process, without restarting, validates the session.
    // The generation read from disk under the lock mismatch must invalidate the live session.
    CHECK_FALSE(AuthManager::getInstance().validateSession(token));
}

TEST_GROUP(Integration_LoopbackAuth) {
    std::string testVaultDir = "test_data/loopback_auth_test";

    void setup() {
        WebServer::getInstance().stop();
        WebServer::getInstance().join();
        AuthManager::getInstance().resetForTesting();

        if (fs::exists(testVaultDir)) {
            fs::remove_all(testVaultDir);
        }
        fs::create_directories(testVaultDir);
        AuthManager::getInstance().setVaultDir(testVaultDir);
        AuthManager::getInstance().setPassword("AdminSecret2026!", "", false);
    }

    void teardown() {
        WebServer::getInstance().stop();
        WebServer::getInstance().join();

        AuthManager::getInstance().resetForTesting();
        if (fs::exists(testVaultDir)) {
            fs::remove_all(testVaultDir);
        }
    }
};

TEST(Integration_LoopbackAuth, LoginSucceedsOn3886) {
    // Start WebServer on standard ports
    CHECK_TRUE(WebServer::getInstance().start("127.0.0.1", 3884, 3886));
    std::this_thread::sleep_for(std::chrono::milliseconds(50));

    httplib::Client cli("127.0.0.1", 3886);
    cli.set_connection_timeout(1, 0);
    cli.set_read_timeout(2, 0);

    json body = {{"password", "AdminSecret2026!"}};
    auto res = cli.Post("/api/auth/login", body.dump(), "application/json");

    CHECK_TRUE(res != nullptr);
    CHECK_EQUAL(200, res->status);

    json resp = json::parse(res->body);
    CHECK_EQUAL("ok", resp.value("status", ""));
    std::string token = resp.value("token", "");
    LONGS_EQUAL(32, token.size());

    // Verify session works against admin endpoint
    httplib::Headers headers = {
        {"Authorization", "Bearer " + token}
    };
    auto statusRes = cli.Get("/api/admin/status", headers);
    CHECK_TRUE(statusRes != nullptr);
    CHECK_EQUAL(200, statusRes->status);

    cli.stop();
    WebServer::getInstance().stop();
    WebServer::getInstance().join();
}

TEST(Integration_LoopbackAuth, DashboardListenerHasNoAdminRoute) {
    CHECK_TRUE(WebServer::getInstance().start("127.0.0.1", 3884, 3886));
    std::this_thread::sleep_for(std::chrono::milliseconds(50));

    // Dashboard listener is on port 3884
    httplib::Client dashCli("127.0.0.1", 3884);
    dashCli.set_connection_timeout(1, 0);
    dashCli.set_read_timeout(2, 0);

    json body = {{"password", "AdminSecret2026!"}};
    auto res = dashCli.Post("/api/auth/login", body.dump(), "application/json");

    // The route MUST NOT exist on the dashboard listener (either 404 or 403, never 200)
    CHECK_TRUE(res != nullptr);
    CHECK_TRUE(res->status == 404 || res->status == 403);
    CHECK_TRUE(res->status != 200);

    dashCli.stop();
    WebServer::getInstance().stop();
    WebServer::getInstance().join();
}

TEST(Integration_LoopbackAuth, ReservedAdminPortRefusesStartup) {
    WebServer::getInstance().stop();
    WebServer::getInstance().join();

    // Reserved ports: 3883 (aimon), 3884 (dashboard), 3885 (gateway), 16880 (meshmon)
    CHECK_FALSE(WebServer::getInstance().start("127.0.0.1", 3999, 3883));
    CHECK_FALSE(WebServer::getInstance().start("127.0.0.1", 3999, 3884));
    CHECK_FALSE(WebServer::getInstance().start("127.0.0.1", 3999, 3885));
    CHECK_FALSE(WebServer::getInstance().start("127.0.0.1", 3999, 16880));
    CHECK_FALSE(WebServer::getInstance().start("127.0.0.1", 3884, 3884));
    CHECK_FALSE(WebServer::getInstance().isRunning());
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
