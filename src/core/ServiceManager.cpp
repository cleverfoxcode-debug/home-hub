#include "ServiceManager.h"
#include <Arduino.h>

namespace hub::core {

ServiceManager::ServiceManager(IService* const* services, std::size_t count) noexcept
    : m_services(services)
    , m_count(count)
{
}

Result ServiceManager::beginAll() {
    for (std::size_t index = 0; index < m_count; ++index) {
        Serial.printf("[ServiceManager] begin service %zu\n", index);
        const auto result = m_services[index]->begin();
        if (result != Result::Ok) {
            Serial.printf("[ServiceManager] service %zu failed with result %d\n", index, static_cast<int>(result));
            return result;
        }
    }
    return Result::Ok;
}

Result ServiceManager::updateAll() {
    for (std::size_t index = 0; index < m_count; ++index) {
        const auto result = m_services[index]->update();
        if (result != Result::Ok) {
            return result;
        }
    }
    return Result::Ok;
}

Result ServiceManager::shutdownAll() {
    for (std::size_t index = 0; index < m_count; ++index) {
        const auto result = m_services[index]->shutdown();
        if (result != Result::Ok) {
            return result;
        }
    }
    return Result::Ok;
}

} // namespace hub::core
