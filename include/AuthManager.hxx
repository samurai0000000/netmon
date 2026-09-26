/*
 * AuthManager.hxx
 *
 * Copyright (C) 2026, Charles Chiou
 */

#ifndef NETMON_AUTHMANAGER_HXX
#define NETMON_AUTHMANAGER_HXX

#include <string>
#include <map>
#include <mutex>
#include <ctime>

class AuthManager {
public:
    static AuthManager &getInstance();

    bool setPassword(const std::string &newPassword,
                     const std::string &oldPassword = "",
                     bool requireOld = false);

    std::string login(const std::string &password);
    bool validateSession(const std::string &token);
    void logout(const std::string &token);

    int getDiskGeneration();

    void setVaultDir(const std::string &dir);
    const std::string &getVaultDir() const;

    void clearSessions();
    void resetForTesting();
    void setSessionTimestampForTesting(const std::string &token, time_t timestamp);

private:
    AuthManager();
    ~AuthManager() = default;
    AuthManager(const AuthManager &) = delete;
    AuthManager &operator=(const AuthManager &) = delete;

    std::string getVaultKeyPath() const;
    std::string getVaultEncPath() const;
    std::string getVaultLockPath() const;

    bool ensureDirectory(const std::string &dir) const;

    struct Session {
        std::string token;
        int         issuedGeneration;
        time_t      lastUsedTime;
    };

    std::string              _vaultDir;
    mutable std::mutex       _vaultDirMutex;

    mutable std::mutex       _sessionMutex;
    std::map<std::string, Session> _sessions;
};

#endif /* NETMON_AUTHMANAGER_HXX */

/*
 * Local variables:
 * mode: C++
 * c-file-style: "BSD"
 * c-basic-offset: 4
 * tab-width: 4
 * indent-tabs-mode: nil
 * End:
 */
