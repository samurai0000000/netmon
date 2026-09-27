/*
 * TestZyxelMain.cxx
 *
 * Copyright (C) 2026, Charles Chiou
 */

#include <iostream>
#include <filesystem>
#include <cstdlib>
#include <openssl/evp.h>
#include <openssl/rand.h>
#include <libssh2.h>

#include "Config.hxx"
#include "AuthManager.hxx"
#include "ZyxelSshClient.hxx"
#include "ZyxelDriver.hxx"

#include <CppUTest/CommandLineTestRunner.h>
#include <CppUTest/MemoryLeakWarningPlugin.h>

namespace fs = std::filesystem;

int main(int argc, char **argv) {
    // 1. Point HOME to temporary directory under test_data/ to guard against touching real home
    std::string testHome = "test_data/test_home_zyxel";
    if (fs::exists(testHome)) {
        fs::remove_all(testHome);
    }
    fs::create_directories(testHome);
    setenv("HOME", fs::absolute(testHome).string().c_str(), 1);

    // Enable thread-safe new/delete overloads for CppUTest memory leak detector
    MemoryLeakWarningPlugin::turnOnThreadSafeNewDeleteOverloads();

    // Warm up one-time static library tables and singletons before CppUTest leak tracking begins
    Config::getInstance();
    {
        libconfig::Config cfgWarmup;
        cfgWarmup.readString("interface = \"br0\";\n");
    }
    libssh2_init(0);
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
    AuthManager::getInstance();
    ZyxelDriver::getInstance();

    {
        std::string warmupPrompt;
        ZyxelSshClient::matchPrompt("Router#", warmupPrompt);
        ZyxelSshClient::stripAnsiEscapes("\033[32mwarmup\033[0m");
        ZyxelSshClient::stripCommandEcho("warmup\r\noutput", "warmup");
        ZyxelSshClient::sanitizeReason("warmup");
    }

    int result = CommandLineTestRunner::RunAllTests(argc, argv);

    libssh2_exit();

    if (fs::exists(testHome)) {
        fs::remove_all(testHome);
    }

    return result;
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
