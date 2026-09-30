/*
 * TestInstanceLock.cxx
 *
 * Copyright (C) 2026, Charles Chiou
 */

#include <iostream>
#include <fstream>
#include <string>
#include <filesystem>
#include <sys/types.h>
#include <unistd.h>

#include "InstanceLock.hxx"
#include <CppUTest/TestHarness.h>

namespace fs = std::filesystem;

TEST_GROUP(InstanceLockTest) {
    std::string testLockDir = "test_data/test_instance_lock";

    void setup() {
        if (fs::exists(testLockDir)) {
            fs::remove_all(testLockDir);
        }
        fs::create_directories(testLockDir);
    }

    void teardown() {
        if (fs::exists(testLockDir)) {
            fs::remove_all(testLockDir);
        }
    }
};

TEST(InstanceLockTest, AcquireAndReleaseNominal) {
    InstanceLock lock1;
    CHECK_FALSE(lock1.isLocked());
    CHECK_TRUE(lock1.acquire(testLockDir));
    CHECK_TRUE(lock1.isLocked());
    CHECK_FALSE(lock1.getHostname().empty());
    CHECK_FALSE(lock1.getLockFilePath().empty());
    CHECK_TRUE(fs::exists(lock1.getLockFilePath()));

    // Verify content of lock file
    std::ifstream ifs(lock1.getLockFilePath());
    CHECK_TRUE(ifs.is_open());
    pid_t filePid = 0;
    std::string fileHost;
    ifs >> filePid >> fileHost;
    ifs.close();
    LONGS_EQUAL(getpid(), filePid);
    STRCMP_EQUAL(lock1.getHostname().c_str(), fileHost.c_str());

    // Second instance attempting to acquire in same directory must fail
    InstanceLock lock2;
    CHECK_FALSE(lock2.acquire(testLockDir));
    CHECK_FALSE(lock2.isLocked());
    LONGS_EQUAL(getpid(), lock2.getExistingPid());

    // Release lock1
    lock1.release();
    CHECK_FALSE(lock1.isLocked());
    CHECK_FALSE(fs::exists(lock1.getLockFilePath()));

    // Now lock2 can acquire successfully
    CHECK_TRUE(lock2.acquire(testLockDir));
    CHECK_TRUE(lock2.isLocked());
    CHECK_TRUE(fs::exists(lock2.getLockFilePath()));
    lock2.release();
    CHECK_FALSE(fs::exists(lock2.getLockFilePath()));
}

TEST(InstanceLockTest, StaleDeadPidOverwritten) {
    InstanceLock probe;
    std::string lockFilePath = testLockDir + "/netmon." + probe.getHostname() + ".pid";

    // Write a definitely dead PID (e.g. 999999) with matching hostname
    std::ofstream ofs(lockFilePath);
    CHECK_TRUE(ofs.is_open());
    ofs << "999999\n" << probe.getHostname() << "\n";
    ofs.close();
    CHECK_TRUE(fs::exists(lockFilePath));

    // A new acquire must detect the dead PID, reclaim the lock, and overwrite with current PID
    InstanceLock lock;
    CHECK_TRUE(lock.acquire(testLockDir));
    CHECK_TRUE(lock.isLocked());

    std::ifstream ifs(lock.getLockFilePath());
    CHECK_TRUE(ifs.is_open());
    pid_t filePid = 0;
    std::string fileHost;
    ifs >> filePid >> fileHost;
    ifs.close();
    LONGS_EQUAL(getpid(), filePid);
    STRCMP_EQUAL(lock.getHostname().c_str(), fileHost.c_str());

    lock.release();
    CHECK_FALSE(fs::exists(lock.getLockFilePath()));
}

TEST(InstanceLockTest, RecycledPidDifferentProcessOverwritten) {
    InstanceLock probe;
    std::string lockFilePath = testLockDir + "/netmon." + probe.getHostname() + ".pid";

    // PID 1 (init/systemd) is alive on Linux, but its /proc/1/comm is NOT "netmon"
    std::ofstream ofs(lockFilePath);
    CHECK_TRUE(ofs.is_open());
    ofs << "1\n" << probe.getHostname() << "\n";
    ofs.close();
    CHECK_TRUE(fs::exists(lockFilePath));

    // Acquire must inspect /proc/1/comm, notice it's not netmon, and reclaim the lock
    InstanceLock lock;
    CHECK_TRUE(lock.acquire(testLockDir));
    CHECK_TRUE(lock.isLocked());

    std::ifstream ifs(lock.getLockFilePath());
    CHECK_TRUE(ifs.is_open());
    pid_t filePid = 0;
    std::string fileHost;
    ifs >> filePid >> fileHost;
    ifs.close();
    LONGS_EQUAL(getpid(), filePid);
    STRCMP_EQUAL(lock.getHostname().c_str(), fileHost.c_str());

    lock.release();
    CHECK_FALSE(fs::exists(lock.getLockFilePath()));
}

TEST(InstanceLockTest, LockFileNameMatchesHostname) {
    InstanceLock lock;
    CHECK_TRUE(lock.acquire(testLockDir));
    std::string expectedEnding = "netmon." + lock.getHostname() + ".pid";
    CHECK_TRUE(lock.getLockFilePath().size() >= expectedEnding.size());
    std::string ending = lock.getLockFilePath().substr(
        lock.getLockFilePath().size() - expectedEnding.size());
    STRCMP_EQUAL(expectedEnding.c_str(), ending.c_str());
    lock.release();
}

TEST(InstanceLockTest, DirectoryCreated) {
    std::string newDir = testLockDir + "/subdir";
    InstanceLock lock;
    CHECK_TRUE(lock.acquire(newDir));
    CHECK_TRUE(lock.isLocked());
    CHECK_TRUE(fs::exists(lock.getLockFilePath()));
    lock.release();
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
