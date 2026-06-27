#pragma once
#include "../types/Result.h"

namespace hub::core {

/**
 * @brief Базовый интерфейс для всех сервисов.
 *
 * Каждый сервис обязан реализовать три метода жизненного цикла:
 *   begin()    — инициализация (вызывается один раз при старте)
 *   update()   — периодическое обновление (вызывается в loop)
 *   shutdown() — остановка и освобождение ресурсов
 */
class IService {
public:
    virtual ~IService() = default;

    virtual Result begin()    = 0;
    virtual Result update()   = 0;
    virtual Result shutdown() = 0;
};

} // namespace hub::core