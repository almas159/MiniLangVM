#include "vm.hpp"
#include "error.hpp"
#include <algorithm>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

Value Value::makeInt(int value) {
    Value result;
    result.type = Type::Int;
    result.data = value;
    return result;
}

Value Value::makeBool(bool value) {
    Value result;
    result.type = Type::Bool;
    result.data = value;
    return result;
}

Value Value::makeString(const std::string& value) {
    Value result;
    result.type = Type::String;
    result.data = value;
    return result;
}

// VM public

int VM::run(const Bytecode& bytecode) {
    bytecode_ = &bytecode;

    stack_.clear();
    locals_.clear();

    locals_.resize(bytecode.localCount, Value::makeInt(0));

    ip_ = 0;
    running_ = true;
    returnCode_ = 0;

    while (running_) {
        if (ip_ == static_cast<int>(bytecode_->code.size())) {
            throw RuntimeError(SourceLocation(), "program reached end without return");
        }

        if (ip_ < 0 || ip_ > static_cast<int>(bytecode_->code.size())) {
            throw RuntimeError(SourceLocation(), "instruction pointer out of range");
        }

        const Instruction& instruction = bytecode_->code[ip_];
        execute(instruction);
    }

    return returnCode_;
}

// VM helpers

void VM::push(const Value& value) {
    stack_.push_back(value);
}

Value VM::pop(SourceLocation location) {
    if (stack_.empty()) {
        throw RuntimeError(location, "stack underflow");
    }

    Value value = stack_.back();
    stack_.pop_back();
    return value;
}

int VM::requireInt(const Value& value,
                   SourceLocation location,
                   const std::string& context) {
    if (value.type != Type::Int) {
        throw RuntimeError(
            location,
            context + ": expected int, got " + typeToString(value.type)
        );
    }

    return std::get<int>(value.data);
}

bool VM::requireBool(const Value& value,
                     SourceLocation location,
                     const std::string& context) {
    if (value.type != Type::Bool) {
        throw RuntimeError(
            location,
            context + ": expected bool, got " + typeToString(value.type)
        );
    }

    return std::get<bool>(value.data);
}

std::string VM::requireString(const Value& value,
                              SourceLocation location,
                              const std::string& context) {
    if (value.type != Type::String) {
        throw RuntimeError(
            location,
            context + ": expected string, got " + typeToString(value.type)
        );
    }

    return std::get<std::string>(value.data);
}

void VM::checkLocalIndex(int index, SourceLocation location) {
    if (index < 0 || index >= static_cast<int>(locals_.size())) {
        throw RuntimeError(location, "invalid local variable index");
    }
}

void VM::checkJumpAddress(int address, SourceLocation location) {
    if (address < 0 || address > static_cast<int>(bytecode_->code.size())) {
        throw RuntimeError(location, "invalid jump address");
    }
}

std::string VM::valueToString(const Value& value) const {
    switch (value.type) {
        case Type::Int:
            return std::to_string(std::get<int>(value.data));

        case Type::Bool:
            return std::get<bool>(value.data) ? "true" : "false";

        case Type::String:
            return std::get<std::string>(value.data);

        case Type::Void:
            return "<void>";

        case Type::Unknown:
            return "<unknown>";
    }

    return "<unknown>";
}

// main executor

void VM::execute(const Instruction& instruction) {
    switch (instruction.op) {
        case OpCode::PushInt:
            push(Value::makeInt(instruction.intArg));
            ip_++;
            return;

        case OpCode::PushBool:
            push(Value::makeBool(instruction.intArg != 0));
            ip_++;
            return;

        case OpCode::PushString:
            push(Value::makeString(instruction.stringArg));
            ip_++;
            return;

        case OpCode::Load:
            checkLocalIndex(instruction.intArg, instruction.location);
            push(locals_[instruction.intArg]);
            ip_++;
            return;

        case OpCode::Store: {
            checkLocalIndex(instruction.intArg, instruction.location);
            Value value = pop(instruction.location);
            locals_[instruction.intArg] = value;
            ip_++;
            return;
        }

        case OpCode::Add:
            executeAdd(instruction);
            ip_++;
            return;

        case OpCode::Sub:
            executeSub(instruction);
            ip_++;
            return;

        case OpCode::Mul:
            executeMul(instruction);
            ip_++;
            return;

        case OpCode::Div:
            executeDiv(instruction);
            ip_++;
            return;

        case OpCode::Mod:
            executeMod(instruction);
            ip_++;
            return;

        case OpCode::Neg: {
            Value value = pop(instruction.location);
            int x = requireInt(value, instruction.location, "NEG");
            push(Value::makeInt(-x));
            ip_++;
            return;
        }

        case OpCode::Not: {
            Value value = pop(instruction.location);
            bool x = requireBool(value, instruction.location, "NOT");
            push(Value::makeBool(!x));
            ip_++;
            return;
        }

        case OpCode::Equal:
            executeEqual(instruction);
            ip_++;
            return;

        case OpCode::NotEqual:
            executeNotEqual(instruction);
            ip_++;
            return;

        case OpCode::Less:
        case OpCode::Greater:
        case OpCode::LessEqual:
        case OpCode::GreaterEqual:
            executeIntComparison(instruction);
            ip_++;
            return;

        case OpCode::Jump:
            checkJumpAddress(instruction.intArg, instruction.location);
            ip_ = instruction.intArg;
            return;

        case OpCode::JumpIfFalse: {
            Value condition = pop(instruction.location);
            bool value = requireBool(condition, instruction.location, "JMP_IF_FALSE");

            if (!value) {
                checkJumpAddress(instruction.intArg, instruction.location);
                ip_ = instruction.intArg;
            } else {
                ip_++;
            }

            return;
        }

        case OpCode::JumpIfTrue: {
            Value condition = pop(instruction.location);
            bool value = requireBool(condition, instruction.location, "JMP_IF_TRUE");

            if (value) {
                checkJumpAddress(instruction.intArg, instruction.location);
                ip_ = instruction.intArg;
            } else {
                ip_++;
            }

            return;
        }

        case OpCode::Print: {
            int count = instruction.intArg;

            if (count < 0 || count > static_cast<int>(stack_.size())) {
                throw RuntimeError(instruction.location, "PRINT has invalid argument count");
            }

            std::vector<Value> arguments;
            arguments.reserve(count);

            for (int i = 0; i < count; ++i) {
                arguments.push_back(pop(instruction.location));
            }

            std::reverse(arguments.begin(), arguments.end());

            for (const Value& value : arguments) {
                std::cout << valueToString(value);
            }

            std::cout << std::endl;

            ip_++;
            return;
        }

        case OpCode::Ret: {
            Value value = pop(instruction.location);
            returnCode_ = requireInt(value, instruction.location, "RET");

            running_ = false;
            ip_++;
            return;
        }
    }

    throw RuntimeError(instruction.location, "unknown opcode");
}

// operations 

void VM::executeAdd(const Instruction& instruction) {
    Value right = pop(instruction.location);
    Value left = pop(instruction.location);

    if (left.type == Type::Int && right.type == Type::Int) {
        int a = std::get<int>(left.data);
        int b = std::get<int>(right.data);
        push(Value::makeInt(a + b));
        return;
    }

    if (left.type == Type::String && right.type == Type::String) {
        const std::string& a = std::get<std::string>(left.data);
        const std::string& b = std::get<std::string>(right.data);
        push(Value::makeString(a + b));
        return;
    }

    throw RuntimeError(
        instruction.location,
        "ADD cannot be applied to " +
        typeToString(left.type) + " and " + typeToString(right.type)
    );
}

void VM::executeSub(const Instruction& instruction) {
    Value right = pop(instruction.location);
    Value left = pop(instruction.location);

    int b = requireInt(right, instruction.location, "SUB");
    int a = requireInt(left, instruction.location, "SUB");

    push(Value::makeInt(a - b));
}

void VM::executeMul(const Instruction& instruction) {
    Value right = pop(instruction.location);
    Value left = pop(instruction.location);

    int b = requireInt(right, instruction.location, "MUL");
    int a = requireInt(left, instruction.location, "MUL");

    push(Value::makeInt(a * b));
}

void VM::executeDiv(const Instruction& instruction) {
    Value right = pop(instruction.location);
    Value left = pop(instruction.location);

    int b = requireInt(right, instruction.location, "DIV");
    int a = requireInt(left, instruction.location, "DIV");

    if (b == 0) {
        throw RuntimeError(instruction.location, "division by zero");
    }

    push(Value::makeInt(a / b));
}

void VM::executeMod(const Instruction& instruction) {
    Value right = pop(instruction.location);
    Value left = pop(instruction.location);

    int b = requireInt(right, instruction.location, "MOD");
    int a = requireInt(left, instruction.location, "MOD");

    if (b == 0) {
        throw RuntimeError(instruction.location, "modulo by zero");
    }

    push(Value::makeInt(a % b));
}

void VM::executeEqual(const Instruction& instruction) {
    Value right = pop(instruction.location);
    Value left = pop(instruction.location);

    if (left.type != right.type) {
        throw RuntimeError(
            instruction.location,
            "EQUAL cannot compare " +
            typeToString(left.type) + " and " + typeToString(right.type)
        );
    }

    bool result = false;

    switch (left.type) {
        case Type::Int:
            result = std::get<int>(left.data) == std::get<int>(right.data);
            break;

        case Type::Bool:
            result = std::get<bool>(left.data) == std::get<bool>(right.data);
            break;

        case Type::String:
            result = std::get<std::string>(left.data) == std::get<std::string>(right.data);
            break;

        case Type::Void:
        case Type::Unknown:
            throw RuntimeError(instruction.location, "invalid values for EQUAL");
    }

    push(Value::makeBool(result));
}

void VM::executeNotEqual(const Instruction& instruction) {
    Value right = pop(instruction.location);
    Value left = pop(instruction.location);

    if (left.type != right.type) {
        throw RuntimeError(
            instruction.location,
            "NOT_EQUAL cannot compare " +
            typeToString(left.type) + " and " + typeToString(right.type)
        );
    }

    bool result = false;

    switch (left.type) {
        case Type::Int:
            result = std::get<int>(left.data) != std::get<int>(right.data);
            break;

        case Type::Bool:
            result = std::get<bool>(left.data) != std::get<bool>(right.data);
            break;

        case Type::String:
            result = std::get<std::string>(left.data) != std::get<std::string>(right.data);
            break;

        case Type::Void:
        case Type::Unknown:
            throw RuntimeError(instruction.location, "invalid values for NOT_EQUAL");
    }

    push(Value::makeBool(result));
}

void VM::executeIntComparison(const Instruction& instruction) {
    Value right = pop(instruction.location);
    Value left = pop(instruction.location);

    int b = requireInt(right, instruction.location, opCodeToString(instruction.op));
    int a = requireInt(left, instruction.location, opCodeToString(instruction.op));

    bool result = false;

    switch (instruction.op) {
        case OpCode::Less:
            result = a < b;
            break;

        case OpCode::Greater:
            result = a > b;
            break;

        case OpCode::LessEqual:
            result = a <= b;
            break;

        case OpCode::GreaterEqual:
            result = a >= b;
            break;

        default:
            throw RuntimeError(instruction.location, "invalid integer comparison opcode");
    }

    push(Value::makeBool(result));
}
