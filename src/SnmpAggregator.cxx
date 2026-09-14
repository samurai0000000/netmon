/*
 * SnmpAggregator.cxx
 *
 * Copyright (C) 2026, Charles Chiou
 */

#include "SnmpAggregator.hxx"
#include "SnmpDatabase.hxx"
#include "Config.hxx"
#include "SecurityCheckpoint.hxx"

#include <net-snmp/net-snmp-config.h>
#include <net-snmp/net-snmp-includes.h>

#include <iostream>
#include <sstream>
#include <algorithm>
#include <chrono>
#include <cstring>

using json = nlohmann::json;

SnmpAggregator &SnmpAggregator::getInstance() {
    static SnmpAggregator instance;
    return instance;
}

SnmpAggregator::SnmpAggregator()
    : _running(false) {
    init_snmp("netmon");
}

SnmpAggregator::~SnmpAggregator() {
    stop();
    join();
}

bool SnmpAggregator::start() {
    if (_running.load()) {
        return false;
    }

    _running.store(true);
    _pollThread = std::thread(&SnmpAggregator::pollLoop, this);
    std::cout << "SnmpAggregator: Engine started successfully" << std::endl;
    return true;
}

void SnmpAggregator::stop() {
    _running.store(false);
}

void SnmpAggregator::join() {
    if (_pollThread.joinable()) {
        _pollThread.join();
    }
}

bool SnmpAggregator::isRunning() const {
    return _running.load();
}

void SnmpAggregator::pollLoop() {
    while (_running.load()) {
        auto targets = Config::getInstance().getSnmpTargets();
        int interval = Config::getInstance().getSnmpPollIntervalSec();
        if (interval < 5) interval = 5;

        for (const auto &target : targets) {
            if (!_running.load()) break;
            if (target.ip.empty()) continue;

            SnmpDeviceState dev;
            {
                std::lock_guard<std::mutex> lock(_mutex);
                auto it = _devices.find(target.ip);
                if (it != _devices.end()) {
                    dev = it->second;
                } else {
                    dev.targetIp = target.ip;
                    dev.wanInterfaceNames = target.wanInterfaces;
                }
            }

            pollTarget(dev, target.community, target.version, target.port);

            {
                std::lock_guard<std::mutex> lock(_mutex);
                _devices[target.ip] = dev;
            }
        }

        // Sleep for interval seconds in small slices for fast exit
        for (int i = 0; i < interval * 10 && _running.load(); ++i) {
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }
    }
}

void SnmpAggregator::pollTarget(SnmpDeviceState &device,
                               const std::string &community,
                               const std::string &version,
                               int port) {
    struct snmp_session session;
    snmp_sess_init(&session);

    std::string peer = device.targetIp + ":" + std::to_string(port > 0 ? port : 161);
    session.peername = const_cast<char *>(peer.c_str());
    session.version = (version == "1") ? SNMP_VERSION_1 : SNMP_VERSION_2c;
    session.community = reinterpret_cast<u_char *>(const_cast<char *>(community.c_str()));
    session.community_len = community.length();
    session.timeout = 800000; // 800ms
    session.retries = 1;

    void *ss = nullptr;
    {
        std::lock_guard<std::mutex> lock(_snmpMutex);
        ss = snmp_open(&session);
    }

    if (!ss) {
        device.reachable = false;
        return;
    }

    device.reachable = true;
    device.lastPollTime = static_cast<int64_t>(time(nullptr));

    querySystemInfo(ss, device);
    walkInterfaces(ss, device);

    {
        std::lock_guard<std::mutex> lock(_snmpMutex);
        snmp_close(static_cast<netsnmp_session *>(ss));
    }
}

bool SnmpAggregator::querySystemInfo(void *ss, SnmpDeviceState &device) {
    netsnmp_session *session = static_cast<netsnmp_session *>(ss);
    struct snmp_pdu *pdu = snmp_pdu_create(SNMP_MSG_GET);

    oid descrOid[] = {1, 3, 6, 1, 2, 1, 1, 1, 0};
    oid upTimeOid[] = {1, 3, 6, 1, 2, 1, 1, 3, 0};
    oid nameOid[] = {1, 3, 6, 1, 2, 1, 1, 5, 0};

    snmp_add_null_var(pdu, descrOid, sizeof(descrOid) / sizeof(oid));
    snmp_add_null_var(pdu, upTimeOid, sizeof(upTimeOid) / sizeof(oid));
    snmp_add_null_var(pdu, nameOid, sizeof(nameOid) / sizeof(oid));

    struct snmp_pdu *response = nullptr;
    int status = 0;
    {
        std::lock_guard<std::mutex> lock(_snmpMutex);
        status = snmp_synch_response(session, pdu, &response);
    }

    if (status != STAT_SUCCESS || !response || response->errstat != SNMP_ERR_NOERROR) {
        if (response) snmp_free_pdu(response);
        return false;
    }

    for (struct variable_list *vars = response->variables; vars; vars = vars->next_variable) {
        if (snmp_oid_compare(vars->name, vars->name_length, descrOid, sizeof(descrOid) / sizeof(oid)) == 0) {
            if (vars->val.string) {
                device.sysDescr = std::string(reinterpret_cast<char *>(vars->val.string), vars->val_len);
            }
        } else if (snmp_oid_compare(vars->name, vars->name_length, upTimeOid, sizeof(upTimeOid) / sizeof(oid)) == 0) {
            if (vars->val.integer) {
                int64_t currentUpTime = *vars->val.integer;
                if (device.sysUpTime > 0 && currentUpTime < device.sysUpTime) {
                    std::cout << "SnmpAggregator: Router reboot detected on "
                              << device.targetIp << "! Resetting rate baselines." << std::endl;
                    for (auto &kv : device.interfaces) {
                        kv.second.prevHcIn = 0;
                        kv.second.prevHcOut = 0;
                        kv.second.lastSampleTime = 0;
                    }
                }
                device.prevSysUpTime = device.sysUpTime;
                device.sysUpTime = currentUpTime;
            }
        } else if (snmp_oid_compare(vars->name, vars->name_length, nameOid, sizeof(nameOid) / sizeof(oid)) == 0) {
            if (vars->val.string) {
                device.sysName = std::string(reinterpret_cast<char *>(vars->val.string), vars->val_len);
            }
        }
    }

    snmp_free_pdu(response);
    return true;
}

namespace {
template <typename Callback>
void walkOidTable(netsnmp_session *session,
                  std::mutex &snmpMutex,
                  const oid *rootOid,
                  size_t rootLen,
                  Callback cb) {
    oid currentOid[MAX_OID_LEN];
    size_t currentLen = rootLen;
    memcpy(currentOid, rootOid, rootLen * sizeof(oid));

    bool running = true;
    while (running) {
        struct snmp_pdu *pdu = snmp_pdu_create(SNMP_MSG_GETNEXT);
        snmp_add_null_var(pdu, currentOid, currentLen);

        struct snmp_pdu *response = nullptr;
        int status = 0;
        {
            std::lock_guard<std::mutex> lock(snmpMutex);
            status = snmp_synch_response(session, pdu, &response);
        }

        if (status != STAT_SUCCESS || !response || response->errstat != SNMP_ERR_NOERROR) {
            if (response) snmp_free_pdu(response);
            break;
        }

        struct variable_list *vars = response->variables;
        if (!vars || snmp_oid_ncompare(rootOid, rootLen, vars->name, vars->name_length, rootLen) != 0) {
            snmp_free_pdu(response);
            break;
        }

        if (vars->type == SNMP_ENDOFMIBVIEW || vars->type == SNMP_NOSUCHOBJECT || vars->type == SNMP_NOSUCHINSTANCE) {
            snmp_free_pdu(response);
            break;
        }

        int ifIndex = 0;
        if (vars->name_length > rootLen) {
            ifIndex = static_cast<int>(vars->name[vars->name_length - 1]);
        }

        cb(ifIndex, vars);

        memcpy(currentOid, vars->name, vars->name_length * sizeof(oid));
        currentLen = vars->name_length;
        snmp_free_pdu(response);
    }
}
} // namespace

bool SnmpAggregator::walkInterfaces(void *ss, SnmpDeviceState &device) {
    netsnmp_session *session = static_cast<netsnmp_session *>(ss);
    int64_t now = static_cast<int64_t>(time(nullptr));

    std::map<int, InterfaceState> indexMap;

    // 1. Walk ifName (1.3.6.1.2.1.31.1.1.1.1)
    oid oidIfName[] = {1, 3, 6, 1, 2, 1, 31, 1, 1, 1, 1};
    walkOidTable(session, _snmpMutex, oidIfName, sizeof(oidIfName) / sizeof(oid),
                 [&](int idx, struct variable_list *v) {
                     if (v->val.string) {
                         indexMap[idx].ifIndex = idx;
                         indexMap[idx].ifName = std::string(reinterpret_cast<char *>(v->val.string), v->val_len);
                     }
                 });

    // 2. Walk ifDescr (1.3.6.1.2.1.2.2.1.2)
    oid oidIfDescr[] = {1, 3, 6, 1, 2, 1, 2, 2, 1, 2};
    walkOidTable(session, _snmpMutex, oidIfDescr, sizeof(oidIfDescr) / sizeof(oid),
                 [&](int idx, struct variable_list *v) {
                     if (v->val.string) {
                         indexMap[idx].ifIndex = idx;
                         indexMap[idx].ifDescr = std::string(reinterpret_cast<char *>(v->val.string), v->val_len);
                         if (indexMap[idx].ifName.empty()) {
                             indexMap[idx].ifName = indexMap[idx].ifDescr;
                         }
                     }
                 });

    // 3. Walk ifType (1.3.6.1.2.1.2.2.1.3)
    oid oidIfType[] = {1, 3, 6, 1, 2, 1, 2, 2, 1, 3};
    walkOidTable(session, _snmpMutex, oidIfType, sizeof(oidIfType) / sizeof(oid),
                 [&](int idx, struct variable_list *v) {
                     if (v->val.integer) indexMap[idx].ifType = static_cast<uint32_t>(*v->val.integer);
                 });

    // 4. Walk ifOperStatus (1.3.6.1.2.1.2.2.1.8)
    oid oidIfOper[] = {1, 3, 6, 1, 2, 1, 2, 2, 1, 8};
    walkOidTable(session, _snmpMutex, oidIfOper, sizeof(oidIfOper) / sizeof(oid),
                 [&](int idx, struct variable_list *v) {
                     if (v->val.integer) indexMap[idx].operStatus = static_cast<int>(*v->val.integer);
                 });

    // 5. Walk ifHighSpeed (1.3.6.1.2.1.31.1.1.1.15)
    oid oidIfHighSpeed[] = {1, 3, 6, 1, 2, 1, 31, 1, 1, 1, 15};
    walkOidTable(session, _snmpMutex, oidIfHighSpeed, sizeof(oidIfHighSpeed) / sizeof(oid),
                 [&](int idx, struct variable_list *v) {
                     if (v->val.integer) indexMap[idx].ifSpeed = static_cast<uint64_t>(*v->val.integer) * 1000000ULL;
                 });

    // 6. Walk ifAlias (1.3.6.1.2.1.31.1.1.1.18)
    oid oidIfAlias[] = {1, 3, 6, 1, 2, 1, 31, 1, 1, 1, 18};
    walkOidTable(session, _snmpMutex, oidIfAlias, sizeof(oidIfAlias) / sizeof(oid),
                 [&](int idx, struct variable_list *v) {
                     if (v->val.string) {
                         indexMap[idx].ifAlias = std::string(reinterpret_cast<char *>(v->val.string), v->val_len);
                     }
                 });

    // 7. Walk 64-bit HC In Octets (1.3.6.1.2.1.31.1.1.1.6)
    oid oidHCIn[] = {1, 3, 6, 1, 2, 1, 31, 1, 1, 1, 6};
    walkOidTable(session, _snmpMutex, oidHCIn, sizeof(oidHCIn) / sizeof(oid),
                 [&](int idx, struct variable_list *v) {
                     if (v->type == ASN_COUNTER64 && v->val.counter64) {
                         indexMap[idx].hcInOctets = (static_cast<uint64_t>(v->val.counter64->high) << 32) |
                                                    v->val.counter64->low;
                     } else if (v->val.integer) {
                         indexMap[idx].hcInOctets = static_cast<uint64_t>(*v->val.integer);
                     }
                 });

    // 8. Walk 64-bit HC Out Octets (1.3.6.1.2.1.31.1.1.1.10)
    oid oidHCOut[] = {1, 3, 6, 1, 2, 1, 31, 1, 1, 1, 10};
    walkOidTable(session, _snmpMutex, oidHCOut, sizeof(oidHCOut) / sizeof(oid),
                 [&](int idx, struct variable_list *v) {
                     if (v->type == ASN_COUNTER64 && v->val.counter64) {
                         indexMap[idx].hcOutOctets = (static_cast<uint64_t>(v->val.counter64->high) << 32) |
                                                     v->val.counter64->low;
                     } else if (v->val.integer) {
                         indexMap[idx].hcOutOctets = static_cast<uint64_t>(*v->val.integer);
                     }
                 });

    // 9. Walk Errors
    oid oidInErrors[] = {1, 3, 6, 1, 2, 1, 2, 2, 1, 14};
    walkOidTable(session, _snmpMutex, oidInErrors, sizeof(oidInErrors) / sizeof(oid),
                 [&](int idx, struct variable_list *v) {
                     if (v->val.integer) indexMap[idx].inErrors = static_cast<uint32_t>(*v->val.integer);
                 });
    oid oidOutErrors[] = {1, 3, 6, 1, 2, 1, 2, 2, 1, 20};
    walkOidTable(session, _snmpMutex, oidOutErrors, sizeof(oidOutErrors) / sizeof(oid),
                 [&](int idx, struct variable_list *v) {
                     if (v->val.integer) indexMap[idx].outErrors = static_cast<uint32_t>(*v->val.integer);
                 });

    // 10. Walk ipAdEntIfIndex (1.3.6.1.2.1.4.20.1.2) to discover IPv4 addresses
    oid oidIpAdEntIfIndex[] = {1, 3, 6, 1, 2, 1, 4, 20, 1, 2};
    walkOidTable(session, _snmpMutex, oidIpAdEntIfIndex, sizeof(oidIpAdEntIfIndex) / sizeof(oid),
                 [&](int, struct variable_list *v) {
                     if (v->val.integer && v->name_length >= 14) {
                         int ifIdx = static_cast<int>(*v->val.integer);
                         std::string ip = std::to_string(v->name[10]) + "." +
                                          std::to_string(v->name[11]) + "." +
                                          std::to_string(v->name[12]) + "." +
                                          std::to_string(v->name[13]);
                         if (indexMap.find(ifIdx) != indexMap.end()) {
                             indexMap[ifIdx].ipAddress = ip;
                         }
                     }
                 });

    // Update state and compute rates keyed by persistent ifName
    std::vector<SnmpSampleRecord> dbBatch;

    for (const auto &kv : indexMap) {
        const auto &scanned = kv.second;
        if (scanned.ifName.empty()) continue;

        auto &st = device.interfaces[scanned.ifName];
        st.ifIndex = scanned.ifIndex;
        st.ifName = scanned.ifName;
        st.ifDescr = scanned.ifDescr;
        st.ifAlias = scanned.ifAlias;
        st.ifType = scanned.ifType;
        if (scanned.ifSpeed > 0) st.ifSpeed = scanned.ifSpeed;
        st.operStatus = scanned.operStatus;
        st.hcInOctets = scanned.hcInOctets;
        st.hcOutOctets = scanned.hcOutOctets;
        st.inErrors = scanned.inErrors;
        st.outErrors = scanned.outErrors;

        // Dynamic IP tracking and change detection
        if (!st.ipAddress.empty() && !scanned.ipAddress.empty() && st.ipAddress != scanned.ipAddress) {
            std::cout << "SnmpAggregator: Interface " << scanned.ifName
                      << " IP address changed from " << st.ipAddress
                      << " to " << scanned.ipAddress << std::endl;
        }
        if (!scanned.ipAddress.empty()) {
            st.ipAddress = scanned.ipAddress;
        }

        if (st.lastSampleTime > 0 && now > st.lastSampleTime) {
            double dt = static_cast<double>(now - st.lastSampleTime);

            // Compute delta with 64-bit wrap math
            uint64_t dIn = (st.hcInOctets >= st.prevHcIn)
                               ? (st.hcInOctets - st.prevHcIn)
                               : ((UINT64_MAX - st.prevHcIn) + st.hcInOctets + 1);
            uint64_t dOut = (st.hcOutOctets >= st.prevHcOut)
                                ? (st.hcOutOctets - st.prevHcOut)
                                : ((UINT64_MAX - st.prevHcOut) + st.hcOutOctets + 1);

            // Sanity cap: reject ridiculous spikes > 100 Gbps
            if (dIn / dt < 12500000000.0) {
                st.rateInBps = static_cast<double>(dIn) / dt;
            }
            if (dOut / dt < 12500000000.0) {
                st.rateOutBps = static_cast<double>(dOut) / dt;
            }

            // Exponential 5-minute moving average
            double alpha = std::min(1.0, dt / 300.0);
            st.avg5MinInBps = (1.0 - alpha) * st.avg5MinInBps + alpha * st.rateInBps;
            st.avg5MinOutBps = (1.0 - alpha) * st.avg5MinOutBps + alpha * st.rateOutBps;

            // Prepare database record
            SnmpSampleRecord rec;
            rec.timestamp = now;
            rec.targetIp = device.targetIp;
            rec.ifName = st.ifName;
            rec.inBytesSec = st.rateInBps;
            rec.outBytesSec = st.rateOutBps;
            rec.inHcOctets = st.hcInOctets;
            rec.outHcOctets = st.hcOutOctets;
            rec.operStatus = st.operStatus;
            rec.inErrors = st.inErrors;
            rec.outErrors = st.outErrors;
            dbBatch.push_back(rec);
        }

        st.prevHcIn = st.hcInOctets;
        st.prevHcOut = st.hcOutOctets;
        st.lastSampleTime = now;
    }

    if (!dbBatch.empty()) {
        SnmpDatabase::getInstance().insertSamplesBatch(dbBatch);
    }

    return true;
}

json SnmpAggregator::getDeviceMetrics(const std::string &targetIp, const std::string &filter) {
    json res;
    std::string ip = targetIp;
    if (ip.empty()) {
        auto targets = Config::getInstance().getSnmpTargets();
        if (!targets.empty()) ip = targets[0].ip;
    }

    std::lock_guard<std::mutex> lock(_mutex);
    auto it = _devices.find(ip);
    if (it == _devices.end()) {
        res["status"] = "unreachable";
        res["error"] = "Target IP not found in SNMP device registry";
        res["target_ip"] = ip;
        return res;
    }

    const auto &dev = it->second;
    res["status"] = dev.reachable ? "ok" : "unreachable";
    res["target_ip"] = dev.targetIp;
    res["sys_name"] = dev.sysName;
    res["sys_descr"] = dev.sysDescr;
    res["uptime_ticks"] = dev.sysUpTime;
    res["uptime_days"] = static_cast<double>(dev.sysUpTime) / (100.0 * 86400.0);
    res["last_poll"] = dev.lastPollTime;

    json ifList = json::array();
    for (const auto &kv : dev.interfaces) {
        const auto &iface = kv.second;

        if (filter == "monitored") {
            if (iface.operStatus != 1) continue;
            if (iface.ifType == 24 || iface.ifName == "lo") continue;
            std::string n = iface.ifName;
            std::transform(n.begin(), n.end(), n.begin(), ::tolower);
            if (n.find("docker") != std::string::npos || n.find("veth") != std::string::npos ||
                n.find("virbr") != std::string::npos || n.find("sit") != std::string::npos ||
                n.find("gre") != std::string::npos || n.find("dummy") != std::string::npos) {
                continue;
            }
            if (iface.hcInOctets == 0 && iface.hcOutOctets == 0) continue;
        } else if (filter == "active") {
            if (iface.operStatus != 1) continue;
            if (iface.hcInOctets == 0 && iface.hcOutOctets == 0) continue;
        }

        json item;
        item["index"] = iface.ifIndex;
        item["name"] = iface.ifName;
        item["description"] = iface.ifDescr;
        item["alias"] = iface.ifAlias;
        item["speed_mbps"] = iface.ifSpeed / 1000000ULL;
        item["oper_status"] = iface.operStatus == 1 ? "up" : "down";
        item["in_octets"] = iface.hcInOctets;
        item["out_octets"] = iface.hcOutOctets;
        item["rate_in_mbps"] = (iface.rateInBps * 8.0) / 1000000.0;
        item["rate_out_mbps"] = (iface.rateOutBps * 8.0) / 1000000.0;
        item["avg5min_in_mbps"] = (iface.avg5MinInBps * 8.0) / 1000000.0;
        item["avg5min_out_mbps"] = (iface.avg5MinOutBps * 8.0) / 1000000.0;
        item["in_errors"] = iface.inErrors;
        item["out_errors"] = iface.outErrors;
        ifList.push_back(item);
    }
    res["interfaces"] = ifList;
    res["interface_count"] = ifList.size();
    return res;
}

json SnmpAggregator::getWanStatus(const std::string &targetIp) {
    json res;
    std::string ip = targetIp;
    if (ip.empty()) {
        auto targets = Config::getInstance().getSnmpTargets();
        if (!targets.empty()) ip = targets[0].ip;
    }

    std::lock_guard<std::mutex> lock(_mutex);
    auto it = _devices.find(ip);
    if (it == _devices.end()) {
        res["status"] = "unreachable";
        res["error"] = "Target IP not found in SNMP device registry";
        res["target_ip"] = ip;
        return res;
    }

    const auto &dev = it->second;
    res["status"] = dev.reachable ? "ok" : "unreachable";
    res["target_ip"] = dev.targetIp;
    res["sys_name"] = dev.sysName;

    std::vector<std::string> wanNames = dev.wanInterfaceNames;

    // Prioritize true PPPoE WAN uplinks (ppp11, ppp12) if present on the device
    bool hasPppWan = (dev.interfaces.find("ppp11") != dev.interfaces.end() ||
                      dev.interfaces.find("ppp12") != dev.interfaces.end());
    if (hasPppWan) {
        bool onlyEth = true;
        for (const auto &w : wanNames) {
            if (w.rfind("ppp", 0) == 0) {
                onlyEth = false;
                break;
            }
        }
        if (wanNames.empty() || onlyEth) {
            wanNames.clear();
            if (dev.interfaces.find("ppp11") != dev.interfaces.end()) wanNames.push_back("ppp11");
            if (dev.interfaces.find("ppp12") != dev.interfaces.end()) wanNames.push_back("ppp12");
        }
    }

    if (wanNames.empty()) {
        for (const auto &kv : dev.interfaces) {
            std::string n = kv.second.ifName + " " + kv.second.ifDescr + " " + kv.second.ifAlias;
            std::transform(n.begin(), n.end(), n.begin(), ::tolower);
            if (n.find("ppp") != std::string::npos || n.find("wan") != std::string::npos ||
                n.find("uplink") != std::string::npos || n.find("internet") != std::string::npos) {
                wanNames.push_back(kv.second.ifName);
            }
        }
    }
    if (wanNames.empty()) {
        // Fallback: take first two non-loopback active physical ports
        for (const auto &kv : dev.interfaces) {
            if (kv.second.operStatus == 1 && kv.second.ifType != 24 && kv.second.ifName != "lo") {
                wanNames.push_back(kv.second.ifName);
                if (wanNames.size() >= 2) break;
            }
        }
    }

    json wanArray = json::array();
    for (const auto &wName : wanNames) {
        auto ifIt = dev.interfaces.find(wName);
        if (ifIt == dev.interfaces.end()) continue;

        const auto &st = ifIt->second;
        json w;
        w["interface"] = st.ifName;
        std::string aliasStr = st.ifAlias.empty() ? st.ifDescr : st.ifAlias;
        if (aliasStr.empty() || aliasStr == st.ifName) {
            if (st.ifName == "ppp11") aliasStr = "WAN 1 (PPPoE)";
            else if (st.ifName == "ppp12") aliasStr = "WAN 2 (PPPoE)";
        }
        w["alias"] = aliasStr;
        w["status"] = st.operStatus == 1 ? "up" : "down";
        w["speed_mbps"] = st.ifSpeed / 1000000ULL;
        w["rate_in_mbps"] = (st.rateInBps * 8.0) / 1000000.0;
        w["rate_out_mbps"] = (st.rateOutBps * 8.0) / 1000000.0;
        w["avg5min_in_mbps"] = (st.avg5MinInBps * 8.0) / 1000000.0;
        w["avg5min_out_mbps"] = (st.avg5MinOutBps * 8.0) / 1000000.0;

        // Outward IP resolution with fallback mapping
        std::string outwardIp = st.ipAddress;
        if (outwardIp.empty()) {
            if (st.ifName == "eth1" && dev.interfaces.find("ppp11") != dev.interfaces.end()) {
                outwardIp = dev.interfaces.at("ppp11").ipAddress;
            } else if (st.ifName == "eth2" && dev.interfaces.find("ppp12") != dev.interfaces.end()) {
                outwardIp = dev.interfaces.at("ppp12").ipAddress;
            }
        }
        w["ip_address"] = outwardIp;
        w["dynamic_ip"] = true;

        // Query 24-hour historical statistics from SQLite
        json histStats = SnmpDatabase::getInstance().queryWanStats(dev.targetIp, st.ifName, 24);
        w["peak_24h_in_mbps"] = histStats.value("peak_in_mbps", 0.0);
        w["peak_24h_in_timestamp"] = histStats.value("peak_in_timestamp", 0);
        w["peak_24h_out_mbps"] = histStats.value("peak_out_mbps", 0.0);
        w["peak_24h_out_timestamp"] = histStats.value("peak_out_timestamp", 0);
        w["daily_total_gb_in"] = histStats.value("total_gb_in", 0.0);
        w["daily_total_gb_out"] = histStats.value("total_gb_out", 0.0);

        wanArray.push_back(w);
    }

    res["wan_interfaces"] = wanArray;
    return res;
}

json SnmpAggregator::getInterfaceCounters(const std::string &targetIp, const std::string &ifName) {
    json res;
    std::string ip = targetIp;
    if (ip.empty()) {
        auto targets = Config::getInstance().getSnmpTargets();
        if (!targets.empty()) ip = targets[0].ip;
    }

    std::lock_guard<std::mutex> lock(_mutex);
    auto it = _devices.find(ip);
    if (it == _devices.end()) {
        res["status"] = "not_found";
        res["error"] = "Target IP not registered";
        return res;
    }

    auto ifIt = it->second.interfaces.find(ifName);
    if (ifIt == it->second.interfaces.end()) {
        res["status"] = "not_found";
        res["error"] = "Interface not found on target device";
        return res;
    }

    const auto &st = ifIt->second;
    res["status"] = "ok";
    res["target_ip"] = ip;
    res["interface"] = st.ifName;
    res["description"] = st.ifDescr;
    res["alias"] = st.ifAlias;
    res["oper_status"] = st.operStatus == 1 ? "up" : "down";
    res["speed_mbps"] = st.ifSpeed / 1000000ULL;
    res["in_octets"] = st.hcInOctets;
    res["out_octets"] = st.hcOutOctets;
    res["rate_in_mbps"] = (st.rateInBps * 8.0) / 1000000.0;
    res["rate_out_mbps"] = (st.rateOutBps * 8.0) / 1000000.0;
    res["in_errors"] = st.inErrors;
    res["out_errors"] = st.outErrors;
    return res;
}

json SnmpAggregator::queryOid(const std::string &targetIp,
                             const std::string &oidStr,
                             const std::string &community) {
    json res;
    std::string ip = targetIp;
    if (ip.empty()) {
        auto targets = Config::getInstance().getSnmpTargets();
        if (!targets.empty()) ip = targets[0].ip;
    }

    res["target_ip"] = ip;
    res["oid"] = oidStr;

    std::string comm = community;
    if (comm.empty()) comm = "public";

    oid targetOid[MAX_OID_LEN];
    size_t targetLen = MAX_OID_LEN;
    if (!read_objid(oidStr.c_str(), targetOid, &targetLen)) {
        res["status"] = "error";
        res["error"] = "Invalid OID format: " + oidStr;
        return res;
    }

    struct snmp_session session;
    snmp_sess_init(&session);
    session.peername = const_cast<char *>(ip.c_str());
    session.version = SNMP_VERSION_2c;
    session.community = reinterpret_cast<u_char *>(const_cast<char *>(comm.c_str()));
    session.community_len = comm.length();
    session.timeout = 1000000; // 1s
    session.retries = 1;

    void *ss = nullptr;
    {
        std::lock_guard<std::mutex> lock(_snmpMutex);
        ss = snmp_open(&session);
    }
    if (!ss) {
        res["status"] = "unreachable";
        res["error"] = "Failed to establish SNMP session with target";
        return res;
    }

    struct snmp_pdu *pdu = snmp_pdu_create(SNMP_MSG_GET);
    snmp_add_null_var(pdu, targetOid, targetLen);

    struct snmp_pdu *response = nullptr;
    int status = 0;
    {
        std::lock_guard<std::mutex> lock(_snmpMutex);
        status = snmp_synch_response(static_cast<netsnmp_session *>(ss), pdu, &response);
    }

    if (status != STAT_SUCCESS || !response || response->errstat != SNMP_ERR_NOERROR) {
        res["status"] = "error";
        res["error"] = "SNMP GET failed or returned error";
        if (response) snmp_free_pdu(response);
        std::lock_guard<std::mutex> lock(_snmpMutex);
        snmp_close(static_cast<netsnmp_session *>(ss));
        return res;
    }

    struct variable_list *vars = response->variables;
    if (vars) {
        res["status"] = "ok";
        res["type"] = static_cast<int>(vars->type);
        if (vars->val.integer) {
            res["value_int"] = *vars->val.integer;
        }
        if (vars->val.string) {
            res["value_str"] = std::string(reinterpret_cast<char *>(vars->val.string), vars->val_len);
        }
    } else {
        res["status"] = "empty";
    }

    snmp_free_pdu(response);
    {
        std::lock_guard<std::mutex> lock(_snmpMutex);
        snmp_close(static_cast<netsnmp_session *>(ss));
    }
    return res;
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
