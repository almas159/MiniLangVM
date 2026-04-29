#pragma once
#include "ast.hpp"
#include "bytecode.hpp"
#include "source_location.hpp"
#include <string>
#include <variant>
#include <vector>

struct Value {
    Type type = Type::Unknown;
    std::variant<int, bool, std::string> data;

    static Value makeInt(int value);
    static Value makeBool(bool value);
    static Value makeString(const std::string& value);
};

class VM {
public:
    int run(const Bytecode& bytecode);

private:
    const Bytecode* bytecode_ = nullptr;

    std::vector<Value> stack_;
    std::vector<Value> locals_;

    int ip_ = 0;
    bool running_ = false;
    int returnCode_ = 0;

    void execute(const Instruction& instruction);

    void push(const Value& value);
    Value pop(SourceLocation location);

    int requireInt(const Value& value, SourceLocation location, const std::string& context);
    bool requireBool(const Value& value, SourceLocation location, const std::string& context);
    std::string requireString(const Value& value, SourceLocation location, const std::string& context);

    void checkLocalIndex(int index, SourceLocation location);
    void checkJumpAddress(int address, SourceLocation location);

    std::string valueToString(const Value& value) const;

    void executeAdd(const Instruction& instruction);
    void executeSub(const Instruction& instruction);
    void executeMul(const Instruction& instruction);
    void executeDiv(const Instruction& instruction);
    void executeMod(const Instruction& instruction);

    void executeEqual(const Instruction& instruction);
    void executeNotEqual(const Instruction& instruction);

    void executeIntComparison(const Instruction& instruction);
};
