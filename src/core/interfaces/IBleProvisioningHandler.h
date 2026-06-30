#pragma once

namespace hub::core {

/**
 * @brief Интерфейс обработки BLE-команд от мобильного приложения.
 *
 * BleProvisioningService вызывает методы этого интерфейса
 * при получении JSON-команд от Flutter-клиента.
 *
 * Протокол:
 *   запрос  → {"command": "...", ...}
 *   ответ   → {"status": "ok", ...} | {"status": "error", "message": "..."}
 */
class IBleProvisioningHandler {
public:
    virtual ~IBleProvisioningHandler() = default;

    /**
     * @brief Вызывается при команде get_status.
     * @return true если хаб уже настроен (есть сохранённый Wi-Fi).
     */
    virtual bool onGetStatus() = 0;

    /**
     * @brief Вызывается при команде setup — первичная настройка Wi-Fi.
     * @param ssid     SSID сети от клиента (может быть nullptr для AP-режима).
     * @param pass     Пароль сети от клиента (может быть nullptr для AP-режима).
     * @param outToken Буфер для токена (минимум 64 байта).
     * @return true при успехе, false при ошибке.
     */
    virtual bool onSetup(const char* ssid, const char* pass,
                         char* outToken) = 0;

    /**
     * @brief Вызывается при команде enable_ap — включение standalone AP-режима.
     * Хаб создаёт собственную сеть без роутера.
     */
    virtual void enableAccessPoint() = 0;

    /**
     * @brief Вызывается после успешной настройки Wi-Fi.
     * Provisioning Service должен отключить рекламу provisioning UUID,
     * но оставить BLE-стек активным для дальнейшего использования.
     */
    virtual void disableBleProvisioning() = 0;

    /**
     * @brief Проверить, готов ли сетевой интерфейс.
     * @return true если Wi-Fi подключён ИЛИ AP-режим активен.
     */
    virtual bool isNetworkReady() const = 0;
};

} // namespace hub::core
