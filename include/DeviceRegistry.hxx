/*
 * DeviceRegistry.hxx
 *
 * Copyright (C) 2026, Charles Chiou
 */

#ifndef NETMON_DEVICEREGISTRY_HXX
#define NETMON_DEVICEREGISTRY_HXX

#include <string>
#include <vector>
#include <unordered_map>
#include <mutex>
#include <ctime>

struct DeviceInfo {
    std::string mac;
    std::string ip;
    std::string name;
    std::string vendor;
    std::string category; // "infrastructure", "known", "visitor", "unregistered"
    time_t      firstSeen = 0;
    time_t      lastSeen = 0;
};

class DeviceRegistry {
public:
    static DeviceRegistry &getInstance();

    bool load(const std::string &customPath = "");
    bool save();

    bool upsertDevice(const std::string &mac, const std::string &ip = "");
    bool nameDevice(const std::string &mac, const std::string &name,
                    const std::string &category = "");

    bool getDevice(const std::string &mac, DeviceInfo &outDevice) const;
    std::vector<DeviceInfo> getAllDevices() const;
    std::vector<DeviceInfo> getUnregisteredDevices() const;
    std::vector<DeviceInfo> getVisitorDevices() const;
    std::vector<DeviceInfo> getKnownDevices() const;

    size_t getDeviceCount() const;
    const std::string &getFilePath() const;

private:
    DeviceRegistry();
    ~DeviceRegistry() = default;
    DeviceRegistry(const DeviceRegistry &) = delete;
    DeviceRegistry &operator=(const DeviceRegistry &) = delete;

    std::string _filePath;
    mutable std::mutex _mutex;
    std::unordered_map<std::string, DeviceInfo> _devices;
    bool _dirty = false;
};

#endif /* NETMON_DEVICEREGISTRY_HXX */

/*
 * Local variables:
 * mode: C++
 * c-file-style: "BSD"
 * c-basic-offset: 4
 * tab-width: 4
 * indent-tabs-mode: nil
 * End:
 */
