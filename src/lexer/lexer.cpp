#include "lexer/lexer.hpp"

#include <cctype>
#include <unordered_map>

namespace nova {
namespace {

[[nodiscard]] bool is_digit(char c) noexcept {
    return c >= '0' && c <= '9';
}
[[nodiscard]] bool is_alpha(char c) noexcept {
    return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || c == '_';
}
[[nodiscard]] bool is_alphanumeric(char c) noexcept {
    return is_alpha(c) || is_digit(c);
}

} // namespace

std::vector<Token> Lexer::scan_tokens() {
    while (!at_end()) {
        skip_trivia();
        if (at_end()) break;
        start_ = current_;
        token_start_ = SourcePosition{current_, line_, column_};
        scan_token();
    }
    tokens_.push_back(Token{TokenType::EndOfFile, "", {current_, line_, column_}});
    return std::move(tokens_);
}

bool Lexer::at_end() const noexcept { return current_ >= source_.size(); }
char Lexer::peek() const noexcept { return at_end() ? '\0' : source_[current_]; }
char Lexer::peek_next() const noexcept {
    return current_ + 1 >= source_.size() ? '\0' : source_[current_ + 1];
}

char Lexer::advance() noexcept {
    const char value = source_[current_++];
    if (value == '\n') {
        ++line_;
        column_ = 1;
    } else {
        ++column_;
    }
    return value;
}

bool Lexer::match(char expected) noexcept {
    if (at_end() || peek() != expected) return false;
    (void)advance();
    return true;
}

void Lexer::skip_trivia() {
    for (;;) {
        const char c = peek();
        if (c == ' ' || c == '\r' || c == '\t' || c == '\n') {
            (void)advance();
        } else if (c == '/' && peek_next() == '/') {
            while (!at_end() && peek() != '\n') (void)advance();
        } else {
            return;
        }
    }
}

void Lexer::add_token(TokenType type) {
    tokens_.push_back(Token{type, std::string(source_.substr(start_, current_ - start_)), token_start_});
}

void Lexer::scan_token() {
    const char c = advance();
    switch (c) {
        case '(': add_token(TokenType::LeftParen); break;
        case ')': add_token(TokenType::RightParen); break;
        case '{': add_token(TokenType::LeftBrace); break;
        case '}': add_token(TokenType::RightBrace); break;
        case ',': add_token(TokenType::Comma); break;
        case ';': add_token(TokenType::Semicolon); break;
        case ':': add_token(TokenType::Colon); break;
        case '+': add_token(TokenType::Plus); break;
        case '-': add_token(match('>') ? TokenType::Arrow : TokenType::Minus); break;
        case '*': add_token(TokenType::Star); break;
        case '/': add_token(TokenType::Slash); break;
        case '%': add_token(TokenType::Percent); break;
        case '!': add_token(match('=') ? TokenType::BangEqual : TokenType::Bang); break;
        case '=': add_token(match('=') ? TokenType::EqualEqual : TokenType::Equal); break;
        case '<': add_token(match('=') ? TokenType::LessEqual : TokenType::Less); break;
        case '>': add_token(match('=') ? TokenType::GreaterEqual : TokenType::Greater); break;
        case '&':
            if (match('&')) add_token(TokenType::AndAnd);
            else throw CompileError("Expected '&' after '&'.", token_start_);
            break;
        case '|':
            if (match('|')) add_token(TokenType::OrOr);
            else throw CompileError("Expected '|' after '|'.", token_start_);
            break;
        default:
            if (is_digit(c)) scan_number();
            else if (is_alpha(c)) scan_identifier();
            else throw CompileError("Unexpected character '" + std::string(1, c) + "'.", token_start_);
            break;
    }
}

void Lexer::scan_number() {
    while (is_digit(peek())) (void)advance();
    add_token(TokenType::Integer);
}

void Lexer::scan_identifier() {
    while (is_alphanumeric(peek())) (void)advance();
    const std::string_view word = source_.substr(start_, current_ - start_);
    static const std::unordered_map<std::string_view, TokenType> keywords{
        {"let", TokenType::Let}, {"fn", TokenType::Fn},
        {"if", TokenType::If}, {"else", TokenType::Else},
        {"while", TokenType::While}, {"for", TokenType::For}, {"return", TokenType::Return},
        {"print", TokenType::Print}, {"true", TokenType::True},
        {"false", TokenType::False}, {"int", TokenType::Int}, {"bool", TokenType::Bool}
    };
    const auto it = keywords.find(word);
    add_token(it == keywords.end() ? TokenType::Identifier : it->second);
}

} // namespace nova
