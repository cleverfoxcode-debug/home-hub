#pragma once

#include <cstdint>

namespace hub::core::utils {

/**
 * @brief Менеджер аппаратного Watchdog (TWDT) ESP32.
 *
 * Инициализирует Task Watchdog Timer и предоставляет метод feed()
 * для сброса таймера. Если feed() не вызывается в течение таймаута,
 * ESP32 перезагружается.
 *
 * Использование:
 *   WatchdogManager wd(10);  // таймаут 10 секунд
 *   wd.begin();
 *   // в цикле:
 *   wd.feed();
 */
class WatchdogManager {
public:
    /**
     * @param timeoutSec Таймаут watchdog в секундах (1-60).
     *                    По умолчанию 10 секунд.
     */
    explicit WatchdogManager(std::uint32_t timeoutSec = 10) noexcept
        : m_timeoutSec(timeoutSec)
        , m_initialized(false) {}

    /// Инициализировать TWDT и добавить текущую задачу под надзор.
    /// @param timeoutSec Таймаут watchdog в секундах (переопределяет значение из конструктора).
    void begin(std::uint32_t timeoutSec = 0);

    /// Сбросить watchdog (вызывать периодически, чаще чем timeoutSec).
    void feed();

    /// Остановить watchdog.
    void end();

private:
    std::uint32_t m_timeoutSec;
    bool          m_initialized;
};

} // namespace hub::core::utils