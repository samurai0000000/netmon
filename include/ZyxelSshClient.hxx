/*
 * ZyxelSshClient.hxx
 *
 * Copyright (C) 2026, Charles Chiou
 */

#ifndef NETMON_ZYXELSSHCLIENT_HXX
#define NETMON_ZYXELSSHCLIENT_HXX

#include <string>
#include <vector>
#include <mutex>
#include <atomic>
#include <cstdint>
#include <functional>

#include <libssh2.h>

enum class SshClientState {
    DISCONNECTED,
    CONNECTING,
    AUTHENTICATED,
    DEGRADED_AUTH_FAILED,
    DEGRADED_HOSTKEY_MISMATCH,
    ERROR_DISCONNECTED
};

enum class SshResult {
    SUCCESS = 0,
    ERR_DISCONNECTED,
    ERR_CONNECT_FAILED,
    ERR_HANDSHAKE_FAILED,
    ERR_HOSTKEY_MISMATCH,
    ERR_AUTH_FAILED,
    ERR_CHANNEL_FAILED,
    ERR_PTY_FAILED,
    ERR_TIMEOUT,
    ERR_LOCKED,
    ERR_EXEC_FAILED,
    ERR_SYNTAX,
    ERR_INTERRUPTED,
    ERR_BUSY
};

class ZyxelSshClient {
public:
    ZyxelSshClient();
    ~ZyxelSshClient();

    ZyxelSshClient(const ZyxelSshClient &) = delete;
    ZyxelSshClient &operator=(const ZyxelSshClient &) = delete;

    void configure(const std::string &host, int port,
                   const std::string &user,
                   const std::string &pinPath = "");

    SshResult connect(const std::string &password);
    void disconnect();

    bool isConnected() const;
    SshClientState getState() const;

    SshResult executeCommand(const std::string &command,
                            std::string &outputOut,
                            int timeoutMs = 5000);

    SshResult unwindToRootPrompt(int maxAttempts = 5);
    std::string getLastMatchedPrompt() const;
    bool sendKeepalive();
    void resetForTesting();

    void cancelActiveCommand();
    bool isCancelled() const;

    using ChannelReader = std::function<ssize_t(char *buf, size_t buflen, bool &eofOut)>;
    using ChannelWriter = std::function<void(const char *cmd, size_t len)>;

    SshResult drainUntilPromptForTesting(
        const ChannelReader &reader,
        const ChannelWriter &writer,
        std::string &outputOut,
        int timeoutMs = 5000);

    // Pure parsing, transformation & sanitization utilities (testable without network)
    static std::string stripAnsiEscapes(const std::string &input);
    static bool matchPrompt(const std::string &buffer, std::string &matchedPrompt);
    static std::string stripCommandEcho(const std::string &buffer, const std::string &commandSent);
    static std::string stripTrailingPrompt(const std::string &buffer);
    static bool isConfigLocked(const std::string &buffer);
    static bool isSyntaxError(const std::string &buffer);
    static std::string sanitizeReason(const std::string &rawReason);
    static std::string sanitizeIpToObjectName(const std::string &ip);

private:
    SshResult verifyHostKey(LIBSSH2_SESSION *session);
    SshResult drainUntilPrompt(std::string &outputOut, int timeoutMs = 5000);
    SshResult drainUntilPromptWithReader(
        const ChannelReader &reader,
        const ChannelWriter &writer,
        std::string &outputOut,
        int timeoutMs = 5000);
    SshResult executeCommandUnlocked(const std::string &command,
                                    std::string &outputOut,
                                    int timeoutMs = 5000);
    SshResult unwindToRootPromptUnlocked(int maxAttempts = 5);

    mutable std::mutex _sshMutex;

    std::string        _host;
    int                _port;
    std::string        _user;
    std::string        _pinPath;

    int                _socketFd;
    LIBSSH2_SESSION   *_session;
    LIBSSH2_CHANNEL   *_channel;

    std::atomic<SshClientState> _state;
    std::string        _lastMatchedPrompt;
    std::atomic<bool>  _cancelled;
};

#endif /* NETMON_ZYXELSSHCLIENT_HXX */

/*
 * Local variables:
 * mode: C++
 * c-file-style: "BSD"
 * c-basic-offset: 4
 * tab-width: 4
 * indent-tabs-mode: nil
 * End:
 */
