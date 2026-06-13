#include <cstdlib>
#include "vm.hpp"
#include "error.hpp"

#include <algorithm>
#include <iostream>
#include <limits>
#include <sstream>
#include <string>
#include <vector>

namespace minilang {


Value Value::makeVoid() {
    Value result;
    result.type = Type::Void;
    result.data = std::monostate{};
    return result;
}

Value Value::makeInt(int value) {
    Value result;
    result.type = Type::Int;
    result.data = value;
    return result;
}

Value Value::makeUInt(unsigned int value) {
    Value result;
    result.type = Type::UInt;
    result.data = value;
    return result;
}


Value Value::makeFloat(double value) {
    Value result;
    result.type = Type::Float;
    result.data = value;
    return result;
}

Value Value::makeBool(bool value) {
    Value result;
    result.type = Type::Bool;
    result.data = value;
    return result;
}

Value Value::makeIntArray(std::vector<Value> elements) {
    Value result;
    result.type = Type::IntArray;

    auto array = std::make_shared<ArrayValue>();
    array->elements = std::move(elements);

    result.data = array;
    return result;
}

Value Value::makeStruct(const std::string& typeName, std::vector<Value> fields) {
    Value result;
    result.type = Type::Struct;

    auto object = std::make_shared<StructValue>();
    object->typeName = typeName;
    object->fields = std::move(fields);

    result.data = object;
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
    frames_.clear();

    if (bytecode.entryFunction < 0 ||
        bytecode.entryFunction >= static_cast<int>(bytecode.functions.size())) {
        throw RuntimeError(SourceLocation(), "invalid entry function");
    }

    const FunctionInfo& mainFunction = bytecode.functions[bytecode.entryFunction];

    if (mainFunction.address < 0) {
        throw RuntimeError(SourceLocation(), "main function has invalid address");
    }

    CallFrame mainFrame;
    mainFrame.functionIndex = bytecode.entryFunction;
    mainFrame.returnIp = -1;
    mainFrame.locals.resize(mainFunction.localCount, Value::makeInt(0));

    frames_.push_back(std::move(mainFrame));

    ip_ = mainFunction.address;
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

VM::CallFrame& VM::currentFrame() {
    if (frames_.empty()) {
        throw RuntimeError(SourceLocation(), "call stack is empty");
    }

    return frames_.back();
}

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

double VM::requireFloat(const Value& value,
                        SourceLocation location,
                        const std::string& context) {
    if (value.type != Type::Float) {
        throw RuntimeError(
            location,
            context + ": expected float64, got " + typeToString(value.type)
        );
    }

    return std::get<double>(value.data);
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

void VM::checkLocalIndex(int index, SourceLocation location) {
    const auto& locals = currentFrame().locals;

    if (index < 0 || index >= static_cast<int>(locals.size())) {
        throw RuntimeError(location, "invalid local variable index");
    }
}

void VM::checkJumpAddress(int address, SourceLocation location) {
    if (address < 0 || address > static_cast<int>(bytecode_->code.size())) {
        throw RuntimeError(location, "invalid jump address");
    }
}

void VM::checkFunctionIndex(int index, SourceLocation location) {
    if (index < 0 || index >= static_cast<int>(bytecode_->functions.size())) {
        throw RuntimeError(location, "invalid function index");
    }

    if (bytecode_->functions[index].address < 0) {
        throw RuntimeError(location, "function has invalid address");
    }
}

std::string VM::valueToString(const Value& value) const {
    switch (value.type) {
        case Type::Int:
            return std::to_string(std::get<int>(value.data));

        case Type::UInt:
            return std::to_string(std::get<unsigned int>(value.data));

        case Type::Float: {
            std::ostringstream out;
            out << std::get<double>(value.data);
            return out.str();
        }

        case Type::IntArray: {
            auto array = std::get<std::shared_ptr<ArrayValue>>(value.data);
            std::string result = "[";

            for (std::size_t i = 0; i < array->elements.size(); ++i) {
                if (i > 0) {
                    result += ", ";
                }

                result += valueToString(array->elements[i]);
            }

            result += "]";
            return result;
        }

        case Type::Struct: {
            auto object = std::get<std::shared_ptr<StructValue>>(value.data);
            std::string result = object->typeName + " { ";

            for (std::size_t i = 0; i < object->fields.size(); ++i) {
                if (i > 0) {
                    result += ", ";
                }

                result += valueToString(object->fields[i]);
            }

            result += " }";
            return result;
        }

        case Type::Bool:
            return std::get<bool>(value.data) ? "true" : "false";

        case Type::String:
            return std::get<std::string>(value.data);

        case Type::Generic:
            return "<generic>";

        case Type::Void:
            return "<void>";

        case Type::Unknown:
            return "<unknown>";
    }

    return "<unknown>";
}

static bool valuesEqual(const Value& left, const Value& right) {
    if (left.type != right.type) {
        return false;
    }

    switch (left.type) {
        case Type::Int:
            return std::get<int>(left.data) == std::get<int>(right.data);

        case Type::UInt:
            return std::get<unsigned int>(left.data) ==
                   std::get<unsigned int>(right.data);

        case Type::Float:
            return std::get<double>(left.data) == std::get<double>(right.data);

        case Type::Bool:
            return std::get<bool>(left.data) == std::get<bool>(right.data);

        case Type::String:
            return std::get<std::string>(left.data) ==
                   std::get<std::string>(right.data);

        case Type::IntArray: {
            auto leftArray = std::get<std::shared_ptr<ArrayValue>>(left.data);
            auto rightArray = std::get<std::shared_ptr<ArrayValue>>(right.data);

            if (leftArray->elements.size() != rightArray->elements.size()) {
                return false;
            }

            for (std::size_t i = 0; i < leftArray->elements.size(); ++i) {
                if (!valuesEqual(leftArray->elements[i], rightArray->elements[i])) {
                    return false;
                }
            }

            return true;
        }

        case Type::Struct: {
            auto leftObject = std::get<std::shared_ptr<StructValue>>(left.data);
            auto rightObject = std::get<std::shared_ptr<StructValue>>(right.data);

            if (leftObject->typeName != rightObject->typeName) {
                return false;
            }

            if (leftObject->fields.size() != rightObject->fields.size()) {
                return false;
            }

            for (std::size_t i = 0; i < leftObject->fields.size(); ++i) {
                if (!valuesEqual(leftObject->fields[i], rightObject->fields[i])) {
                    return false;
                }
            }

            return true;
        }

        case Type::Generic:
        case Type::Void:
        case Type::Unknown:
            return false;
    }

    return false;
}

// Main executor

static Value executeBitwiseBinary(OpCode op, const Value& left, const Value& right, SourceLocation location) {
    if (op == OpCode::ShiftLeft || op == OpCode::ShiftRight) {
        int shift = 0;

        if (right.type == Type::Int) {
            shift = std::get<int>(right.data);
        } else if (right.type == Type::UInt) {
            unsigned int rawShift = std::get<unsigned int>(right.data);

            if (rawShift >= 32u) {
                throw RuntimeError(location, "invalid shift count");
            }

            shift = static_cast<int>(rawShift);
        } else {
            throw RuntimeError(location, "shift count must be int or uint");
        }

        if (shift < 0 || shift >= 32) {
            throw RuntimeError(location, "invalid shift count");
        }

        if (left.type == Type::Int) {
            int value = std::get<int>(left.data);

            if (op == OpCode::ShiftLeft) {
                return Value::makeInt(value << shift);
            }

            return Value::makeInt(value >> shift);
        }

        if (left.type == Type::UInt) {
            unsigned int value = std::get<unsigned int>(left.data);

            if (op == OpCode::ShiftLeft) {
                return Value::makeUInt(value << shift);
            }

            return Value::makeUInt(value >> shift);
        }

        throw RuntimeError(location, "shift operator can be applied only to int or uint");
    }

    if (left.type != right.type) {
        throw RuntimeError(location, "bitwise operands must have the same type");
    }

    if (left.type == Type::Int) {
        int a = std::get<int>(left.data);
        int b = std::get<int>(right.data);

        switch (op) {
            case OpCode::BitAnd:
                return Value::makeInt(a & b);

            case OpCode::BitOr:
                return Value::makeInt(a | b);

            case OpCode::BitXor:
                return Value::makeInt(a ^ b);

            default:
                break;
        }
    }

    if (left.type == Type::UInt) {
        unsigned int a = std::get<unsigned int>(left.data);
        unsigned int b = std::get<unsigned int>(right.data);

        switch (op) {
            case OpCode::BitAnd:
                return Value::makeUInt(a & b);

            case OpCode::BitOr:
                return Value::makeUInt(a | b);

            case OpCode::BitXor:
                return Value::makeUInt(a ^ b);

            default:
                break;
        }
    }

    throw RuntimeError(location, "bitwise operator can be applied only to int or uint");
}

static Value executeBitwiseNot(const Value& value, SourceLocation location) {
    if (value.type == Type::Int) {
        return Value::makeInt(~std::get<int>(value.data));
    }

    if (value.type == Type::UInt) {
        return Value::makeUInt(~std::get<unsigned int>(value.data));
    }

    throw RuntimeError(location, "bitwise not operator can be applied only to int or uint");
}

static Value executeToInt(const Value& value, SourceLocation location) {
    if (value.type != Type::String) {
        throw RuntimeError(location, "toInt argument must be string");
    }

    const std::string& text = std::get<std::string>(value.data);

    if (text.empty()) {
        throw RuntimeError(location, "toInt cannot parse empty string");
    }

    char* end = nullptr;
    long parsed = std::strtol(text.c_str(), &end, 10);

    if (end == text.c_str() || *end != '\0') {
        throw RuntimeError(location, "toInt cannot parse integer");
    }

    if (parsed < -2147483648L || parsed > 2147483647L) {
        throw RuntimeError(location, "toInt value is out of int range");
    }

    return Value::makeInt(static_cast<int>(parsed));
}

void VM::execute(const Instruction& instruction) {
    switch (instruction.op) {
        case OpCode::ToInt: {
            Value value = pop(instruction.location);
            push(executeToInt(value, instruction.location));
            ip_++;
            return;
        }


        case OpCode::BitAnd:
        case OpCode::BitOr:
        case OpCode::BitXor:
        case OpCode::ShiftLeft:
        case OpCode::ShiftRight: {
            Value right = pop(instruction.location);
            Value left = pop(instruction.location);
            push(executeBitwiseBinary(instruction.op, left, right, instruction.location));
            ip_++;
            return;
        }

        case OpCode::BitNot: {
            Value value = pop(instruction.location);
            push(executeBitwiseNot(value, instruction.location));
            ip_++;
            return;
        }


        case OpCode::PushInt:
            push(Value::makeInt(instruction.intArg));
            ip_++;
            return;

        case OpCode::PushUInt:
            push(Value::makeUInt(static_cast<unsigned int>(instruction.intArg)));
            ip_++;
            return;

        case OpCode::PushFloat:
            push(Value::makeFloat(instruction.floatArg));
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

        case OpCode::PushVoid:
            push(Value::makeVoid());
            ip_++;
            return;

        case OpCode::Load:
            checkLocalIndex(instruction.intArg, instruction.location);
            push(currentFrame().locals[instruction.intArg]);
            ip_++;
            return;

        case OpCode::Store: {
            checkLocalIndex(instruction.intArg, instruction.location);
            Value value = pop(instruction.location);
            currentFrame().locals[instruction.intArg] = value;
            ip_++;
            return;
        }

        case OpCode::Pop:
            pop(instruction.location);
            ip_++;
            return;

        case OpCode::MakeArray:
            executeMakeArray(instruction);
            ip_++;
            return;

        case OpCode::LoadIndex:
            executeLoadIndex(instruction);
            ip_++;
            return;

        case OpCode::StoreIndex:
            executeStoreIndex(instruction);
            ip_++;
            return;

        case OpCode::MakeStruct:
            executeMakeStruct(instruction);
            ip_++;
            return;

        case OpCode::LoadField:
            executeLoadField(instruction);
            ip_++;
            return;

        case OpCode::StoreField:
            executeStoreField(instruction);
            ip_++;
            return;

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
            if (value.type == Type::Int) {
                int x = requireInt(value, instruction.location, "NEG");
                push(Value::makeInt(-x));
                ip_++;
                return;
            }

            if (value.type == Type::Float) {
                double x = requireFloat(value, instruction.location, "NEG");
                push(Value::makeFloat(-x));
                ip_++;
                return;
            }

            throw RuntimeError(
                instruction.location,
                "NEG cannot be applied to " + typeToString(value.type)
            );
        }

        case OpCode::Not: {
            Value value = pop(instruction.location);
            bool x = requireBool(value, instruction.location, "NOT");
            push(Value::makeBool(!x));
            ip_++;
            return;
        }

        case OpCode::CastInt: {
            Value value = pop(instruction.location);

            if (value.type == Type::Int) {
                push(value);
            } else if (value.type == Type::UInt) {
                push(Value::makeInt(static_cast<int>(std::get<unsigned int>(value.data))));
            } else if (value.type == Type::Float) {
                push(Value::makeInt(static_cast<int>(std::get<double>(value.data))));
            } else if (value.type == Type::Bool) {
                push(Value::makeInt(std::get<bool>(value.data) ? 1 : 0));
            } else {
                throw RuntimeError(
                    instruction.location,
                    "CAST_INT cannot be applied to " + typeToString(value.type)
                );
            }

            ip_++;
            return;
        }

        case OpCode::CastUInt: {
            Value value = pop(instruction.location);

            if (value.type == Type::UInt) {
                push(value);
            } else if (value.type == Type::Int) {
                push(Value::makeUInt(static_cast<unsigned int>(std::get<int>(value.data))));
            } else if (value.type == Type::Float) {
                push(Value::makeUInt(static_cast<unsigned int>(std::get<double>(value.data))));
            } else if (value.type == Type::Bool) {
                push(Value::makeUInt(std::get<bool>(value.data) ? 1u : 0u));
            } else {
                throw RuntimeError(
                    instruction.location,
                    "CAST_UINT cannot be applied to " + typeToString(value.type)
                );
            }

            ip_++;
            return;
        }

        case OpCode::CastFloat: {
            Value value = pop(instruction.location);

            if (value.type == Type::Float) {
                push(value);
            } else if (value.type == Type::Int) {
                push(Value::makeFloat(static_cast<double>(std::get<int>(value.data))));
            } else if (value.type == Type::UInt) {
                push(Value::makeFloat(static_cast<double>(std::get<unsigned int>(value.data))));
            } else if (value.type == Type::Bool) {
                push(Value::makeFloat(std::get<bool>(value.data) ? 1.0 : 0.0));
            } else {
                throw RuntimeError(
                    instruction.location,
                    "CAST_FLOAT cannot be applied to " + typeToString(value.type)
                );
            }

            ip_++;
            return;
        }

        case OpCode::CastBool: {
            Value value = pop(instruction.location);

            if (value.type == Type::Bool) {
                push(value);
            } else if (value.type == Type::Int) {
                push(Value::makeBool(std::get<int>(value.data) != 0));
            } else if (value.type == Type::UInt) {
                push(Value::makeBool(std::get<unsigned int>(value.data) != 0));
            } else if (value.type == Type::Float) {
                push(Value::makeBool(std::get<double>(value.data) != 0.0));
            } else {
                throw RuntimeError(
                    instruction.location,
                    "CAST_BOOL cannot be applied to " + typeToString(value.type)
                );
            }

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

        case OpCode::Call:
            executeCall(instruction);
            return;

        case OpCode::Input: {
            std::string line;
            std::getline(std::cin, line);
            push(Value::makeString(line));
            ip_++;
            return;
        }

        case OpCode::Exit: {
            Value codeValue = pop(instruction.location);
            returnCode_ = requireInt(codeValue, instruction.location, "EXIT");
            frames_.clear();
            running_ = false;
            ip_++;
            return;
        }

        case OpCode::Panic: {
            Value messageValue = pop(instruction.location);

            if (messageValue.type != Type::String) {
                throw RuntimeError(
                    instruction.location,
                    "PANIC: expected string, got " + typeToString(messageValue.type)
                );
            }

            throw RuntimeError(
                instruction.location,
                "panic: " + std::get<std::string>(messageValue.data)
            );
        }

        case OpCode::Assert: {
            Value condition = pop(instruction.location);
            bool ok = requireBool(condition, instruction.location, "ASSERT");

            if (!ok) {
                std::string message = instruction.stringArg;

                if (message.empty()) {
                    message = "assertion failed";
                }

                throw RuntimeError(
                    instruction.location,
                    message
                );
            }

            ip_++;
            return;
        }

        case OpCode::Len: {
            Value value = pop(instruction.location);

            if (value.type == Type::String) {
                const std::string& s = std::get<std::string>(value.data);
                push(Value::makeInt(static_cast<int>(s.size())));
                ip_++;
                return;
            }

            if (value.type == Type::IntArray) {
                auto array = std::get<std::shared_ptr<ArrayValue>>(value.data);
                push(Value::makeInt(static_cast<int>(array->elements.size())));
                ip_++;
                return;
            }

            throw RuntimeError(
                instruction.location,
                "LEN cannot be applied to " + typeToString(value.type)
            );
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
            Value returnValue = pop(instruction.location);

            if (frames_.size() == 1) {
                returnCode_ = requireInt(returnValue, instruction.location, "RET from main");
                frames_.pop_back();
                running_ = false;
                ip_++;
                return;
            }

            int returnIp = currentFrame().returnIp;

            frames_.pop_back();

            push(returnValue);
            ip_ = returnIp;
            return;
        }
    }

    throw RuntimeError(instruction.location, "unknown opcode");
}

// Array operations

void VM::executeMakeArray(const Instruction& instruction) {
    int count = instruction.intArg;

    if (count < 0 || count > static_cast<int>(stack_.size())) {
        throw RuntimeError(instruction.location, "MAKE_ARRAY has invalid element count");
    }

    std::vector<Value> elements(count);

    for (int i = count - 1; i >= 0; --i) {
        Value value = pop(instruction.location);

        elements[i] = value;
    }

    push(Value::makeIntArray(std::move(elements)));
}

void VM::executeLoadIndex(const Instruction& instruction) {
    Value indexValue = pop(instruction.location);
    int index = requireInt(indexValue, instruction.location, "array index");

    Value arrayValue;

    if (instruction.intArg >= 0) {
        checkLocalIndex(instruction.intArg, instruction.location);
        arrayValue = currentFrame().locals[instruction.intArg];
    } else {
        arrayValue = pop(instruction.location);
    }

    if (arrayValue.type != Type::IntArray) {
        throw RuntimeError(instruction.location, "LOAD_INDEX target is not an array");
    }

    auto array = std::get<std::shared_ptr<ArrayValue>>(arrayValue.data);

    if (index < 0 || index >= static_cast<int>(array->elements.size())) {
        throw RuntimeError(instruction.location, "array index out of bounds");
    }

    push(array->elements[index]);
}

void VM::executeStoreIndex(const Instruction& instruction) {
    Value value = pop(instruction.location);
    Value indexValue = pop(instruction.location);
    int index = requireInt(indexValue, instruction.location, "array index");

    Value arrayValue;

    if (instruction.intArg >= 0) {
        checkLocalIndex(instruction.intArg, instruction.location);
        arrayValue = currentFrame().locals[instruction.intArg];
    } else {
        arrayValue = pop(instruction.location);
    }

    if (arrayValue.type != Type::IntArray) {
        throw RuntimeError(instruction.location, "STORE_INDEX target is not an array");
    }

    auto array = std::get<std::shared_ptr<ArrayValue>>(arrayValue.data);

    if (index < 0 || index >= static_cast<int>(array->elements.size())) {
        throw RuntimeError(instruction.location, "array index out of bounds");
    }

    array->elements[index] = value;
}

// Struct operations

void VM::executeMakeStruct(const Instruction& instruction) {
    int count = instruction.intArg;

    if (count < 0 || count > static_cast<int>(stack_.size())) {
        throw RuntimeError(instruction.location, "MAKE_STRUCT has invalid field count");
    }

    std::vector<Value> fields(count);

    for (int i = count - 1; i >= 0; --i) {
        fields[i] = pop(instruction.location);
    }

    push(Value::makeStruct(instruction.stringArg, std::move(fields)));
}

void VM::executeLoadField(const Instruction& instruction) {
    checkLocalIndex(instruction.intArg, instruction.location);

    Value& objectValue = currentFrame().locals[instruction.intArg];

    if (objectValue.type != Type::Struct) {
        throw RuntimeError(instruction.location, "LOAD_FIELD target is not a struct");
    }

    auto object = std::get<std::shared_ptr<StructValue>>(objectValue.data);

    int fieldIndex = instruction.intArg2;

    if (fieldIndex < 0 || fieldIndex >= static_cast<int>(object->fields.size())) {
        throw RuntimeError(instruction.location, "struct field index out of bounds");
    }

    push(object->fields[fieldIndex]);
}

void VM::executeStoreField(const Instruction& instruction) {
    checkLocalIndex(instruction.intArg, instruction.location);

    Value value = pop(instruction.location);

    Value& objectValue = currentFrame().locals[instruction.intArg];

    if (objectValue.type != Type::Struct) {
        throw RuntimeError(instruction.location, "STORE_FIELD target is not a struct");
    }

    auto object = std::get<std::shared_ptr<StructValue>>(objectValue.data);

    int fieldIndex = instruction.intArg2;

    if (fieldIndex < 0 || fieldIndex >= static_cast<int>(object->fields.size())) {
        throw RuntimeError(instruction.location, "struct field index out of bounds");
    }

    object->fields[fieldIndex] = value;
}

// Function calls

void VM::executeCall(const Instruction& instruction) {
    int functionIndex = instruction.intArg;

    checkFunctionIndex(functionIndex, instruction.location);

    const FunctionInfo& function = bytecode_->functions[functionIndex];

    int argumentCount = static_cast<int>(function.parameterTypes.size());

    if (argumentCount > static_cast<int>(stack_.size())) {
        throw RuntimeError(instruction.location, "CALL has not enough arguments on stack");
    }

    std::vector<Value> arguments(argumentCount);

    for (int i = argumentCount - 1; i >= 0; --i) {
        arguments[i] = pop(instruction.location);
    }

    for (int i = 0; i < argumentCount; ++i) {
    }

    CallFrame frame;
    frame.functionIndex = functionIndex;
    frame.returnIp = ip_ + 1;
    frame.locals.resize(function.localCount, Value::makeInt(0));

    for (int i = 0; i < argumentCount; ++i) {
        frame.locals[i] = arguments[i];
    }

    frames_.push_back(std::move(frame));

    ip_ = function.address;
}

// Operations

void VM::executeAdd(const Instruction& instruction) {
    Value right = pop(instruction.location);
    Value left = pop(instruction.location);

    if (left.type == Type::Int && right.type == Type::Int) {
        int a = std::get<int>(left.data);
        int b = std::get<int>(right.data);
        push(Value::makeInt(a + b));
        return;
    }

    if (left.type == Type::UInt && right.type == Type::UInt) {
        unsigned int a = std::get<unsigned int>(left.data);
        unsigned int b = std::get<unsigned int>(right.data);
        push(Value::makeUInt(a + b));
        return;
    }

    if (left.type == Type::Float && right.type == Type::Float) {
        double a = std::get<double>(left.data);
        double b = std::get<double>(right.data);
        push(Value::makeFloat(a + b));
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

    if (left.type == Type::Int && right.type == Type::Int) {
        int b = requireInt(right, instruction.location, "SUB");
        int a = requireInt(left, instruction.location, "SUB");

        push(Value::makeInt(a - b));
        return;
    }

    if (left.type == Type::UInt && right.type == Type::UInt) {
        unsigned int b = std::get<unsigned int>(right.data);
        unsigned int a = std::get<unsigned int>(left.data);

        push(Value::makeUInt(a - b));
        return;
    }

    if (left.type == Type::Float && right.type == Type::Float) {
        double b = requireFloat(right, instruction.location, "SUB");
        double a = requireFloat(left, instruction.location, "SUB");

        push(Value::makeFloat(a - b));
        return;
    }

    throw RuntimeError(
        instruction.location,
        "SUB cannot be applied to " +
        typeToString(left.type) + " and " + typeToString(right.type)
    );
}

void VM::executeMul(const Instruction& instruction) {
    Value right = pop(instruction.location);
    Value left = pop(instruction.location);

    if (left.type == Type::Int && right.type == Type::Int) {
        int b = requireInt(right, instruction.location, "MUL");
        int a = requireInt(left, instruction.location, "MUL");

        push(Value::makeInt(a * b));
        return;
    }

    if (left.type == Type::UInt && right.type == Type::UInt) {
        unsigned int b = std::get<unsigned int>(right.data);
        unsigned int a = std::get<unsigned int>(left.data);

        push(Value::makeUInt(a * b));
        return;
    }

    if (left.type == Type::Float && right.type == Type::Float) {
        double b = requireFloat(right, instruction.location, "MUL");
        double a = requireFloat(left, instruction.location, "MUL");

        push(Value::makeFloat(a * b));
        return;
    }

    throw RuntimeError(
        instruction.location,
        "MUL cannot be applied to " +
        typeToString(left.type) + " and " + typeToString(right.type)
    );
}

void VM::executeDiv(const Instruction& instruction) {
    Value right = pop(instruction.location);
    Value left = pop(instruction.location);

    if (left.type == Type::Int && right.type == Type::Int) {
        int b = requireInt(right, instruction.location, "DIV");
        int a = requireInt(left, instruction.location, "DIV");

        if (b == 0) {
            throw RuntimeError(instruction.location, "division by zero");
        }

        push(Value::makeInt(a / b));
        return;
    }

    if (left.type == Type::UInt && right.type == Type::UInt) {
        unsigned int b = std::get<unsigned int>(right.data);
        unsigned int a = std::get<unsigned int>(left.data);

        if (b == 0u) {
            throw RuntimeError(instruction.location, "division by zero");
        }

        push(Value::makeUInt(a / b));
        return;
    }

    if (left.type == Type::Float && right.type == Type::Float) {
        double b = requireFloat(right, instruction.location, "DIV");
        double a = requireFloat(left, instruction.location, "DIV");

        if (b == 0.0) {
            throw RuntimeError(instruction.location, "division by zero");
        }

        push(Value::makeFloat(a / b));
        return;
    }

    throw RuntimeError(
        instruction.location,
        "DIV cannot be applied to " +
        typeToString(left.type) + " and " + typeToString(right.type)
    );
}

void VM::executeMod(const Instruction& instruction) {
    Value right = pop(instruction.location);
    Value left = pop(instruction.location);

    if (left.type == Type::Int && right.type == Type::Int) {
        int b = requireInt(right, instruction.location, "MOD");
        int a = requireInt(left, instruction.location, "MOD");

        if (b == 0) {
            throw RuntimeError(instruction.location, "modulo by zero");
        }

        push(Value::makeInt(a % b));
        return;
    }

    if (left.type == Type::UInt && right.type == Type::UInt) {
        unsigned int b = std::get<unsigned int>(right.data);
        unsigned int a = std::get<unsigned int>(left.data);

        if (b == 0u) {
            throw RuntimeError(instruction.location, "modulo by zero");
        }

        push(Value::makeUInt(a % b));
        return;
    }

    throw RuntimeError(
        instruction.location,
        "MOD cannot be applied to " +
        typeToString(left.type) + " and " + typeToString(right.type)
    );
}

void VM::executeEqual(const Instruction& instruction) {
    Value right = pop(instruction.location);
    Value left = pop(instruction.location);

    push(Value::makeBool(valuesEqual(left, right)));
}

void VM::executeNotEqual(const Instruction& instruction) {
    Value right = pop(instruction.location);
    Value left = pop(instruction.location);

    push(Value::makeBool(!valuesEqual(left, right)));
}

void VM::executeIntComparison(const Instruction& instruction) {
    Value right = pop(instruction.location);
    Value left = pop(instruction.location);

    bool result = false;

    if (left.type == Type::Int && right.type == Type::Int) {
        int b = requireInt(right, instruction.location, opCodeToString(instruction.op));
        int a = requireInt(left, instruction.location, opCodeToString(instruction.op));

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
                throw RuntimeError(instruction.location, "invalid comparison opcode");
        }

        push(Value::makeBool(result));
        return;
    }

    if (left.type == Type::UInt && right.type == Type::UInt) {
        unsigned int b = std::get<unsigned int>(right.data);
        unsigned int a = std::get<unsigned int>(left.data);

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
                throw RuntimeError(instruction.location, "invalid comparison opcode");
        }

        push(Value::makeBool(result));
        return;
    }

    if (left.type == Type::Float && right.type == Type::Float) {
        double b = requireFloat(right, instruction.location, opCodeToString(instruction.op));
        double a = requireFloat(left, instruction.location, opCodeToString(instruction.op));

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
                throw RuntimeError(instruction.location, "invalid comparison opcode");
        }

        push(Value::makeBool(result));
        return;
    }

    throw RuntimeError(
        instruction.location,
        opCodeToString(instruction.op) + " cannot compare " +
        typeToString(left.type) + " and " + typeToString(right.type)
    );
}

} 
