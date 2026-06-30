#pragma once

#include <Arduino.h>
#include "../types/Result.h"

namespace hub::core::utils {

/**
 * @brief Хранилище токена владельца (админа) в NVS.
 *
 * Используется для сохранения токена, полученного при первичной
 * настройке через BLE. Токен используется для авторизации админа
 * в WebSocket API (будущая функциональность).
 */
class OwnerTokenStorage {
public:
    explicit OwnerTokenStorage(const char* namespaceName = "owner") noexcept
        : m_namespaceName(namespaceName) {}

    /**
     * @brief Сохраняет токен владельца в NVS.
     * @param token Токен для сохранения.
     * @return Result::Ok при успехе.
     */
    Result save(const String& token);

    /**
     * @brief Загружает токен владельца из NVS.
     * @param outToken Буфер для токена.
     * @return Result::Ok если токен найден, Result::NotFound если нет.
     */
    Result load(String& outToken);

    /**
     * @brief Проверяет, сохранён ли токен владельца.
     * @return true если токен существует.
     */
    bool hasOwner() const;

    /**
     * @brief Проверяет, совпадает ли токен с сохранённым.
     * @param token Токен для проверки.
     * @return true если токен верный.
     */
    bool verify(const char* token) const;

    /**
     * @brief Удаляет токен владельца из NVS.
     */
    void clear();

private:
    const char* m_namespaceName;
};

} // namespace hub::core::utils