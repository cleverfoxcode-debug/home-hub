#pragma once

#include "../../core/interfaces/IService.h"

namespace hub::services {

class WiFiService final : public hub::core::IService {
public:
    WiFiService() = default;

    hub::core::Result begin() override;
    hub::core::Result update() override;
    hub::core::Result shutdown() override;
};

} // namespace hub::services
