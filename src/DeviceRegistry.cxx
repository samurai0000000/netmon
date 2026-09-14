/*
 * DeviceRegistry.cxx
 *
 * Copyright (C) 2026, Charles Chiou
 */

#include "DeviceRegistry.hxx"
#include "OuiDatabase.hxx"
#include "MacVendorResolver.hxx"
#include "VendorTaxonomy.hxx"
#include "DnsResolver.hxx"
#include "Config.hxx"

#include <libconfig.h++>
#include <iostream>
#include <fstream>
#include <algorithm>
#include <sys/stat.h>
#include <filesystem>

namespace fs = std::filesystem;

DeviceRegistry &DeviceRegistry::getInstance() {
    static DeviceRegistry instance;
    return instance;
}

DeviceRegistry::DeviceRegistry()
    : _filePath("")
    , _dirty(false) {
}

std::string DeviceRegistry::nameSourceToString(NameSource source) {
    switch (source) {
    case NameSource::MANUAL:
        return "manual";
    case NameSource::DNS_PTR:
        return "dns_ptr";
    case NameSource::DHCP_OPT12:
        return "dhcp_opt12";
    case NameSource::OUI_FALLBACK:
    default:
        return "oui_fallback";
    }
}

NameSource DeviceRegistry::stringToNameSource(const std::string &str) {
    if (str == "manual") return NameSource::MANUAL;
    if (str == "dns_ptr") return NameSource::DNS_PTR;
    if (str == "dhcp_opt12") return NameSource::DHCP_OPT12;
    return NameSource::OUI_FALLBACK;
}

void DeviceRegistry::initEnrichment() {
    MacVendorResolver::getInstance().setVendorCallback(
        [this](const std::string &mac, const std::string &vendor) {
            onVendorResolved(mac, vendor);
        });

    DnsResolver::getInstance().setDnsCallback(
        [this](const std::string &mac, const std::string &ip,
               const std::string &hostname, bool confirmed) {
            onDnsResolved(mac, ip, hostname, confirmed);
        });

    MacVendorResolver::getInstance().start();
    DnsResolver::getInstance().start("selfso.com");
}

bool DeviceRegistry::load(const std::string &customPath) {
    std::lock_guard<std::mutex> lock(_mutex);

    if (!customPath.empty()) {
        _filePath = Config::resolveHomePath(customPath);
    } else {
        _filePath = Config::resolveHomePath(Config::getInstance().getDevicesFile());
    }

    if (!fs::exists(_filePath)) {
        return true;
    }

    libconfig::Config cfg;
    try {
        cfg.readFile(_filePath.c_str());
    } catch (const libconfig::FileIOException &fioex) {
        std::cerr << "Cannot read devices file: " << _filePath << std::endl;
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

            std::string srcStr;
            if (item.lookupValue("source", srcStr)) {
                dev.nameSource = stringToNameSource(srcStr);
            } else {
                // Heuristic for legacy entries without source tag:
                // If named manually (not visitor_phone_* / visitor_guest_*), treat as MANUAL
                if (!dev.name.empty() &&
                    dev.name.find("visitor_phone_") != 0 &&
                    dev.name.find("visitor_guest_") != 0) {
                    dev.nameSource = NameSource::MANUAL;
                } else {
                    dev.nameSource = NameSource::OUI_FALLBACK;
                }
            }

            long long fsVal = 0, lsVal = 0;
            if (item.lookupValue("first_seen", fsVal)) {
                dev.firstSeen = static_cast<time_t>(fsVal);
            }
            if (item.lookupValue("last_seen", lsVal)) {
                dev.lastSeen = static_cast<time_t>(lsVal);
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

    // Initialize enrichment engines
    initEnrichment();

    // Clean legacy artifacts and enqueue for enrichment
    scrubLegacyVisitorPhoneNames();

    return true;
}

void DeviceRegistry::scrubLegacyVisitorPhoneNames() {
    bool modified = false;

    for (auto &pair : _devices) {
        auto &dev = pair.second;
        bool isLaa = OuiDatabase::isRandomizedMac(dev.mac);

        // Strip legacy auto-generated visitor labels
        if (dev.name.find("visitor_phone_") == 0 ||
            dev.name.find("visitor_guest_") == 0) {
            dev.name = "";
            dev.nameSource = NameSource::OUI_FALLBACK;
            modified = true;
        }

        // Re-evaluate category based on true vendor taxonomy
        std::string trueVendor = MacVendorResolver::getInstance().resolve(dev.mac, dev.ip);
        if (!trueVendor.empty() && trueVendor != "Unknown Vendor") {
            dev.vendor = trueVendor;
        }

        auto catEnum = VendorTaxonomy::classify(dev.vendor, isLaa, dev.ip);
        std::string newCat = VendorTaxonomy::categoryToString(catEnum);
        if (dev.category != newCat && dev.nameSource != NameSource::MANUAL) {
            dev.category = newCat;
            modified = true;
        }

        // Enqueue for enrichment lookups (Vendor + Reverse DNS)
        if (!dev.ip.empty()) {
            DnsResolver::getInstance().enqueueLookup(dev.mac, dev.ip);
        }
        MacVendorResolver::getInstance().enqueueLookup(dev.mac, dev.ip);
    }

    if (modified) {
        save();
    }
}

bool DeviceRegistry::save() {
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
            item.add("source", libconfig::Setting::TypeString) = nameSourceToString(dev.nameSource);
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
                if (!ip.empty()) {
                    DnsResolver::getInstance().enqueueLookup(normMac, ip);
                }
            }
            if (it->second.vendor.empty() || it->second.vendor == "Unknown Vendor") {
                it->second.vendor = MacVendorResolver::getInstance().resolve(normMac, ip);
            }
            return true;
        }

        // New device
        DeviceInfo dev;
        dev.mac = normMac;
        dev.ip = ip;
        dev.firstSeen = now;
        dev.lastSeen = now;
        dev.nameSource = NameSource::OUI_FALLBACK;

        bool isLaa = OuiDatabase::isRandomizedMac(normMac);
        dev.vendor = MacVendorResolver::getInstance().resolve(normMac, ip);

        auto catEnum = VendorTaxonomy::classify(dev.vendor, isLaa, ip);
        dev.category = VendorTaxonomy::categoryToString(catEnum);

        std::string last4 = (normMac.length() >= 5) ? normMac.substr(normMac.length() - 5) : "";
        last4.erase(std::remove(last4.begin(), last4.end(), ':'), last4.end());

        if (dev.category == "visitor") {
            dev.name = "visitor_" + last4;
        } else {
            dev.name = ""; // Will be populated by DnsResolver
        }

        _devices[normMac] = dev;
        _dirty = true;
        shouldSave = true;

        if (!ip.empty()) {
            DnsResolver::getInstance().enqueueLookup(normMac, ip);
        }
        MacVendorResolver::getInstance().enqueueLookup(normMac, ip);
    }

    if (shouldSave) {
        save();
    }

    return true;
}

void DeviceRegistry::onVendorResolved(const std::string &mac, const std::string &vendor) {
    std::string normMac = OuiDatabase::normalizeMac(mac);
    bool shouldSave = false;

    {
        std::lock_guard<std::mutex> lock(_mutex);
        auto it = _devices.find(normMac);
        if (it != _devices.end()) {
            it->second.vendor = vendor;
            if (it->second.nameSource != NameSource::MANUAL) {
                bool isLaa = OuiDatabase::isRandomizedMac(normMac);
                auto cat = VendorTaxonomy::classify(vendor, isLaa, it->second.ip);
                it->second.category = VendorTaxonomy::categoryToString(cat);
            }
            _dirty = true;
            shouldSave = true;
        }
    }

    if (shouldSave) {
        save();
    }
}

void DeviceRegistry::onDnsResolved(const std::string &mac, const std::string &ip,
                                  const std::string &hostname, bool forwardConfirmed) {
    if (hostname.empty()) {
        return;
    }

    std::string normMac = OuiDatabase::normalizeMac(mac);
    bool shouldSave = false;

    {
        std::lock_guard<std::mutex> lock(_mutex);
        auto it = _devices.find(normMac);
        if (it != _devices.end()) {
            if (it->second.nameSource != NameSource::MANUAL) {
                it->second.name = hostname;
                it->second.nameSource = NameSource::DNS_PTR;
                _dirty = true;
                shouldSave = true;
            }
        }
    }

    if (shouldSave) {
        save();
    }
}

void DeviceRegistry::onDhcpHostnameSniffed(const std::string &mac, const std::string &hostname) {
    if (hostname.empty()) {
        return;
    }

    std::string normMac = OuiDatabase::normalizeMac(mac);
    bool shouldSave = false;

    {
        std::lock_guard<std::mutex> lock(_mutex);
        auto it = _devices.find(normMac);
        if (it != _devices.end()) {
            // Only adopt DHCP Option 12 hostname if not manually named and not already DNS confirmed
            if (it->second.nameSource == NameSource::OUI_FALLBACK ||
                (it->second.nameSource != NameSource::MANUAL && it->second.name.empty())) {
                it->second.name = hostname;
                it->second.nameSource = NameSource::DHCP_OPT12;
                _dirty = true;
                shouldSave = true;
            }
        }
    }

    if (shouldSave) {
        save();
    }
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
        it->second.nameSource = NameSource::MANUAL;
        if (!category.empty()) {
            it->second.category = category;
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

std::vector<DeviceInfo> DeviceRegistry::getIotDevices() const {
    std::lock_guard<std::mutex> lock(_mutex);
    std::vector<DeviceInfo> result;
    for (const auto &pair : _devices) {
        if (pair.second.category == "iot") {
            result.push_back(pair.second);
        }
    }
    return result;
}

std::vector<DeviceInfo> DeviceRegistry::getInfrastructureDevices() const {
    std::lock_guard<std::mutex> lock(_mutex);
    std::vector<DeviceInfo> result;
    for (const auto &pair : _devices) {
        if (pair.second.category == "infrastructure") {
            result.push_back(pair.second);
        }
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
        if (pair.second.category == "known") {
            result.push_back(pair.second);
        }
    }
    return result;
}

size_t DeviceRegistry::getDeviceCount() const {
    std::lock_guard<std::mutex> lock(_mutex);
    return _devices.size();
}

const std::string &DeviceRegistry::getFilePath() const {
    return _filePath;
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
