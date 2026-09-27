/*
 * TestCredentialVault.cxx
 *
 * Copyright (C) 2026, Charles Chiou
 */

#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <filesystem>
#include <sys/stat.h>
#include <unistd.h>

#include "AuthManager.hxx"
#include "Config.hxx"

#include <CppUTest/TestHarness.h>

namespace fs = std::filesystem;

TEST_GROUP(CredentialVaultTest) {
    std::string testVaultDir = "test_data/test_credential_vault";

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

TEST(CredentialVaultTest, RejectsEmptyRouterPassword) {
    CHECK_FALSE(AuthManager::getInstance().setRouterPassword(""));
    std::string retrieved;
    CHECK_FALSE(AuthManager::getInstance().getRouterPassword(retrieved));
    CHECK_TRUE(retrieved.empty());
}

TEST(CredentialVaultTest, SetAndGetRouterPassword) {
    std::string secret = "ZyXel_USG_Flex_Secret_2026!";
    CHECK_TRUE(AuthManager::getInstance().setRouterPassword(secret));

    std::string retrieved;
    CHECK_TRUE(AuthManager::getInstance().getRouterPassword(retrieved));
    STRCMP_EQUAL(secret.c_str(), retrieved.c_str());
}

TEST(CredentialVaultTest, ClearRouterPassword) {
    std::string secret = "ZyXel_Temporary_Key_999";
    CHECK_TRUE(AuthManager::getInstance().setRouterPassword(secret));

    std::string retrieved;
    CHECK_TRUE(AuthManager::getInstance().getRouterPassword(retrieved));
    STRCMP_EQUAL(secret.c_str(), retrieved.c_str());

    CHECK_TRUE(AuthManager::getInstance().clearRouterPassword());
    retrieved.clear();
    CHECK_FALSE(AuthManager::getInstance().getRouterPassword(retrieved));
    CHECK_TRUE(retrieved.empty());
}

TEST(CredentialVaultTest, AdminPasswordChangePreservesRouterPassword) {
    std::string routerSecret = "Gateway_SSH_Pass_4432";
    CHECK_TRUE(AuthManager::getInstance().setRouterPassword(routerSecret));

    // Set initial admin login password
    CHECK_TRUE(AuthManager::getInstance().setPassword("AdminInitialPass1!", "", false));

    // Verify router password survived
    std::string retrieved;
    CHECK_TRUE(AuthManager::getInstance().getRouterPassword(retrieved));
    STRCMP_EQUAL(routerSecret.c_str(), retrieved.c_str());

    // Update admin login password
    CHECK_TRUE(AuthManager::getInstance().setPassword("AdminUpdatedPass2!", "AdminInitialPass1!", true));

    // Admin login works with new password
    std::string adminToken = AuthManager::getInstance().login("AdminUpdatedPass2!");
    CHECK_FALSE(adminToken.empty());
    CHECK_TRUE(AuthManager::getInstance().validateSession(adminToken));

    // Router password is still intact and retrievable
    retrieved.clear();
    CHECK_TRUE(AuthManager::getInstance().getRouterPassword(retrieved));
    STRCMP_EQUAL(routerSecret.c_str(), retrieved.c_str());
}

TEST(CredentialVaultTest, RouterPasswordChangePreservesAdminGenerationAndSessions) {
    // Set admin password and login to acquire an active session token
    CHECK_TRUE(AuthManager::getInstance().setPassword("WebAdminPass123", "", false));
    int initialGen = AuthManager::getInstance().getDiskGeneration();
    CHECK_TRUE(initialGen > 0);

    std::string token = AuthManager::getInstance().login("WebAdminPass123");
    CHECK_FALSE(token.empty());
    CHECK_TRUE(AuthManager::getInstance().validateSession(token));

    // Update router password
    CHECK_TRUE(AuthManager::getInstance().setRouterPassword("NewRouterSshPass_7788"));

    // Verify generation on disk did NOT change (must not bump generation)
    int afterGen = AuthManager::getInstance().getDiskGeneration();
    LONGS_EQUAL(initialGen, afterGen);

    // Verify existing admin session token remains 100% valid (no logout side-effect)
    CHECK_TRUE(AuthManager::getInstance().validateSession(token));

    // Verify router password updated
    std::string retrieved;
    CHECK_TRUE(AuthManager::getInstance().getRouterPassword(retrieved));
    STRCMP_EQUAL("NewRouterSshPass_7788", retrieved.c_str());
}

TEST(CredentialVaultTest, BoundarySpecialCharactersAndLargeSecret) {
    std::string specialChars = "P@$$w0rd!#%^&*()_+-=[]{}|;':\",./<>?`~\\";
    CHECK_TRUE(AuthManager::getInstance().setRouterPassword(specialChars));

    std::string retrieved;
    CHECK_TRUE(AuthManager::getInstance().getRouterPassword(retrieved));
    STRCMP_EQUAL(specialChars.c_str(), retrieved.c_str());

    // 1 KB secret string
    std::string largeSecret(1024, 'X');
    for (size_t i = 0; i < largeSecret.size(); ++i) {
        largeSecret[i] = static_cast<char>('A' + (i % 26));
    }
    CHECK_TRUE(AuthManager::getInstance().setRouterPassword(largeSecret));
    retrieved.clear();
    CHECK_TRUE(AuthManager::getInstance().getRouterPassword(retrieved));
    STRCMP_EQUAL(largeSecret.c_str(), retrieved.c_str());
}

TEST(CredentialVaultTest, TamperedVaultFailsDecryptionGracefully) {
    CHECK_TRUE(AuthManager::getInstance().setRouterPassword("SecretBeforeTamper"));

    std::string encPath = testVaultDir + "/vault.enc";
    std::ifstream is(encPath, std::ios::binary);
    CHECK_TRUE(is.is_open());
    std::vector<char> buffer((std::istreambuf_iterator<char>(is)),
                             std::istreambuf_iterator<char>());
    is.close();

    // Corrupt ciphertext byte
    CHECK_TRUE(buffer.size() > 30);
    buffer[29] ^= 0xFF;

    std::ofstream os(encPath, std::ios::binary | std::ios::trunc);
    os.write(buffer.data(), buffer.size());
    os.close();

    std::string retrieved;
    CHECK_FALSE(AuthManager::getInstance().getRouterPassword(retrieved));
    CHECK_TRUE(retrieved.empty());
}

TEST(CredentialVaultTest, FilePermissionsEnforce0600) {
    CHECK_TRUE(AuthManager::getInstance().setRouterPassword("SecretForPerms"));

    std::string keyPath = testVaultDir + "/vault.key";
    std::string encPath = testVaultDir + "/vault.enc";

    struct stat stKey, stEnc;
    LONGS_EQUAL(0, stat(keyPath.c_str(), &stKey));
    LONGS_EQUAL(0, stat(encPath.c_str(), &stEnc));

    LONGS_EQUAL(0600, stKey.st_mode & 0777);
    LONGS_EQUAL(0600, stEnc.st_mode & 0777);
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
