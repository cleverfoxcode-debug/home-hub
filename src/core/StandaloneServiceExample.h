#pragma once

#include "ServiceRunner.h"
#include "../services/network/WiFiService.h"

namespace hub::core {

class StandaloneServiceExample {
public:
    StandaloneServiceExample() noexcept
        : m_runner(m_wifiService) {}

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
    hub::services::WiFiService m_wifiService;
    ServiceRunner m_runner;
};

} // namespace hub::core
