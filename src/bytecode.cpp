#include "bytecode.hpp"
#include <sstream>
#include <string>

std::string opCodeToString(OpCode op) {
    switch (op) {
        case OpCode::PushInt:
            return "PUSH_INT";

        case OpCode::PushBool:
            return "PUSH_BOOL";

        case OpCode::PushString:
            return "PUSH_STRING";

        case OpCode::Load:
            return "LOAD";

        case OpCode::Store:
            return "STORE";

        case OpCode::Add:
            return "ADD";

        case OpCode::Sub:
            return "SUB";

        case OpCode::Mul:
            return "MUL";

        case OpCode::Div:
            return "DIV";

        case OpCode::Mod:
            return "MOD";

        case OpCode::Neg:
            return "NEG";

        case OpCode::Not:
            return "NOT";

        case OpCode::Equal:
            return "EQUAL";

        case OpCode::NotEqual:
            return "NOT_EQUAL";

        case OpCode::Less:
            return "LESS";

        case OpCode::Greater:
            return "GREATER";

        case OpCode::LessEqual:
            return "LESS_EQUAL";

        case OpCode::GreaterEqual:
            return "GREATER_EQUAL";

        case OpCode::Jump:
            return "JMP";

        case OpCode::JumpIfFalse:
            return "JMP_IF_FALSE";

        case OpCode::JumpIfTrue:
            return "JMP_IF_TRUE";

        case OpCode::Print:
            return "PRINT";

        case OpCode::Ret:
            return "RET";
    }

    return "UNKNOWN";
}

std::string instructionToString(const Instruction& instruction, int index) {
    std::ostringstream out;

    out << index << ": " << opCodeToString(instruction.op);

    switch (instruction.op) {
        case OpCode::PushInt:
        case OpCode::Load:
        case OpCode::Store:
        case OpCode::Jump:
        case OpCode::JumpIfFalse:
        case OpCode::JumpIfTrue:
        case OpCode::Print:
            out << " " << instruction.intArg;
            break;

        case OpCode::PushBool:
            out << " " << (instruction.intArg ? "true" : "false");
            break;

        case OpCode::PushString:
            out << " \"" << instruction.stringArg << "\"";
            break;

        default:
            break;
    }

    return out.str();
}

std::string bytecodeToString(const Bytecode& bytecode) {
    std::ostringstream out;

    out << "Local variables count: " << bytecode.localCount << "\n";

    for (std::size_t i = 0; i < bytecode.code.size(); ++i) {
        out << instructionToString(bytecode.code[i], static_cast<int>(i)) << "\n";
    }

    return out.str();
}
