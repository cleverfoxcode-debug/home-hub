#pragma once

#include <Arduino.h>
#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLE2902.h>
#include "../../core/interfaces/IService.h"
#include "../../core/interfaces/IBleProvisioningHandler.h"
#include "../../core/utils/OwnerTokenStorage.h"

namespace hub::services {

/**
 * @brief BLE-сервер для первичной настройки хаба.
 *
 * Реализует две характеристики:
 *   - wifiConfig  (WRITE)  — приём JSON-команд от Flutter
 *   - hubResponse (NOTIFY) — отправка JSON-ответов во Flutter
 *
 * Команды: get_status | setup | auth
 */
class BleProvisioningService final : public hub::core::IService {
public:
    explicit BleProvisioningService(hub::core::IBleProvisioningHandler& handler,
                                    hub::core::utils::OwnerTokenStorage& ownerStorage) noexcept
        : m_handler(handler)
        , m_ownerStorage(ownerStorage) {}

    hub::core::Result begin()    override;
    hub::core::Result update()   override;
    hub::core::Result shutdown() override;

    /// Отключить provisioning-режим (перестать рекламировать provisioning UUID).
    /// BLE-стек остаётся активным.
    void disableProvisioning();

    bool isActive() const;
    bool isProvisioningActive() const;

private:
    hub::core::IBleProvisioningHandler&      m_handler;
    hub::core::utils::OwnerTokenStorage&     m_ownerStorage;

    BLEServer*         m_server         = nullptr;
    BLECharacteristic* m_writeChar      = nullptr; // wifiConfig — WRITE
    BLECharacteristic* m_notifyChar     = nullptr; // hubResponse — NOTIFY

    String m_pendingResponse; // ответ для отправки в update()
    bool   m_active = false;
    bool   m_provisioningActive = true;  // provisioning активен до disableProvisioning()

    /// Отправляет JSON-ответ через notify-характеристику.
    void sendResponse(const String& json);

    /// Формирует JSON-ответ успеха с произвольными полями.
    static String makeOk(const String& extra = "");

    /// Формирует JSON-ответ ошибки.
    static String makeError(const char* message);

    /// Сохранить токен владельца.
    void saveOwnerToken(const String& token);
    /// Проверить, есть ли токен.
    bool hasOwner() const;
    /// Проверить токен.
    bool verifyOwnerToken(const char* token) const;

    friend class ProvisioningCallbacks;
};

} // namespace hub::services