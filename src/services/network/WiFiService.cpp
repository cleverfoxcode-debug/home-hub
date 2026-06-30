#include "WiFiService.h"
#include <Arduino.h>
#include <Preferences.h>
#include <WiFi.h>
#include <ESPmDNS.h>
#include <Config.h>
#include <cstdlib>

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

// Размеры для генерации токена и PIN
constexpr size_t kTokenSize = 64;
constexpr size_t kPinSize   = 8;

// NVS namespace
constexpr char kNvsNamespace[] = "wifi";
constexpr char kNvsConfigured[] = "configured";
constexpr char kNvsSsid[] = "ssid";
constexpr char kNvsPassword[] = "password";
constexpr char kNvsToken[] = "token";
constexpr char kNvsPin[] = "pin";

// Символы для генерации токена
constexpr char kTokenChars[] = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz";
constexpr size_t kTokenCharsCount = sizeof(kTokenChars) - 1;

// Символы для генерации PIN
constexpr char kPinChars[] = "0123456789";
constexpr size_t kPinCharsCount = sizeof(kPinChars) - 1;
}

WiFiService::WiFiService(hub::core::ILogger& logger) noexcept
    : m_logger(logger) {
    // Инициализируем генератор случайных чисел
    randomSeed(analogRead(0));
}

// ─── IBleProvisioningHandler ──────────────────────────────────────────────────

bool WiFiService::onGetStatus() {
    loadStoredConfiguration();
    return m_hasStoredConfig;
}

bool WiFiService::onSetup(const char* ssid, const char* pass,
                          char* outToken) {
    if (ssid == nullptr || pass == nullptr || outToken == nullptr) {
        m_logger.error("WiFi onSetup: invalid arguments");
        return false;
    }

    // Сохраняем SSID и пароль в NVS
    saveStoredConfiguration(String(ssid), String(pass));

    // Генерируем токен
    generateToken(outToken, kTokenSize);

    // Сохраняем токен
    {
        Preferences preferences;
        if (preferences.begin(kNvsNamespace, false)) {
            preferences.putString(kNvsToken, outToken);
            preferences.end();
        }
    }
    m_storedToken = outToken;

    // Пытаемся подключиться к Wi-Fi
    transitionTo(ConnectionState::Connecting);
    WiFi.mode(WIFI_STA);
    WiFi.setAutoReconnect(true);
    connectToNetwork();

    // Ждём подключения не дольше 10 секунд
    unsigned long start = millis();
    while (millis() - start < 10000) {
        if (WiFi.status() == WL_CONNECTED) {
            transitionTo(ConnectionState::Connected);
            m_lastErrorMessage = "";
            m_logger.info("WiFi setup: connected successfully");
            // После успешной настройки отключаем provisioning
            disableBleProvisioning();
            return true;
        }
        delay(100);
    }

    // Если не подключились — возвращаем false, но данные сохранены
    m_logger.warn("WiFi setup: connection timeout, credentials saved");
    transitionTo(ConnectionState::SetupMode);
    m_lastErrorMessage = "Wi-Fi credentials saved but connection timeout";
    return false;
}

void WiFiService::enableAccessPoint() {
    // Включаем standalone AP-режим по команде из приложения
    startAccessPoint();
    m_logger.info("WiFi AP enabled by user command");
}

void WiFiService::disableBleProvisioning() {
    // После успешной настройки отключаем provisioning-режим
    // BLE-стек остаётся активным, но provisioning UUID больше не рекламируется
    m_isProvisioningMode = false;
    m_logger.info("WiFi BLE provisioning disabled");
}

// ─── Приватные методы ─────────────────────────────────────────────────────────

void WiFiService::generateToken(char* outToken, size_t size) {
    if (size < 2) return;
    for (size_t i = 0; i < size - 1; ++i) {
        outToken[i] = kTokenChars[rand() % kTokenCharsCount];
    }
    outToken[size - 1] = '\0';
}

void WiFiService::generatePin(char* outPin, size_t size) {
    if (size < 2) return;
    for (size_t i = 0; i < size - 1; ++i) {
        outPin[i] = kPinChars[rand() % kPinCharsCount];
    }
    outPin[size - 1] = '\0';
}

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
    if (!preferences.begin(kNvsNamespace, true)) {
        m_logger.error("WiFi failed to read preferences");
        return;
    }

    m_hasStoredConfig = preferences.getBool(kNvsConfigured, false);
    if (m_hasStoredConfig) {
        m_storedSsid = preferences.getString(kNvsSsid, "").c_str();
        m_storedPassword = preferences.getString(kNvsPassword, "").c_str();
        m_storedToken = preferences.getString(kNvsToken, "").c_str();
        m_storedPin = preferences.getString(kNvsPin, "").c_str();
        m_logger.info("WiFi loaded stored configuration");
    } else {
        m_storedSsid = "";
        m_storedPassword = "";
        m_storedToken = "";
        m_storedPin = "";
    }

    preferences.end();
}

void WiFiService::saveStoredConfiguration(const String& ssid, const String& password) {
    Preferences preferences;
    if (!preferences.begin(kNvsNamespace, false)) {
        m_logger.error("WiFi failed to write preferences");
        return;
    }

    preferences.putBool(kNvsConfigured, true);
    preferences.putString(kNvsSsid, ssid);
    preferences.putString(kNvsPassword, password);
    preferences.end();

    m_hasStoredConfig = true;
    m_storedSsid = ssid;
    m_storedPassword = password;
    m_logger.info("WiFi saved configuration");
}

void WiFiService::clearStoredConfiguration() {
    Preferences preferences;
    if (!preferences.begin(kNvsNamespace, false)) {
        m_logger.error("WiFi failed to clear preferences");
        return;
    }

    preferences.clear();
    preferences.end();

    m_hasStoredConfig = false;
    m_storedSsid = "";
    m_storedPassword = "";
    m_storedToken = "";
    m_storedPin = "";
    m_logger.info("WiFi cleared settings");
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

bool WiFiService::isNetworkReady() const {
    return m_state == ConnectionState::Connected ||
           m_state == ConnectionState::SetupMode;
}

void WiFiService::transitionTo(ConnectionState newState) {
    if (m_state == newState) {
        return;
    }

    m_state = newState;
    m_isProvisioningMode = (newState == ConnectionState::SetupMode);
    m_isConnected = (newState == ConnectionState::Connected);
}

// ─── IService ─────────────────────────────────────────────────────────────────

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

            // Выводим IP-адрес в лог
            const IPAddress ip = WiFi.localIP();
            m_logger.info(std::string("WiFi connected, IP: ") + ip.toString().c_str());

            // Запускаем mDNS — устройство будет доступно как home-hub.local
            if (MDNS.begin(config::OTAHostname)) {
                m_logger.info(std::string("mDNS started: ") + config::OTAHostname + ".local");
            } else {
                m_logger.warn("mDNS failed to start");
            }
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

    return hub::core::Result::Ok;
}

hub::core::Result WiFiService::shutdown() {
    MDNS.end();
    WiFi.disconnect(true);
    WiFi.mode(WIFI_OFF);
    m_isConnected = false;
    m_lastConnectionAttemptMs = 0;
    m_logger.info("WiFi shutdown");
    return hub::core::Result::Ok;
}

} // namespace hub::services