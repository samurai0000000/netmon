/*
 * TestMain.cxx
 *
 * Copyright (C) 2026, Charles Chiou
 */

#include <iostream>
#include <thread>
#include <chrono>

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-parameter"
#pragma GCC diagnostic ignored "-Wshadow"
#include <httplib.h>
#pragma GCC diagnostic pop

#include <openssl/evp.h>
#include <openssl/rand.h>

#include <sqlite3.h>
#include "Config.hxx"
#include "SecurityCheckpoint.hxx"
#include "SnmpDatabase.hxx"
#include "AimonGatewayClient.hxx"
#include "NetMonShell.hxx"
#include "SyslogServer.hxx"
#include "AuthManager.hxx"

#include <CppUTest/CommandLineTestRunner.h>
#include <CppUTest/MemoryLeakWarningPlugin.h>

int main(int argc, char **argv) {
    // Enable thread-safe new/delete overloads for CppUTest memory leak detector
    MemoryLeakWarningPlugin::turnOnThreadSafeNewDeleteOverloads();

    // Warm up one-time static library tables and singletons before CppUTest leak tracking begins
    Config::getInstance();
    OPENSSL_init_crypto(OPENSSL_INIT_LOAD_CRYPTO_STRINGS, nullptr);
    unsigned char randWarmup[32];
    RAND_bytes(randWarmup, sizeof(randWarmup));
    {
        unsigned char key[32] = {0};
        unsigned char salt[16] = {0};
        unsigned char hash[32] = {0};
        PKCS5_PBKDF2_HMAC("warmup", 6, salt, sizeof(salt), 1, EVP_sha256(), sizeof(hash), hash);
        EVP_CIPHER_CTX *ctx = EVP_CIPHER_CTX_new();
        if (ctx) {
            EVP_EncryptInit_ex(ctx, EVP_aes_256_gcm(), nullptr, key, salt);
            EVP_CIPHER_CTX_free(ctx);
        }
    }
    sqlite3_initialize();
    {
        sqlite3 *warmupDb = nullptr;
        sqlite3_open(":memory:", &warmupDb);
        if (warmupDb) {
            sqlite3_close(warmupDb);
        }
    }
    SecurityCheckpoint::getInstance();
    SnmpDatabase::getInstance();
    AimonGatewayClient::getInstance();
    NetMonShell::getInstance();
    SyslogServer::getInstance();
    AuthManager::getInstance();

    {
        httplib::Server warmupServer;
        warmupServer.Get("/warmup", [](const httplib::Request &, httplib::Response &res) {
            res.status = 200;
            res.set_content("ok", "text/plain");
        });
        warmupServer.Post("/warmup", [](const httplib::Request &, httplib::Response &res) {
            res.status = 200;
            res.set_content("ok", "application/json");
        });
        std::thread wt([&]() {
            warmupServer.listen("127.0.0.1", 3899);
        });
        std::this_thread::sleep_for(std::chrono::milliseconds(30));

        httplib::Client warmupClient("127.0.0.1", 3899);
        warmupClient.set_connection_timeout(1, 0);
        warmupClient.Get("/warmup");
        httplib::Headers warmupHeaders = {{"Authorization", "Bearer warmup"}};
        warmupClient.Get("/warmup", warmupHeaders);
        warmupClient.Post("/warmup", "{}", "application/json");
        warmupClient.stop();

        warmupServer.stop();
        if (wt.joinable()) {
            wt.join();
        }
    }

    return CommandLineTestRunner::RunAllTests(argc, argv);
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
