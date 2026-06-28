#include "WiFiService.h"
#include <Arduino.h>
#include <WiFi.h>
#include <Config.h>
#include <WiFiClient.h>

namespace hub::services {

namespace {
constexpr unsigned long kRetryIntervalMs = 5000;
}

void WiFiService::connectToNetwork() {
    if (WiFi.status() == WL_CONNECTED) {
        return;
    }

    if (m_lastConnectionAttemptMs != 0 && (millis() - m_lastConnectionAttemptMs) < kRetryIntervalMs) {
        return;
    }

    m_lastConnectionAttemptMs = millis();
    Serial.println("[WiFiService] trying to connect to Wi-Fi...");
    WiFi.begin(config::WiFiSsid, config::WiFiPassword);
}

void WiFiService::startAccessPoint() {
    WiFi.mode(WIFI_AP);
    const bool started = WiFi.softAP(config::AccessPointSsid, config::AccessPointPassword);
    if (started) {
        m_isProvisioningMode = true;
        Serial.println("[WiFiService] AP mode started");
        Serial.print("[WiFiService] connect to: ");
        Serial.println(config::AccessPointSsid);
    }
}

hub::core::Result WiFiService::begin() {
    WiFi.mode(WIFI_STA);
    WiFi.setAutoReconnect(true);
    connectToNetwork();

    if (WiFi.status() != WL_CONNECTED) {
        startAccessPoint();
    }

    return hub::core::Result::Ok;
}

hub::core::Result WiFiService::update() {
    if (WiFi.status() == WL_CONNECTED) {
        if (!m_isConnected) {
            m_isConnected = true;
            m_isProvisioningMode = false;
            Serial.println("[WiFiService] connected");
        }
        return hub::core::Result::Ok;
    }

    m_isConnected = false;
    if (m_isProvisioningMode) {
        WiFiClient client = WiFi.softAPgetStationNum() > 0 ? WiFiClient() : WiFiClient();
        if (client) {
            Serial.println("[WiFiService] provisioning client connected");
        }
    }

    if (!m_isProvisioningMode) {
        connectToNetwork();
    }
    return hub::core::Result::Ok;
}

hub::core::Result WiFiService::shutdown() {
    WiFi.disconnect(true);
    WiFi.mode(WIFI_OFF);
    m_isConnected = false;
    m_lastConnectionAttemptMs = 0;
    return hub::core::Result::Ok;
}

} // namespace hub::services
