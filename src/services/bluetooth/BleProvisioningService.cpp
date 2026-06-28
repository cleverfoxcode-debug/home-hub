#include "BleProvisioningService.h"

namespace {
constexpr char kBleServiceUuid[] = "4fafc201-1fb5-459e-8fcc-c5c9c5c5d4b0";
constexpr char kBleCharacteristicUuid[] = "beb5483e-36e1-4688-bd8c-ccde5f4e4f0d";
} // namespace

namespace hub::services {

class ProvisioningCallbacks final : public BLECharacteristicCallbacks {
public:
    explicit ProvisioningCallbacks(hub::core::IBleProvisioningHandler& handler) noexcept
        : m_handler(handler) {}

    void onWrite(BLECharacteristic* characteristic) override {
        const std::string value = characteristic->getValue();
        if (value.empty()) {
            return;
        }

        const std::string input(value.begin(), value.end());
        if (input == "SCAN") {
            characteristic->setValue("SCAN_REQUESTED");
            m_handler.onProvisioningScanRequest();
            return;
        }

        const size_t separator = input.find('|');
        if (separator == std::string::npos) {
            characteristic->setValue("ERR:format");
            return;
        }

        const std::string ssid = input.substr(0, separator);
        const std::string password = input.substr(separator + 1);

        m_handler.onProvisioningCredentials(ssid.c_str(), password.c_str());
        characteristic->setValue("OK");
    }

private:
    hub::core::IBleProvisioningHandler& m_handler;
};

hub::core::Result BleProvisioningService::begin() {
    if (m_active) {
        return hub::core::Result::Ok;
    }

    BLEDevice::init("HomeHub-Setup");
    m_server = BLEDevice::createServer();
    BLEService* service = m_server->createService(kBleServiceUuid);
    m_characteristic = service->createCharacteristic(
        kBleCharacteristicUuid,
        BLECharacteristic::PROPERTY_READ | BLECharacteristic::PROPERTY_WRITE);

    m_characteristic->setCallbacks(new ProvisioningCallbacks(m_handler));
    m_characteristic->setValue("ready");
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

    // Забираем результат сканирования из handler'а
    const char* scanResponse = m_handler.consumeBleScanResponse();
    if (scanResponse != nullptr && scanResponse[0] != '\0' && m_characteristic != nullptr) {
        m_characteristic->setValue(scanResponse);
        m_characteristic->notify();
    }

    if (!m_response.isEmpty() && m_characteristic != nullptr) {
        m_characteristic->setValue(m_response.c_str());
        m_characteristic->notify();
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
    m_server = nullptr;
    m_characteristic = nullptr;
    m_active = false;
    m_response = "";
    Serial.println("[BleProvisioningService] BLE stopped");
    return hub::core::Result::Ok;
}

void BleProvisioningService::setResponse(const String& response) {
    m_response = response;
}

bool BleProvisioningService::isActive() const {
    return m_active;
}

} // namespace hub::services