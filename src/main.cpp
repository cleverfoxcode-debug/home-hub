#include <Arduino.h>
#include "app/Application.h"
#include "services/logger/SerialLogger.h"
#include "services/network/WiFiService.h"
#include "services/heartbeat/HeartbeatService.h"
#include "services/ota/OTAServer.h"

static hub::services::SerialLogger     logger;
static hub::services::WiFiService      wifiService;
static hub::services::OTAServer        otaServer;
static hub::services::HeartbeatService heartbeatService;
static hub::core::IService*            services[] = {
    &logger,
    &wifiService,
    &otaServer,
    &heartbeatService,
};
static hub::app::Application app(logger, services, sizeof(services) / sizeof(services[0]));

void setup() {
    app.begin();
}

void loop() {
    app.update();
}