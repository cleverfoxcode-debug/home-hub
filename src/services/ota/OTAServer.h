#pragma once

#include "../../core/interfaces/IService.h"

namespace hub::services {

class OTAServer final : public hub::core::IService {
public:
    OTAServer() = default;

    hub::core::Result begin() override;
    hub::core::Result update() override;
    hub::core::Result shutdown() override;
};

} // namespace hub::services
