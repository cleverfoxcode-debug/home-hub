#include "HeartbeatService.h"
#include <Arduino.h>
#include <Config.h>

namespace hub::services {

hub::core::Result HeartbeatService::begin() {
    m_lastTick = millis();
    m_ledToggleMs = millis();
    m_ledPin = config::StatusLedPin;

    // Настраиваем пин LED
    pinMode(m_ledPin, OUTPUT);
    digitalWrite(m_ledPin, HIGH);  // LED горит постоянно при старте (SolidOn)
    m_ledState = true;

    // Запускаем watchdog с таймаутом из конфига
    m_watchdog.begin(config::WatchdogTimeoutSec);

    return hub::core::Result::Ok;
}

hub::core::Result HeartbeatService::update() {
    // Сброс watchdog — если сервис зависнет, ESP32 перезагрузится
    m_watchdog.feed();

    // Обновление LED раз в 100 мс
    updateLed();

    // Периодический тик (1 секунда)
    if (millis() - m_lastTick >= 1000) {
        m_lastTick = millis();
        // TODO: отправка keepalive, пинг внешних сервисов и т.д.
    }

    return hub::core::Result::Ok;
}

hub::core::Result HeartbeatService::shutdown() {
    digitalWrite(m_ledPin, LOW);  // LED выключен
    m_watchdog.end();
    return hub::core::Result::Ok;
}

void HeartbeatService::setLedMode(LedMode mode) {
    if (m_ledMode == mode) {
        return;
    }
    m_ledMode = mode;
    m_ledToggleMs = millis();
    // Принудительно включаем LED при смене режима
    m_ledState = true;
    digitalWrite(m_ledPin, HIGH);
}

void HeartbeatService::updateLed() {
    const unsigned long now = millis();
    unsigned long interval = 0;

    switch (m_ledMode) {
        case LedMode::Off:
            digitalWrite(m_ledPin, LOW);
            m_ledState = false;
            return;

        case LedMode::SolidOn:
            digitalWrite(m_ledPin, HIGH);
            m_ledState = true;
            return;

        case LedMode::Normal:
            interval = 1000;  // 1 сек
            break;

        case LedMode::Slow:
            interval = 2000;  // 2 сек
            break;

        case LedMode::Fast:
            interval = 200;   // 200 мс
            break;
    }

    if (interval > 0 && (now - m_ledToggleMs >= interval)) {
        m_ledToggleMs = now;
        m_ledState = !m_ledState;
        digitalWrite(m_ledPin, m_ledState ? HIGH : LOW);
    }
}

} // namespace hub::services