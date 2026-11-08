#pragma once

#include <cstddef>
#include <stdexcept>
#include <string>
#include <utility>

namespace nova {

struct SourcePosition {
    std::size_t offset{0};
    std::size_t line{1};
    std::size_t column{1};
};

class CompileError final : public std::runtime_error {
public:
    CompileError(std::string message, SourcePosition position)
        : std::runtime_error(std::move(message)), position_(position) {}

    [[nodiscard]] const SourcePosition& position() const noexcept { return position_; }

private:
    SourcePosition position_;
};

} // namespace nova
