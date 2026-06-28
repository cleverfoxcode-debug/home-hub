#pragma once

#include <cstdint>

#ifndef WIFI_SSID
#define WIFI_SSID "YOUR_SSID"
#endif

#ifndef WIFI_PASSWORD
#define WIFI_PASSWORD "YOUR_PASSWORD"
#endif

#ifndef AP_SSID
#define AP_SSID "HomeHub-Setup"
#endif

#ifndef AP_PASSWORD
#define AP_PASSWORD "homehub123"
#endif

#ifndef OTA_HOSTNAME
#define OTA_HOSTNAME "home-hub"
#endif

#ifndef OTA_PASSWORD
#define OTA_PASSWORD "homehub"
#endif

#ifndef STATUS_LED_PIN
#define STATUS_LED_PIN 2
#endif

#ifndef WATCHDOG_TIMEOUT_SEC
#define WATCHDOG_TIMEOUT_SEC 10
#endif

namespace config {
    constexpr std::uint32_t SerialBaudRate = 115200;
    constexpr std::uint8_t  StatusLedPin   = STATUS_LED_PIN;
    constexpr std::uint32_t WatchdogTimeoutSec = WATCHDOG_TIMEOUT_SEC;
    constexpr const char*    WiFiSsid       = WIFI_SSID;
    constexpr const char*    WiFiPassword   = WIFI_PASSWORD;
    constexpr const char*    OTAHostname    = OTA_HOSTNAME;
    constexpr const char*    OTAPassword    = OTA_PASSWORD;
    constexpr const char*    AccessPointSsid = AP_SSID;
    constexpr const char*    AccessPointPassword = AP_PASSWORD;
}