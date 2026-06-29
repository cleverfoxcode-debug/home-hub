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
     * @param ssid     SSID сети от клиента.
     * @param pass     Пароль сети от клиента.
     * @param outToken Буфер для токена (минимум 64 байта).
     * @param outPin   Буфер для PIN-кода (минимум 8 байт).
     * @return true при успехе, false при ошибке подключения к Wi-Fi.
     */
    virtual bool onSetup(const char* ssid, const char* pass,
                         char* outToken, char* outPin) = 0;

    /**
     * @brief Вызывается при команде auth — повторная аутентификация по PIN.
     * @param pin      PIN-код от клиента.
     * @param outToken Буфер для нового токена (минимум 64 байта).
     * @return true если PIN верный, false если неверный.
     */
    virtual bool onAuth(const char* pin, char* outToken) = 0;
};

} // namespace hub::core