/*
 * UnixAuth.hxx
 *
 * Copyright (C) 2026, Charles Chiou
 */

#ifndef NETMON_UNIXAUTH_HXX
#define NETMON_UNIXAUTH_HXX

#include <string>
#include <functional>

class UnixAuth {
public:
    // Authenticate the current executing UNIX user with a password
    static bool authenticate(const std::string &password);

    // Authenticate a specific UNIX user with a password
    static bool authenticateUser(const std::string &username, const std::string &password);

    // Get current login username (from getlogin / getpwuid)
    static std::string getCurrentUsername();

    // Testing override hook
    using AuthVerifierFunc = std::function<bool(const std::string &user, const std::string &pass)>;
    static void setAuthVerifierForTesting(AuthVerifierFunc verifier);
    static void resetForTesting();

private:
    UnixAuth() = delete;
    ~UnixAuth() = delete;
    UnixAuth(const UnixAuth &) = delete;
    UnixAuth &operator=(const UnixAuth &) = delete;
};

#endif /* NETMON_UNIXAUTH_HXX */

/*
 * Local variables:
 * mode: C++
 * c-file-style: "BSD"
 * c-basic-offset: 4
 * tab-width: 4
 * indent-tabs-mode: nil
 * End:
 */
