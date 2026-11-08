#include "semantic/analyzer.hpp"

#include <algorithm>
#include <type_traits>
#include <utility>

namespace nova {

[[noreturn]] void SemanticAnalyzer::fail(const std::string& message, SourcePosition position) {
    throw CompileError(message, position);
}

void SemanticAnalyzer::analyze(const Program& program) {
    scopes_.clear();
    functions_.clear();
    current_return_type_ = Type::Void;
    inside_function_ = false;
    scopes_.emplace_back();
    predeclare_functions(program);
    for (const auto& statement : program) analyze_statement(*statement);
}

void SemanticAnalyzer::predeclare_functions(const Program& program) {
    for (const auto& statement : program) {
        const auto* function = std::get_if<FunctionDeclaration>(&statement->node);
        if (function == nullptr) continue;
        if (functions_.contains(function->name))
            fail("Function '" + function->name + "' is already declared.", statement->position);
        FunctionSignature signature;
        signature.return_type = function->return_type;
        signature.position = statement->position;
        for (const auto& parameter : function->parameters) signature.parameters.push_back(parameter.type);
        functions_.emplace(function->name, std::move(signature));
    }
}

void SemanticAnalyzer::declare_variable(const std::string& name, Type type, SourcePosition position) {
    auto& scope = scopes_.back();
    if (scope.contains(name)) fail("Variable '" + name + "' is already declared in this scope.", position);
    scope.emplace(name, type);
}

Type SemanticAnalyzer::lookup_variable(const std::string& name, SourcePosition position) const {
    // Function bodies cannot capture top-level variables: top-level declarations
    // are emitted inside generated main(), not as C++ globals.
    const std::size_t excluded_global_scope = inside_function_ ? 1U : 0U;
    for (std::size_t index = scopes_.size(); index > excluded_global_scope; --index) {
        const auto found = scopes_[index - 1U].find(name);
        if (found != scopes_[index - 1U].end()) return found->second;
    }
    fail("Undefined variable '" + name + "'.", position);
}

bool SemanticAnalyzer::definitely_returns(const Stmt& statement) {
    return std::visit([](const auto& node) -> bool {
        using Node = std::decay_t<decltype(node)>;
        if constexpr (std::is_same_v<Node, ReturnStatement>) {
            return true;
        } else if constexpr (std::is_same_v<Node, BlockStatement>) {
            for (const auto& child : node.statements) {
                if (definitely_returns(*child)) return true;
            }
            return false;
        } else if constexpr (std::is_same_v<Node, IfStatement>) {
            return node.else_branch && definitely_returns(*node.then_branch) &&
                   definitely_returns(*node.else_branch);
        } else if constexpr (std::is_same_v<Node, WhileStatement> ||
                             std::is_same_v<Node, ForStatement>) {
            return false;
        } else {
            return false;
        }
    }, statement.node);
}

void SemanticAnalyzer::analyze_block(const BlockStatement& block, bool create_scope) {
    if (create_scope) scopes_.emplace_back();
    for (const auto& statement : block.statements) analyze_statement(*statement);
    if (create_scope) scopes_.pop_back();
}

void SemanticAnalyzer::analyze_statement(const Stmt& statement) {
    std::visit([this, &statement](const auto& node) {
        using Node = std::decay_t<decltype(node)>;
        if constexpr (std::is_same_v<Node, VariableDeclaration>) {
            const Type initializer_type = analyze_expression(*node.initializer);
            const Type variable_type = node.declared_type.value_or(initializer_type);
            if (variable_type != initializer_type)
                fail("Initializer for '" + node.name + "' has type " + type_name(initializer_type) +
                     ", expected " + type_name(variable_type) + ".", statement.position);
            declare_variable(node.name, variable_type, statement.position);
        } else if constexpr (std::is_same_v<Node, ExpressionStatement>) {
            (void)analyze_expression(*node.expression);
        } else if constexpr (std::is_same_v<Node, PrintStatement>) {
            (void)analyze_expression(*node.expression);
        } else if constexpr (std::is_same_v<Node, BlockStatement>) {
            analyze_block(node);
        } else if constexpr (std::is_same_v<Node, IfStatement>) {
            if (analyze_expression(*node.condition) != Type::Bool)
                fail("If condition must have type bool.", node.condition->position);
            analyze_statement(*node.then_branch);
            if (node.else_branch) analyze_statement(*node.else_branch);
        } else if constexpr (std::is_same_v<Node, WhileStatement>) {
            if (analyze_expression(*node.condition) != Type::Bool)
                fail("While condition must have type bool.", node.condition->position);
            analyze_statement(*node.body);
        } else if constexpr (std::is_same_v<Node, ForStatement>) {
            scopes_.emplace_back();
            if (node.initializer) analyze_statement(*node.initializer);
            if (analyze_expression(*node.condition) != Type::Bool)
                fail("For condition must have type bool.", node.condition->position);
            (void)analyze_expression(*node.update);
            analyze_statement(*node.body);
            scopes_.pop_back();
        } else if constexpr (std::is_same_v<Node, FunctionDeclaration>) {
            if (inside_function_ || scopes_.size() != 1U)
                fail("Function declarations are only allowed at top level.", statement.position);
            const bool previous_inside = inside_function_;
            const Type previous_return = current_return_type_;
            inside_function_ = true;
            current_return_type_ = node.return_type;
            scopes_.emplace_back();
            for (const auto& parameter : node.parameters)
                declare_variable(parameter.name, parameter.type, parameter.position);
            for (const auto& body_statement : node.body) analyze_statement(*body_statement);
            const bool returns = std::any_of(node.body.begin(), node.body.end(),
                [](const StmtPtr& child) { return definitely_returns(*child); });
            if (!returns)
                fail("Function '" + node.name + "' does not return a value on every control path.", statement.position);
            scopes_.pop_back();
            inside_function_ = previous_inside;
            current_return_type_ = previous_return;
        } else if constexpr (std::is_same_v<Node, ReturnStatement>) {
            if (!inside_function_) fail("'return' is only valid inside a function.", statement.position);
            const Type actual = analyze_expression(*node.value);
            if (actual != current_return_type_)
                fail("Return value has type " + std::string(type_name(actual)) + ", expected " +
                     type_name(current_return_type_) + ".", statement.position);
        }
    }, statement.node);
}

Type SemanticAnalyzer::analyze_expression(const Expr& expression) {
    return std::visit([this, &expression](const auto& node) -> Type {
        using Node = std::decay_t<decltype(node)>;
        if constexpr (std::is_same_v<Node, IntegerExpr>) {
            return Type::Int;
        } else if constexpr (std::is_same_v<Node, BooleanExpr>) {
            return Type::Bool;
        } else if constexpr (std::is_same_v<Node, VariableExpr>) {
            return lookup_variable(node.name, expression.position);
        } else if constexpr (std::is_same_v<Node, AssignExpr>) {
            const Type target = lookup_variable(node.name, expression.position);
            const Type value = analyze_expression(*node.value);
            if (target != value)
                fail("Cannot assign " + std::string(type_name(value)) + " to variable '" + node.name +
                     "' of type " + type_name(target) + ".", expression.position);
            return target;
        } else if constexpr (std::is_same_v<Node, UnaryExpr>) {
            const Type operand = analyze_expression(*node.operand);
            if (node.op == TokenType::Minus && operand == Type::Int) return Type::Int;
            if (node.op == TokenType::Bang && operand == Type::Bool) return Type::Bool;
            fail("Invalid operand type for unary operator.", expression.position);
        } else if constexpr (std::is_same_v<Node, BinaryExpr>) {
            const Type left = analyze_expression(*node.left);
            const Type right = analyze_expression(*node.right);
            switch (node.op) {
                case TokenType::Plus: case TokenType::Minus: case TokenType::Star:
                case TokenType::Slash: case TokenType::Percent:
                    if (left == Type::Int && right == Type::Int) return Type::Int;
                    fail("Arithmetic operators require int operands.", expression.position);
                case TokenType::Less: case TokenType::LessEqual:
                case TokenType::Greater: case TokenType::GreaterEqual:
                    if (left == Type::Int && right == Type::Int) return Type::Bool;
                    fail("Comparison operators require int operands.", expression.position);
                case TokenType::EqualEqual: case TokenType::BangEqual:
                    if (left == right) return Type::Bool;
                    fail("Equality operands must have the same type.", expression.position);
                case TokenType::AndAnd: case TokenType::OrOr:
                    if (left == Type::Bool && right == Type::Bool) return Type::Bool;
                    fail("Logical operators require bool operands.", expression.position);
                default: fail("Unsupported binary operator.", expression.position);
            }
        } else if constexpr (std::is_same_v<Node, CallExpr>) {
            const auto found = functions_.find(node.callee);
            if (found == functions_.end()) fail("Undefined function '" + node.callee + "'.", expression.position);
            if (node.arguments.size() != found->second.parameters.size())
                fail("Function '" + node.callee + "' expects " +
                     std::to_string(found->second.parameters.size()) + " argument(s), got " +
                     std::to_string(node.arguments.size()) + ".", expression.position);
            for (std::size_t i = 0; i < node.arguments.size(); ++i) {
                const Type actual = analyze_expression(*node.arguments[i]);
                const Type expected = found->second.parameters[i];
                if (actual != expected)
                    fail("Argument " + std::to_string(i + 1) + " of '" + node.callee +
                         "' has type " + type_name(actual) + ", expected " + type_name(expected) + ".",
                         node.arguments[i]->position);
            }
            return found->second.return_type;
        }
    }, expression.node);
}

} // namespace nova
