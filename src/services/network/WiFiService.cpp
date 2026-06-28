#include "WiFiService.h"
#include <Arduino.h>
#include <Preferences.h>
#include <WiFi.h>
#include <Config.h>
#include "../../core/utils/WifiDebugCommands.h"

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
// Глобальные указатели и текущий сервис Wi-Fi.
hub::services::WiFiService* g_wifiService = nullptr;
}

void WiFiService::connectToNetwork() {
    // Если уже подключены к Wi-Fi, повторное подключение не нужно.
    if (WiFi.status() == WL_CONNECTED) {
        return;
    }

    // Не пытаемся подключаться слишком часто подряд, чтобы не перегружать модуль.
    if (m_lastConnectionAttemptMs != 0 && (millis() - m_lastConnectionAttemptMs) < kRetryIntervalMs) {
        return;
    }

    m_lastConnectionAttemptMs = millis();
    ++m_failedConnectionAttempts;

    // Если у нас есть сохранённые данные, используем их.
    // Иначе пробуем использовать значения по умолчанию из конфигурации.
    const char* ssid = m_hasStoredConfig ? m_storedSsid.c_str() : config::WiFiSsid;
    const char* password = m_hasStoredConfig ? m_storedPassword.c_str() : config::WiFiPassword;

    Serial.print("[WiFiService] trying to connect to Wi-Fi: ");
    Serial.println(ssid);
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
    // Поднимаем собственную точку доступа только в случае реального отсутствия подходящей Wi-Fi сети.
    // Это аварийный fallback, а не режим первичной настройки.
    WiFi.mode(WIFI_AP);
    const bool started = WiFi.softAP(config::AccessPointSsid, config::AccessPointPassword);
    if (started) {
        transitionTo(ConnectionState::SetupMode);
        if (!m_bleProvisioningActive) {
            startBleProvisioning();
        }
        m_lastErrorMessage = "No visible Wi-Fi networks or saved router disappeared. AP fallback enabled.";
        Serial.println("[WiFiService] AP fallback started");
    }
}

void WiFiService::loadStoredConfiguration() {
    // Читаем сохранённые SSID и пароль из non-volatile памяти ESP32.
    Preferences preferences;
    if (!preferences.begin("wifi", true)) {
        Serial.println("[WiFiService] failed to read Wi-Fi preferences");
        return;
    }

    // Проверяем, были ли сохранены настройки Wi-Fi.
    m_hasStoredConfig = preferences.getBool("configured", false);
    if (m_hasStoredConfig) {
        m_storedSsid = preferences.getString("ssid", "").c_str();
        m_storedPassword = preferences.getString("password", "").c_str();
        Serial.println("[WiFiService] loaded stored Wi-Fi configuration");
    } else {
        m_storedSsid = "";
        m_storedPassword = "";
    }

    preferences.end();
}

void WiFiService::saveStoredConfiguration(const String& ssid, const String& password) {
    // Сохраняем SSID и пароль в память ESP32, чтобы после перезагрузки хаб снова мог подключиться к Wi-Fi.
    Preferences preferences;
    if (!preferences.begin("wifi", false)) {
        Serial.println("[WiFiService] failed to write Wi-Fi preferences");
        return;
    }

    preferences.putBool("configured", true);
    preferences.putString("ssid", ssid);
    preferences.putString("password", password);
    preferences.end();

    m_hasStoredConfig = true;
    m_storedSsid = ssid;
    m_storedPassword = password;
    Serial.println("[WiFiService] saved Wi-Fi configuration");
}

void WiFiService::clearStoredConfiguration() {
    // Удаляем сохранённые настройки, чтобы принудительно заставить пользователя пройти повторную настройку.
    Preferences preferences;
    if (!preferences.begin("wifi", false)) {
        Serial.println("[WiFiService] failed to clear Wi-Fi preferences");
        return;
    }

    preferences.clear();
    preferences.end();

    m_hasStoredConfig = false;
    m_storedSsid = "";
    m_storedPassword = "";
    Serial.println("[WiFiService] cleared Wi-Fi settings");
}

void WiFiService::setProvisioningCredentials(const char* ssid, const char* password) {
    // Это внешняя точка входа для данных, которые пришли во время первичной настройки.
    if (ssid == nullptr || password == nullptr) {
        return;
    }

    saveStoredConfiguration(String(ssid), String(password));
}

void WiFiService::applyProvisioningCredentials(const char* ssid, const char* password) {
    // Принимаем SSID и пароль из BLE-процесса настройки и пробуем подключиться.
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
        stopBleProvisioning();
        Serial.println("[WiFiService] BLE credentials received and connection established");
    } else {
        transitionTo(ConnectionState::SetupMode);
        m_lastErrorMessage = "BLE credentials received. Waiting for connection.";
        Serial.println("[WiFiService] BLE credentials received. Waiting for connection.");
    }
}

void WiFiService::resetStoredWiFiSettings() {
    // Сбрасываем сохранённые настройки и переводим устройство обратно в BLE-режим настройки.
    clearStoredConfiguration();
    WiFi.disconnect(true);
    WiFi.mode(WIFI_STA);
    WiFi.setAutoReconnect(true);
    transitionTo(ConnectionState::SetupMode);
    startBleProvisioning();
    m_lastErrorMessage = "Wi-Fi settings reset. Please reconfigure via BLE.";
    Serial.println("[WiFiService] Wi-Fi settings reset, BLE provisioning enabled");
}

void WiFiService::requestBleNetworkScan() {
    // Запускаем поиск доступных Wi-Fi сетей и подготовим ответ для BLE-клиента.
    if (!m_bleProvisioningActive) {
        return;
    }

    m_bleScanRequested = true;
    m_bleScanInProgress = true;
    m_bleResponse = "";
    Serial.println("[WiFiService] BLE scan requested");
}

void WiFiService::startBleProvisioning() {
    if (m_bleProvisioningActive) {
        return;
    }

    m_bleProvisioningService.begin();
    m_bleProvisioningActive = m_bleProvisioningService.isActive();
    g_wifiService = this;
    Serial.println("[WiFiService] BLE provisioning started");
}

void WiFiService::stopBleProvisioning() {
    if (!m_bleProvisioningActive) {
        return;
    }

    m_bleProvisioningService.shutdown();
    g_wifiService = nullptr;
    m_bleProvisioningActive = false;
    Serial.println("[WiFiService] BLE provisioning stopped");
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

void WiFiService::transitionTo(ConnectionState newState) {
    // Переключаем внутреннее состояние сервиса.
    // Это помогает понять, что сейчас делает хаб: подключается, подключён, восстанавливается или находится в режиме настройки.
    if (m_state == newState) {
        return;
    }

    m_state = newState;
    m_isProvisioningMode = (newState == ConnectionState::SetupMode);
    m_isConnected = (newState == ConnectionState::Connected);
}

hub::core::Result WiFiService::begin() {
    // Настраиваем кнопку сброса на GPIO0.
    // Если её удержать, будут удалены сохранённые Wi-Fi настройки.
    pinMode(kResetButtonPin, INPUT_PULLUP);

    if (digitalRead(kResetButtonPin) == LOW) {
        Serial.println("[WiFiService] reset button pressed");
        resetStoredWiFiSettings();
        return hub::core::Result::Ok;
    }

    // Загружаем сохранённые данные Wi‑Fi, если они есть.
    // Если данных нет, сразу переходим в BLE-режим первичной настройки.
    loadStoredConfiguration();
    if (!m_hasStoredConfig) {
        transitionTo(ConnectionState::SetupMode);
        startBleProvisioning();
        m_lastErrorMessage = "No saved Wi-Fi settings. Waiting for BLE provisioning.";
        return hub::core::Result::Ok;
    }

    transitionTo(ConnectionState::Connecting);
    WiFi.mode(WIFI_STA);
    WiFi.setAutoReconnect(true);
    connectToNetwork();

    return hub::core::Result::Ok;
}

hub::core::Result WiFiService::update() {
    // Главный цикл работы сервиса.
    // Здесь решается, подключены ли мы к домашней сети, пытаемся ли переподключиться или находимся в режиме настройки.
    if (Serial.available() > 0) {
        std::string input;
        while (Serial.available() > 0) {
            char c = static_cast<char>(Serial.read());
            if (c == '\n' || c == '\r') {
                break;
            }
            input.push_back(c);
        }

        const auto command = hub::core::utils::parseWifiDebugCommand(input);
        switch (command) {
            case hub::core::utils::WifiDebugCommand::Help:
                Serial.println("wifi help | wifi status | wifi ble | wifi ap | wifi connect | wifi reset | wifi scan");
                break;
            case hub::core::utils::WifiDebugCommand::Status:
                Serial.printf("state=%d connected=%d ble=%d\n", static_cast<int>(m_state), m_isConnected ? 1 : 0, m_bleProvisioningActive ? 1 : 0);
                break;
            case hub::core::utils::WifiDebugCommand::StartBle:
                startBleProvisioning();
                Serial.println("BLE provisioning started");
                break;
            case hub::core::utils::WifiDebugCommand::StartAp:
                startAccessPoint();
                Serial.println("AP fallback requested");
                break;
            case hub::core::utils::WifiDebugCommand::Connect:
                WiFi.mode(WIFI_STA);
                WiFi.setAutoReconnect(true);
                connectToNetwork();
                Serial.println("Wi-Fi connection attempt started");
                break;
            case hub::core::utils::WifiDebugCommand::Reset:
                resetStoredWiFiSettings();
                Serial.println("Wi-Fi settings reset requested");
                break;
            case hub::core::utils::WifiDebugCommand::Scan:
                requestBleNetworkScan();
                Serial.println("BLE scan requested");
                break;
            default:
                break;
        }
    }

    if (WiFi.status() == WL_CONNECTED) {
        if (m_state != ConnectionState::Connected) {
            transitionTo(ConnectionState::Connected);
            m_hasConnectedBefore = true;
            m_failedConnectionAttempts = 0;
            m_routerRecoveryAttempts = 0;
            m_lastErrorMessage = "";
            Serial.println("[WiFiService] connected");
        }
        return hub::core::Result::Ok;
    }

    if (m_state == ConnectionState::Connected) {
        // У нас была связь, но теперь её нет.
        // Сначала делаем несколько попыток переподключения, а не сразу уходить в режим настройки.
        transitionTo(ConnectionState::Recovering);
        m_routerRecoveryAttempts = 0;
        m_lastErrorMessage = "Home Wi-Fi connection lost. Trying to reconnect first.";
        Serial.println("[WiFiService] home Wi-Fi connection lost. Trying to reconnect first.");
        return hub::core::Result::Ok;
    }

    if (m_state == ConnectionState::SetupMode && WiFi.softAPgetStationNum() > 0) {
        Serial.println("[WiFiService] provisioning client connected");
    }

    if (m_state != ConnectionState::SetupMode) {
        // Пытаемся подключиться к домашней сети, если мы не в режиме настройки.
        connectToNetwork();

        if (m_hasStoredConfig && m_failedConnectionAttempts >= kMaxFailedAttempts) {
            ++m_routerRecoveryAttempts;
            if (m_routerRecoveryAttempts >= kRouterRecoveryAttempts) {
                if (!hasVisibleHomeNetwork()) {
                    m_routerRecoveryAttempts = 0;
                    m_failedConnectionAttempts = 0;
                    m_lastErrorMessage = "No visible Wi-Fi networks or saved router disappeared. Switching to AP fallback.";
                    Serial.println("[WiFiService] no visible Wi-Fi networks or saved router disappeared. Switching to AP fallback.");
                    stopBleProvisioning();
                    startAccessPoint();
                } else {
                    m_lastErrorMessage = "Wi-Fi reconnect attempt failed. Waiting and retrying.";
                    Serial.print("[WiFiService] reconnect retry ");
                    Serial.print(m_routerRecoveryAttempts);
                    Serial.println(" of 3");
                }
            } else {
                m_lastErrorMessage = "Wi-Fi reconnect attempt failed. Waiting and retrying.";
                Serial.print("[WiFiService] reconnect retry ");
                Serial.print(m_routerRecoveryAttempts);
                Serial.println(" of 3");
            }
        }
    }

    if (m_state == ConnectionState::SetupMode && m_hasStoredConfig) {
        // Если роутер вернётся, пока мы в режиме настройки, пробуем подключиться к нему повторно.
        WiFi.mode(WIFI_STA);
        WiFi.setAutoReconnect(true);
        connectToNetwork();
        if (WiFi.status() == WL_CONNECTED) {
            transitionTo(ConnectionState::Connected);
            m_failedConnectionAttempts = 0;
            m_routerRecoveryAttempts = 0;
            m_lastErrorMessage = "";
            Serial.println("[WiFiService] home Wi-Fi reappeared. Returning to normal mode.");
        }
    }

    if (m_bleScanRequested && m_bleProvisioningActive) {
        // Выполняем поиск доступных Wi-Fi сетей по BLE-команде и готовим ответ для приложения.
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

        m_bleProvisioningService.setResponse(m_bleResponse);
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
