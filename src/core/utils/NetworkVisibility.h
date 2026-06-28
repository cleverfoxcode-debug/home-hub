#pragma once

#include <string>
#include <vector>

namespace hub::core::utils {

inline bool isHomeNetworkVisible(const std::string& savedSsid, const std::vector<std::string>& visibleNetworks) {
    if (savedSsid.empty()) {
        return false;
    }

    for (const auto& network : visibleNetworks) {
        if (network == savedSsid) {
            return true;
        }
    }

    return false;
}

} // namespace hub::core::utils
