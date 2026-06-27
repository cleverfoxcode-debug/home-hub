#pragma once

namespace hub::core {

class NonMovable {
protected:
    NonMovable() = default;
    ~NonMovable() = default;

public:
    NonMovable(NonMovable&&)            = delete;
    NonMovable& operator=(NonMovable&&) = delete;
};

} // namespace hub::core
