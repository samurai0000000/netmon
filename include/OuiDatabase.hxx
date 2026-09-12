/*
 * OuiDatabase.hxx
 *
 * Copyright (C) 2026, Charles Chiou
 */

#ifndef NETMON_OUIDATABASE_HXX
#define NETMON_OUIDATABASE_HXX

#include <string>
#include <unordered_map>
#include <cstdint>

class OuiDatabase {
public:
    static OuiDatabase &getInstance();

    std::string lookup(const std::string &mac) const;
    std::string lookup(const uint8_t mac[6]) const;

    static bool isRandomizedMac(const std::string &mac);
    static bool isRandomizedMac(const uint8_t mac[6]);

    static bool isMobileVendor(const std::string &vendor);
    static std::string normalizeMac(const std::string &mac);

private:
    OuiDatabase();
    ~OuiDatabase() = default;
    OuiDatabase(const OuiDatabase &) = delete;
    OuiDatabase &operator=(const OuiDatabase &) = delete;

    void initializeDatabase();

    std::unordered_map<std::string, std::string> _ouiMap;
};

#endif /* NETMON_OUIDATABASE_HXX */

/*
 * Local variables:
 * mode: C++
 * c-file-style: "BSD"
 * c-basic-offset: 4
 * tab-width: 4
 * indent-tabs-mode: nil
 * End:
 */
