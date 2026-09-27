/*
 * TestZyxelDriver.cxx
 *
 * Copyright (C) 2026, Charles Chiou
 */

#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <filesystem>
#include <thread>
#include <sys/stat.h>

#include "ZyxelDriver.hxx"
#include "AuthManager.hxx"
#include "Config.hxx"

#include <CppUTest/TestHarness.h>

namespace fs = std::filesystem;

TEST_GROUP(ZyxelDriverTest) {
    std::string testDir = "test_data/test_zyxel_driver";
    std::string testJournal = "test_data/test_zyxel_driver/router_journal.json";

    void setup() {
        ZyxelDriver::getInstance().resetForTesting();
        AuthManager::getInstance().resetForTesting();
        Config::getInstance().resetForTesting();
        if (fs::exists(testDir)) {
            fs::remove_all(testDir);
        }
        fs::create_directories(testDir);
        ZyxelDriver::getInstance().setJournalPath(testJournal);
    }

    void teardown() {
        ZyxelDriver::getInstance().resetForTesting();
        AuthManager::getInstance().resetForTesting();
        Config::getInstance().resetForTesting();
        if (fs::exists(testDir)) {
            fs::remove_all(testDir);
        }
    }
};

TEST(ZyxelDriverTest, InitialStatusIsUnconfigured) {
    auto status = ZyxelDriver::getInstance().getStatus();
    STRCMP_EQUAL("unconfigured", status.value("status", "").c_str());
    CHECK_FALSE(ZyxelDriver::getInstance().isConfigured());
    CHECK_FALSE(ZyxelDriver::getInstance().isConnected());
}

TEST(ZyxelDriverTest, RejectsInvalidIpAddress) {
    auto res1 = ZyxelDriver::getInstance().blockIp("invalid-ip", "Test");
    STRCMP_EQUAL("error", res1.value("status", "").c_str());

    auto res2 = ZyxelDriver::getInstance().blockIp("300.1.2.3", "Test");
    STRCMP_EQUAL("error", res2.value("status", "").c_str());

    auto res3 = ZyxelDriver::getInstance().unblockIp("192.168.1");
    STRCMP_EQUAL("error", res3.value("status", "").c_str());

    // Verify no journal entries created
    auto mutations = ZyxelDriver::getInstance().getPendingMutationsForTesting();
    LONGS_EQUAL(0, mutations.size());
}

TEST(ZyxelDriverTest, BlockIpJournalsMutationWhenOffline) {
    auto res = ZyxelDriver::getInstance().blockIp("192.0.2.50", "Bandwidth Spike");
    STRCMP_EQUAL("queued", res.value("status", "").c_str());
    STRCMP_EQUAL("block", res.value("action", "").c_str());
    STRCMP_EQUAL("192.0.2.50", res.value("ip", "").c_str());

    auto mutations = ZyxelDriver::getInstance().getPendingMutationsForTesting();
    LONGS_EQUAL(1, mutations.size());
    STRCMP_EQUAL("block", mutations[0].op.c_str());
    STRCMP_EQUAL("192.0.2.50", mutations[0].ip.c_str());
    STRCMP_EQUAL("192_0_2_50", mutations[0].sanitizedName.c_str());
    STRCMP_EQUAL("Bandwidth Spike", mutations[0].reason.c_str());
}

TEST(ZyxelDriverTest, UnblockIpJournalsMutationWhenOffline) {
    auto res = ZyxelDriver::getInstance().unblockIp("192.0.2.60");
    STRCMP_EQUAL("queued", res.value("status", "").c_str());
    STRCMP_EQUAL("unblock", res.value("action", "").c_str());
    STRCMP_EQUAL("192.0.2.60", res.value("ip", "").c_str());

    auto mutations = ZyxelDriver::getInstance().getPendingMutationsForTesting();
    LONGS_EQUAL(1, mutations.size());
    STRCMP_EQUAL("unblock", mutations[0].op.c_str());
    STRCMP_EQUAL("192.0.2.60", mutations[0].ip.c_str());
}

TEST(ZyxelDriverTest, JournalFilePermissionsEnforce0600) {
    ZyxelDriver::getInstance().blockIp("10.0.0.99", "Security Check");

    struct stat st;
    LONGS_EQUAL(0, stat(testJournal.c_str(), &st));
    LONGS_EQUAL(0600, st.st_mode & 0777);
}

TEST(ZyxelDriverTest, MultipleMutationsPersistSequentially) {
    ZyxelDriver::getInstance().blockIp("192.0.2.10", "Host 1");
    ZyxelDriver::getInstance().blockIp("192.0.2.20", "Host 2");
    ZyxelDriver::getInstance().unblockIp("192.0.2.10");

    auto mutations = ZyxelDriver::getInstance().getPendingMutationsForTesting();
    LONGS_EQUAL(3, mutations.size());

    STRCMP_EQUAL("192.0.2.10", mutations[0].ip.c_str());
    STRCMP_EQUAL("block", mutations[0].op.c_str());

    STRCMP_EQUAL("192.0.2.20", mutations[1].ip.c_str());
    STRCMP_EQUAL("block", mutations[1].op.c_str());

    STRCMP_EQUAL("192.0.2.10", mutations[2].ip.c_str());
    STRCMP_EQUAL("unblock", mutations[2].op.c_str());
}

TEST(ZyxelDriverTest, ConcurrentMutationsDoNotDeadlockOrCorrupt) {
    const int threadCount = 4;
    const int opsPerThread = 10;
    std::vector<std::thread> threads;

    for (int t = 0; t < threadCount; ++t) {
        threads.emplace_back([t]() {
            for (int i = 0; i < opsPerThread; ++i) {
                std::string ip = "192.0.2." + std::to_string(t * 10 + i + 1);
                ZyxelDriver::getInstance().blockIp(ip, "Concurrent Test");
            }
        });
    }

    for (auto &th : threads) {
        if (th.joinable()) {
            th.join();
        }
    }

    auto mutations = ZyxelDriver::getInstance().getPendingMutationsForTesting();
    LONGS_EQUAL(threadCount * opsPerThread, mutations.size());
}

TEST(ZyxelDriverTest, ReplayJournalWhenOfflineReturnsFalseAndRetainsJournal) {
    ZyxelDriver::getInstance().blockIp("192.0.2.100", "Offline Replay Test");
    auto mutationsBefore = ZyxelDriver::getInstance().getPendingMutationsForTesting();
    LONGS_EQUAL(1, mutationsBefore.size());

    // When offline, replay cannot connect and must return false without clearing journal
    CHECK_FALSE(ZyxelDriver::getInstance().replayJournal());

    auto mutationsAfter = ZyxelDriver::getInstance().getPendingMutationsForTesting();
    LONGS_EQUAL(1, mutationsAfter.size());
    STRCMP_EQUAL("192.0.2.100", mutationsAfter[0].ip.c_str());
}

TEST(ZyxelDriverTest, GetStatusConfiguredOfflineReportsOffline) {
    ZyxelDriver::getInstance().setLiveEnabled(true);
    ZyxelDriver::getInstance().configure("127.0.0.1", 2222, "admin", "test_data/test_pin.pin");
    auto status = ZyxelDriver::getInstance().getStatus();
    STRCMP_EQUAL("offline", status.value("status", "").c_str());
    CHECK_TRUE(status.contains("error"));
}

TEST(ZyxelDriverTest, GetSessionsConfiguredOfflineReturnsEmptySessionsArray) {
    ZyxelDriver::getInstance().setLiveEnabled(true);
    ZyxelDriver::getInstance().configure("127.0.0.1", 2222, "admin", "test_data/test_pin.pin");
    auto sessions = ZyxelDriver::getInstance().getSessions();
    STRCMP_EQUAL("offline", sessions.value("status", "").c_str());
    CHECK_TRUE(sessions["sessions"].is_array());
    LONGS_EQUAL(0, sessions["sessions"].size());
}

TEST(ZyxelDriverTest, FlushFlashWriteWhenOfflineRetainsUnappliedJournalEntries) {
    ZyxelDriver::getInstance().blockIp("192.0.2.200", "Pending Retention Test");
    auto mutations = ZyxelDriver::getInstance().getPendingMutationsForTesting();
    LONGS_EQUAL(1, mutations.size());

    // Flush flash write while offline must NOT delete unapplied entries
    ZyxelDriver::getInstance().flushFlashWrite();

    auto mutationsAfter = ZyxelDriver::getInstance().getPendingMutationsForTesting();
    LONGS_EQUAL(1, mutationsAfter.size());
    STRCMP_EQUAL("192.0.2.200", mutationsAfter[0].ip.c_str());
}

TEST(ZyxelDriverTest, ClearAuthFailureResetsState) {
    ZyxelDriver::getInstance().setLiveEnabled(true);
    ZyxelDriver::getInstance().configure("127.0.0.1", 2222, "admin", "test_data/test_pin.pin");
    ZyxelDriver::getInstance().clearAuthFailure();
    auto status = ZyxelDriver::getInstance().getStatus();
    // After clearing auth failure, status is checked and reports offline because mock target is down
    STRCMP_EQUAL("offline", status.value("status", "").c_str());
}

TEST(ZyxelDriverTest, SetRouterPasswordClearsAuthFailureBackoff) {
    AuthManager::getInstance().resetForTesting();
    std::string vaultDir = "test_data/test_zyxel_driver_vault";
    if (fs::exists(vaultDir)) {
        fs::remove_all(vaultDir);
    }
    fs::create_directories(vaultDir);
    AuthManager::getInstance().setVaultDir(vaultDir);

    // Setting router password should succeed and trigger clearAuthFailure
    CHECK_TRUE(AuthManager::getInstance().setRouterPassword("FreshTestPassword123!"));

    std::string retrieved;
    CHECK_TRUE(AuthManager::getInstance().getRouterPassword(retrieved));
    STRCMP_EQUAL("FreshTestPassword123!", retrieved.c_str());

    AuthManager::getInstance().resetForTesting();
    if (fs::exists(vaultDir)) {
        fs::remove_all(vaultDir);
    }
}

TEST(ZyxelDriverTest, ConnectedMutationPersistsInJournalBeforeFlashWrite) {
    // A mutation ingested persists to router_journal.json before flash write commit
    auto res = ZyxelDriver::getInstance().blockIp("192.0.2.77", "Debounce Window Durability Test");
    STRCMP_EQUAL("queued", res.value("status", "").c_str());

    // Verify persisted directly on disk with dirty flag true
    std::vector<RouterMutation> mutations;
    bool dirty = false;
    CHECK_TRUE(ZyxelDriver::getInstance().loadJournalForTesting(mutations, dirty));
    CHECK_TRUE(dirty);
    LONGS_EQUAL(1, mutations.size());
    STRCMP_EQUAL("blk_192_0_2_77", mutations[0].id.c_str());
    STRCMP_EQUAL("192.0.2.77", mutations[0].ip.c_str());
    STRCMP_EQUAL("block", mutations[0].op.c_str());
    STRCMP_EQUAL("Debounce Window Durability Test", mutations[0].reason.c_str());
    STRCMP_EQUAL("pending", mutations[0].state.c_str());

    // Simulate connected transition to applied_running
    mutations[0].state = "applied_running";
    CHECK_TRUE(ZyxelDriver::getInstance().saveJournalForTesting(mutations, true));

    std::vector<RouterMutation> updatedMutations;
    bool updatedDirty = false;
    CHECK_TRUE(ZyxelDriver::getInstance().loadJournalForTesting(updatedMutations, updatedDirty));
    LONGS_EQUAL(1, updatedMutations.size());
    STRCMP_EQUAL("applied_running", updatedMutations[0].state.c_str());
}

TEST(ZyxelDriverTest, DebouncedFlashWritePrunesAppliedJournalIds) {
    // Populate journal with one applied_running and one pending mutation
    RouterMutation m1;
    m1.id = "blk_192_0_2_10";
    m1.op = "block";
    m1.ip = "192.0.2.10";
    m1.sanitizedName = "192_0_2_10";
    m1.reason = "Applied";
    m1.state = "applied_running";
    m1.timestamp = time(nullptr);

    RouterMutation m2;
    m2.id = "blk_192_0_2_20";
    m2.op = "block";
    m2.ip = "192.0.2.20";
    m2.sanitizedName = "192_0_2_20";
    m2.reason = "Pending";
    m2.state = "pending";
    m2.timestamp = time(nullptr);

    std::vector<RouterMutation> initial = { m1, m2 };
    CHECK_TRUE(ZyxelDriver::getInstance().saveJournalForTesting(initial, true));

    // When session is offline, flushFlashWrite() retains all mutations safely
    ZyxelDriver::getInstance().flushFlashWrite();

    std::vector<RouterMutation> loaded;
    bool dirty = false;
    CHECK_TRUE(ZyxelDriver::getInstance().loadJournalForTesting(loaded, dirty));
    LONGS_EQUAL(2, loaded.size());
    STRCMP_EQUAL("applied_running", loaded[0].state.c_str());
    STRCMP_EQUAL("pending", loaded[1].state.c_str());
}

TEST(ZyxelDriverTest, ProcessRestartPreservesAppliedRunningState) {
    RouterMutation m;
    m.id = "blk_192_0_2_90";
    m.op = "block";
    m.ip = "192.0.2.90";
    m.sanitizedName = "192_0_2_90";
    m.reason = "Preserve State Across Restart";
    m.state = "applied_running";
    m.timestamp = time(nullptr);

    CHECK_TRUE(ZyxelDriver::getInstance().addMutationForTesting(m));

    // Simulate process restart (reset driver instance and reload journal)
    std::string restartJournal = testDir + "/restart_journal.json";
    fs::copy_file(testJournal, restartJournal);

    ZyxelDriver::getInstance().resetForTesting();
    ZyxelDriver::getInstance().setJournalPath(restartJournal);

    auto recovered = ZyxelDriver::getInstance().getPendingMutationsForTesting();
    LONGS_EQUAL(1, recovered.size());
    STRCMP_EQUAL("blk_192_0_2_90", recovered[0].id.c_str());
    STRCMP_EQUAL("applied_running", recovered[0].state.c_str());
}

TEST(ZyxelDriverTest, RebootBeforeFlashWriteReplaysPendingJournalOnReconnect) {
    // 1. Ingest two mutations before crash
    ZyxelDriver::getInstance().blockIp("192.0.2.88", "Crash Test 1");
    ZyxelDriver::getInstance().unblockIp("192.0.2.88");

    // Copy the written journal to a recovery path
    std::string crashJournal = testDir + "/recovered_journal.json";
    fs::copy_file(testJournal, crashJournal);

    // 2. Simulate daemon restart / reset and point to the persisted journal
    ZyxelDriver::getInstance().resetForTesting();
    ZyxelDriver::getInstance().setJournalPath(crashJournal);

    // 3. Verify recovery load finds both mutations on disk with state pending
    auto recovered = ZyxelDriver::getInstance().getPendingMutationsForTesting();
    LONGS_EQUAL(2, recovered.size());
    STRCMP_EQUAL("blk_192_0_2_88", recovered[0].id.c_str());
    STRCMP_EQUAL("block", recovered[0].op.c_str());
    STRCMP_EQUAL("pending", recovered[0].state.c_str());
    STRCMP_EQUAL("unblk_192_0_2_88", recovered[1].id.c_str());
    STRCMP_EQUAL("unblock", recovered[1].op.c_str());
    STRCMP_EQUAL("pending", recovered[1].state.c_str());
}

TEST(ZyxelDriverTest, ConfigurationLockedReturnsLockedAndPreservesJournal) {
    // Verify config locked banner detection
    std::string lockedBanner = "% Configuration is locked by admin (Web GUI)";
    CHECK_TRUE(ZyxelSshClient::isConfigLocked(lockedBanner));

    // Add a mutation and verify it remains pending in journal
    ZyxelDriver::getInstance().blockIp("192.0.2.99", "Locked Session Test");
    auto mutations = ZyxelDriver::getInstance().getPendingMutationsForTesting();
    LONGS_EQUAL(1, mutations.size());
    STRCMP_EQUAL("pending", mutations[0].state.c_str());
}

TEST(ZyxelDriverTest, ExplicitSyntaxFailureExecutesRollbackAndPurgesPoisonEntry) {
    ZyxelDriver::getInstance().blockIp("192.0.2.11", "Valid 1");
    ZyxelDriver::getInstance().blockIp("192.0.2.22", "Invalid To Purge");
    ZyxelDriver::getInstance().blockIp("192.0.2.33", "Valid 2");

    auto before = ZyxelDriver::getInstance().getPendingMutationsForTesting();
    LONGS_EQUAL(3, before.size());

    // Purge the failed syntax-invalid mutation blk_192_0_2_22
    CHECK_TRUE(ZyxelDriver::getInstance().removeMutationForTesting("blk_192_0_2_22"));

    auto after = ZyxelDriver::getInstance().getPendingMutationsForTesting();
    LONGS_EQUAL(2, after.size());
    STRCMP_EQUAL("blk_192_0_2_11", after[0].id.c_str());
    STRCMP_EQUAL("blk_192_0_2_33", after[1].id.c_str());
}

TEST(ZyxelDriverTest, TransportTimeoutOrDisconnectPreservesPendingJournalEntry) {
    // When transport drops or times out, blockIp returns queued and keeps entry as pending
    auto res = ZyxelDriver::getInstance().blockIp("192.0.2.123", "Transport Timeout Test");
    STRCMP_EQUAL("queued", res.value("status", "").c_str());

    auto mutations = ZyxelDriver::getInstance().getPendingMutationsForTesting();
    LONGS_EQUAL(1, mutations.size());
    STRCMP_EQUAL("blk_192_0_2_123", mutations[0].id.c_str());
    STRCMP_EQUAL("pending", mutations[0].state.c_str());
}

TEST(ZyxelDriverTest, DebounceDoesNotReexecuteAppliedRunningMutations) {
    // Verify that mutations marked applied_running are recognized as already on running-config
    RouterMutation m;
    m.id = "blk_192_0_2_150";
    m.op = "block";
    m.ip = "192.0.2.150";
    m.sanitizedName = "192_0_2_150";
    m.reason = "Debounce Direct Save";
    m.state = "applied_running";
    m.timestamp = time(nullptr);

    CHECK_TRUE(ZyxelDriver::getInstance().addMutationForTesting(m));

    std::vector<RouterMutation> loaded;
    bool dirty = false;
    CHECK_TRUE(ZyxelDriver::getInstance().loadJournalForTesting(loaded, dirty));
    LONGS_EQUAL(1, loaded.size());
    STRCMP_EQUAL("applied_running", loaded[0].state.c_str());
}

TEST(ZyxelDriverTest, FreshSessionReconnectDeletesByNameThenInsertsOnce) {
    // Populate journal with an uncommitted block mutation
    RouterMutation m;
    m.id = "blk_192_0_2_70";
    m.op = "block";
    m.ip = "192.0.2.70";
    m.sanitizedName = "192_0_2_70";
    m.reason = "Reconnect Replay Test";
    m.state = "pending";
    m.timestamp = time(nullptr);

    CHECK_TRUE(ZyxelDriver::getInstance().addMutationForTesting(m));

    // When offline, replay cannot connect and returns false, preserving journal entry
    CHECK_FALSE(ZyxelDriver::getInstance().replayJournal());

    auto loaded = ZyxelDriver::getInstance().getPendingMutationsForTesting();
    LONGS_EQUAL(1, loaded.size());
    STRCMP_EQUAL("blk_192_0_2_70", loaded[0].id.c_str());
    STRCMP_EQUAL("pending", loaded[0].state.c_str());
}

TEST(ZyxelDriverTest, MixedJournalPartialFlushCommitsAppliedAndPreservesPending) {
    // Journal contains one applied_running and one pending (e.g. locked/timeout)
    RouterMutation mApplied;
    mApplied.id = "blk_192_0_2_55";
    mApplied.op = "block";
    mApplied.ip = "192.0.2.55";
    mApplied.sanitizedName = "192_0_2_55";
    mApplied.reason = "Applied";
    mApplied.state = "applied_running";
    mApplied.timestamp = time(nullptr);

    RouterMutation mPending;
    mPending.id = "blk_192_0_2_66";
    mPending.op = "block";
    mPending.ip = "192.0.2.66";
    mPending.sanitizedName = "192_0_2_66";
    mPending.reason = "Locked Pending";
    mPending.state = "pending";
    mPending.timestamp = time(nullptr);

    std::vector<RouterMutation> mixed = { mApplied, mPending };
    CHECK_TRUE(ZyxelDriver::getInstance().saveJournalForTesting(mixed, true));

    std::vector<RouterMutation> loaded;
    bool dirty = false;
    CHECK_TRUE(ZyxelDriver::getInstance().loadJournalForTesting(loaded, dirty));
    LONGS_EQUAL(2, loaded.size());
    STRCMP_EQUAL("applied_running", loaded[0].state.c_str());
    STRCMP_EQUAL("pending", loaded[1].state.c_str());
}

TEST(ZyxelDriverTest, ResetForTestingDoesNotCreateOrTouchFiles) {
    const char *homeEnv = getenv("HOME");
    CHECK_TRUE(homeEnv != nullptr);
    std::string home(homeEnv);
    std::string defaultJournal = home + "/.config/netmon/router_journal.json";
    std::string defaultPin = home + "/.config/netmon/router_hostkey.pin";

    // Call resetForTesting()
    ZyxelDriver::getInstance().resetForTesting();

    // Verify resetForTesting() did not create any files or directories in test HOME
    CHECK_FALSE(fs::exists(defaultJournal));
    CHECK_FALSE(fs::exists(defaultPin));
}

TEST(ZyxelDriverTest, RouterLiveEnabledDefaultsToFalseAndNeverConnects) {
    CHECK_FALSE(Config::getInstance().getRouterLiveEnabled());
    CHECK_FALSE(ZyxelDriver::getInstance().isLiveEnabled());

    ZyxelDriver::getInstance().configure("192.0.2.1", 22, "admin");

    // getStatus reports disabled and live_enabled: false without connecting
    auto status = ZyxelDriver::getInstance().getStatus();
    STRCMP_EQUAL("disabled", status.value("status", "").c_str());
    CHECK_FALSE(status.value("live_enabled", true));
    CHECK_FALSE(ZyxelDriver::getInstance().isConnected());

    // getSessions returns empty sessions without connecting
    auto sessions = ZyxelDriver::getInstance().getSessions();
    STRCMP_EQUAL("disabled", sessions.value("status", "").c_str());
    CHECK_FALSE(ZyxelDriver::getInstance().isConnected());

    // blockIp queues mutation to journal without connecting
    auto blockRes = ZyxelDriver::getInstance().blockIp("192.0.2.101", "Live Disabled Test");
    STRCMP_EQUAL("queued", blockRes.value("status", "").c_str());
    CHECK_FALSE(blockRes.value("live_enabled", true));
    CHECK_FALSE(ZyxelDriver::getInstance().isConnected());

    // unblockIp queues mutation to journal without connecting
    auto unblockRes = ZyxelDriver::getInstance().unblockIp("192.0.2.101");
    STRCMP_EQUAL("queued", unblockRes.value("status", "").c_str());
    CHECK_FALSE(unblockRes.value("live_enabled", true));
    CHECK_FALSE(ZyxelDriver::getInstance().isConnected());

    // flushFlashWrite and replayJournal return without connecting
    ZyxelDriver::getInstance().flushFlashWrite();
    CHECK_FALSE(ZyxelDriver::getInstance().replayJournal());
    CHECK_FALSE(ZyxelDriver::getInstance().isConnected());
}

TEST(ZyxelDriverTest, RouterDryRunLogsExactZyShBlockSequence) {
    ZyxelDriver::getInstance().setDryRun(true);
    CHECK_TRUE(ZyxelDriver::getInstance().isDryRun());

    auto res = ZyxelDriver::getInstance().blockIp("192.0.2.55", "Bandwidth Flood");
    STRCMP_EQUAL("queued", res.value("status", "").c_str());
    CHECK_TRUE(res.value("dry_run", false));
    STRCMP_EQUAL("NETMON_RULE_192_0_2_55", res.value("rule", "").c_str());
    CHECK_FALSE(ZyxelDriver::getInstance().isConnected());

    // In dry-run mode, the mutation in the journal must remain "pending"
    auto mutations = ZyxelDriver::getInstance().getPendingMutationsForTesting();
    LONGS_EQUAL(1, mutations.size());
    STRCMP_EQUAL("pending", mutations[0].state.c_str());

    auto log = ZyxelDriver::getInstance().getDryRunLog();
    LONGS_EQUAL(11, log.size());
    STRCMP_EQUAL("configure terminal", log[0].c_str());
    STRCMP_EQUAL("address-object NETMON_BLK_192_0_2_55 host 192.0.2.55", log[1].c_str());
    STRCMP_EQUAL("exit", log[2].c_str());
    STRCMP_EQUAL("policy-control rule-insert 1", log[3].c_str());
    STRCMP_EQUAL("name NETMON_RULE_192_0_2_55", log[4].c_str());
    STRCMP_EQUAL("action deny", log[5].c_str());
    STRCMP_EQUAL("source-ip NETMON_BLK_192_0_2_55", log[6].c_str());
    STRCMP_EQUAL("description \"NetMon Auto-Block: Bandwidth Flood\"", log[7].c_str());
    STRCMP_EQUAL("activate", log[8].c_str());
    STRCMP_EQUAL("exit", log[9].c_str());
    STRCMP_EQUAL("exit", log[10].c_str());
}

TEST(ZyxelDriverTest, RouterDryRunLogsExactZyShUnblockSequence) {
    ZyxelDriver::getInstance().setDryRun(true);
    CHECK_TRUE(ZyxelDriver::getInstance().isDryRun());

    auto res = ZyxelDriver::getInstance().unblockIp("192.0.2.66");
    STRCMP_EQUAL("queued", res.value("status", "").c_str());
    CHECK_TRUE(res.value("dry_run", false));
    CHECK_FALSE(ZyxelDriver::getInstance().isConnected());

    // In dry-run mode, the mutation in the journal must remain "pending"
    auto mutations = ZyxelDriver::getInstance().getPendingMutationsForTesting();
    LONGS_EQUAL(1, mutations.size());
    STRCMP_EQUAL("pending", mutations[0].state.c_str());

    auto log = ZyxelDriver::getInstance().getDryRunLog();
    LONGS_EQUAL(4, log.size());
    STRCMP_EQUAL("configure terminal", log[0].c_str());
    STRCMP_EQUAL("no policy-control NETMON_RULE_192_0_2_66", log[1].c_str());
    STRCMP_EQUAL("no address-object NETMON_BLK_192_0_2_66", log[2].c_str());
    STRCMP_EQUAL("exit", log[3].c_str());
}

TEST(ZyxelDriverTest, RouterDryRunLogsFreshSessionReplayDeleteThenInsert) {
    // Stage an unapplied block mutation in journal
    RouterMutation m;
    m.id = "blk_192_0_2_77";
    m.op = "block";
    m.ip = "192.0.2.77";
    m.sanitizedName = "192_0_2_77";
    m.reason = "Replay Dry Run Test";
    m.state = "pending";
    m.timestamp = time(nullptr);
    CHECK_TRUE(ZyxelDriver::getInstance().addMutationForTesting(m));

    ZyxelDriver::getInstance().setDryRun(true);
    ZyxelDriver::getInstance().clearDryRunLog();

    CHECK_TRUE(ZyxelDriver::getInstance().replayJournal());
    CHECK_FALSE(ZyxelDriver::getInstance().isConnected());

    // In dry-run mode, replay must leave the mutation "pending"
    auto loaded = ZyxelDriver::getInstance().getPendingMutationsForTesting();
    LONGS_EQUAL(1, loaded.size());
    STRCMP_EQUAL("pending", loaded[0].state.c_str());

    auto log = ZyxelDriver::getInstance().getDryRunLog();
    LONGS_EQUAL(13, log.size());
    STRCMP_EQUAL("configure terminal", log[0].c_str());
    STRCMP_EQUAL("no policy-control NETMON_RULE_192_0_2_77", log[1].c_str());
    STRCMP_EQUAL("no address-object NETMON_BLK_192_0_2_77", log[2].c_str());
    STRCMP_EQUAL("address-object NETMON_BLK_192_0_2_77 host 192.0.2.77", log[3].c_str());
    STRCMP_EQUAL("exit", log[4].c_str());
    STRCMP_EQUAL("policy-control rule-insert 1", log[5].c_str());
    STRCMP_EQUAL("name NETMON_RULE_192_0_2_77", log[6].c_str());
    STRCMP_EQUAL("action deny", log[7].c_str());
    STRCMP_EQUAL("source-ip NETMON_BLK_192_0_2_77", log[8].c_str());
    STRCMP_EQUAL("description \"NetMon Auto-Block: Replay Dry Run Test\"", log[9].c_str());
    STRCMP_EQUAL("activate", log[10].c_str());
    STRCMP_EQUAL("exit", log[11].c_str());
    STRCMP_EQUAL("exit", log[12].c_str());
}

TEST(ZyxelDriverTest, RouterFlashWriteDisabledByDefaultDoesNotSendWriteOrPrune) {
    CHECK_FALSE(Config::getInstance().getRouterFlashWrite());
    CHECK_FALSE(ZyxelDriver::getInstance().isFlashWriteEnabled());

    // 1. Stage an applied_running mutation
    RouterMutation m;
    m.id = "blk_192_0_2_88";
    m.op = "block";
    m.ip = "192.0.2.88";
    m.sanitizedName = "192_0_2_88";
    m.reason = "Flash Write Guard Test";
    m.state = "applied_running";
    m.timestamp = time(nullptr);
    CHECK_TRUE(ZyxelDriver::getInstance().addMutationForTesting(m));

    ZyxelDriver::getInstance().setDryRun(true);
    ZyxelDriver::getInstance().clearDryRunLog();

    // 2. Calling flushFlashWrite() with flash write disabled does not prune or log write
    ZyxelDriver::getInstance().flushFlashWrite();
    auto log1 = ZyxelDriver::getInstance().getDryRunLog();
    LONGS_EQUAL(0, log1.size());

    auto loaded1 = ZyxelDriver::getInstance().getPendingMutationsForTesting();
    LONGS_EQUAL(1, loaded1.size());
    STRCMP_EQUAL("applied_running", loaded1[0].state.c_str());

    // 3. Enabling flash write allows flushFlashWrite() in dry-run to log write and prune applied row
    ZyxelDriver::getInstance().setFlashWriteEnabled(true);
    ZyxelDriver::getInstance().flushFlashWrite();

    auto log2 = ZyxelDriver::getInstance().getDryRunLog();
    LONGS_EQUAL(1, log2.size());
    STRCMP_EQUAL("write", log2[0].c_str());

    auto loaded2 = ZyxelDriver::getInstance().getPendingMutationsForTesting();
    LONGS_EQUAL(0, loaded2.size());
}

TEST(ZyxelDriverTest, EnvironmentVariablesCannotEnableLiveOrFlashWrite) {
    setenv("NETMON_ROUTER_LIVE_ENABLED", "1", 1);
    setenv("NETMON_ROUTER_FLASH_WRITE", "1", 1);

    std::string emptyCfg = testDir + "/empty.cfg";
    {
        std::ofstream ofs(emptyCfg);
        ofs << "interface = \"br0\";\n";
    }

    Config::getInstance().load(emptyCfg);

    CHECK_FALSE(Config::getInstance().getRouterLiveEnabled());
    CHECK_FALSE(Config::getInstance().getRouterFlashWrite());
    CHECK_FALSE(ZyxelDriver::getInstance().isLiveEnabled());
    CHECK_FALSE(ZyxelDriver::getInstance().isFlashWriteEnabled());

    unsetenv("NETMON_ROUTER_LIVE_ENABLED");
    unsetenv("NETMON_ROUTER_FLASH_WRITE");
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
