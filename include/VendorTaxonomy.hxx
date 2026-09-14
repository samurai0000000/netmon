/*
 * VendorTaxonomy.hxx
 *
 * Copyright (C) 2026, Charles Chiou
 */

#ifndef NETMON_VENDORTAXONOMY_HXX
#define NETMON_VENDORTAXONOMY_HXX

#include <string>
#include <vector>

class VendorTaxonomy {
public:
    enum class Category {
        IOT,
        INFRASTRUCTURE,
        KNOWN,
        VISITOR,
        UNREGISTERED
    };

    static Category classify(const std::string &vendor,
                             bool isLaaMac,
                             const std::string &ip,
                             const std::vector<std::string> &guestSubnets = {});

    static std::string categoryToString(Category cat);
    static Category stringToCategory(const std::string &str);

    static bool isGuestIp(const std::string &ip,
                          const std::vector<std::string> &guestSubnets = {});
};

#endif /* NETMON_VENDORTAXONOMY_HXX */

/*
 * Local variables:
 * mode: C++
 * c-file-style: "BSD"
 * c-basic-offset: 4
 * tab-width: 4
 * indent-tabs-mode: nil
 * End:
 */
