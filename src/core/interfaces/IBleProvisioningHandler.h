#pragma once

#include <cstddef>

namespace hub::core {

/**
 * @brief Интерфейс для обработки событий BLE-провижининга.
 *
 * BleProvisioningService вызывает методы этого интерфейса
 * при получении данных от BLE-клиента.
 */
class IBleProvisioningHandler {
public:
    virtual ~IBleProvisioningHandler() = default;

    /// Вызывается при получении SSID и пароля от BLE-клиента.
    virtual void onProvisioningCredentials(const char* ssid, const char* password) = 0;

    /// Вызывается при запросе сканирования Wi-Fi сетей от BLE-клиента.
    virtual void onProvisioningScanRequest() = 0;

    /// Возвращает последний результат сканирования (пустая строка, если нового ответа нет).
    virtual const char* consumeBleScanResponse() = 0;
};

} // namespace hub::core
