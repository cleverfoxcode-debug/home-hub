#include "WatchdogManager.h"
#include <esp_task_wdt.h>

namespace hub::core::utils {

void WatchdogManager::begin(std::uint32_t timeoutSec) {
    if (m_initialized) {
        return;
    }

    // Если передан таймаут, используем его, иначе из конструктора
    if (timeoutSec > 0) {
        m_timeoutSec = timeoutSec;
    }

    // Инициализируем TWDT с заданным таймаутом
    esp_task_wdt_init(m_timeoutSec, true);  // true = перезагрузка при срабатывании
    esp_task_wdt_add(nullptr);              // добавляем текущую задачу (loop)
    m_initialized = true;
}

void WatchdogManager::feed() {
    if (!m_initialized) {
        return;
    }

    esp_task_wdt_reset();
}

void WatchdogManager::end() {
    if (!m_initialized) {
        return;
    }

    esp_task_wdt_delete(nullptr);
    esp_task_wdt_deinit();
    m_initialized = false;
}

} // namespace hub::core::utils