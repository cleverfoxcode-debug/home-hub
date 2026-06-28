#include "Application.h"

namespace hub::app {

Application::Application(hub::core::ILogger& logger, hub::core::IService* const* services, std::size_t serviceCount) noexcept
    : m_logger(logger)
    , m_serviceManager(services, serviceCount)
{
}

hub::core::Result Application::begin() {
    m_logger.info("Application begin invoked");
    const auto result = m_serviceManager.beginAll();
    if (result != hub::core::Result::Ok) {
        m_logger.error("Application failed to start");
        return result;
    }

    m_logger.info("=== Home Hub starting ===");
    m_logger.info("Application ready");
    return hub::core::Result::Ok;
}

hub::core::Result Application::update() {
    // Если запрошен shutdown, выполняем его и больше не обновляем сервисы
    if (m_shutdownRequested) {
        m_shutdownRequested = false;
        return shutdown();
    }

    return m_serviceManager.updateAll();
}

hub::core::Result Application::shutdown() {
    m_logger.info("Application shutting down");
    const auto result = m_serviceManager.shutdownAll();
    m_logger.info("Application shutdown complete");
    return result;
}

void Application::requestShutdown() {
    m_shutdownRequested = true;
}

bool Application::isShutdownRequested() const {
    return m_shutdownRequested;
}

} // namespace hub::app