/*
 * UnixAuth.cxx
 *
 * Copyright (C) 2026, Charles Chiou
 */

#include "UnixAuth.hxx"
#include <cstdlib>
#include <cstring>
#include <unistd.h>
#include <pwd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <mutex>
#include <iostream>

static std::mutex g_testMutex;
static UnixAuth::AuthVerifierFunc g_testVerifier = nullptr;

void UnixAuth::setAuthVerifierForTesting(AuthVerifierFunc verifier) {
    std::lock_guard<std::mutex> lock(g_testMutex);
    g_testVerifier = verifier;
}

void UnixAuth::resetForTesting() {
    std::lock_guard<std::mutex> lock(g_testMutex);
    g_testVerifier = nullptr;
}

std::string UnixAuth::getCurrentUsername() {
    uid_t uid = getuid();
    struct passwd *pw = getpwuid(uid);
    if (pw && pw->pw_name && pw->pw_name[0] != '\0') {
        return std::string(pw->pw_name);
    }

    const char *login = getlogin();
    if (login && login[0] != '\0') {
        return std::string(login);
    }

    const char *userEnv = getenv("USER");
    if (userEnv && userEnv[0] != '\0') {
        return std::string(userEnv);
    }

    return "samurai";
}

bool UnixAuth::authenticate(const std::string &password) {
    std::string user = getCurrentUsername();
    return authenticateUser(user, password);
}

bool UnixAuth::authenticateUser(const std::string &username, const std::string &password) {
    {
        std::lock_guard<std::mutex> lock(g_testMutex);
        if (g_testVerifier) {
            return g_testVerifier(username, password);
        }
    }

    if (username.empty() || password.empty()) {
        return false;
    }

    // Sanitize username: only alphanumeric, underscore, and hyphen
    for (char c : username) {
        if (!((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') ||
              (c >= '0' && c <= '9') || c == '_' || c == '-')) {
            return false;
        }
    }

    int pipeFds[2];
    if (pipe(pipeFds) != 0) {
        return false;
    }

    pid_t pid = fork();
    if (pid < 0) {
        close(pipeFds[0]);
        close(pipeFds[1]);
        return false;
    }

    if (pid == 0) {
        // Child: execute /sbin/unix_chkpwd
        close(pipeFds[1]);
        if (dup2(pipeFds[0], STDIN_FILENO) < 0) {
            _exit(127);
        }
        close(pipeFds[0]);

        int devNull = open("/dev/null", O_WRONLY);
        if (devNull >= 0) {
            dup2(devNull, STDOUT_FILENO);
            dup2(devNull, STDERR_FILENO);
            close(devNull);
        }

        execl("/sbin/unix_chkpwd", "unix_chkpwd", username.c_str(), "nullok", (char *)NULL);
        _exit(127);
    }

    // Parent
    close(pipeFds[0]);

    // unix_chkpwd expects the password on stdin followed by a NUL byte ('\0')
    ssize_t pwLen = static_cast<ssize_t>(password.size());
    ssize_t written = write(pipeFds[1], password.data(), pwLen);
    char nul = '\0';
    ssize_t nulWritten = write(pipeFds[1], &nul, 1);
    close(pipeFds[1]);

    (void)written;
    (void)nulWritten;

    int status = 0;
    while (waitpid(pid, &status, 0) < 0) {
        if (errno == EINTR) {
            continue;
        }
        return false;
    }

    if (WIFEXITED(status) && WEXITSTATUS(status) == 0) {
        return true;
    }

    return false;
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
