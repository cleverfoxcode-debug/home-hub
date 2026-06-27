#include "WiFiService.h"
#include <WiFi.h>
#include <Config.h>

namespace hub::services {

hub::core::Result WiFiService::begin() {
    WiFi.mode(WIFI_STA);
    WiFi.begin(config::WiFiSsid, config::WiFiPassword);

    const auto start = millis();
    while (WiFi.status() != WL_CONNECTED) {
        if (millis() - start > 10000) {
            return hub::core::Result::Timeout;
        }
        delay(100);
    }

    return hub::core::Result::Ok;
}

hub::core::Result WiFiService::update() {
    if (WiFi.status() != WL_CONNECTED) {
        return hub::core::Result::InvalidState;
    }
    return hub::core::Result::Ok;
}

hub::core::Result WiFiService::shutdown() {
    WiFi.disconnect(true);
    WiFi.mode(WIFI_OFF);
    return hub::core::Result::Ok;
}

} // namespace hub::services
