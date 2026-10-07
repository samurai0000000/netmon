/*
 * ZyxelTypes.hxx
 *
 * Copyright (C) 2026, Charles Chiou
 */

#ifndef NETMON_ZYXEL_TYPES_HXX
#define NETMON_ZYXEL_TYPES_HXX

#include <string>
#include <vector>
#include <nlohmann/json.hpp>

struct ZyxelVersionInfo {
    std::string model;
    std::string firmwareVersion;
    std::string buildDate;
    std::string serialNumber;

    nlohmann::json toJson() const {
        return {
            {"model", model},
            {"firmware_version", firmwareVersion},
            {"build_date", buildDate},
            {"serial_number", serialNumber}
        };
    }
};

struct ZyxelSystemLoad {
    double      cpuUsagePercent = 0.0;
    double      memUsagePercent = 0.0;
    std::string uptime;

    nlohmann::json toJson() const {
        return {
            {"cpu_usage_percent", cpuUsagePercent},
            {"mem_usage_percent", memUsagePercent},
            {"uptime", uptime}
        };
    }
};

struct ZyxelSessionSummary {
    int    activeSessions = 0;
    int    maxSessions = 0;
    double sessionUsagePercent = 0.0;

    nlohmann::json toJson() const {
        return {
            {"active_sessions", activeSessions},
            {"max_sessions", maxSessions},
            {"session_usage_percent", sessionUsagePercent}
        };
    }
};

struct ZyxelInterfaceInfo {
    std::string name;
    bool        linkStatus = false;
    std::string ip;
    std::string netmask;
    std::string mac;
    int         mtu = 1500;

    nlohmann::json toJson() const {
        return {
            {"name", name},
            {"link_status", linkStatus ? "up" : "down"},
            {"ip", ip},
            {"netmask", netmask},
            {"mac", mac},
            {"mtu", mtu}
        };
    }
};

struct ZyxelRouteEntry {
    std::string destination;
    std::string netmask;
    std::string gateway;
    std::string interface;
    int         metric = 0;

    nlohmann::json toJson() const {
        return {
            {"destination", destination},
            {"netmask", netmask},
            {"gateway", gateway},
            {"interface", interface},
            {"metric", metric}
        };
    }
};

struct ZyxelZoneInfo {
    std::string              name;
    std::vector<std::string> interfaces;

    nlohmann::json toJson() const {
        return {
            {"name", name},
            {"interfaces", interfaces}
        };
    }
};

struct ZyxelArpEntry {
    std::string ip;
    std::string mac;
    std::string interface;

    nlohmann::json toJson() const {
        return {
            {"ip", ip},
            {"mac", mac},
            {"interface", interface}
        };
    }
};

struct ZyxelAddressObject {
    std::string name;
    std::string type;               // "HOST", "RANGE", "SUBNET"
    std::string ip;
    std::string secondaryIpOrMask;  // For range end or subnet mask
    int         refCount = 0;

    nlohmann::json toJson() const {
        return {
            {"name", name},
            {"type", type},
            {"ip", ip},
            {"secondary_ip_or_mask", secondaryIpOrMask},
            {"ref_count", refCount}
        };
    }
};

struct ZyxelAddressGroup {
    std::string              name;
    std::vector<std::string> members;

    nlohmann::json toJson() const {
        return {
            {"name", name},
            {"members", members}
        };
    }
};

struct ZyxelServiceObject {
    std::string name;
    std::string protocol;           // "tcp", "udp", "icmp"
    int         portStart = 0;
    int         portEnd = 0;
    int         refCount = 0;

    nlohmann::json toJson() const {
        return {
            {"name", name},
            {"protocol", protocol},
            {"port_start", portStart},
            {"port_end", portEnd},
            {"ref_count", refCount}
        };
    }
};

struct ZyxelServiceGroup {
    std::string              name;
    std::vector<std::string> members;

    nlohmann::json toJson() const {
        return {
            {"name", name},
            {"members", members}
        };
    }
};

struct ZyxelFirewallRule {
    int         index = 0;
    std::string name;
    std::string fromZone;
    std::string toZone;
    std::string sourceIp;
    std::string destinationIp;
    std::string service;
    std::string action;             // "allow", "deny", "reject"
    bool        active = true;
    std::string description;

    nlohmann::json toJson() const {
        return {
            {"index", index},
            {"name", name},
            {"from_zone", fromZone},
            {"to_zone", toZone},
            {"source_ip", sourceIp},
            {"destination_ip", destinationIp},
            {"service", service},
            {"action", action},
            {"active", active},
            {"description", description}
        };
    }
};

struct ZyxelVirtualServerRule {
    int         index = 0;
    std::string name;
    std::string interface;
    std::string originalIp;
    std::string mapToIp;
    std::string originalService;
    std::string mappedService;
    bool        active = true;

    nlohmann::json toJson() const {
        return {
            {"index", index},
            {"name", name},
            {"interface", interface},
            {"original_ip", originalIp},
            {"map_to_ip", mapToIp},
            {"original_service", originalService},
            {"mapped_service", mappedService},
            {"active", active}
        };
    }
};

struct ZyxelHopProbe {
    std::string ip;
    double      rttMs = 0.0;
    bool        timeout = false;
    std::string icmpFlag;           // e.g. "!H", "!N", "!P", "!X"

    nlohmann::json toJson() const {
        nlohmann::json j = {
            {"ip", ip},
            {"rtt_ms", rttMs},
            {"timeout", timeout}
        };
        if (!icmpFlag.empty()) {
            j["icmp_flag"] = icmpFlag;
        }
        return j;
    }
};

struct ZyxelHop {
    int                        hopIndex = 0;
    std::vector<ZyxelHopProbe> probes;
    std::string                primaryIp;
    bool                       isCompleteTimeout = false;

    nlohmann::json toJson() const {
        nlohmann::json probeArr = nlohmann::json::array();
        for (const auto &p : probes) {
            probeArr.push_back(p.toJson());
        }
        return {
            {"hop_index", hopIndex},
            {"primary_ip", primaryIp},
            {"complete_timeout", isCompleteTimeout},
            {"probes", probeArr}
        };
    }
};

struct ZyxelDiagnosticResult {
    std::string           type;     // "ping", "traceroute"
    int                   packetsTransmitted = 0;
    int                   packetsReceived = 0;
    double                packetLossPercent = 0.0;
    double                minLatencyMs = 0.0;
    double                avgLatencyMs = 0.0;
    double                maxLatencyMs = 0.0;
    std::vector<ZyxelHop> hops;
    std::string           rawOutput;

    nlohmann::json toJson() const {
        nlohmann::json j = {
            {"type", type},
            {"packets_transmitted", packetsTransmitted},
            {"packets_received", packetsReceived},
            {"packet_loss_percent", packetLossPercent},
            {"min_latency_ms", minLatencyMs},
            {"avg_latency_ms", avgLatencyMs},
            {"max_latency_ms", maxLatencyMs},
            {"raw_output", rawOutput}
        };
        if (type == "traceroute" && !hops.empty()) {
            nlohmann::json hopsArr = nlohmann::json::array();
            for (const auto &h : hops) {
                hopsArr.push_back(h.toJson());
            }
            j["hops"] = hopsArr;
        }
        return j;
    }
};

struct ZyxelAppPatrolSummary {
    uint64_t forwardedKb = 0;
    uint64_t droppedKb = 0;
    uint64_t rejectedKb = 0;
    uint64_t matchedConnections = 0;

    nlohmann::json toJson() const {
        return {
            {"forwarded_kb", forwardedKb},
            {"dropped_kb", droppedKb},
            {"rejected_kb", rejectedKb},
            {"matched_connections", matchedConnections}
        };
    }
};

struct ZyxelIdpSummary {
    bool     enabled = false;
    uint64_t threatsDetected = 0;
    uint64_t packetsDropped = 0;
    uint64_t connectionsReset = 0;

    nlohmann::json toJson() const {
        return {
            {"enabled", enabled},
            {"threats_detected", threatsDetected},
            {"packets_dropped", packetsDropped},
            {"connections_reset", connectionsReset}
        };
    }
};

struct ZyxelSecurityTelemetry {
    time_t                timestamp = 0;
    ZyxelSessionSummary   sessionSummary;
    ZyxelAppPatrolSummary appPatrolSummary;
    ZyxelIdpSummary       idpSummary;
    bool                  hasSessionSummary = false;
    bool                  hasAppPatrol = false;
    bool                  hasIdp = false;

    nlohmann::json toJson() const {
        return {
            {"timestamp", timestamp},
            {"sessions", sessionSummary.toJson()},
            {"app_patrol", appPatrolSummary.toJson()},
            {"idp", idpSummary.toJson()},
            {"has_session_summary", hasSessionSummary},
            {"has_app_patrol", hasAppPatrol},
            {"has_idp", hasIdp}
        };
    }
};

struct ZyxelDiskEntry {
    int         index = 0;
    std::string name;
    int         sizeMb = 0;
    std::string usage;

    nlohmann::json toJson() const {
        return {
            {"index", index},
            {"name", name},
            {"size_mb", sizeMb},
            {"usage", usage}
        };
    }
};

struct ZyxelExtensionSlotEntry {
    int         slot = 0;
    std::string device;
    std::string status;

    nlohmann::json toJson() const {
        return {
            {"slot", slot},
            {"device", device},
            {"status", status}
        };
    }
};

struct ZyxelSocketEntry {
    int         index = 0;
    std::string proto;
    std::string localAddress;
    std::string foreignAddress;
    std::string state;

    nlohmann::json toJson() const {
        return {
            {"index", index},
            {"proto", proto},
            {"local_address", localAddress},
            {"foreign_address", foreignAddress},
            {"state", state}
        };
    }
};

struct ZyxelDhcpBindingEntry {
    int         index = 0;
    std::string interfaceName;
    std::string ip;
    std::string mac;
    std::string reserved;
    std::string hostName;
    std::string expirationTime;
    std::string description;

    nlohmann::json toJson() const {
        return {
            {"index", index},
            {"interface", interfaceName},
            {"ip", ip},
            {"mac", mac},
            {"reserved", reserved},
            {"host_name", hostName},
            {"expiration_time", expirationTime},
            {"description", description}
        };
    }
};

struct ZyxelArpGratuitousInfo {
    bool active = false;
    int  intervalSeconds = 0;

    nlohmann::json toJson() const {
        return {
            {"active", active},
            {"interval_seconds", intervalSeconds}
        };
    }
};

struct ZyxelZoneBindingEntry {
    int         index = 0;
    std::string interfaceName;
    std::string zoneName;

    nlohmann::json toJson() const {
        return {
            {"index", index},
            {"interface", interfaceName},
            {"zone", zoneName}
        };
    }
};

struct ZyxelRouteSettingsEntry {
    std::string route;
    std::string netmask;
    std::string nexthop;
    int         metric = 0;

    nlohmann::json toJson() const {
        return {
            {"route", route},
            {"netmask", netmask},
            {"nexthop", nexthop},
            {"metric", metric}
        };
    }
};

struct ZyxelOspfGlobalInfo {
    std::string routerId;
    bool        redistributeRip = false;
    std::string redistributeRipType;
    std::string redistributeRipMetric;
    bool        redistributeStatic = false;
    std::string redistributeStaticType;
    std::string redistributeStaticMetric;

    nlohmann::json toJson() const {
        return {
            {"router_id", routerId},
            {"redistribute_rip", redistributeRip},
            {"redistribute_rip_type", redistributeRipType},
            {"redistribute_rip_metric", redistributeRipMetric},
            {"redistribute_static", redistributeStatic},
            {"redistribute_static_type", redistributeStaticType},
            {"redistribute_static_metric", redistributeStaticMetric}
        };
    }
};

struct ZyxelBgpGlobalInfo {
    std::string routerId;
    int         asNumber = 0;
    bool        redistributeConnected = false;
    std::string maximumPaths;
    std::string bgpNetwork;

    nlohmann::json toJson() const {
        return {
            {"router_id", routerId},
            {"as_number", asNumber},
            {"redistribute_connected", redistributeConnected},
            {"maximum_paths", maximumPaths},
            {"bgp_network", bgpNetwork}
        };
    }
};

struct ZyxelRipGlobalInfo {
    std::string authType;
    std::string textString;
    std::string md5Key;
    std::string md5String;
    bool        redistributeOspf = false;
    int         redistributeOspfMetric = 1;
    bool        redistributeStatic = false;
    int         redistributeStaticMetric = 1;

    nlohmann::json toJson() const {
        return {
            {"auth_type", authType},
            {"text_string", textString},
            {"md5_key", md5Key},
            {"md5_string", md5String},
            {"redistribute_ospf", redistributeOspf},
            {"redistribute_ospf_metric", redistributeOspfMetric},
            {"redistribute_static", redistributeStatic},
            {"redistribute_static_metric", redistributeStaticMetric}
        };
    }
};

struct ZyxelScheduleObject {
    std::string name;
    std::string type;       // "Once", "Recurring"
    std::string startEnd;
    int         refCount = 0;

    nlohmann::json toJson() const {
        return {
            {"name", name},
            {"type", type},
            {"start_end", startEnd},
            {"ref_count", refCount}
        };
    }
};

struct ZyxelAddress6Object {
    std::string name;
    std::string type;
    std::string address;
    int         refCount = 0;

    nlohmann::json toJson() const {
        return {
            {"name", name},
            {"type", type},
            {"address", address},
            {"ref_count", refCount}
        };
    }
};

struct ZyxelAccountPppoeEntry {
    std::string profileName;
    std::string protocol;
    std::string username;
    std::string authType;
    std::string serviceName;
    bool        compression = false;
    int         idleTimeout = 0;

    nlohmann::json toJson() const {
        return {
            {"profile_name", profileName},
            {"protocol", protocol},
            {"username", username},
            {"auth_type", authType},
            {"service_name", serviceName},
            {"compression", compression},
            {"idle_timeout", idleTimeout}
        };
    }
};

struct ZyxelDdnsStatusEntry {
    int         index = 0;
    std::string profileName;
    std::string domainName;
    std::string effectiveIp;
    std::string status;
    std::string updateTime;

    nlohmann::json toJson() const {
        return {
            {"index", index},
            {"profile_name", profileName},
            {"domain_name", domainName},
            {"effective_ip", effectiveIp},
            {"status", status},
            {"update_time", updateTime}
        };
    }
};

struct ZyxelAlgStatus {
    std::string protocol;
    bool        active = false;
    bool        transformation = false;
    int         signalingPort = 0;

    nlohmann::json toJson() const {
        return {
            {"protocol", protocol},
            {"active", active},
            {"transformation", transformation},
            {"signaling_port", signalingPort}
        };
    }
};

struct ZyxelSecurePolicyStatus {
    bool        active = false;
    bool        asymmetricalRoute = false;
    std::string defaultRule;
    bool        tcpFlagDetect = false;

    nlohmann::json toJson() const {
        return {
            {"active", active},
            {"asymmetrical_route", asymmetricalRoute},
            {"default_rule", defaultRule},
            {"tcp_flag_detect", tcpFlagDetect}
        };
    }
};

struct ZyxelBwmRule {
    std::string index;
    bool        active = false;
    std::string description;
    std::string bwmType;
    std::string src;
    std::string dst;
    uint32_t    inboundKbps = 0;
    uint32_t    outboundKbps = 0;

    nlohmann::json toJson() const {
        return {
            {"index", index},
            {"active", active},
            {"description", description},
            {"bwm_type", bwmType},
            {"src", src},
            {"dst", dst},
            {"inbound_kbps", inboundKbps},
            {"outbound_kbps", outboundKbps}
        };
    }
};

struct ZyxelBwmStatus {
    bool active = false;
    bool tcpAckActive = false;

    nlohmann::json toJson() const {
        return {
            {"active", active},
            {"tcp_ack_active", tcpAckActive}
        };
    }
};

struct ZyxelCdrStatus {
    bool        active = false;
    std::string blockedBy;
    int         blockPeriod = 0;
    std::string emailAlert;

    nlohmann::json toJson() const {
        return {
            {"active", active},
            {"blocked_by", blockedBy},
            {"block_period", blockPeriod},
            {"email_alert", emailAlert}
        };
    }
};

struct ZyxelCdrRule {
    int         index = 0;
    std::string category;
    int         occurrence = 0;
    int         duration = 0;
    std::string containment;
    std::string eventType;

    nlohmann::json toJson() const {
        return {
            {"index", index},
            {"category", category},
            {"occurrence", occurrence},
            {"duration", duration},
            {"containment", containment},
            {"event_type", eventType}
        };
    }
};

struct ZyxelSslInspectionStatus {
    std::string certMode;
    bool        tls13Active = false;
    int         timeoutSec = 0;
    std::string timeoutAction;

    nlohmann::json toJson() const {
        return {
            {"cert_mode", certMode},
            {"tls13_active", tls13Active},
            {"timeout_sec", timeoutSec},
            {"timeout_action", timeoutAction}
        };
    }
};

struct ZyxelContentFilterSettings {
    bool        defaultBlock = false;
    std::string licenseKey;
    int         serviceTimeout = 0;
    std::string blockMessage;

    nlohmann::json toJson() const {
        return {
            {"default_block", defaultBlock},
            {"license_key", licenseKey},
            {"service_timeout", serviceTimeout},
            {"block_message", blockMessage}
        };
    }
};

struct ZyxelVpnCounters {
    uint64_t inbpktCount = 0;
    uint64_t outbpktCount = 0;
    uint32_t inbpktThroughput = 0;
    uint32_t outbpktThroughput = 0;

    nlohmann::json toJson() const {
        return {
            {"inbpkt_count", inbpktCount},
            {"outbpkt_count", outbpktCount},
            {"inbpkt_throughput", inbpktThroughput},
            {"outbpkt_throughput", outbpktThroughput}
        };
    }
};

struct ZyxelL2tpStatus {
    bool        activate = false;
    std::string crypto;
    std::string addressPool;
    std::string authentication;
    std::string certificate;
    std::string user;
    int         keepaliveTimer = 60;
    std::string firstDnsServer;

    nlohmann::json toJson() const {
        return {
            {"activate", activate},
            {"crypto", crypto},
            {"address_pool", addressPool},
            {"authentication", authentication},
            {"certificate", certificate},
            {"user", user},
            {"keepalive_timer", keepaliveTimer},
            {"first_dns_server", firstDnsServer}
        };
    }
};

struct ZyxelVcpUser {
    int         index = 0;
    std::string username;
    std::string userType;
    std::string description;

    nlohmann::json toJson() const {
        return {
            {"index", index},
            {"username", username},
            {"user_type", userType},
            {"description", description}
        };
    }
};

struct ZyxelIsakmpSa {
    int         no = 0;
    std::string name;
    std::string uptime;
    std::string timeout;
    std::string cookie;
    std::string algorithm;
    std::string localAddress;
    std::string remoteAddress;
    std::string ikeId;
    std::string status;

    nlohmann::json toJson() const {
        return {
            {"no", no},
            {"name", name},
            {"uptime", uptime},
            {"timeout", timeout},
            {"cookie", cookie},
            {"algorithm", algorithm},
            {"local_address", localAddress},
            {"remote_address", remoteAddress},
            {"ike_id", ikeId},
            {"status", status}
        };
    }
};

struct ZyxelCnmStatus {
    bool        active = false;
    std::string status;
    bool        isConnected = false;

    nlohmann::json toJson() const {
        return {
            {"active", active},
            {"status", status},
            {"is_connected", isConnected}
        };
    }
};

struct ZyxelSecuReporterStatus {
    bool active = false;
    bool sendReporter = false;
    int  uploadInterval = 600;
    int  uploadFilesize = 10;
    bool banner = false;

    nlohmann::json toJson() const {
        return {
            {"active", active},
            {"send_reporter", sendReporter},
            {"upload_interval", uploadInterval},
            {"upload_filesize", uploadFilesize},
            {"banner", banner}
        };
    }
};

struct ZyxelWebAuthStatus {
    bool        policyActive = false;
    std::string defaultAuth;
    std::string logoutIp;
    bool        sessionPage = false;

    nlohmann::json toJson() const {
        return {
            {"policy_active", policyActive},
            {"default_auth", defaultAuth},
            {"logout_ip", logoutIp},
            {"session_page", sessionPage}
        };
    }
};

struct ZyxelUserEntry {
    int         no = 0;
    std::string username;
    std::string userType;
    std::string description;

    nlohmann::json toJson() const {
        return {
            {"no", no},
            {"username", username},
            {"user_type", userType},
            {"description", description}
        };
    }
};

struct ZyxelUserRetrySettings {
    bool retryLimitActive = false;
    int  maxRetryCount = 5;
    int  lockoutPeriod = 30;

    nlohmann::json toJson() const {
        return {
            {"retry_limit_active", retryLimitActive},
            {"max_retry_count", maxRetryCount},
            {"lockout_period", lockoutPeriod}
        };
    }
};

struct ZyxelCaSpaceUsage {
    uint64_t total = 0;
    uint64_t available = 0;
    uint64_t inUse = 0;
    uint64_t myCert = 0;

    nlohmann::json toJson() const {
        return {
            {"total", total},
            {"available", available},
            {"in_use", inUse},
            {"my_cert", myCert}
        };
    }
};

struct ZyxelCaCertInfo {
    std::string certificate;
    std::string type;
    std::string subject;
    std::string issuer;
    std::string status;

    nlohmann::json toJson() const {
        return {
            {"certificate", certificate},
            {"type", type},
            {"subject", subject},
            {"issuer", issuer},
            {"status", status}
        };
    }
};

struct ZyxelCapwapApInfo {
    int  onlineMgntAp = 0;
    int  offlineMgntAp = 0;
    int  unMgntAp = 0;
    int  stationCount = 0;
    int  remoteApCount = 0;
    bool updateAvailable = false;

    nlohmann::json toJson() const {
        return {
            {"online_mgnt_ap", onlineMgntAp},
            {"offline_mgnt_ap", offlineMgntAp},
            {"un_mgnt_ap", unMgntAp},
            {"station_count", stationCount},
            {"remote_ap_count", remoteApCount},
            {"update_available", updateAvailable}
        };
    }
};

struct ZyxelCapwapFallback {
    bool        fallbackEnabled = false;
    int         fallbackInterval = 30;
    std::string idleTimeout = "default";

    nlohmann::json toJson() const {
        return {
            {"fallback_enabled", fallbackEnabled},
            {"fallback_interval", fallbackInterval},
            {"idle_timeout", idleTimeout}
        };
    }
};

struct ZyxelRogueApInfo {
    int rogueApCount = 0;
    int friendlyApCount = 0;
    int suspectedRogueApCount = 0;
    int adhocCount = 0;
    int unclassifiedApCount = 0;
    int totalDevices = 0;

    nlohmann::json toJson() const {
        return {
            {"rogue_ap_count", rogueApCount},
            {"friendly_ap_count", friendlyApCount},
            {"suspected_rogue_ap_count", suspectedRogueApCount},
            {"adhoc_count", adhocCount},
            {"unclassified_ap_count", unclassifiedApCount},
            {"total_devices", totalDevices}
        };
    }
};

struct ZyxelRogueApStatus {
    bool detectionStatus = false;
    int  detectionInterval = 30;
    bool weakSecurity = true;
    bool unmanagedAp = true;
    bool hiddenSsid = true;
    bool ssidKeyword = true;

    nlohmann::json toJson() const {
        return {
            {"detection_status", detectionStatus},
            {"detection_interval", detectionInterval},
            {"weak_security", weakSecurity},
            {"unmanaged_ap", unmanagedAp},
            {"hidden_ssid", hiddenSsid},
            {"ssid_keyword", ssidKeyword}
        };
    }
};

struct ZyxelAutoHealingConfig {
    bool activated = false;
    int  interval = 10;
    int  powerThresholdDbm = -70;
    int  healingThresholdDbm = -85;
    int  margin = 2;

    nlohmann::json toJson() const {
        return {
            {"activated", activated},
            {"interval", interval},
            {"power_threshold_dbm", powerThresholdDbm},
            {"healing_threshold_dbm", healingThresholdDbm},
            {"margin", margin}
        };
    }
};

struct ZyxelFrameCaptureConfig {
    std::string captureSource = "none";
    std::string filePrefix = "monitor";
    int         fileSize = 1000;
    bool        active = false;

    nlohmann::json toJson() const {
        return {
            {"capture_source", captureSource},
            {"file_prefix", filePrefix},
            {"file_size", fileSize},
            {"active", active}
        };
    }
};

struct ZyxelZyMeshInfo {
    int         onlineRootAp = 0;
    int         onlineRepeaterAp = 0;
    int         offlineRootAp = 0;
    int         offlineRepeaterAp = 0;
    std::string provisionGroupMac;

    nlohmann::json toJson() const {
        return {
            {"online_root_ap", onlineRootAp},
            {"online_repeater_ap", onlineRepeaterAp},
            {"offline_root_ap", offlineRootAp},
            {"offline_repeater_ap", offlineRepeaterAp},
            {"provision_group_mac", provisionGroupMac}
        };
    }
};

#endif /* NETMON_ZYXEL_TYPES_HXX */

/*
 * Local variables:
 * mode: C++
 * c-file-style: "BSD"
 * c-basic-offset: 4
 * tab-width: 4
 * indent-tabs-mode: nil
 * End:
 */
