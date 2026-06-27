#include "SerialLogger.h"
#include <Arduino.h>
#include <Config.h>

namespace hub::services {

hub::core::Result SerialLogger::begin() {
    Serial.begin(config::SerialBaudRate);
    return hub::core::Result::Ok;
}

hub::core::Result SerialLogger::shutdown() {
    Serial.end();
    return hub::core::Result::Ok;
}

static void printLine(const char* prefix, std::string_view message) {
    Serial.print(prefix);
    Serial.write(message.data(), static_cast<size_t>(message.size()));
    Serial.println();
}

void SerialLogger::info(std::string_view message) {
    printLine("[INFO]  ", message);
}

void SerialLogger::warn(std::string_view message) {
    printLine("[WARN]  ", message);
}

void SerialLogger::error(std::string_view message) {
    printLine("[ERROR] ", message);
}

} // namespace hub::services
