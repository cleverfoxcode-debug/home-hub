#pragma once

#include <string>
#include <cctype>
#include <algorithm>

namespace hub::core::utils {

enum class WifiDebugCommand {
    None,
    Help,
    Status,
    StartBle,
    StartAp,
    Connect,
    Reset,
    Scan
};

inline std::string trimCopy(const std::string& value) {
    const auto begin = std::find_if_not(value.begin(), value.end(), [](unsigned char ch) {
        return std::isspace(ch) != 0;
    });

    if (begin == value.end()) {
        return {};
    }

    const auto end = std::find_if_not(value.rbegin(), value.rend(), [](unsigned char ch) {
        return std::isspace(ch) != 0;
    }).base();

    return std::string(begin, end);
}

inline WifiDebugCommand parseWifiDebugCommand(const std::string& input) {
    const std::string command = trimCopy(input);
    if (command.empty()) {
        return WifiDebugCommand::None;
    }

    if (command == "wifi help" || command == "help") {
        return WifiDebugCommand::Help;
    }
    if (command == "wifi status" || command == "status") {
        return WifiDebugCommand::Status;
    }
    if (command == "wifi ble" || command == "ble") {
        return WifiDebugCommand::StartBle;
    }
    if (command == "wifi ap" || command == "ap") {
        return WifiDebugCommand::StartAp;
    }
    if (command == "wifi connect" || command == "connect") {
        return WifiDebugCommand::Connect;
    }
    if (command == "wifi reset" || command == "reset") {
        return WifiDebugCommand::Reset;
    }
    if (command == "wifi scan" || command == "scan") {
        return WifiDebugCommand::Scan;
    }

    return WifiDebugCommand::None;
}

} // namespace hub::core::utils
