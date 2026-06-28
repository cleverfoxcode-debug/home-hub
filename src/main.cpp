#include <Arduino.h>
#include "app/Application.h"
#include "services/logger/SerialLogger.h"

static hub::services::SerialLogger logger;

// ─── SERVICE_BLE ────────────────────────────────────────────
#if defined(SERVICE_BLE)
#include "services/network/WiFiService.h"
#include "services/bluetooth/BleProvisioningService.h"
static hub::services::WiFiService           wifiService(logger);
static hub::services::BleProvisioningService bleService(wifiService);
static hub::core::IService* services[] = { &logger, &bleService };

// ─── SERVICE_WIFI ───────────────────────────────────────────
#elif defined(SERVICE_WIFI)
#include "services/network/WiFiService.h"
static hub::services::WiFiService wifiService(logger);
static hub::core::IService* services[] = { &logger, &wifiService };

// ─── SERVICE_OTA ────────────────────────────────────────────
#elif defined(SERVICE_OTA)
#include "services/ota/OTAServer.h"
static hub::services::OTAServer otaServer;
static hub::core::IService* services[] = { &logger, &otaServer };

// ─── SERVICE_HEARTBEAT ──────────────────────────────────────
#elif defined(SERVICE_HEARTBEAT)
#include "services/heartbeat/HeartbeatService.h"
static hub::services::HeartbeatService heartbeatService;
static hub::core::IService* services[] = { &logger, &heartbeatService };

// ─── Все сервисы (по умолчанию) ────────────────────────────
#else
#include "services/network/WiFiService.h"
#include "services/bluetooth/BleProvisioningService.h"
#include "services/ota/OTAServer.h"
#include "services/heartbeat/HeartbeatService.h"
static hub::services::WiFiService           wifiService(logger);
static hub::services::BleProvisioningService bleService(wifiService);
static hub::services::OTAServer              otaServer;
static hub::services::HeartbeatService       heartbeatService;
static hub::core::IService* services[] = {
    &logger,
    &wifiService,
    &bleService,
    &otaServer,
    &heartbeatService,
};
#endif

static hub::app::Application app(logger, services, sizeof(services) / sizeof(services[0]));

/// Обработчик перезагрузки по запросу (например, после OTA или по кнопке).
/// Выполняет graceful shutdown всех сервисов перед перезагрузкой.
void gracefulRestart() {
    app.shutdown();
    delay(100);
    esp_restart();
}

void setup() {
    // Устанавливаем обработчик на перезагрузку по запросу
    // Например, при вызове ESP.restart() из OTA или по кнопке
    app.begin();
}

void loop() {
    hub::core::Result result = app.update();

    // Если какой-то сервис вернул ошибку, делаем shutdown и перезагружаемся
    if (result != hub::core::Result::Ok) {
        logger.error("Application update failed, restarting");
        gracefulRestart();
    }
}