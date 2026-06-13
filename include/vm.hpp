#pragma once
#include "ast.hpp"
#include "bytecode.hpp"
#include "source_location.hpp"

#include <memory>
#include <string>
#include <variant>
#include <vector>

namespace minilang {

struct ArrayValue;
struct StructValue;

struct Value {
    Type type = Type::Unknown;
    std::variant<std::monostate, int, unsigned int, double, bool, std::string, std::shared_ptr<ArrayValue>, std::shared_ptr<StructValue>> data;

    static Value makeVoid();
    static Value makeInt(int value);
    static Value makeUInt(unsigned int value);
    static Value makeFloat(double value);
    static Value makeBool(bool value);
    static Value makeString(const std::string& value);
    static Value makeIntArray(std::vector<Value> elements);
    static Value makeStruct(const std::string& typeName, std::vector<Value> fields);
};

struct ArrayValue {
    std::vector<Value> elements;
};

struct StructValue {
    std::string typeName;
    std::vector<Value> fields;
};

class VM {
public:
    int run(const Bytecode& bytecode);

private:
    struct CallFrame {
        int functionIndex = -1;
        int returnIp = -1;
        std::vector<Value> locals;
    };

    const Bytecode* bytecode_ = nullptr;

    std::vector<Value> stack_;
    std::vector<CallFrame> frames_;

    int ip_ = 0;
    bool running_ = false;
    int returnCode_ = 0;

    void execute(const Instruction& instruction);

    CallFrame& currentFrame();

    void push(const Value& value);
    Value pop(SourceLocation location);

    int requireInt(const Value& value, SourceLocation location, const std::string& context);
    unsigned int requireUInt(const Value& value, SourceLocation location, const std::string& context);
    double requireFloat(const Value& value, SourceLocation location, const std::string& context);
    bool requireBool(const Value& value, SourceLocation location, const std::string& context);

    void checkLocalIndex(int index, SourceLocation location);
    void checkJumpAddress(int address, SourceLocation location);
    void checkFunctionIndex(int index, SourceLocation location);

    std::string valueToString(const Value& value) const;

    void executeCall(const Instruction& instruction);

    void executeMakeArray(const Instruction& instruction);
    void executeLoadIndex(const Instruction& instruction);
    void executeStoreIndex(const Instruction& instruction);

    void executeMakeStruct(const Instruction& instruction);
    void executeLoadField(const Instruction& instruction);
    void executeStoreField(const Instruction& instruction);

    void executeAdd(const Instruction& instruction);
    void executeSub(const Instruction& instruction);
    void executeMul(const Instruction& instruction);
    void executeDiv(const Instruction& instruction);
    void executeMod(const Instruction& instruction);

    void executeEqual(const Instruction& instruction);
    void executeNotEqual(const Instruction& instruction);

    void executeIntComparison(const Instruction& instruction);
};

} 
