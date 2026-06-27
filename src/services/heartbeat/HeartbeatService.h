#pragma once

#include "../../core/interfaces/IService.h"

namespace hub::services {

class HeartbeatService final : public hub::core::IService {
public:
    HeartbeatService() = default;

    hub::core::Result begin() override;
    hub::core::Result update() override;
    hub::core::Result shutdown() override;

private:
    unsigned long m_lastTick = 0;
};

} // namespace hub::services
