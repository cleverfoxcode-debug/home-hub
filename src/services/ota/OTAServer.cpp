#include "OTAServer.h"
#include <ArduinoOTA.h>
#include <Config.h>

namespace hub::services {

hub::core::Result OTAServer::begin() {
    ArduinoOTA.setHostname(config::OTAHostname);
    ArduinoOTA.setPassword(config::OTAPassword);

    ArduinoOTA.onStart([]() {
        // Можно добавить логику начала OTA
    });

    ArduinoOTA.onEnd([]() {
        // OTA завершена
    });

    ArduinoOTA.onError([](ota_error_t error) {
        // Ошибка OTA
    });

    ArduinoOTA.begin();
    return hub::core::Result::Ok;
}

hub::core::Result OTAServer::update() {
    ArduinoOTA.handle();
    return hub::core::Result::Ok;
}

hub::core::Result OTAServer::shutdown() {
    return hub::core::Result::Ok;
}

} // namespace hub::services
