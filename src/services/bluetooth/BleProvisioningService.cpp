#include "BleProvisioningService.h"
#include <ArduinoJson.h>

namespace {

// UUID должны совпадать с BleUuids в Flutter-приложении
constexpr char kServiceUuid[]     = "4fafc201-1fb5-459e-8fcc-c5c9c331914b";
constexpr char kWriteCharUuid[]   = "beb5483e-36e1-4688-b7f5-ea07361b26a8"; // wifiConfig
constexpr char kNotifyCharUuid[]  = "cba1d466-344c-4be3-ab31-10701b551fe1"; // hubResponse

constexpr size_t kTokenBufSize = 64;
constexpr size_t kPinBufSize   = 8;

} // namespace

namespace hub::services {

// ─── Callbacks ───────────────────────────────────────────────────────────────

class ProvisioningCallbacks final : public BLECharacteristicCallbacks {
public:
    explicit ProvisioningCallbacks(BleProvisioningService& service) noexcept
        : m_service(service) {}

    void onWrite(BLECharacteristic* characteristic) override {
        const std::string raw = characteristic->getValue();
        if (raw.empty()) return;

        StaticJsonDocument<256> doc;
        const DeserializationError err = deserializeJson(doc, raw);

        if (err) {
            m_service.m_pendingResponse = BleProvisioningService::makeError("invalid_json");
            return;
        }

        const char* command = doc["command"] | "";

        if (strcmp(command, "get_status") == 0) {
            _handleGetStatus();
        } else if (strcmp(command, "setup") == 0) {
            _handleSetup(doc);
        } else if (strcmp(command, "auth") == 0) {
            _handleAuth(doc);
        } else {
            m_service.m_pendingResponse = BleProvisioningService::makeError("unknown_command");
        }
    }

private:
    BleProvisioningService& m_service;

    void _handleGetStatus() {
        const bool configured = m_service.m_handler.onGetStatus();
        // {"status": "ok", "configured": true|false}
        m_service.m_pendingResponse = BleProvisioningService::makeOk(
            String(R"("configured":)") + (configured ? "true" : "false")
        );
    }

    void _handleSetup(const JsonDocument& doc) {
        const char* ssid = doc["ssid"] | "";
        const char* pass = doc["pass"] | "";

        if (ssid[0] == '\0' || pass[0] == '\0') {
            m_service.m_pendingResponse = BleProvisioningService::makeError("missing_fields");
            return;
        }

        char token[kTokenBufSize] = {};
        char pin[kPinBufSize]     = {};

        const bool ok = m_service.m_handler.onSetup(ssid, pass, token, pin);
        if (!ok) {
            m_service.m_pendingResponse = BleProvisioningService::makeError("wifi_failed");
            return;
        }

        // {"status": "ok", "token": "...", "pin": "..."}
        m_service.m_pendingResponse = BleProvisioningService::makeOk(
            String(R"("token":")") + token + R"(","pin":")") + pin + "\""
        );
    }

    void _handleAuth(const JsonDocument& doc) {
        const char* pin = doc["pin"] | "";

        if (pin[0] == '\0') {
            m_service.m_pendingResponse = BleProvisioningService::makeError("missing_fields");
            return;
        }

        char token[kTokenBufSize] = {};

        const bool ok = m_service.m_handler.onAuth(pin, token);
        if (!ok) {
            m_service.m_pendingResponse = BleProvisioningService::makeError("wrong_pin");
            return;
        }

        // {"status": "ok", "token": "..."}
        m_service.m_pendingResponse = BleProvisioningService::makeOk(
            String(R"("token":")") + token + "\""
        );
    }
};

// ─── Lifecycle ───────────────────────────────────────────────────────────────

hub::core::Result BleProvisioningService::begin() {
    if (m_active) return hub::core::Result::Ok;

    BLEDevice::init("SmartNest-Hub");
    m_server = BLEDevice::createServer();

    BLEService* service = m_server->createService(kServiceUuid);

    // Характеристика для приёма команд (WRITE)
    m_writeChar = service->createCharacteristic(
        kWriteCharUuid,
        BLECharacteristic::PROPERTY_WRITE
    );
    m_writeChar->setCallbacks(new ProvisioningCallbacks(*this));

    // Характеристика для отправки ответов (NOTIFY)
    m_notifyChar = service->createCharacteristic(
        kNotifyCharUuid,
        BLECharacteristic::PROPERTY_NOTIFY
    );
    m_notifyChar->addDescriptor(new BLE2902()); // обязательно для notify

    service->start();

    BLEAdvertising* advertising = BLEDevice::getAdvertising();
    advertising->addServiceUUID(kServiceUuid);
    advertising->setScanResponse(true);
    advertising->setMinPreferred(0x06);
    advertising->setMinPreferred(0x12);
    BLEDevice::startAdvertising();

    m_active = true;
    Serial.println("[BleProvisioningService] BLE started");
    return hub::core::Result::Ok;
}

hub::core::Result BleProvisioningService::update() {
    if (!m_active) return hub::core::Result::Ok;

    // Отправляем ответ если он появился после onWrite
    if (!m_pendingResponse.isEmpty() && m_notifyChar != nullptr) {
        m_notifyChar->setValue(m_pendingResponse.c_str());
        m_notifyChar->notify();
        m_pendingResponse = "";
    }

    return hub::core::Result::Ok;
}

hub::core::Result BleProvisioningService::shutdown() {
    if (!m_active) return hub::core::Result::Ok;

    BLEDevice::stopAdvertising();
    BLEDevice::deinit(true);

    m_server      = nullptr;
    m_writeChar   = nullptr;
    m_notifyChar  = nullptr;
    m_active      = false;
    m_pendingResponse = "";

    Serial.println("[BleProvisioningService] BLE stopped");
    return hub::core::Result::Ok;
}

bool BleProvisioningService::isActive() const {
    return m_active;
}

// ─── Helpers ─────────────────────────────────────────────────────────────────

String BleProvisioningService::makeOk(const String& extra) {
    if (extra.isEmpty()) {
        return R"({"status":"ok"})";
    }
    return String(R"({"status":"ok",)") + extra + "}";
}

String BleProvisioningService::makeError(const char* message) {
    return String(R"({"status":"error","message":")") + message + "\"}";
}

} // namespace hub::services