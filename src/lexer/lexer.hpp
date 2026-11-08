#pragma once

#include "lexer/token.hpp"

#include <string_view>
#include <vector>

namespace nova {

class Lexer final {
public:
    explicit Lexer(std::string_view source) noexcept : source_(source) {}
    [[nodiscard]] std::vector<Token> scan_tokens();

private:
    [[nodiscard]] bool at_end() const noexcept;
    [[nodiscard]] char peek() const noexcept;
    [[nodiscard]] char peek_next() const noexcept;
    char advance() noexcept;
    bool match(char expected) noexcept;
    void skip_trivia();
    void scan_token();
    void scan_number();
    void scan_identifier();
    void add_token(TokenType type);

    std::string_view source_;
    std::vector<Token> tokens_;
    std::size_t start_{0};
    std::size_t current_{0};
    std::size_t line_{1};
    std::size_t column_{1};
    SourcePosition token_start_{};
};

} // namespace nova
