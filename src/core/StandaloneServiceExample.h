#pragma once

#include "ServiceRunner.h"
#include "../services/network/WiFiService.h"
#include "utils/OwnerTokenStorage.h"

namespace hub::core {

/// Заглушка логгера для изолированного тестирования сервиса.
class NullLogger : public ILogger {
public:
    Result begin()    override { return Result::Ok; }
    Result update()   override { return Result::Ok; }
    Result shutdown() override { return Result::Ok; }
    void info(std::string_view)  override {}
    void warn(std::string_view)  override {}
    void error(std::string_view) override {}
};

class StandaloneServiceExample {
public:
    StandaloneServiceExample() noexcept
        : m_wifiService(m_nullLogger)
        , m_runner(m_wifiService)
        , m_ownerStorage() {}

    Result begin() {
        return m_runner.runOnce();
    }

    Result update() {
        return m_runner.runOnce();
    }

    Result shutdown() {
        return m_runner.shutdown();
    }

    hub::services::WiFiService& wifiService() {
        return m_wifiService;
    }

    hub::core::utils::OwnerTokenStorage& ownerStorage() {
        return m_ownerStorage;
    }

    /// Получить статус через BLE-интерфейс
    bool onGetStatus() { return m_wifiService.onGetStatus(); }
    bool onSetup(const char* ssid, const char* pass, char* token) {
        return m_wifiService.onSetup(ssid, pass, token);
    }
    void enableAccessPoint() { m_wifiService.enableAccessPoint(); }
    void disableBleProvisioning() { m_wifiService.disableBleProvisioning(); }

private:
    NullLogger m_nullLogger;
    hub::services::WiFiService m_wifiService;
    ServiceRunner m_runner;
    hub::core::utils::OwnerTokenStorage m_ownerStorage;
};

} // namespace hub::core
