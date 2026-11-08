#pragma once

#include "ast/statement.hpp"

#include <string>
#include <unordered_map>
#include <vector>

namespace nova {

class SemanticAnalyzer final {
public:
    void analyze(const Program& program);

private:
    struct FunctionSignature { std::vector<Type> parameters; Type return_type; SourcePosition position; };
    using Scope = std::unordered_map<std::string, Type>;

    void predeclare_functions(const Program& program);
    void analyze_statement(const Stmt& statement);
    void analyze_block(const BlockStatement& block, bool create_scope = true);
    [[nodiscard]] Type analyze_expression(const Expr& expression);
    [[nodiscard]] static bool definitely_returns(const Stmt& statement);
    [[nodiscard]] Type lookup_variable(const std::string& name, SourcePosition position) const;
    void declare_variable(const std::string& name, Type type, SourcePosition position);
    [[noreturn]] static void fail(const std::string& message, SourcePosition position);

    std::vector<Scope> scopes_;
    std::unordered_map<std::string, FunctionSignature> functions_;
    Type current_return_type_{Type::Void};
    bool inside_function_{false};
};

} // namespace nova
