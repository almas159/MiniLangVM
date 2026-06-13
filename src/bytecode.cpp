#include "bytecode.hpp"
#include <sstream>
#include <string>

namespace minilang {

std::string opCodeToString(OpCode op) {
    switch (op) {
        case OpCode::PushInt:
            return "PUSH_INT";

        case OpCode::PushUInt:
            return "PUSH_UINT";

        case OpCode::PushFloat:
            return "PUSH_FLOAT";

        case OpCode::PushBool:
            return "PUSH_BOOL";

        case OpCode::PushString:
            return "PUSH_STRING";

        case OpCode::PushVoid:
            return "PUSH_VOID";

        case OpCode::Load:
            return "LOAD";

        case OpCode::Store:
            return "STORE";

        case OpCode::Pop:
            return "POP";

        case OpCode::MakeArray:
            return "MAKE_ARRAY";

        case OpCode::LoadIndex:
            return "LOAD_INDEX";

        case OpCode::StoreIndex:
            return "STORE_INDEX";

        case OpCode::MakeStruct:
            return "MAKE_STRUCT";

        case OpCode::LoadField:
            return "LOAD_FIELD";

        case OpCode::StoreField:
            return "STORE_FIELD";

        case OpCode::Add:
            return "ADD";

        case OpCode::Sub:
            return "SUB";

        case OpCode::Mul:
            return "MUL";

        case OpCode::Div:
            return "DIV";

        case OpCode::BitAnd:
            return "BIT_AND";

        case OpCode::BitOr:
            return "BIT_OR";

        case OpCode::BitXor:
            return "BIT_XOR";

        case OpCode::BitNot:
            return "BIT_NOT";

        case OpCode::ShiftLeft:
            return "SHIFT_LEFT";

        case OpCode::ShiftRight:
            return "SHIFT_RIGHT";

        case OpCode::Mod:
            return "MOD";

        case OpCode::Neg:
            return "NEG";

        case OpCode::Not:
            return "NOT";

        case OpCode::CastInt:
            return "CAST_INT";

        case OpCode::CastUInt:
            return "CAST_UINT";

        case OpCode::CastFloat:
            return "CAST_FLOAT";

        case OpCode::CastBool:
            return "CAST_BOOL";

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

        case OpCode::Call:
            return "CALL";

        case OpCode::Input:
            return "INPUT";

        case OpCode::Exit:
            return "EXIT";

        case OpCode::Panic:
            return "PANIC";

        case OpCode::Assert:
            return "ASSERT";

        case OpCode::Len:
            return "LEN";

        case OpCode::ToInt:
            return "TO_INT";

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
        case OpCode::PushUInt:
        case OpCode::Load:
        case OpCode::Store:
        case OpCode::Jump:
        case OpCode::JumpIfFalse:
        case OpCode::JumpIfTrue:
        case OpCode::Call:
        case OpCode::Print:
        case OpCode::MakeArray:
        case OpCode::LoadIndex:
        case OpCode::StoreIndex:
        case OpCode::MakeStruct:
            out << " " << instruction.intArg;
            break;

        case OpCode::LoadField:
        case OpCode::StoreField:
            out << " " << instruction.intArg << " " << instruction.intArg2;
            break;

        case OpCode::PushFloat:
            out << " " << instruction.floatArg;
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

    out << "Functions:\n";

    for (std::size_t i = 0; i < bytecode.functions.size(); ++i) {
        const FunctionInfo& function = bytecode.functions[i];

        out << i << ": " << function.name
            << " address=" << function.address
            << " locals=" << function.localCount
            << " params=" << function.parameterTypes.size()
            << "\n";
    }

    out << "\nCode:\n";

    for (std::size_t i = 0; i < bytecode.code.size(); ++i) {
        out << instructionToString(bytecode.code[i], static_cast<int>(i)) << "\n";
    }

    return out.str();
}

} 
