#include "WiFiService.h"
#include <Arduino.h>
#include <Preferences.h>
#include <WiFi.h>
#include <Config.h>

namespace hub::services {

namespace {
// Пауза между попытками подключения, чтобы не нагружать Wi-Fi модуль.
constexpr unsigned long kRetryIntervalMs = 5000;
// После скольких неудачных попыток начать считать, что домашняя сеть недоступна.
constexpr unsigned int kMaxFailedAttempts = 5;
// Сколько попыток восстановления после потери связи с роутером сделать до fallback-режима.
constexpr unsigned int kRouterRecoveryAttempts = 3;
// GPIO, к которому подключена кнопка сброса настроек Wi-Fi.
constexpr uint8_t kResetButtonPin = 0;
}

WiFiService::WiFiService(hub::core::ILogger& logger) noexcept
    : m_logger(logger) {}

void WiFiService::connectToNetwork() {
    if (WiFi.status() == WL_CONNECTED) {
        return;
    }

    if (m_lastConnectionAttemptMs != 0 && (millis() - m_lastConnectionAttemptMs) < kRetryIntervalMs) {
        return;
    }

    m_lastConnectionAttemptMs = millis();
    ++m_failedConnectionAttempts;

    const char* ssid = m_hasStoredConfig ? m_storedSsid.c_str() : config::WiFiSsid;
    const char* password = m_hasStoredConfig ? m_storedPassword.c_str() : config::WiFiPassword;

    m_logger.info("WiFi trying to connect");
    WiFi.begin(ssid, password);
}

bool WiFiService::hasVisibleHomeNetwork() const {
    if (!m_hasStoredConfig || m_storedSsid.isEmpty()) {
        return false;
    }

    const int networkCount = WiFi.scanNetworks(true, true);
    if (networkCount <= 0) {
        return false;
    }

    for (int i = 0; i < networkCount && i < 10; ++i) {
        if (WiFi.SSID(i) == m_storedSsid) {
            return true;
        }
    }

    return false;
}

void WiFiService::startAccessPoint() {
    WiFi.mode(WIFI_AP);
    const bool started = WiFi.softAP(config::AccessPointSsid, config::AccessPointPassword);
    if (started) {
        transitionTo(ConnectionState::SetupMode);
        m_lastErrorMessage = "No visible Wi-Fi networks. AP fallback enabled.";
        m_logger.warn("WiFi AP fallback started");
    }
}

void WiFiService::loadStoredConfiguration() {
    Preferences preferences;
    if (!preferences.begin("wifi", true)) {
        m_logger.error("WiFi failed to read preferences");
        return;
    }

    m_hasStoredConfig = preferences.getBool("configured", false);
    if (m_hasStoredConfig) {
        m_storedSsid = preferences.getString("ssid", "").c_str();
        m_storedPassword = preferences.getString("password", "").c_str();
        m_logger.info("WiFi loaded stored configuration");
    } else {
        m_storedSsid = "";
        m_storedPassword = "";
    }

    preferences.end();
}

void WiFiService::saveStoredConfiguration(const String& ssid, const String& password) {
    Preferences preferences;
    if (!preferences.begin("wifi", false)) {
        m_logger.error("WiFi failed to write preferences");
        return;
    }

    preferences.putBool("configured", true);
    preferences.putString("ssid", ssid);
    preferences.putString("password", password);
    preferences.end();

    m_hasStoredConfig = true;
    m_storedSsid = ssid;
    m_storedPassword = password;
    m_logger.info("WiFi saved configuration");
}

void WiFiService::clearStoredConfiguration() {
    Preferences preferences;
    if (!preferences.begin("wifi", false)) {
        m_logger.error("WiFi failed to clear preferences");
        return;
    }

    preferences.clear();
    preferences.end();

    m_hasStoredConfig = false;
    m_storedSsid = "";
    m_storedPassword = "";
    m_logger.info("WiFi cleared settings");
}

void WiFiService::onProvisioningCredentials(const char* ssid, const char* password) {
    if (ssid == nullptr || password == nullptr) {
        return;
    }

    saveStoredConfiguration(String(ssid), String(password));
    transitionTo(ConnectionState::Connecting);
    WiFi.mode(WIFI_STA);
    WiFi.setAutoReconnect(true);
    connectToNetwork();

    if (WiFi.status() == WL_CONNECTED) {
        transitionTo(ConnectionState::Connected);
        m_lastErrorMessage = "";
        m_logger.info("WiFi BLE credentials received and connected");
    } else {
        transitionTo(ConnectionState::SetupMode);
        m_lastErrorMessage = "BLE credentials received. Waiting for connection.";
        m_logger.warn("WiFi BLE credentials received, waiting for connection");
    }
}

void WiFiService::onProvisioningScanRequest() {
    m_bleScanRequested = true;
    m_bleScanInProgress = true;
    m_bleResponse = "";
    m_logger.info("WiFi BLE scan requested");
}

void WiFiService::resetStoredWiFiSettings() {
    clearStoredConfiguration();
    WiFi.disconnect(true);
    WiFi.mode(WIFI_STA);
    WiFi.setAutoReconnect(true);
    transitionTo(ConnectionState::SetupMode);
    m_lastErrorMessage = "Wi-Fi settings reset. Please reconfigure via BLE.";
    m_logger.warn("WiFi settings reset");
}

WiFiService::ConnectionState WiFiService::state() const {
    return m_state;
}

bool WiFiService::requiresSetup() const {
    return m_state == ConnectionState::SetupMode;
}

const String& WiFiService::lastErrorMessage() const {
    return m_lastErrorMessage;
}

const String& WiFiService::lastBleResponse() const {
    return m_bleResponse;
}

const char* WiFiService::consumeBleScanResponse() {
    if (m_bleResponse.isEmpty()) {
        return "";
    }
    // Копируем во временный буфер, чтобы можно было безопасно очистить m_bleResponse
    static String result;
    result = m_bleResponse;
    m_bleResponse = "";
    return result.c_str();
}

void WiFiService::transitionTo(ConnectionState newState) {
    if (m_state == newState) {
        return;
    }

    m_state = newState;
    m_isProvisioningMode = (newState == ConnectionState::SetupMode);
    m_isConnected = (newState == ConnectionState::Connected);
}

hub::core::Result WiFiService::begin() {
    pinMode(kResetButtonPin, INPUT_PULLUP);

    if (digitalRead(kResetButtonPin) == LOW) {
        m_logger.warn("WiFi reset button pressed");
        resetStoredWiFiSettings();
        return hub::core::Result::Ok;
    }

    loadStoredConfiguration();
    if (!m_hasStoredConfig) {
        transitionTo(ConnectionState::SetupMode);
        m_lastErrorMessage = "No saved Wi-Fi settings.";
        m_logger.info("WiFi no saved settings, waiting for provisioning");
        return hub::core::Result::Ok;
    }

    transitionTo(ConnectionState::Connecting);
    WiFi.mode(WIFI_STA);
    WiFi.setAutoReconnect(true);
    connectToNetwork();

    return hub::core::Result::Ok;
}

hub::core::Result WiFiService::update() {
    if (WiFi.status() == WL_CONNECTED) {
        if (m_state != ConnectionState::Connected) {
            transitionTo(ConnectionState::Connected);
            m_hasConnectedBefore = true;
            m_failedConnectionAttempts = 0;
            m_routerRecoveryAttempts = 0;
            m_lastErrorMessage = "";
            m_logger.info("WiFi connected");
        }
        return hub::core::Result::Ok;
    }

    if (m_state == ConnectionState::Connected) {
        transitionTo(ConnectionState::Recovering);
        m_routerRecoveryAttempts = 0;
        m_lastErrorMessage = "Home Wi-Fi connection lost. Trying to reconnect.";
        m_logger.warn("WiFi connection lost, trying to reconnect");
        return hub::core::Result::Ok;
    }

    if (m_state == ConnectionState::SetupMode && WiFi.softAPgetStationNum() > 0) {
        // provisioning client connected
    }

    if (m_state != ConnectionState::SetupMode) {
        connectToNetwork();

        if (m_hasStoredConfig && m_failedConnectionAttempts >= kMaxFailedAttempts) {
            ++m_routerRecoveryAttempts;
            if (m_routerRecoveryAttempts >= kRouterRecoveryAttempts) {
                if (!hasVisibleHomeNetwork()) {
                    m_routerRecoveryAttempts = 0;
                    m_failedConnectionAttempts = 0;
                    m_lastErrorMessage = "No visible Wi-Fi networks. Switching to AP fallback.";
                    m_logger.warn("WiFi no visible networks, switching to AP fallback");
                    startAccessPoint();
                } else {
                    m_lastErrorMessage = "Wi-Fi reconnect attempt failed. Retrying.";
                    m_logger.warn("WiFi reconnect retrying");
                }
            } else {
                m_lastErrorMessage = "Wi-Fi reconnect attempt failed. Retrying.";
                m_logger.warn("WiFi reconnect retrying");
            }
        }
    }

    if (m_state == ConnectionState::SetupMode && m_hasStoredConfig) {
        WiFi.mode(WIFI_STA);
        WiFi.setAutoReconnect(true);
        connectToNetwork();
        if (WiFi.status() == WL_CONNECTED) {
            transitionTo(ConnectionState::Connected);
            m_failedConnectionAttempts = 0;
            m_routerRecoveryAttempts = 0;
            m_lastErrorMessage = "";
            m_logger.info("WiFi home network reappeared, returned to normal mode");
        }
    }

    if (m_bleScanRequested) {
        m_bleScanRequested = false;
        m_bleScanInProgress = false;
        int networkCount = WiFi.scanNetworks(true, true);
        if (networkCount > 0) {
            String networks;
            for (int i = 0; i < networkCount && i < 10; ++i) {
                if (i > 0) {
                    networks += ";";
                }
                networks += WiFi.SSID(i);
            }
            m_bleResponse = networks;
        } else {
            m_bleResponse = "NO_NETWORKS";
        }
        m_logger.info("WiFi BLE scan completed");
    }

    return hub::core::Result::Ok;
}

hub::core::Result WiFiService::shutdown() {
    WiFi.disconnect(true);
    WiFi.mode(WIFI_OFF);
    m_isConnected = false;
    m_lastConnectionAttemptMs = 0;
    m_logger.info("WiFi shutdown");
    return hub::core::Result::Ok;
}

} // namespace hub::services