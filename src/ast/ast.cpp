#include "ast/expression.hpp"

namespace nova {

const char* type_name(Type type) noexcept {
    switch (type) {
        case Type::Int: return "int";
        case Type::Bool: return "bool";
        case Type::Void: return "void";
    }
    return "unknown";
}

} // namespace nova
