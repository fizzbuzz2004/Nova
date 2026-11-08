#pragma once

#include "diagnostics/error.hpp"
#include "lexer/token.hpp"

#include <cstdint>
#include <memory>
#include <string>
#include <variant>
#include <utility>
#include <vector>

namespace nova {

enum class Type { Int, Bool, Void };
[[nodiscard]] const char* type_name(Type type) noexcept;

struct Expr;
using ExprPtr = std::unique_ptr<Expr>;

struct IntegerExpr { std::int64_t value; };
struct BooleanExpr { bool value; };
struct VariableExpr { std::string name; };
struct UnaryExpr { TokenType op; ExprPtr operand; };
struct BinaryExpr { ExprPtr left; TokenType op; ExprPtr right; };
struct AssignExpr { std::string name; ExprPtr value; };
struct CallExpr { std::string callee; std::vector<ExprPtr> arguments; };

using ExprNode = std::variant<IntegerExpr, BooleanExpr, VariableExpr, UnaryExpr,
                              BinaryExpr, AssignExpr, CallExpr>;

struct Expr {
    SourcePosition position;
    ExprNode node;

    template <typename Node>
    Expr(SourcePosition source_position, Node&& value)
        : position(source_position), node(std::forward<Node>(value)) {}
};

} // namespace nova
