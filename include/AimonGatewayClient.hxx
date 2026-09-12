/*
 * AimonGatewayClient.hxx
 *
 * Copyright (C) 2026, Charles Chiou
 */

#ifndef NETMON_AIMON_GATEWAY_CLIENT_HXX
#define NETMON_AIMON_GATEWAY_CLIENT_HXX

#include <string>
#include <memory>
#include <thread>
#include <atomic>
#include <mutex>
#include "RouterDriver.hxx"

class AimonGatewayClient {
public:
    static AimonGatewayClient &getInstance();

    bool start(const std::string &host = "", uint16_t port = 0);
    void stop();
    void join();

    bool isConnected() const;

    void setRouterDriver(std::shared_ptr<RouterDriver> driver);

private:
    AimonGatewayClient();
    ~AimonGatewayClient();
    AimonGatewayClient(const AimonGatewayClient &) = delete;
    AimonGatewayClient &operator=(const AimonGatewayClient &) = delete;

    void run();
    bool connectToGateway();
    void disconnect();
    bool sendRegistration();
    void processIncoming();

    void handleRequest(const std::string &line);
    void sendResponse(const std::string &jsonResponse);

    // Tool execution handlers
    std::string toolLanGetDevices(const std::string &categoryFilter);
    std::string toolLanGetUnregisteredDevices();
    std::string toolLanNameDevice(const std::string &mac, const std::string &name,
                                  const std::string &category);
    std::string toolLanGetTopTalkers(int limit, int windowMinutes);
    std::string toolLanGetTrafficSummary();
    std::string toolFirewallGetStatus();
    std::string toolFirewallGetSessions();
    std::string toolFirewallBlockIp(const std::string &ip, const std::string &reason);
    std::string toolFirewallUnblockIp(const std::string &ip);
    std::string toolSnmpGetDeviceMetrics(const std::string &targetIp);

    std::shared_ptr<RouterDriver> _routerDriver;

    std::string _host;
    uint16_t _port;

    std::atomic<bool> _running;
    std::atomic<bool> _connected;
    int _sockFd;

    std::thread _thread;
    std::mutex _sendMutex;
};

#endif /* NETMON_AIMON_GATEWAY_CLIENT_HXX */

/*
 * Local variables:
 * mode: C++
 * c-file-style: "BSD"
 * c-basic-offset: 4
 * tab-width: 4
 * indent-tabs-mode: nil
 * End:
 */
