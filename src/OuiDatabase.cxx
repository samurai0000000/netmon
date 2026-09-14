/*
 * OuiDatabase.cxx
 *
 * Copyright (C) 2026, Charles Chiou
 */

#include "OuiDatabase.hxx"
#include <algorithm>
#include <cctype>
#include <cstdio>
#include <sstream>

OuiDatabase &OuiDatabase::getInstance() {
    static OuiDatabase instance;
    return instance;
}

OuiDatabase::OuiDatabase() {
    initializeDatabase();
}

std::string OuiDatabase::normalizeMac(const std::string &mac) {
    std::string clean;
    clean.reserve(17);
    for (char c : mac) {
        if (std::isxdigit(static_cast<unsigned char>(c))) {
            clean.push_back(static_cast<char>(std::tolower(static_cast<unsigned char>(c))));
        }
    }
    if (clean.length() != 12) {
        return mac;
    }

    std::string formatted;
    formatted.reserve(17);
    for (size_t i = 0; i < 12; i += 2) {
        if (i > 0) {
            formatted.push_back(':');
        }
        formatted.push_back(clean[i]);
        formatted.push_back(clean[i + 1]);
    }
    return formatted;
}

bool OuiDatabase::isRandomizedMac(const uint8_t mac[6]) {
    // In 802.3 MAC addresses, bit 1 of the first byte indicates Locally Administered Address (LAA)
    // Mobile OS private Wi-Fi address feature (iOS, Android 10+, Windows 10+) sets this bit.
    return (mac[0] & 0x02) != 0;
}

bool OuiDatabase::isRandomizedMac(const std::string &mac) {
    std::string norm = normalizeMac(mac);
    if (norm.length() >= 2) {
        unsigned int firstByte = 0;
        if (sscanf(norm.c_str(), "%02x", &firstByte) == 1) {
            return (firstByte & 0x02) != 0;
        }
    }
    return false;
}

bool OuiDatabase::isMobileVendor(const std::string &vendor) {
    std::string v = vendor;
    std::transform(v.begin(), v.end(), v.begin(),
                   [](unsigned char c){ return std::tolower(c); });

    if (v.find("apple") != std::string::npos ||
        v.find("samsung") != std::string::npos ||
        v.find("google") != std::string::npos ||
        v.find("xiaomi") != std::string::npos ||
        v.find("huawei") != std::string::npos ||
        v.find("oneplus") != std::string::npos ||
        v.find("oppo") != std::string::npos ||
        v.find("vivo") != std::string::npos ||
        v.find("randomized") != std::string::npos ||
        v.find("private") != std::string::npos) {
        return true;
    }
    return false;
}

std::string OuiDatabase::lookup(const uint8_t mac[6]) const {
    if (isRandomizedMac(mac)) {
        return "Private / Randomized MAC (Mobile OS)";
    }

    char prefix[9];
    snprintf(prefix, sizeof(prefix), "%02x:%02x:%02x", mac[0], mac[1], mac[2]);
    auto it = _ouiMap.find(prefix);
    if (it != _ouiMap.end()) {
        return it->second;
    }

    return "Unknown Vendor";
}

std::string OuiDatabase::lookup(const std::string &mac) const {
    std::string norm = normalizeMac(mac);
    if (isRandomizedMac(norm)) {
        return "Private / Randomized MAC (Mobile OS)";
    }

    if (norm.length() >= 8) {
        std::string prefix = norm.substr(0, 8);
        auto it = _ouiMap.find(prefix);
        if (it != _ouiMap.end()) {
            return it->second;
        }
    }

    return "Unknown Vendor";
}

void OuiDatabase::initializeDatabase() {
    // Zyxel Communications
    _ouiMap["f4:4d:5c"] = "Zyxel Communications Corp";
    _ouiMap["00:13:49"] = "Zyxel Communications Corp";
    _ouiMap["00:19:cb"] = "Zyxel Communications Corp";
    _ouiMap["00:1a:e2"] = "Zyxel Communications Corp";
    _ouiMap["00:23:f8"] = "Zyxel Communications Corp";
    _ouiMap["00:26:44"] = "Zyxel Communications Corp";
    _ouiMap["04:4f:4c"] = "Zyxel Communications Corp";
    _ouiMap["10:20:ba"] = "Zyxel Communications Corp";
    _ouiMap["40:4a:03"] = "Zyxel Communications Corp";
    _ouiMap["50:67:ae"] = "Zyxel Communications Corp";
    _ouiMap["74:31:70"] = "Zyxel Communications Corp";
    _ouiMap["88:e0:f3"] = "Zyxel Communications Corp";
    _ouiMap["c8:54:4b"] = "Zyxel Communications Corp";

    // Virtualization / Hypervisors (Xen, KVM, QEMU)
    _ouiMap["00:16:3e"] = "Xen / QEMU Virtual NIC";
    _ouiMap["52:54:00"] = "QEMU / KVM Virtual NIC";
    _ouiMap["00:0c:29"] = "VMware, Inc.";
    _ouiMap["00:50:56"] = "VMware, Inc.";
    _ouiMap["08:00:27"] = "Oracle VirtualBox";

    // Raspberry Pi Foundation
    _ouiMap["b8:27:eb"] = "Raspberry Pi Foundation";
    _ouiMap["dc:a6:32"] = "Raspberry Pi Foundation";
    _ouiMap["e4:5f:01"] = "Raspberry Pi Foundation";
    _ouiMap["d8:3a:dd"] = "Raspberry Pi Foundation";
    _ouiMap["28:cd:c1"] = "Raspberry Pi Foundation";

    // Espressif Inc (ESP32 / ESP8266 IoT nodes)
    _ouiMap["10:20:ba"] = "Espressif Inc (ESP32)";
    _ouiMap["24:0a:c4"] = "Espressif Inc (ESP32)";
    _ouiMap["24:62:ab"] = "Espressif Inc (ESP32)";
    _ouiMap["24:6f:28"] = "Espressif Inc (ESP32)";
    _ouiMap["24:dc:c3"] = "Espressif Inc (ESP32)";
    _ouiMap["30:ae:a4"] = "Espressif Inc (ESP32)";
    _ouiMap["3c:61:05"] = "Espressif Inc (ESP32)";
    _ouiMap["3c:71:bf"] = "Espressif Inc (ESP32)";
    _ouiMap["40:22:d8"] = "Espressif Inc (ESP32)";
    _ouiMap["40:91:51"] = "Espressif Inc (ESP32)";
    _ouiMap["48:3f:da"] = "Espressif Inc (ESP32)";
    _ouiMap["48:55:19"] = "Espressif Inc (ESP32)";
    _ouiMap["4c:75:25"] = "Espressif Inc (ESP32)";
    _ouiMap["54:43:b2"] = "Espressif Inc (ESP32)";
    _ouiMap["5c:cf:7f"] = "Espressif Inc (ESP8266)";
    _ouiMap["60:01:94"] = "Espressif Inc (ESP8266)";
    _ouiMap["64:e8:33"] = "Espressif Inc (ESP32)";
    _ouiMap["68:c6:3a"] = "Espressif Inc (ESP32)";
    _ouiMap["70:03:9f"] = "Espressif Inc (ESP32)";
    _ouiMap["70:b3:d5"] = "Espressif Inc (ESP32)";
    _ouiMap["7c:df:a1"] = "Espressif Inc (ESP32)";
    _ouiMap["80:7d:3a"] = "Espressif Inc (ESP32)";
    _ouiMap["84:0d:8e"] = "Espressif Inc (ESP32)";
    _ouiMap["84:f3:eb"] = "Espressif Inc (ESP8266)";
    _ouiMap["8c:aa:b5"] = "Espressif Inc (ESP32)";
    _ouiMap["94:b5:55"] = "Espressif Inc (ESP32)";
    _ouiMap["94:b9:7e"] = "Espressif Inc (ESP32)";
    _ouiMap["98:3d:ae"] = "Espressif Inc (ESP32)";
    _ouiMap["a0:20:a6"] = "Espressif Inc (ESP32)";
    _ouiMap["a4:cf:12"] = "Espressif Inc (ESP8266)";
    _ouiMap["a4:e5:7c"] = "Espressif Inc (ESP32)";
    _ouiMap["ac:0b:fb"] = "Espressif Inc (ESP32)";
    _ouiMap["ac:67:b2"] = "Espressif Inc (ESP32)";
    _ouiMap["b4:e6:2d"] = "Espressif Inc (ESP32)";
    _ouiMap["bc:dd:c2"] = "Espressif Inc (ESP32)";
    _ouiMap["c4:4f:33"] = "Espressif Inc (ESP32)";
    _ouiMap["c8:2b:96"] = "Espressif Inc (ESP32)";
    _ouiMap["cc:50:e3"] = "Espressif Inc (ESP32)";
    _ouiMap["d4:8c:49"] = "Espressif Inc (ESP32)";
    _ouiMap["d4:d4:da"] = "Espressif Inc (ESP32)";
    _ouiMap["d8:bc:38"] = "Espressif Inc (ESP32)";
    _ouiMap["dc:4f:22"] = "Espressif Inc (ESP32)";
    _ouiMap["e0:98:06"] = "Espressif Inc (ESP32)";
    _ouiMap["e8:68:e7"] = "Espressif Inc (ESP8266)";
    _ouiMap["ec:62:60"] = "Espressif Inc (ESP32)";
    _ouiMap["f0:08:d1"] = "Espressif Inc (ESP32)";
    _ouiMap["f4:cf:a2"] = "Espressif Inc (ESP8266)";

    // Intel Corporation
    _ouiMap["00:1b:21"] = "Intel Corporation";
    _ouiMap["00:1e:67"] = "Intel Corporation";
    _ouiMap["00:21:6a"] = "Intel Corporation";
    _ouiMap["00:23:14"] = "Intel Corporation";
    _ouiMap["14:02:ec"] = "Intel Corporation";
    _ouiMap["68:05:ca"] = "Intel Corporation";
    _ouiMap["8c:8c:aa"] = "Intel Corporation";
    _ouiMap["98:3b:8f"] = "Intel Corporation";
    _ouiMap["a4:4c:c8"] = "Intel Corporation";

    // Apple, Inc.
    _ouiMap["00:17:f2"] = "Apple, Inc.";
    _ouiMap["00:1c:b3"] = "Apple, Inc.";
    _ouiMap["00:26:08"] = "Apple, Inc.";
    _ouiMap["00:3e:e1"] = "Apple, Inc.";
    _ouiMap["38:c9:86"] = "Apple, Inc.";
    _ouiMap["68:45:cc"] = "Apple, Inc.";
    _ouiMap["b8:3c:28"] = "Apple, Inc.";

    // Google, Inc. (Nest Hub, Smart Displays, Chromecast)
    _ouiMap["00:1a:11"] = "Google, Inc.";
    _ouiMap["94:45:60"] = "Google, Inc.";
    _ouiMap["ac:67:84"] = "Google, Inc.";
    _ouiMap["d8:6c:63"] = "Google, Inc.";
    _ouiMap["f4:f5:d8"] = "Google, Inc.";

    // Sony Corporation (Bravia Smart TVs, AV Receivers)
    _ouiMap["00:01:4a"] = "Sony Corporation";
    _ouiMap["84:c7:ea"] = "Sony Corporation";
    _ouiMap["94:db:56"] = "Sony Home Entertainment";
    _ouiMap["ac:80:0a"] = "Sony Corporation";
    _ouiMap["fc:f1:36"] = "Sony Corporation";

    // Brother Industries, LTD. (Printers)
    _ouiMap["00:80:77"] = "Brother Industries, LTD.";
    _ouiMap["30:05:5c"] = "Brother Industries, LTD.";
    _ouiMap["3c:2a:f4"] = "Brother Industries, LTD.";

    // Nabu Casa, Inc. (Home Assistant)
    _ouiMap["20:f8:3b"] = "Nabu Casa, Inc.";

    // ALPSALPINE CO., LTD.
    _ouiMap["44:eb:2e"] = "ALPSALPINE CO,.LTD";

    // EliteGroup Computer Systems (ECS Mini PCs)
    _ouiMap["00:0a:79"] = "EliteGroup Computer Systems Co., LTD";
    _ouiMap["88:ae:dd"] = "EliteGroup Computer Systems Co., LTD";

    // AMPAK Technology (IoT Wi-Fi Modules)
    _ouiMap["9c:b8:b4"] = "AMPAK Technology,Inc.";

    // ASUSTek Computer Inc.
    _ouiMap["00:1d:60"] = "ASUSTek COMPUTER INC.";
    _ouiMap["a0:ad:9f"] = "ASUSTek COMPUTER INC.";

    // D-Link International (IP Cameras, Networking)
    _ouiMap["00:14:d1"] = "D-Link International";
    _ouiMap["14:d6:4d"] = "D-Link International";
    _ouiMap["28:10:7b"] = "D-Link International";
    _ouiMap["b0:c5:54"] = "D-Link International";

    // Hewlett Packard Enterprise (Servers, iLO)
    _ouiMap["14:02:ec"] = "Hewlett Packard Enterprise";
    _ouiMap["28:80:23"] = "Hewlett Packard Enterprise";
    _ouiMap["3c:a8:2a"] = "Hewlett Packard Enterprise";
    _ouiMap["94:57:a5"] = "Hewlett Packard Enterprise";

    // Samsung Electronics
    _ouiMap["00:07:ab"] = "Samsung Electronics";
    _ouiMap["00:12:47"] = "Samsung Electronics";
    _ouiMap["00:15:b9"] = "Samsung Electronics";
    _ouiMap["00:17:d5"] = "Samsung Electronics";
    _ouiMap["00:21:19"] = "Samsung Electronics";
    _ouiMap["00:23:d7"] = "Samsung Electronics";
    _ouiMap["00:26:37"] = "Samsung Electronics";
    _ouiMap["08:08:c2"] = "Samsung Electronics";
    _ouiMap["08:d4:2b"] = "Samsung Electronics";
    _ouiMap["10:30:47"] = "Samsung Electronics";
    _ouiMap["18:3a:2d"] = "Samsung Electronics";
    _ouiMap["24:4b:03"] = "Samsung Electronics";
    _ouiMap["34:c0:59"] = "Samsung Electronics";
    _ouiMap["44:f4:59"] = "Samsung Electronics";
    _ouiMap["50:01:d9"] = "Samsung Electronics";
    _ouiMap["54:44:08"] = "Samsung Electronics";
    _ouiMap["60:6b:bd"] = "Samsung Electronics";
    _ouiMap["78:40:e4"] = "Samsung Electronics";
    _ouiMap["84:25:db"] = "Samsung Electronics";
    _ouiMap["90:18:7c"] = "Samsung Electronics";
    _ouiMap["94:35:0a"] = "Samsung Electronics";
    _ouiMap["a0:82:1f"] = "Samsung Electronics";
    _ouiMap["ac:5f:3e"] = "Samsung Electronics";
    _ouiMap["b0:ec:71"] = "Samsung Electronics";
    _ouiMap["bc:85:56"] = "Samsung Electronics";
    _ouiMap["c4:73:1e"] = "Samsung Electronics";
    _ouiMap["cc:07:ab"] = "Samsung Electronics";
    _ouiMap["d0:59:e4"] = "Samsung Electronics";
    _ouiMap["d8:57:ef"] = "Samsung Electronics";
    _ouiMap["e8:50:8b"] = "Samsung Electronics";
    _ouiMap["f8:04:2e"] = "Samsung Electronics";

    // Google LLC
    _ouiMap["00:1a:11"] = "Google LLC";
    _ouiMap["3c:5a:37"] = "Google LLC";
    _ouiMap["54:60:09"] = "Google LLC";
    _ouiMap["64:16:66"] = "Google LLC";
    _ouiMap["70:3e:ac"] = "Google LLC";
    _ouiMap["94:eb:2c"] = "Google LLC";
    _ouiMap["a4:77:33"] = "Google LLC";
    _ouiMap["d4:f5:47"] = "Google LLC";
    _ouiMap["f4:f5:e8"] = "Google LLC";
    _ouiMap["f8:0f:f9"] = "Google LLC";

    // Huawei Technologies
    _ouiMap["00:18:82"] = "Huawei Technologies";
    _ouiMap["00:1e:10"] = "Huawei Technologies";
    _ouiMap["00:25:9e"] = "Huawei Technologies";
    _ouiMap["04:25:c5"] = "Huawei Technologies";
    _ouiMap["04:79:70"] = "Huawei Technologies";
    _ouiMap["08:19:a6"] = "Huawei Technologies";
    _ouiMap["10:47:80"] = "Huawei Technologies";
    _ouiMap["20:0b:c7"] = "Huawei Technologies";
    _ouiMap["28:6e:d4"] = "Huawei Technologies";
    _ouiMap["34:0a:98"] = "Huawei Technologies";
    _ouiMap["48:46:fb"] = "Huawei Technologies";
    _ouiMap["54:89:98"] = "Huawei Technologies";
    _ouiMap["60:e3:27"] = "Huawei Technologies";
    _ouiMap["78:6a:89"] = "Huawei Technologies";
    _ouiMap["80:b6:86"] = "Huawei Technologies";
    _ouiMap["90:17:ac"] = "Huawei Technologies";
    _ouiMap["a4:ba:76"] = "Huawei Technologies";
    _ouiMap["ac:e2:15"] = "Huawei Technologies";
    _ouiMap["b4:30:52"] = "Huawei Technologies";
    _ouiMap["c8:d1:5e"] = "Huawei Technologies";
    _ouiMap["d4:6a:a8"] = "Huawei Technologies";
    _ouiMap["dc:d2:fc"] = "Huawei Technologies";
    _ouiMap["e8:cd:2d"] = "Huawei Technologies";
    _ouiMap["f8:e8:11"] = "Huawei Technologies";

    // Xiaomi Communications
    _ouiMap["00:9e:c8"] = "Xiaomi Communications";
    _ouiMap["14:f6:5a"] = "Xiaomi Communications";
    _ouiMap["18:59:36"] = "Xiaomi Communications";
    _ouiMap["28:6c:07"] = "Xiaomi Communications";
    _ouiMap["34:ce:00"] = "Xiaomi Communications";
    _ouiMap["38:a4:ed"] = "Xiaomi Communications";
    _ouiMap["50:64:2b"] = "Xiaomi Communications";
    _ouiMap["64:09:80"] = "Xiaomi Communications";
    _ouiMap["74:23:44"] = "Xiaomi Communications";
    _ouiMap["78:11:dc"] = "Xiaomi Communications";
    _ouiMap["88:c3:97"] = "Xiaomi Communications";
    _ouiMap["9c:99:a0"] = "Xiaomi Communications";
    _ouiMap["a4:c3:f0"] = "Xiaomi Communications";
    _ouiMap["b0:38:29"] = "Xiaomi Communications";
    _ouiMap["c4:0b:cb"] = "Xiaomi Communications";
    _ouiMap["d0:31:10"] = "Xiaomi Communications";
    _ouiMap["f4:8e:38"] = "Xiaomi Communications";

    // Amazon Technologies
    _ouiMap["00:bb:3a"] = "Amazon Technologies";
    _ouiMap["18:74:2e"] = "Amazon Technologies";
    _ouiMap["34:d2:70"] = "Amazon Technologies";
    _ouiMap["44:65:0d"] = "Amazon Technologies";
    _ouiMap["50:dc:e7"] = "Amazon Technologies";
    _ouiMap["68:37:e9"] = "Amazon Technologies";
    _ouiMap["74:75:48"] = "Amazon Technologies";
    _ouiMap["84:d6:d0"] = "Amazon Technologies";
    _ouiMap["ac:63:be"] = "Amazon Technologies";
    _ouiMap["fc:65:de"] = "Amazon Technologies";

    // Ubiquiti Networks
    _ouiMap["00:15:6d"] = "Ubiquiti Networks";
    _ouiMap["00:27:22"] = "Ubiquiti Networks";
    _ouiMap["04:18:d6"] = "Ubiquiti Networks";
    _ouiMap["24:a4:3c"] = "Ubiquiti Networks";
    _ouiMap["44:d9:e7"] = "Ubiquiti Networks";
    _ouiMap["68:72:51"] = "Ubiquiti Networks";
    _ouiMap["74:83:c2"] = "Ubiquiti Networks";
    _ouiMap["78:8a:20"] = "Ubiquiti Networks";
    _ouiMap["80:2a:a8"] = "Ubiquiti Networks";
    _ouiMap["dc:9f:db"] = "Ubiquiti Networks";
    _ouiMap["f0:9f:c2"] = "Ubiquiti Networks";

    // TP-Link
    _ouiMap["00:14:78"] = "TP-Link Technologies";
    _ouiMap["00:19:e0"] = "TP-Link Technologies";
    _ouiMap["00:21:27"] = "TP-Link Technologies";
    _ouiMap["00:23:cd"] = "TP-Link Technologies";
    _ouiMap["00:25:86"] = "TP-Link Technologies";
    _ouiMap["14:cf:92"] = "TP-Link Technologies";
    _ouiMap["18:a6:f7"] = "TP-Link Technologies";
    _ouiMap["1c:3b:f3"] = "TP-Link Technologies";
    _ouiMap["30:de:4b"] = "TP-Link Technologies";
    _ouiMap["50:c7:bf"] = "TP-Link Technologies";
    _ouiMap["60:32:b1"] = "TP-Link Technologies";
    _ouiMap["70:4f:57"] = "TP-Link Technologies";
    _ouiMap["74:da:88"] = "TP-Link Technologies";
    _ouiMap["98:48:27"] = "TP-Link Technologies";
    _ouiMap["b0:4e:26"] = "TP-Link Technologies";
    _ouiMap["c0:25:e9"] = "TP-Link Technologies";
    _ouiMap["c4:e9:84"] = "TP-Link Technologies";
    _ouiMap["d8:0d:17"] = "TP-Link Technologies";
    _ouiMap["e8:48:b8"] = "TP-Link Technologies";
    _ouiMap["f4:f2:6d"] = "TP-Link Technologies";
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
