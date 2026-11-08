#pragma once

#include "ast/expression.hpp"

#include <memory>
#include <optional>
#include <string>
#include <vector>
#include <variant>
#include <utility>

namespace nova {

struct Stmt;
using StmtPtr = std::unique_ptr<Stmt>;

struct VariableDeclaration {
    std::string name;
    std::optional<Type> declared_type;
    ExprPtr initializer;
};
struct ExpressionStatement { ExprPtr expression; };
struct PrintStatement { ExprPtr expression; };
struct BlockStatement { std::vector<StmtPtr> statements; };
struct IfStatement { ExprPtr condition; StmtPtr then_branch; StmtPtr else_branch; };
struct WhileStatement { ExprPtr condition; StmtPtr body; };
struct Parameter { std::string name; Type type; SourcePosition position; };
struct FunctionDeclaration {
    std::string name;
    std::vector<Parameter> parameters;
    Type return_type;
    std::vector<StmtPtr> body;
};
struct ReturnStatement { ExprPtr value; };

using StmtNode = std::variant<VariableDeclaration, ExpressionStatement, PrintStatement,
                              BlockStatement, IfStatement, WhileStatement,
                              FunctionDeclaration, ReturnStatement>;

struct Stmt {
    SourcePosition position;
    StmtNode node;

    template <typename Node>
    Stmt(SourcePosition source_position, Node&& value)
        : position(source_position), node(std::forward<Node>(value)) {}
};

using Program = std::vector<StmtPtr>;

} // namespace nova
