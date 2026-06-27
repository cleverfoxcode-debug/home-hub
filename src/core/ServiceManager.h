#pragma once

#include "interfaces/IService.h"
#include <cstddef>

namespace hub::core {

class ServiceManager {
public:
    ServiceManager(IService* const* services, std::size_t count) noexcept;

    Result beginAll();
    Result updateAll();
    Result shutdownAll();

private:
    IService* const* m_services;
    std::size_t      m_count;
};

} // namespace hub::core
