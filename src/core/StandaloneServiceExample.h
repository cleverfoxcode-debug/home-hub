#pragma once

#include "ServiceRunner.h"
#include "../services/network/WiFiService.h"

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
        , m_runner(m_wifiService) {}

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

private:
    NullLogger m_nullLogger;
    hub::services::WiFiService m_wifiService;
    ServiceRunner m_runner;
};

} // namespace hub::core
