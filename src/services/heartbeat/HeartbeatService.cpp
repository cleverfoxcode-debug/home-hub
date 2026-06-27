#include "HeartbeatService.h"
#include <Arduino.h>

namespace hub::services {

hub::core::Result HeartbeatService::begin() {
    m_lastTick = millis();
    return hub::core::Result::Ok;
}

hub::core::Result HeartbeatService::update() {
    if (millis() - m_lastTick >= 1000) {
        m_lastTick = millis();
    }
    return hub::core::Result::Ok;
}

hub::core::Result HeartbeatService::shutdown() {
    return hub::core::Result::Ok;
}

} // namespace hub::services
