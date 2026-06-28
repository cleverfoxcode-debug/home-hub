#pragma once

#include <Arduino.h>
#include "../../core/interfaces/IService.h"
#include "../../core/interfaces/ILogger.h"
#include "../../core/interfaces/IBleProvisioningHandler.h"

namespace hub::services {

class WiFiService final : public hub::core::IService
                        , public hub::core::IBleProvisioningHandler {
public:
    enum class ConnectionState {
        Booting,
        Connecting,
        Connected,
        Recovering,
        SetupMode
    };

    explicit WiFiService(hub::core::ILogger& logger) noexcept;

    hub::core::Result begin() override;
    hub::core::Result update() override;
    hub::core::Result shutdown() override;

    // IBleProvisioningHandler
    void onProvisioningCredentials(const char* ssid, const char* password) override;
    void onProvisioningScanRequest() override;
    const char* consumeBleScanResponse() override;

    void resetStoredWiFiSettings();
    ConnectionState state() const;
    bool requiresSetup() const;
    const String& lastErrorMessage() const;
    const String& lastBleResponse() const;

private:
    void connectToNetwork();
    void startAccessPoint();
    bool hasVisibleHomeNetwork() const;
    void loadStoredConfiguration();
    void saveStoredConfiguration(const String& ssid, const String& password);
    void clearStoredConfiguration();
    void transitionTo(ConnectionState newState);
    void startBleProvisioning();
    void stopBleProvisioning();

private:
    hub::core::ILogger& m_logger;
    unsigned long m_lastConnectionAttemptMs = 0;
    unsigned int  m_failedConnectionAttempts = 0;
    unsigned int  m_routerRecoveryAttempts = 0;
    bool          m_isConnected = false;
    bool          m_isProvisioningMode = false;
    bool          m_hasStoredConfig = false;
    bool          m_hasConnectedBefore = false;
    bool          m_bleProvisioningActive = false;
    bool          m_bleScanRequested = false;
    bool          m_bleScanInProgress = false;
    ConnectionState m_state = ConnectionState::Booting;
    String        m_storedSsid;
    String        m_storedPassword;
    String        m_lastErrorMessage;
    String        m_bleResponse;
};

} // namespace hub::services