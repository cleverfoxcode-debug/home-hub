#pragma once

#include <Arduino.h>
#include "../../core/interfaces/IService.h"
#include "../../core/utils/WatchdogManager.h"

namespace hub::services {

class HeartbeatService final : public hub::core::IService {
public:
    HeartbeatService() = default;

    hub::core::Result begin() override;
    hub::core::Result update() override;
    hub::core::Result shutdown() override;

private:
    /// Режимы мигания LED
    enum class LedMode {
        Off,       ///< LED выключен
        Normal,    ///< медленное мигание (1 сек вкл, 1 сек выкл) — всё ОК
        Slow,      ///< очень медленное мигание (2 сек вкл, 2 сек выкл) — режим настройки
        Fast,      ///< быстрое мигание (200 мс) — ошибка
        SolidOn,   ///< постоянно горит — инициализация
    };

    void setLedMode(LedMode mode);
    void updateLed();

private:
    unsigned long                        m_lastTick = 0;
    unsigned long                        m_ledToggleMs = 0;
    hub::core::utils::WatchdogManager    m_watchdog;
    LedMode                              m_ledMode = LedMode::SolidOn;
    bool                                 m_ledState = false;
    uint8_t                              m_ledPin = 0;
};

} // namespace hub::services