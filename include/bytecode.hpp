#pragma once
#include "source_location.hpp"
#include <string>
#include <utility>
#include <vector>

enum class OpCode {
    PushInt,
    PushBool,
    PushString,

    Load,
    Store,

    Add,
    Sub,
    Mul,
    Div,
    Mod,

    Neg,
    Not,

    Equal,
    NotEqual,
    Less,
    Greater,
    LessEqual,
    GreaterEqual,

    Jump,
    JumpIfFalse,
    JumpIfTrue,

    Print,
    Ret
};

struct Instruction {
    OpCode op;
    int intArg = 0;
    std::string stringArg;
    SourceLocation location;

    Instruction(OpCode op, SourceLocation location = {})
        : op(op), location(location) {}

    Instruction(OpCode op, int intArg, SourceLocation location = {})
        : op(op), intArg(intArg), location(location) {}

    Instruction(OpCode op, std::string stringArg, SourceLocation location = {})
        : op(op), stringArg(std::move(stringArg)), location(location) {}
};

struct Bytecode {
    std::vector<Instruction> code;
    int localCount = 0;
};

std::string opCodeToString(OpCode op);
std::string instructionToString(const Instruction& instruction, int index);
std::string bytecodeToString(const Bytecode& bytecode);
