#pragma once
#include <cstdint>

namespace hub::core {

struct Version {
    std::uint16_t major;
    std::uint16_t minor;
    std::uint16_t patch;
};

} // namespace hub::core