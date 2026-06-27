#pragma once
#include <string_view>
#include "../../core/interfaces/ILogger.h"

namespace hub::services {

/**
 * @brief Логгер через Serial (UART0).
 *
 * Реализует ILogger — зависеть нужно от интерфейса, а не от этого класса.
 * final запрещает наследование: этот класс — конечная реализация.
 */
class SerialLogger final : public hub::core::ILogger {
public:
    SerialLogger() = default;

    // IService
    hub::core::Result begin()    override;
    hub::core::Result update()   override { return hub::core::Result::Ok; }
    hub::core::Result shutdown() override;

    // ILogger
    void info(std::string_view message)  override;
    void warn(std::string_view message)  override;
    void error(std::string_view message) override;
};

} // namespace hub::services
