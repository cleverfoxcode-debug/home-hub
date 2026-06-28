#pragma once

#include "interfaces/IService.h"

namespace hub::core {

class ServiceRunner {
public:
    explicit ServiceRunner(IService& service) noexcept
        : m_service(service) {}

    Result runOnce() {
        if (!m_started) {
            m_started = true;
            return m_service.begin();
        }

        return m_service.update();
    }

    Result shutdown() {
        if (!m_shutdown) {
            m_shutdown = true;
            return m_service.shutdown();
        }

        return Result::Ok;
    }

private:
    IService& m_service;
    bool m_started = false;
    bool m_shutdown = false;
};

} // namespace hub::core
