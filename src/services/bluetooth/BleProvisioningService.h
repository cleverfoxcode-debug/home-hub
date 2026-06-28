#pragma once

#include <Arduino.h>
#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include "../../core/interfaces/IService.h"

namespace hub::services {

class BleProvisioningService final : public hub::core::IService {
public:
    BleProvisioningService() = default;

    hub::core::Result begin() override;
    hub::core::Result update() override;
    hub::core::Result shutdown() override;

    bool isActive() const;
    void setResponse(const String& response);
    String response() const;

private:
    bool m_active = false;
    String m_response;
};

} // namespace hub::services
