#pragma once

#include "ast/statement.hpp"
#include "lexer/token.hpp"

#include <vector>

namespace nova {

class Parser final {
public:
    explicit Parser(const std::vector<Token>& tokens) noexcept : tokens_(tokens) {}
    [[nodiscard]] Program parse();

private:
    [[nodiscard]] bool at_end() const noexcept;
    [[nodiscard]] const Token& peek() const noexcept;
    [[nodiscard]] const Token& previous() const noexcept;
    bool match(TokenType type) noexcept;
    const Token& consume(TokenType type, const char* message);

    StmtPtr declaration();
    StmtPtr function_declaration();
    StmtPtr variable_declaration();
    StmtPtr statement();
    StmtPtr print_statement();
    StmtPtr if_statement();
    StmtPtr while_statement();
    StmtPtr for_statement();
    StmtPtr return_statement();
    StmtPtr block_statement();
    StmtPtr expression_statement();
    std::vector<StmtPtr> block_contents();

    [[nodiscard]] Type parse_type();
    [[nodiscard]] ExprPtr expression();
    [[nodiscard]] ExprPtr parse_precedence(int minimum_precedence);
    [[nodiscard]] ExprPtr prefix();
    [[nodiscard]] ExprPtr finish_call(const Token& name);
    [[nodiscard]] static int precedence(TokenType type) noexcept;

    const std::vector<Token>& tokens_;
    std::size_t current_{0};
};

} // namespace nova
