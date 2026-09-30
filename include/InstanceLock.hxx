/*
 * InstanceLock.hxx
 *
 * Copyright (C) 2026, Charles Chiou
 */

#ifndef NETMON_INSTANCE_LOCK_HXX
#define NETMON_INSTANCE_LOCK_HXX

#include <string>
#include <sys/types.h>

class InstanceLock {
public:
    InstanceLock();
    ~InstanceLock();

    bool acquire(const std::string &lockDir = "");
    void release();

    const std::string &getHostname() const;
    const std::string &getLockFilePath() const;
    pid_t getExistingPid() const;
    bool isLocked() const;

private:
    std::string _hostname;
    std::string _lockFilePath;
    int _lockFd;
    pid_t _existingPid;
    bool _locked;
};

#endif /* NETMON_INSTANCE_LOCK_HXX */

/*
 * Local variables:
 * mode: C++
 * c-file-style: "BSD"
 * c-basic-offset: 4
 * tab-width: 4
 * indent-tabs-mode: nil
 * End:
 */
