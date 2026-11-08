#pragma once

#include "ast/statement.hpp"

#include <string>

namespace nova {

class CppGenerator final {
public:
    [[nodiscard]] std::string generate(const Program& program);

private:
    void emit_statement(const Stmt& statement, int indent);
    void emit_block(const BlockStatement& block, int indent);
    [[nodiscard]] std::string emit_expression(const Expr& expression) const;
    [[nodiscard]] static std::string variable_name(const std::string& name);
    [[nodiscard]] static std::string function_name(const std::string& name);
    [[nodiscard]] static std::string cpp_type(Type type);
    void write_indent(int indent);

    std::string output_;
};

} // namespace nova
