#pragma once

#include <cstddef>
#include "../core/Core.h"
#include "../core/ServiceManager.h"
#include "../core/interfaces/ILogger.h"

namespace hub::app {

/**
 * @brief Корневой объект приложения.
 *
 * Application владеет сервисами и управляет их жизненным циклом.
 * Внутри используется менеджер сервисов, что упрощает добавление
 * новых модулей и гарантирует упорядоченное создание/закрытие.
 *
 * Поддерживает graceful shutdown: при вызове requestShutdown()
 * в следующем цикле update() будет вызван shutdown() для всех сервисов.
 */
class Application : public hub::core::NonCopyable {
public:
    Application(hub::core::ILogger& logger, hub::core::IService* const* services, std::size_t serviceCount) noexcept;

    hub::core::Result begin();
    hub::core::Result update();
    hub::core::Result shutdown();

    /// Запросить graceful shutdown. shutdown() будет вызван в следующем update().
    void requestShutdown();

    /// true, если был запрошен shutdown.
    bool isShutdownRequested() const;

private:
    hub::core::ILogger&      m_logger;
    hub::core::ServiceManager m_serviceManager;
    bool                      m_shutdownRequested = false;
};

} // namespace hub::app