#pragma once

#include "../../core/interfaces/IService.h"

namespace hub::services {

class WiFiService final : public hub::core::IService {
public:
    WiFiService() = default;

    hub::core::Result begin() override;
    hub::core::Result update() override;
    hub::core::Result shutdown() override;

private:
    void connectToNetwork();
    void startAccessPoint();

private:
    unsigned long m_lastConnectionAttemptMs = 0;
    bool          m_isConnected = false;
    bool          m_isProvisioningMode = false;
};

} // namespace hub::services
