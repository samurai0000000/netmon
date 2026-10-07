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
    ERR_BUSY,
    ERR_UNSAFE_UNWIND,
    ERR_HOSTKEY_REJECTED,
    ERR_REJECTED
};

enum class PromptState {
    USER,
    ROOT,
    CONFIG,
    POLICY_SUBMODE,
    OTHER_SUBMODE,
    UNKNOWN
};

struct ZyshTransport {
    std::function<SshResult(const std::string &host, int port,
                            const std::string &user, const std::string &password,
                            std::string &hostKeySha256Out)> open;
    std::function<ssize_t(char *buf, size_t len, bool &eofOut)> read;
    std::function<ssize_t(const char *buf, size_t len)> write;
    std::function<bool()> keepalive;
    std::function<void()> close;
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
    PromptState getPromptState() const;
    bool policyInactiveAcknowledged() const;
    bool sendKeepalive();
    void resetForTesting();

    void setTransport(ZyshTransport transport);
    SshResult probeHostKey(std::string &sha256HexOut);
    SshResult commitHostKeyPin(const std::string &sha256Hex);
    static SshResult writeHostKeyPin(const std::string &path, const std::string &sha256Hex);

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
    static bool matchPrompt(const std::string &hostname,
                            const std::string &buffer,
                            std::string &matchedPrompt,
                            PromptState &stateOut);
    static bool classifyPromptLine(const std::string &buffer, PromptState &stateOut);
    static std::string stripCommandEcho(const std::string &buffer, const std::string &commandSent);
    static std::string stripTrailingPrompt(const std::string &buffer);
    static bool isConfigLocked(const std::string &buffer);
    static bool isSyntaxError(const std::string &buffer);
    static std::string sanitizeReason(const std::string &rawReason);
    static std::string sanitizeIpToObjectName(const std::string &ip);

private:
    SshResult verifyHostKey(LIBSSH2_SESSION *session);
    SshResult verifyPinHex(const std::string &currentHex);
    SshResult connectTransportUnlocked(const std::string &password);
    void disconnectUnlocked();
    void dropSessionUnlocked();
    bool acceptPrompt(const std::string &buffer);
    void notePolicyAck(const std::string &command);
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
    std::string        _hostname;
    std::string        _lastMatchedPrompt;
    PromptState        _promptState;
    bool               _policyInactiveAck;
    std::atomic<bool>  _cancelled;

    bool               _hasTransport;
    ZyshTransport      _transport;
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
