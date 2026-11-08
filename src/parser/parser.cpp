#include "parser/parser.hpp"

#include <charconv>
#include <limits>
#include <string>
#include <system_error>
#include <utility>

namespace nova {

bool Parser::at_end() const noexcept { return peek().type == TokenType::EndOfFile; }
const Token& Parser::peek() const noexcept { return tokens_[current_]; }
const Token& Parser::previous() const noexcept { return tokens_[current_ - 1]; }

bool Parser::match(TokenType type) noexcept {
    if (at_end() || peek().type != type) return false;
    ++current_;
    return true;
}

const Token& Parser::consume(TokenType type, const char* message) {
    if (peek().type == type) return tokens_[current_++];
    throw CompileError(message, peek().position);
}

Program Parser::parse() {
    Program program;
    while (!at_end()) program.push_back(declaration());
    return program;
}

StmtPtr Parser::declaration() {
    if (match(TokenType::Fn)) return function_declaration();
    if (match(TokenType::Let)) return variable_declaration();
    return statement();
}

StmtPtr Parser::function_declaration() {
    const Token& name = consume(TokenType::Identifier, "Expected function name after 'fn'.");
    (void)consume(TokenType::LeftParen, "Expected '(' after function name.");
    std::vector<Parameter> parameters;
    if (peek().type != TokenType::RightParen) {
        do {
            const Token& parameter = consume(TokenType::Identifier, "Expected parameter name.");
            (void)consume(TokenType::Colon, "Expected ':' after parameter name.");
            parameters.push_back(Parameter{parameter.lexeme, parse_type(), parameter.position});
        } while (match(TokenType::Comma));
    }
    (void)consume(TokenType::RightParen, "Expected ')' after parameters.");
    (void)consume(TokenType::Arrow, "Expected '->' before function return type.");
    const Type return_type = parse_type();
    (void)consume(TokenType::LeftBrace, "Expected '{' before function body.");
    return std::make_unique<Stmt>(name.position,
        FunctionDeclaration{name.lexeme, std::move(parameters), return_type, block_contents()});
}

StmtPtr Parser::variable_declaration() {
    const Token& name = consume(TokenType::Identifier, "Expected variable name after 'let'.");
    std::optional<Type> declared_type;
    if (match(TokenType::Colon)) declared_type = parse_type();
    (void)consume(TokenType::Equal, "Expected '=' in variable declaration.");
    ExprPtr initializer = expression();
    (void)consume(TokenType::Semicolon, "Expected ';' after variable declaration.");
    return std::make_unique<Stmt>(name.position,
        VariableDeclaration{name.lexeme, declared_type, std::move(initializer)});
}

StmtPtr Parser::statement() {
    if (match(TokenType::Print)) return print_statement();
    if (match(TokenType::If)) return if_statement();
    if (match(TokenType::While)) return while_statement();
    if (match(TokenType::Return)) return return_statement();
    if (match(TokenType::LeftBrace)) return block_statement();
    return expression_statement();
}

StmtPtr Parser::print_statement() {
    const SourcePosition position = previous().position;
    (void)consume(TokenType::LeftParen, "Expected '(' after 'print'.");
    ExprPtr value = expression();
    (void)consume(TokenType::RightParen, "Expected ')' after print value.");
    (void)consume(TokenType::Semicolon, "Expected ';' after print statement.");
    return std::make_unique<Stmt>(position, PrintStatement{std::move(value)});
}

StmtPtr Parser::if_statement() {
    const SourcePosition position = previous().position;
    (void)consume(TokenType::LeftParen, "Expected '(' after 'if'.");
    ExprPtr condition = expression();
    (void)consume(TokenType::RightParen, "Expected ')' after condition.");
    (void)consume(TokenType::LeftBrace, "Expected '{' before if body.");
    StmtPtr then_branch = std::make_unique<Stmt>(position, BlockStatement{block_contents()});
    StmtPtr else_branch;
    if (match(TokenType::Else)) {
        if (match(TokenType::If)) {
            // if_statement() expects the 'if' keyword to have been consumed.
            else_branch = if_statement();
        } else {
            (void)consume(TokenType::LeftBrace, "Expected '{' before else body.");
            else_branch = std::make_unique<Stmt>(previous().position, BlockStatement{block_contents()});
        }
    }
    return std::make_unique<Stmt>(position,
        IfStatement{std::move(condition), std::move(then_branch), std::move(else_branch)});
}

StmtPtr Parser::while_statement() {
    const SourcePosition position = previous().position;
    (void)consume(TokenType::LeftParen, "Expected '(' after 'while'.");
    ExprPtr condition = expression();
    (void)consume(TokenType::RightParen, "Expected ')' after condition.");
    (void)consume(TokenType::LeftBrace, "Expected '{' before while body.");
    StmtPtr body = std::make_unique<Stmt>(position, BlockStatement{block_contents()});
    return std::make_unique<Stmt>(position, WhileStatement{std::move(condition), std::move(body)});
}

StmtPtr Parser::return_statement() {
    const SourcePosition position = previous().position;
    ExprPtr value = expression();
    (void)consume(TokenType::Semicolon, "Expected ';' after return value.");
    return std::make_unique<Stmt>(position, ReturnStatement{std::move(value)});
}

StmtPtr Parser::block_statement() {
    const SourcePosition position = previous().position;
    return std::make_unique<Stmt>(position, BlockStatement{block_contents()});
}

std::vector<StmtPtr> Parser::block_contents() {
    std::vector<StmtPtr> statements;
    while (!at_end() && peek().type != TokenType::RightBrace) {
        statements.push_back(declaration());
    }
    (void)consume(TokenType::RightBrace, "Expected '}' after block.");
    return statements;
}

StmtPtr Parser::expression_statement() {
    const SourcePosition position = peek().position;
    ExprPtr value = expression();
    (void)consume(TokenType::Semicolon, "Expected ';' after expression.");
    return std::make_unique<Stmt>(position, ExpressionStatement{std::move(value)});
}

Type Parser::parse_type() {
    if (match(TokenType::Int)) return Type::Int;
    if (match(TokenType::Bool)) return Type::Bool;
    throw CompileError("Expected type 'int' or 'bool'.", peek().position);
}

ExprPtr Parser::expression() { return parse_precedence(1); }

int Parser::precedence(TokenType type) noexcept {
    switch (type) {
        case TokenType::Equal: return 1;
        case TokenType::OrOr: return 2;
        case TokenType::AndAnd: return 3;
        case TokenType::EqualEqual: case TokenType::BangEqual: return 4;
        case TokenType::Less: case TokenType::LessEqual:
        case TokenType::Greater: case TokenType::GreaterEqual: return 5;
        case TokenType::Plus: case TokenType::Minus: return 6;
        case TokenType::Star: case TokenType::Slash: case TokenType::Percent: return 7;
        default: return 0;
    }
}

ExprPtr Parser::parse_precedence(int minimum_precedence) {
    ExprPtr left = prefix();
    while (!at_end()) {
        if (peek().type == TokenType::LeftParen) {
            if (9 < minimum_precedence) break;
            const auto* variable = std::get_if<VariableExpr>(&left->node);
            if (variable == nullptr) throw CompileError("Only named functions can be called.", peek().position);
            const Token name{TokenType::Identifier, variable->name, left->position};
            left = finish_call(name);
            continue;
        }
        const int current_precedence = precedence(peek().type);
        if (current_precedence < minimum_precedence || current_precedence == 0) break;
        const Token op = tokens_[current_++];
        const int next_minimum = op.type == TokenType::Equal ? current_precedence : current_precedence + 1;
        ExprPtr right = parse_precedence(next_minimum);
        if (op.type == TokenType::Equal) {
            const SourcePosition position = left->position;
            const auto* variable = std::get_if<VariableExpr>(&left->node);
            if (variable == nullptr) throw CompileError("Invalid assignment target.", position);
            const std::string variable_name = variable->name;
            left = std::make_unique<Expr>(position,
                AssignExpr{variable_name, std::move(right)});
        } else {
            const SourcePosition position = left->position;
            left = std::make_unique<Expr>(position,
                BinaryExpr{std::move(left), op.type, std::move(right)});
        }
    }
    return left;
}

ExprPtr Parser::prefix() {
    const Token token = tokens_[current_++];
    switch (token.type) {
        case TokenType::Integer: {
            std::int64_t value{};
            const char* first = token.lexeme.data();
            const char* last = first + token.lexeme.size();
            const auto result = std::from_chars(first, last, value);
            if (result.ec == std::errc::result_out_of_range)
                throw CompileError("Integer literal is outside the supported 64-bit range.", token.position);
            if (result.ec != std::errc{} || result.ptr != last)
                throw CompileError("Invalid integer literal.", token.position);
            return std::make_unique<Expr>(token.position, IntegerExpr{value});
        }
        case TokenType::True: return std::make_unique<Expr>(token.position, BooleanExpr{true});
        case TokenType::False: return std::make_unique<Expr>(token.position, BooleanExpr{false});
        case TokenType::Identifier:
            return std::make_unique<Expr>(token.position, VariableExpr{token.lexeme});
        case TokenType::Minus: case TokenType::Bang: {
            ExprPtr operand = parse_precedence(8);
            return std::make_unique<Expr>(token.position, UnaryExpr{token.type, std::move(operand)});
        }
        case TokenType::LeftParen: {
            ExprPtr inner = expression();
            (void)consume(TokenType::RightParen, "Expected ')' after expression.");
            return inner;
        }
        default:
            throw CompileError("Expected an expression, found " + std::string(token_type_name(token.type)) + ".",
                               token.position);
    }
}

ExprPtr Parser::finish_call(const Token& name) {
    (void)consume(TokenType::LeftParen, "Expected '(' before function arguments.");
    std::vector<ExprPtr> arguments;
    if (peek().type != TokenType::RightParen) {
        do { arguments.push_back(expression()); } while (match(TokenType::Comma));
    }
    (void)consume(TokenType::RightParen, "Expected ')' after function arguments.");
    return std::make_unique<Expr>(name.position, CallExpr{name.lexeme, std::move(arguments)});
}

} // namespace nova
