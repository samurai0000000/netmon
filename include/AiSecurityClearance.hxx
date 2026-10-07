/*
 * AiSecurityClearance.hxx
 *
 * Copyright (C) 2026, Charles Chiou
 */

#ifndef NETMON_AI_SECURITY_CLEARANCE_HXX
#define NETMON_AI_SECURITY_CLEARANCE_HXX

#include <string>
#include <vector>
#include <map>
#include <mutex>
#include <atomic>
#include <chrono>
#include <cstdint>
#include <functional>
#include <thread>
#include <queue>
#include <future>
#include <condition_variable>
#include <memory>

enum class ClearanceLevel {
    LEVEL3 = 3,   // Read and diagnostics (every connected clearance socket; no approval)
    LEVEL2 = 2,   // Network, firewall, NAT, and object mutation (only approved socket, until disconnect or 300s)
    LEVEL1 = 1    // Device management (human console only; always rejected on agent socket)
};

enum class ClearanceTier {
    NONE = 0,                 // Zero clearance granted; all DO commands rejected
    READ = 3,                 // Green text, default 0 (indefinite) timeout
    READ_WRITE = 2,           // Red flashing text, default 300s timeout, downgrades to READ
    READ_WRITE_PASSWORD = 1   // Bold red flashing text, default 300s timeout, downgrades to READ
};

enum class LineClassification {
    LEVEL3,
    LEVEL2_ROOT,
    LEVEL2_SUBMODE,
    LEVEL1,
    UNCLASSIFIED
};

#ifdef OK
#undef OK
#endif

enum class ClearanceResult {
    OK,
    NOT_AUTHORIZED,
    FORBIDDEN,
    UNCLASSIFIED,
    BUSY,
    CANCELED,
    TIMEOUT,
    LOCKED,
    SYNTAX,
    DISCONNECTED,
    FAILED
};

std::string clearanceResultToString(ClearanceResult result);
ClearanceResult stringToClearanceResult(const std::string &s);
std::string clearanceLevelToString(ClearanceLevel level);
std::string clearanceTierToString(ClearanceTier tier);

class AiSecurityClassifier {
public:
    static LineClassification classify(const std::string &line,
                                      const std::string &currentPrompt,
                                      int &timeoutMsOut,
                                      std::string &matchedMethodOut,
                                      bool policyInactiveAcked = true);

    static bool isControlOrChained(const std::string &line);
    static bool isNameValid(const std::string &name);
    static bool isIpv4Valid(const std::string &ip);
    static bool isPortValid(const std::string &portStr);
    static bool isPositionValid(const std::string &posStr);
    static bool isMetricValid(const std::string &metricStr);
    static bool isProtocolValid(const std::string &protoStr);
    static bool isMaskValid(const std::string &maskStr);

    static bool isSecurePolicySubmode(const std::string &prompt);
    static bool isRootPrompt(const std::string &prompt);

    static std::vector<std::string> tokenize(const std::string &line);
};

enum class ClientClearanceState {
    NONE = 0,
    PENDING = 1,
    LEVEL3 = 2,
    LEVEL2 = 3,
    DENIED = 4,
    READ = LEVEL3,
    READ_WRITE = LEVEL2
};

struct ClearanceConnection {
    uint64_t              connectionId = 0;
    int                   socketFd = -1;
    std::string           peerAddress;
    uint16_t              peerPort = 0;
    int                   protocolVersion = 0;
    ClientClearanceState  clearanceState = ClientClearanceState::NONE;
    ClearanceTier         requestedTier = ClearanceTier::READ_WRITE;
    ClearanceTier         activeTier = ClearanceTier::NONE;
    bool                  hasBeenGranted = false;
    std::chrono::steady_clock::time_point deadline;
    std::string           currentSubmode;
    std::string           inFlightCommand;
    bool                  hasInFlight = false;
    std::string           metadata;
};

struct ClearanceStatus {
    uint64_t              connectionId = 0;
    std::string           formattedConnId;
    std::string           peerAddress;
    uint16_t              peerPort = 0;
    ClientClearanceState  clearanceState = ClientClearanceState::NONE;
    ClearanceTier         activeTier = ClearanceTier::NONE;
    uint32_t              remainingSeconds = 0;
    std::string           metadata;
};

struct ClearanceAuditEntry {
    time_t      timestamp;
    uint64_t    connectionId;
    std::string peerAddress;
    uint16_t    peerPort;
    std::string command;
    std::string matchedMethod;
    int         requiredLevel;
    int         effectiveLevel;
    std::string decision;
    std::string reason;
    int         sshResult;
    int         durationMs;
    std::string outputSummary;
};

class AiSecurityClearanceManager {
public:
    static AiSecurityClearanceManager &getInstance();

    static std::string formatConnId(uint64_t id);
    static uint64_t parseConnId(const std::string &str);

    bool start(const std::string &bindAddress = "0.0.0.0", uint16_t port = 3885);
    void stop();
    bool isListening() const;
    uint16_t getPort() const { return _port; }
    const std::string &getBindAddress() const { return _bindAddress; }

    // Wire protocol message handlers (framing: length + UTF-8 string)
    std::string handleClientMessage(uint64_t connId, const std::string &msg);
    void handleSocketClosed(uint64_t connId);

    // Operator console interface
    bool hasPendingApproval(uint64_t &outConnId, std::string &outPeerIp, uint16_t &outPeerPort, ClearanceTier *outRequestedTier = nullptr) const;
    bool consoleApprove(uint64_t connId, uint32_t seconds = 300, ClearanceTier tier = ClearanceTier::READ_WRITE);
    bool consoleDeny(uint64_t connId, uint32_t seconds = 0);
    std::vector<ClearanceStatus> getActiveClearances() const;

    // Session prompt owner (Section 3.1)
    uint64_t getSessionOwner() const;
    void clearSessionOwner();

    // Audit query
    std::vector<ClearanceAuditEntry> getAuditLog(size_t limit = 100) const;
    void clearAuditLog();

    // Testing and lifecycle controls
    void resetForTesting();
    uint64_t registerConnectionForTesting(int fakeFd, const std::string &peerIp = "127.0.0.1", uint16_t peerPort = 49152);
    void setConnectionLevelForTesting(uint64_t connId, ClientClearanceState state, int remainingSeconds = 300);

private:
    AiSecurityClearanceManager();
    ~AiSecurityClearanceManager();
    AiSecurityClearanceManager(const AiSecurityClearanceManager &) = delete;
    AiSecurityClearanceManager &operator=(const AiSecurityClearanceManager &) = delete;

    struct RouterWorkResult {
        int sshResult = -1;
        std::string output;
        std::string matchedPrompt;
    };

    struct RouterWorkItem {
        uint64_t connId = 0;
        std::string command;
        int timeoutMs = 5000;
        std::promise<RouterWorkResult> promise;
    };

    void listenerWorker();
    void clientWorker(int clientFd, uint64_t connId, const std::string &peerIp, uint16_t peerPort);
    void routerWorker();

    std::string executeDo(std::unique_lock<std::recursive_mutex> &lock, ClearanceConnection &conn, const std::string &cmdLine);
    void logAudit(const ClearanceAuditEntry &entry);
    void logGrantRevoke(uint64_t connId, const std::string &grantType, const std::string &reason);

    mutable std::recursive_mutex        _mutex;
    std::string                         _bindAddress;
    uint16_t                            _port;
    std::atomic<bool>                   _running;
    int                                 _listenFd;
    std::thread                         _listenerThread;

    // Single router worker thread and queue (Section 5.2)
    std::thread                         _routerWorkerThread;
    mutable std::mutex                  _routerQueueMutex;
    std::condition_variable             _routerQueueCv;
    std::queue<std::shared_ptr<RouterWorkItem>> _routerQueue;

    // Track client threads to join cleanly on stop()
    struct ClientThreadRecord {
        std::thread th;
        std::shared_ptr<std::atomic<bool>> done;
    };
    mutable std::mutex                  _clientThreadsMutex;
    std::vector<ClientThreadRecord>     _clientThreads;

    uint64_t                            _nextConnId;
    std::map<uint64_t, ClearanceConnection> _connections;

    // Single pending approval and single Level 2 socket (Section 5.1)
    uint64_t                            _pendingApprovalConnId;
    uint64_t                            _activeLevel2ConnId;

    // Single SSH session owner (Section 3.1)
    uint64_t                            _sessionOwnerConnId;

    // Active command execution on router (Section 5.2)
    uint64_t                            _routerBusyConnId;

    std::vector<ClearanceAuditEntry>    _auditLog;
};

#endif /* NETMON_AI_SECURITY_CLEARANCE_HXX */

/*
 * Local variables:
 * mode: C++
 * c-file-style: "BSD"
 * c-basic-offset: 4
 * tab-width: 4
 * indent-tabs-mode: nil
 * End:
 */
