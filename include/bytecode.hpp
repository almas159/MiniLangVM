#pragma once
#include "ast.hpp"
#include "source_location.hpp"

#include <string>
#include <utility>
#include <vector>

namespace minilang {

enum class OpCode {
    PushInt,
    PushUInt,
    PushFloat,
    PushBool,
    PushString,
    PushVoid,

    Load,
    Store,
    Pop,

    MakeArray,
    LoadIndex,
    StoreIndex,

    MakeStruct,
    LoadField,
    StoreField,

    Add,
    Sub,
    Mul,
    Div,
    Mod,
    BitAnd,
    BitOr,
    BitXor,
    BitNot,
    ShiftLeft,
    ShiftRight,

    Neg,
    Not,

    CastInt,
    CastUInt,
    CastFloat,
    CastBool,

    Equal,
    NotEqual,
    Less,
    Greater,
    LessEqual,
    GreaterEqual,

    Jump,
    JumpIfFalse,
    JumpIfTrue,

    Call,
    Input,
    Exit,
    Panic,
    Assert,
    Len,
    ToInt,
    Print,
    Ret
};

struct Instruction {
    OpCode op;
    int intArg = 0;
    int intArg2 = 0;
    double floatArg = 0.0;
    std::string stringArg;
    SourceLocation location;

    Instruction(OpCode op, SourceLocation location = {})
        : op(op), location(location) {}

    Instruction(OpCode op, int intArg, SourceLocation location = {})
        : op(op), intArg(intArg), location(location) {}

    Instruction(OpCode op, int intArg, int intArg2, SourceLocation location = {})
        : op(op), intArg(intArg), intArg2(intArg2), location(location) {}

    Instruction(OpCode op, double floatArg, SourceLocation location = {})
        : op(op), floatArg(floatArg), location(location) {}

    Instruction(OpCode op, std::string stringArg, SourceLocation location = {})
        : op(op), stringArg(std::move(stringArg)), location(location) {}
};

struct FunctionInfo {
    std::string name;
    Type returnType = Type::Unknown;
    std::vector<Type> parameterTypes;
    int localCount = 0;
    int address = -1;
};

struct Bytecode {
    std::vector<Instruction> code;
    std::vector<FunctionInfo> functions;
    int entryFunction = -1;
};

std::string opCodeToString(OpCode op);
std::string instructionToString(const Instruction& instruction, int index);
std::string bytecodeToString(const Bytecode& bytecode);

} 
