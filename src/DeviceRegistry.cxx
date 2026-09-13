/*
 * DeviceRegistry.cxx
 *
 * Copyright (C) 2026, Charles Chiou
 */

#include "DeviceRegistry.hxx"
#include "Config.hxx"
#include "OuiDatabase.hxx"
#include <iostream>
#include <algorithm>
#include <libconfig.h++>
#include <sys/stat.h>

DeviceRegistry &DeviceRegistry::getInstance() {
    static DeviceRegistry instance;
    return instance;
}

DeviceRegistry::DeviceRegistry()
    : _filePath("")
    , _dirty(false) {
}

const std::string &DeviceRegistry::getFilePath() const {
    return _filePath;
}

size_t DeviceRegistry::getDeviceCount() const {
    std::lock_guard<std::mutex> lock(_mutex);
    return _devices.size();
}

bool DeviceRegistry::load(const std::string &customPath) {
    std::lock_guard<std::mutex> lock(_mutex);

    if (!customPath.empty()) {
        _filePath = Config::resolveHomePath(customPath);
    } else {
        _filePath = Config::resolveHomePath(Config::getInstance().getDevicesFile());
    }

    _devices.clear();

    libconfig::Config cfg;
    try {
        cfg.readFile(_filePath.c_str());
    } catch (const libconfig::FileIOException &) {
        // File does not exist yet; initialize an empty registry.
        // Dynamic discovery (e.g. LanSniffer scanning /proc/net/arp)
        // will populate detected devices at runtime.
        try {
            libconfig::Config outCfg;
            libconfig::Setting &root = outCfg.getRoot();
            root.add("devices", libconfig::Setting::TypeList);
            outCfg.writeFile(_filePath.c_str());
            _dirty = false;
        } catch (...) {
        }
        return true;
    } catch (const libconfig::ParseException &pex) {
        std::cerr << "Devices config parse error at " << pex.getFile() << ":"
                  << pex.getLine() << " - " << pex.getError() << std::endl;
        return false;
    }

    try {
        libconfig::Setting &devList = cfg.lookup("devices");
        int count = devList.getLength();
        for (int i = 0; i < count; ++i) {
            libconfig::Setting &item = devList[i];
            DeviceInfo dev;
            std::string mac;
            item.lookupValue("mac", mac);
            dev.mac = OuiDatabase::normalizeMac(mac);
            item.lookupValue("ip", dev.ip);
            item.lookupValue("name", dev.name);
            item.lookupValue("vendor", dev.vendor);
            item.lookupValue("category", dev.category);

            long long fs = 0, ls = 0;
            if (item.lookupValue("first_seen", fs)) {
                dev.firstSeen = static_cast<time_t>(fs);
            }
            if (item.lookupValue("last_seen", ls)) {
                dev.lastSeen = static_cast<time_t>(ls);
            }

            if (dev.vendor.empty() || dev.vendor == "Unknown Vendor") {
                dev.vendor = OuiDatabase::getInstance().lookup(dev.mac);
            }

            if (!dev.mac.empty()) {
                _devices[dev.mac] = dev;
            }
        }
    } catch (const std::exception &ex) {
        std::cerr << "Error reading devices list: " << ex.what() << std::endl;
        return false;
    }

    return true;
}

bool DeviceRegistry::save() {
    std::lock_guard<std::mutex> lock(_mutex);

    if (_filePath.empty()) {
        _filePath = Config::resolveHomePath(Config::getInstance().getDevicesFile());
    }

    libconfig::Config cfg;
    libconfig::Setting &root = cfg.getRoot();

    try {
        libconfig::Setting &devList = root.add("devices", libconfig::Setting::TypeList);

        for (const auto &pair : _devices) {
            const auto &dev = pair.second;
            libconfig::Setting &item = devList.add(libconfig::Setting::TypeGroup);
            item.add("mac", libconfig::Setting::TypeString) = dev.mac;
            item.add("ip", libconfig::Setting::TypeString) = dev.ip;
            item.add("name", libconfig::Setting::TypeString) = dev.name;
            item.add("vendor", libconfig::Setting::TypeString) = dev.vendor;
            item.add("category", libconfig::Setting::TypeString) = dev.category;
            item.add("first_seen", libconfig::Setting::TypeInt64) = static_cast<long long>(dev.firstSeen);
            item.add("last_seen", libconfig::Setting::TypeInt64) = static_cast<long long>(dev.lastSeen);
        }

        cfg.writeFile(_filePath.c_str());
        chmod(_filePath.c_str(), 0600);
        _dirty = false;
        return true;
    } catch (const std::exception &ex) {
        std::cerr << "Failed to save devices.cfg: " << ex.what() << std::endl;
        return false;
    }
}

bool DeviceRegistry::upsertDevice(const std::string &mac, const std::string &ip) {
    if (mac.empty() || mac == "00:00:00:00:00:00" || mac == "ff:ff:ff:ff:ff:ff") {
        return false;
    }

    std::string normMac = OuiDatabase::normalizeMac(mac);
    time_t now = time(nullptr);
    bool shouldSave = false;

    {
        std::lock_guard<std::mutex> lock(_mutex);
        auto it = _devices.find(normMac);
        if (it != _devices.end()) {
            it->second.lastSeen = now;
            if (!ip.empty() && it->second.ip != ip) {
                it->second.ip = ip;
            }
            if (it->second.vendor.empty() || it->second.vendor == "Unknown Vendor") {
                it->second.vendor = OuiDatabase::getInstance().lookup(normMac);
            }
            return true;
        }

        // New device
        DeviceInfo dev;
        dev.mac = normMac;
        dev.ip = ip;
        dev.vendor = OuiDatabase::getInstance().lookup(normMac);
        dev.firstSeen = now;
        dev.lastSeen = now;

        std::string last4 = (normMac.length() >= 5) ? normMac.substr(normMac.length() - 5) : "";
        last4.erase(std::remove(last4.begin(), last4.end(), ':'), last4.end());

        if (ip.find("192.168.11.") == 0) {
            dev.category = "visitor";
            dev.name = "visitor_guest_" + last4;
        } else if (OuiDatabase::isMobileVendor(dev.vendor) ||
                   OuiDatabase::isRandomizedMac(normMac)) {
            dev.category = "visitor";
            dev.name = "visitor_phone_" + last4;
        } else {
            dev.category = "unregistered";
            dev.name = "";
        }

        _devices[normMac] = dev;
        _dirty = true;
        shouldSave = true;
    }

    if (shouldSave) {
        save();
    }

    return true;
}

bool DeviceRegistry::nameDevice(const std::string &mac, const std::string &name,
                                const std::string &category) {
    std::string normMac = OuiDatabase::normalizeMac(mac);

    {
        std::lock_guard<std::mutex> lock(_mutex);
        auto it = _devices.find(normMac);
        if (it == _devices.end()) {
            return false;
        }

        it->second.name = name;
        if (!category.empty()) {
            it->second.category = category;
        } else if (it->second.category == "unregistered") {
            it->second.category = "known";
        }
        _dirty = true;
    }

    return save();
}

bool DeviceRegistry::getDevice(const std::string &mac, DeviceInfo &outDevice) const {
    std::string normMac = OuiDatabase::normalizeMac(mac);
    std::lock_guard<std::mutex> lock(_mutex);
    auto it = _devices.find(normMac);
    if (it != _devices.end()) {
        outDevice = it->second;
        return true;
    }
    return false;
}

std::vector<DeviceInfo> DeviceRegistry::getAllDevices() const {
    std::lock_guard<std::mutex> lock(_mutex);
    std::vector<DeviceInfo> result;
    result.reserve(_devices.size());
    for (const auto &pair : _devices) {
        result.push_back(pair.second);
    }
    return result;
}

std::vector<DeviceInfo> DeviceRegistry::getUnregisteredDevices() const {
    std::lock_guard<std::mutex> lock(_mutex);
    std::vector<DeviceInfo> result;
    for (const auto &pair : _devices) {
        if (pair.second.category == "unregistered") {
            result.push_back(pair.second);
        }
    }
    return result;
}

std::vector<DeviceInfo> DeviceRegistry::getVisitorDevices() const {
    std::lock_guard<std::mutex> lock(_mutex);
    std::vector<DeviceInfo> result;
    for (const auto &pair : _devices) {
        if (pair.second.category == "visitor") {
            result.push_back(pair.second);
        }
    }
    return result;
}

std::vector<DeviceInfo> DeviceRegistry::getKnownDevices() const {
    std::lock_guard<std::mutex> lock(_mutex);
    std::vector<DeviceInfo> result;
    for (const auto &pair : _devices) {
        if (pair.second.category == "known" || pair.second.category == "infrastructure") {
            result.push_back(pair.second);
        }
    }
    return result;
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
