#include "ast.hpp"
#include <string>

std::string typeToString(Type type) {
    switch (type) {
        case Type::Int:
            return "int";

        case Type::Bool:
            return "bool";

        case Type::String:
            return "string";

        case Type::Void:
            return "void";

        case Type::Unknown:
            return "unknown";
    }

    return "unknown";
}

std::string binaryOpToString(BinaryOp op) {
    switch (op) {
        case BinaryOp::Add:
            return "+";

        case BinaryOp::Sub:
            return "-";

        case BinaryOp::Mul:
            return "*";

        case BinaryOp::Div:
            return "/";

        case BinaryOp::Mod:
            return "%";

        case BinaryOp::Less:
            return "<";

        case BinaryOp::Greater:
            return ">";

        case BinaryOp::LessEqual:
            return "<=";

        case BinaryOp::GreaterEqual:
            return ">=";

        case BinaryOp::Equal:
            return "==";

        case BinaryOp::NotEqual:
            return "!=";

        case BinaryOp::And:
            return "&&";

        case BinaryOp::Or:
            return "||";
    }

    return "?";
}

std::string unaryOpToString(UnaryOp op) {
    switch (op) {
        case UnaryOp::Negate:
            return "-";

        case UnaryOp::Not:
            return "!";
    }

    return "?";
}
