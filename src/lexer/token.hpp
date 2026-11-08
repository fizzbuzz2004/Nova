#pragma once

#include "diagnostics/error.hpp"

#include <string>
#include <string_view>

namespace nova {

enum class TokenType {
    LeftParen, RightParen, LeftBrace, RightBrace,
    Comma, Semicolon, Colon,
    Plus, Minus, Star, Slash, Percent,
    Bang, BangEqual, Equal, EqualEqual,
    Less, LessEqual, Greater, GreaterEqual,
    AndAnd, OrOr, Arrow,
    Identifier, Integer,
    Let, Fn, If, Else, While, For, Return, Print,
    True, False, Int, Bool,
    EndOfFile
};

[[nodiscard]] std::string_view token_type_name(TokenType type) noexcept;

struct Token {
    TokenType type;
    std::string lexeme;
    SourcePosition position;
};

} // namespace nova
