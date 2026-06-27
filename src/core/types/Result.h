#pragma once

namespace hub::core {

enum class Result {
    Ok = 0,
    Error,
    InvalidArgument,
    InvalidState,
    AlreadyInitialized,
    NotInitialized,
    Timeout,
    Unsupported,
    OutOfMemory
};

} // namespace hub::core