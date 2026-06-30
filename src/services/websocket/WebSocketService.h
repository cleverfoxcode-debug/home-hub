// #pragma once

// #include <Arduino.h>
// #include <WebSocketsServer.h>
// #include "../../core/interfaces/IService.h"
// #include "../../core/interfaces/ILogger.h"

// namespace hub::services {

// /**
//  * @brief WebSocket сервер для приёма команд от клиентов.
//  *
//  * Работает только когда Wi-Fi подключён (state == Connected).
//  * Все полученные сообщения выводятся в лог.
//  *
//  * Порт: 81 (по умолчанию)
//  */
// class WebSocketService final : public hub::core::IService {
// public:
//     explicit WebSocketService(hub::core::ILogger& logger) noexcept;

//     hub::core::Result begin() override;
//     hub::core::Result update() override;
//     hub::core::Result shutdown() override;

//     /// Включить/выключить сервер (вызывается из WiFiService)
//     void setEnabled(bool enabled);

// private:
//     void onEvent(uint8_t num, WStype_t type, uint8_t* payload, size_t length);

// private:
//     hub::core::ILogger&      m_logger;
//     WebSocketsServer*        m_server = nullptr;
//     bool                     m_enabled = false;
//     bool                     m_initialized = false;
// };

// } // namespace hub::services