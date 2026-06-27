#pragma once
#include <string_view>
#include "IService.h"

namespace hub::core {

/**
 * @brief Интерфейс логгера.
 *
 * Зависеть нужно от этого интерфейса, а не от конкретной реализации.
 * Это позволит в будущем заменить Serial-логгер на SD-логгер или UDP-логгер
 * без изменения кода, который использует логгер.
 */
class ILogger : public IService {
public:
    virtual void info(std::string_view message)  = 0;
    virtual void warn(std::string_view message)  = 0;
    virtual void error(std::string_view message) = 0;
};

} // namespace hub::core
