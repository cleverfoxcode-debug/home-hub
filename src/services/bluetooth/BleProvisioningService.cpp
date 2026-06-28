#include "BleProvisioningService.h"

namespace {
constexpr char kBleServiceUuid[] = "4fafc201-1fb5-459e-8fcc-c5c9c5c5d4b0";
constexpr char kBleCharacteristicUuid[] = "beb5483e-36e1-4688-bd8c-ccde5f4e4f0d";

BLEServer* g_bleServer = nullptr;
BLECharacteristic* g_bleCharacteristic = nullptr;

class ProvisioningCallbacks final : public BLECharacteristicCallbacks {
public:
    void onWrite(BLECharacteristic* characteristic) override {
        const std::string value = characteristic->getValue();
        if (value.empty()) {
            return;
        }

        const std::string input(value.begin(), value.end());
        if (input == "SCAN") {
            characteristic->setValue("SCAN_REQUESTED");
            return;
        }

        const size_t separator = input.find('|');
        if (separator == std::string::npos) {
            characteristic->setValue("ERR:format");
            return;
        }

        characteristic->setValue("OK");
    }
};
} // namespace

namespace hub::services {

hub::core::Result BleProvisioningService::begin() {
    if (m_active) {
        return hub::core::Result::Ok;
    }

    BLEDevice::init("HomeHub-Setup");
    g_bleServer = BLEDevice::createServer();
    BLEService* service = g_bleServer->createService(kBleServiceUuid);
    g_bleCharacteristic = service->createCharacteristic(
        kBleCharacteristicUuid,
        BLECharacteristic::PROPERTY_READ | BLECharacteristic::PROPERTY_WRITE);

    g_bleCharacteristic->setCallbacks(new ProvisioningCallbacks());
    g_bleCharacteristic->setValue("ready");
    service->start();

    BLEAdvertising* advertising = BLEDevice::getAdvertising();
    advertising->addServiceUUID(kBleServiceUuid);
    advertising->setScanResponse(true);
    advertising->setMinPreferred(0x06);
    advertising->setMinPreferred(0x12);
    BLEDevice::startAdvertising();

    m_active = true;
    Serial.println("[BleProvisioningService] BLE started");
    return hub::core::Result::Ok;
}

hub::core::Result BleProvisioningService::update() {
    if (!m_active) {
        return hub::core::Result::Ok;
    }

    if (!m_response.isEmpty() && g_bleCharacteristic != nullptr) {
        g_bleCharacteristic->setValue(m_response.c_str());
        g_bleCharacteristic->notify();
        m_response = "";
    }

    return hub::core::Result::Ok;
}

hub::core::Result BleProvisioningService::shutdown() {
    if (!m_active) {
        return hub::core::Result::Ok;
    }

    BLEDevice::stopAdvertising();
    BLEDevice::deinit(true);
    g_bleServer = nullptr;
    g_bleCharacteristic = nullptr;
    m_active = false;
    m_response = "";
    Serial.println("[BleProvisioningService] BLE stopped");
    return hub::core::Result::Ok;
}

bool BleProvisioningService::isActive() const {
    return m_active;
}

void BleProvisioningService::setResponse(const String& response) {
    m_response = response;
}

String BleProvisioningService::response() const {
    return m_response;
}

} // namespace hub::services
