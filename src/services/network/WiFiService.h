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
    bool onGetStatus() override;
    bool onSetup(const char* ssid, const char* pass,
                 char* outToken) override;
    void enableAccessPoint() override;
    void disableBleProvisioning() override;

    void resetStoredWiFiSettings();
    ConnectionState state() const;
    bool requiresSetup() const;
    const String& lastErrorMessage() const;
    bool isNetworkReady() const;  // Wi-Fi подключён ИЛИ AP активен

private:
    void connectToNetwork();
    void startAccessPoint();
    bool hasVisibleHomeNetwork() const;
    void loadStoredConfiguration();
    void saveStoredConfiguration(const String& ssid, const String& password);
    void clearStoredConfiguration();
    void transitionTo(ConnectionState newState);
    void generateToken(char* outToken, size_t size);

private:
    hub::core::ILogger& m_logger;
    unsigned long m_lastConnectionAttemptMs = 0;
    unsigned int  m_failedConnectionAttempts = 0;
    unsigned int  m_routerRecoveryAttempts = 0;
    bool          m_isConnected = false;
    bool          m_isProvisioningMode = false;
    bool          m_hasStoredConfig = false;
    bool          m_hasConnectedBefore = false;
    ConnectionState m_state = ConnectionState::Booting;
    String        m_storedSsid;
    String        m_storedPassword;
    String        m_storedToken;
    String        m_lastErrorMessage;
};

} // namespace hub::services