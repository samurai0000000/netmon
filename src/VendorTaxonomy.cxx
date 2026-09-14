/*
 * VendorTaxonomy.cxx
 *
 * Copyright (C) 2026, Charles Chiou
 */

#include "VendorTaxonomy.hxx"
#include <algorithm>
#include <cctype>

static std::string toLower(const std::string &str) {
    std::string lower = str;
    std::transform(lower.begin(), lower.end(), lower.begin(),
                   [](unsigned char c) { return std::tolower(c); });
    return lower;
}

bool VendorTaxonomy::isGuestIp(const std::string &ip,
                               const std::vector<std::string> &guestSubnets) {
    if (ip.empty()) {
        return false;
    }

    if (!guestSubnets.empty()) {
        for (const auto &subnet : guestSubnets) {
            std::string prefix = subnet;
            size_t slash = prefix.find('/');
            if (slash != std::string::npos) {
                // e.g. "192.168.11.0/24" -> "192.168.11."
                size_t lastDot = prefix.rfind('.', slash);
                if (lastDot != std::string::npos) {
                    prefix = prefix.substr(0, lastDot + 1);
                }
            }
            if (ip.find(prefix) == 0) {
                return true;
            }
        }
        return false;
    }

    // Default guest subnet convention: 192.168.11.*
    return (ip.find("192.168.11.") == 0);
}

VendorTaxonomy::Category VendorTaxonomy::classify(const std::string &vendor,
                                                 bool isLaaMac,
                                                 const std::string &ip,
                                                 const std::vector<std::string> &guestSubnets) {
    // 1. Guest subnet check takes absolute precedence
    if (isGuestIp(ip, guestSubnets)) {
        return Category::VISITOR;
    }

    // 2. Randomized / Locally Administered MAC addresses (Mobile OS private addresses)
    if (isLaaMac) {
        return Category::VISITOR;
    }

    std::string v = toLower(vendor);
    if (v.empty() || v == "unknown vendor" || v == "unidentified vendor") {
        return Category::UNREGISTERED;
    }

    // 3. IoT / Microcontroller / Camera / Smart Home / Media
    static const char *const IOT_KEYWORDS[] = {
        "espressif", "nabu casa", "tuya", "sonoff", "shelly", "allterco",
        "arduino", "alpsalpine", "alps alpine", "ampak",
        "d-link", "hikvision", "dahua", "axis", "reolink", "amcrest", "foscam",
        "hanwha", "trendnet",
        "sony", "lg electronics", "samsung", "roku", "tcl", "vizio",
        "sonos", "bose", "yamaha", "denon", "marantz", "panasonic",
        "google", "amazon", "brother", "canon", "epson", "xerox", "lexmark", "ricoh"
    };

    for (const char *kw : IOT_KEYWORDS) {
        if (v.find(kw) != std::string::npos) {
            return Category::IOT;
        }
    }

    // 4. Infrastructure / Hypervisors / Networking / Enterprise Servers
    static const char *const INFRA_KEYWORDS[] = {
        "xensource", "qemu", "vmware", "virtualbox", "red hat",
        "hewlett packard enterprise", "hpe", "super micro", "supermicro",
        "zyxel", "cisco", "ubiquiti", "mikrotik", "netgear", "tp-link",
        "aruba", "juniper", "fortinet", "synology", "qnap",
        "raspberry pi", "hardkernel", "beagleboard"
    };

    for (const char *kw : INFRA_KEYWORDS) {
        if (v.find(kw) != std::string::npos) {
            return Category::INFRASTRUCTURE;
        }
    }

    // 5. Known Workstations / PCs
    static const char *const WORKSTATION_KEYWORDS[] = {
        "asustek", "elitegroup", "intel", "dell", "lenovo",
        "gigabyte", "msi", "micro-star", "framework", "apple"
    };

    for (const char *kw : WORKSTATION_KEYWORDS) {
        if (v.find(kw) != std::string::npos) {
            return Category::KNOWN;
        }
    }

    return Category::UNREGISTERED;
}

std::string VendorTaxonomy::categoryToString(Category cat) {
    switch (cat) {
    case Category::IOT:
        return "iot";
    case Category::INFRASTRUCTURE:
        return "infrastructure";
    case Category::KNOWN:
        return "known";
    case Category::VISITOR:
        return "visitor";
    case Category::UNREGISTERED:
    default:
        return "unregistered";
    }
}

VendorTaxonomy::Category VendorTaxonomy::stringToCategory(const std::string &str) {
    std::string s = toLower(str);
    if (s == "iot") return Category::IOT;
    if (s == "infrastructure") return Category::INFRASTRUCTURE;
    if (s == "known") return Category::KNOWN;
    if (s == "visitor") return Category::VISITOR;
    return Category::UNREGISTERED;
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
