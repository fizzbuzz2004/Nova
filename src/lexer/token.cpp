#include "lexer/token.hpp"

namespace nova {

std::string_view token_type_name(TokenType type) noexcept {
    switch (type) {
        case TokenType::LeftParen: return "'('";
        case TokenType::RightParen: return "')'";
        case TokenType::LeftBrace: return "'{'";
        case TokenType::RightBrace: return "'}'";
        case TokenType::Comma: return "','";
        case TokenType::Semicolon: return "';'";
        case TokenType::Colon: return "':'";
        case TokenType::Plus: return "'+'";
        case TokenType::Minus: return "'-'";
        case TokenType::Star: return "'*'";
        case TokenType::Slash: return "'/'";
        case TokenType::Percent: return "'%'";
        case TokenType::Bang: return "'!'";
        case TokenType::BangEqual: return "'!='";
        case TokenType::Equal: return "'='";
        case TokenType::EqualEqual: return "'=='";
        case TokenType::Less: return "'<'";
        case TokenType::LessEqual: return "'<='";
        case TokenType::Greater: return "'>'";
        case TokenType::GreaterEqual: return "'>='";
        case TokenType::AndAnd: return "'&&'";
        case TokenType::OrOr: return "'||'";
        case TokenType::Arrow: return "'->'";
        case TokenType::Identifier: return "identifier";
        case TokenType::Integer: return "integer";
        case TokenType::Let: return "'let'";
        case TokenType::Fn: return "'fn'";
        case TokenType::If: return "'if'";
        case TokenType::Else: return "'else'";
        case TokenType::While: return "'while'";
        case TokenType::Return: return "'return'";
        case TokenType::Print: return "'print'";
        case TokenType::True: return "'true'";
        case TokenType::False: return "'false'";
        case TokenType::Int: return "'int'";
        case TokenType::Bool: return "'bool'";
        case TokenType::EndOfFile: return "end of file";
    }
    return "unknown token";
}

} // namespace nova
