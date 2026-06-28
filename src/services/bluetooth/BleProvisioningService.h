#pragma once

#include <Arduino.h>
#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include "../../core/interfaces/IService.h"
#include "../../core/interfaces/IBleProvisioningHandler.h"

namespace hub::services {

class BleProvisioningService final : public hub::core::IService {
public:
    explicit BleProvisioningService(hub::core::IBleProvisioningHandler& handler) noexcept
        : m_handler(handler) {}

    hub::core::Result begin() override;
    hub::core::Result update() override;
    hub::core::Result shutdown() override;

    /// Установить ответ для BLE-клиента (список Wi-Fi сетей).
    void setResponse(const String& response);
    bool isActive() const;

private:
    hub::core::IBleProvisioningHandler& m_handler;
    BLEServer*           m_server = nullptr;
    BLECharacteristic*   m_characteristic = nullptr;
    String               m_response;
    bool                 m_active = false;
};

} // namespace hub::services