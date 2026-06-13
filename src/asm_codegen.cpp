#include "asm_codegen.hpp"
#include "error.hpp"

#include <algorithm>
#include <bit>
#include <cctype>
#include <cstdint>
#include <fstream>
#include <iomanip>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

namespace minilang {

void AsmGenerator::setDiagnostic(SourceLocation location, const std::string& message) const {
    if (diagnostic_) {
        return;
    }

    diagnostic_ = Diagnostic(location, message);
}

void AsmGenerator::failVoid(SourceLocation location, const std::string& message) const {
    setDiagnostic(location, message);
}

Result<void> AsmGenerator::generateExpected(const Program& program, const std::string& outputPath) {
    diagnostic_.reset();

    generate(program, outputPath);

    if (diagnostic_) {
        return std::unexpected(*diagnostic_);
    }

    return {};
}

void AsmGenerator::generate(const Program& program, const std::string& outputPath) {
    std::ofstream out(outputPath);

    if (!out) {
        failVoid(SourceLocation(), "cannot open asm output file: " + outputPath);
        return;
    }

    out_ = &out;
    labelCounter_ = 0;
    loopStack_.clear();

    structFieldCounts_.clear();
    collectStructDeclarations(program);

    *out_ << "default rel\n";
    *out_ << "global main\n";
    *out_ << "extern printf\n";
    *out_ << "extern exit\n\n";

    *out_ << "section .data\n";
    *out_ << "fmt_int: db \"%ld\", 0\n";
    *out_ << "fmt_uint: db \"%lu\", 0\n";
    *out_ << "fmt_float: db \"%f\", 0\n";
    *out_ << "fmt_str: db \"%s\", 0\n";
    *out_ << "fmt_nl: db 10, 0\n";
    *out_ << "fmt_bounds: db \"runtime error: array index out of bounds\", 10, 0\n";
    *out_ << "fmt_shift: db \"runtime error: invalid shift count\", 10, 0\n\n";

    *out_ << "section .text\n\n";

    *out_ << "ml_array_bounds_error:\n";
    *out_ << "    lea rdi, [rel fmt_bounds]\n";
    *out_ << "    xor eax, eax\n";
    *out_ << "    call printf\n";
    *out_ << "    mov rdi, 1\n";
    *out_ << "    call exit\n\n";

    *out_ << "ml_shift_error:\n";
    *out_ << "    lea rdi, [rel fmt_shift]\n";
    *out_ << "    xor eax, eax\n";
    *out_ << "    call printf\n";
    *out_ << "    mov rdi, 1\n";
    *out_ << "    call exit\n\n";

    for (const auto& function : program.functions) {
        emitFunction(*function);
    }

    *out_ << "section .note.GNU-stack noalloc noexec nowrite progbits\n";

    out_ = nullptr;
}

std::string AsmGenerator::makeLabel(const std::string& base) {
    return base + "_" + std::to_string(labelCounter_++);
}

std::string AsmGenerator::mangleFunctionName(const std::string& name) const {
    if (name == "main") {
        return "main";
    }

    std::string result = "ml_";

    for (char ch : name) {
        unsigned char c = static_cast<unsigned char>(ch);

        if (std::isalnum(c) || ch == '_') {
            result.push_back(ch);
        } else {
            result.push_back('_');
        }
    }

    return result;
}

std::string AsmGenerator::emitStringLiteralData(const std::string& value) {
    std::string label = makeLabel("str");

    *out_ << "section .data\n";
    *out_ << label << ": db ";

    for (std::size_t i = 0; i < value.size(); ++i) {
        if (i > 0) {
            *out_ << ", ";
        }

        unsigned char ch = static_cast<unsigned char>(value[i]);
        *out_ << static_cast<int>(ch);
    }

    if (!value.empty()) {
        *out_ << ", ";
    }

    *out_ << "0\n";
    *out_ << "section .text\n";

    return label;
}

std::string AsmGenerator::emitFloatLiteralData(double value) {
    std::string label = makeLabel("flt");

    std::uint64_t bits = std::bit_cast<std::uint64_t>(value);

    *out_ << "section .data\n";
    *out_ << label << ": dq 0x"
          << std::hex << bits << std::dec
          << "\n";
    *out_ << "section .text\n";

    return label;
}

int AsmGenerator::localOffset(int localIndex) const {
    return (localIndex + 1) * 8;
}

int AsmGenerator::arrayBaseOffset(int localIndex, SourceLocation location) const {
    auto found = arrayBaseOffsets_.find(localIndex);

    if (found == arrayBaseOffsets_.end()) {
        return fail<int>(location, "internal error: array has no stack offset in asm backend");
    }

    return found->second;
}

int AsmGenerator::arraySize(int localIndex, SourceLocation location) const {
    auto found = arraySizes_.find(localIndex);

    if (found == arraySizes_.end()) {
        return fail<int>(location, "internal error: array has no size in asm backend");
    }

    return found->second;
}

int AsmGenerator::structBaseOffset(int localIndex, SourceLocation location) const {
    auto found = structBaseOffsets_.find(localIndex);

    if (found == structBaseOffsets_.end()) {
        return fail<int>(location, "internal error: struct has no stack offset in asm backend");
    }

    return found->second;
}

int AsmGenerator::structFieldCount(const std::string& structName, SourceLocation location) const {
    auto found = structFieldCounts_.find(structName);

    if (found == structFieldCounts_.end()) {
        return fail<int>(location, "internal error: unknown struct type in asm backend");
    }

    return found->second;
}

void AsmGenerator::collectStructDeclarations(const Program& program) {
    for (const auto& structDecl : program.structs) {
        structFieldCounts_[structDecl->name] = static_cast<int>(structDecl->fields.size());
    }
}

void AsmGenerator::collectArrayLocalsInBlock(const BlockStmt& block, int& nextOffset) {
    for (const auto& statement : block.statements) {
        collectArrayLocalsInStmt(*statement, nextOffset);
    }
}

void AsmGenerator::collectArrayLocalsInStmt(const Stmt& stmt, int& nextOffset) {
    if (auto* s = dynamic_cast<const BlockStmt*>(&stmt)) {
        collectArrayLocalsInBlock(*s, nextOffset);
        return;
    }

    if (auto* s = dynamic_cast<const VarDeclStmt*>(&stmt)) {
        if (s->type == Type::IntArray) {
            if (s->localIndex < 0 || s->arraySize <= 0) {
                failVoid(s->loc, "invalid int32 array local variable");
    return;
            }

            int baseOffset = nextOffset + 8;

            arrayBaseOffsets_[s->localIndex] = baseOffset;
            arraySizes_[s->localIndex] = s->arraySize;

            nextOffset += s->arraySize * 8;
        }

        if (s->type == Type::Struct) {
            if (s->localIndex < 0 || s->structName.empty()) {
                failVoid(s->loc, "invalid struct local variable");
    return;
            }

            int count = structFieldCount(s->structName, s->loc);
            int baseOffset = nextOffset + 8;

            structBaseOffsets_[s->localIndex] = baseOffset;
            structLocalTypes_[s->localIndex] = s->structName;

            nextOffset += count * 8;
        }

        return;
    }

    if (auto* s = dynamic_cast<const IfStmt*>(&stmt)) {
        collectArrayLocalsInStmt(*s->thenBranch, nextOffset);

        if (s->elseBranch) {
            collectArrayLocalsInStmt(*s->elseBranch, nextOffset);
        }

        return;
    }

    if (auto* s = dynamic_cast<const WhileStmt*>(&stmt)) {
        collectArrayLocalsInStmt(*s->body, nextOffset);
        return;
    }

    if (auto* s = dynamic_cast<const ForStmt*>(&stmt)) {
        if (s->initializer) {
            collectArrayLocalsInStmt(*s->initializer, nextOffset);
        }

        if (s->update) {
            collectArrayLocalsInStmt(*s->update, nextOffset);
        }

        collectArrayLocalsInStmt(*s->body, nextOffset);
        return;
    }

    if (auto* s = dynamic_cast<const SwitchStmt*>(&stmt)) {
        for (const auto& switchCase : s->cases) {
            for (const auto& statement : switchCase.statements) {
                collectArrayLocalsInStmt(*statement, nextOffset);
            }
        }

        for (const auto& statement : s->defaultStatements) {
            collectArrayLocalsInStmt(*statement, nextOffset);
        }

        return;
    }
}

void AsmGenerator::emitArrayBoundsCheck(const std::string& indexReg,
                                        int size,
                                        SourceLocation location) {
    (void)location;

    *out_ << "    cmp " << indexReg << ", 0\n";
    *out_ << "    jl ml_array_bounds_error\n";
    *out_ << "    cmp " << indexReg << ", " << size << "\n";
    *out_ << "    jge ml_array_bounds_error\n";
}

void AsmGenerator::unsupported(SourceLocation location, const std::string& feature) const {
    failVoid(location,
        "x86-64 asm backend does not support " + feature + " yet");
    return;
}

void AsmGenerator::emitFunction(const FunctionDecl& function) {
    if (function.returnType != Type::Int &&
        function.returnType != Type::UInt &&
        function.returnType != Type::Float) {
        failVoid(function.loc, "non-int32/uint/float64 function return type");
    return;
    }

    static const std::vector<std::string> gpArgRegs = {
        "rdi", "rsi", "rdx", "rcx", "r8", "r9"
    };

    static const std::vector<std::string> fpArgRegs = {
        "xmm0", "xmm1", "xmm2", "xmm3",
        "xmm4", "xmm5", "xmm6", "xmm7"
    };

    int gpIndex = 0;
    int fpIndex = 0;

    for (const Parameter& parameter : function.parameters) {
        if (parameter.type == Type::Float) {
            if (fpIndex >= static_cast<int>(fpArgRegs.size())) {
                failVoid(parameter.loc, "functions with more than 8 float64 parameters");
    return;
            }

            fpIndex++;
            continue;
        }

        if (parameter.type == Type::Int ||
            parameter.type == Type::UInt ||
            parameter.type == Type::Bool ||
            parameter.type == Type::String) {
            if (gpIndex >= static_cast<int>(gpArgRegs.size())) {
                failVoid(parameter.loc, "functions with more than 6 integer/string parameters");
    return;
            }

            gpIndex++;
            continue;
        }

        failVoid(parameter.loc, "non-int32/uint/bool/string/float64 function parameters");
    return;
    }

    std::string label = mangleFunctionName(function.name);

    arrayBaseOffsets_.clear();
    arraySizes_.clear();
    structBaseOffsets_.clear();
    structLocalTypes_.clear();

    int rawStackSize = function.localCount * 8;

    collectArrayLocalsInBlock(*function.body, rawStackSize);

    stackSize_ = rawStackSize;

    if (stackSize_ % 16 != 0) {
        stackSize_ += 16 - (stackSize_ % 16);
    }

    currentEndLabel_ = makeLabel(label + "_end");

    *out_ << label << ":\n";
    *out_ << "    push rbp\n";
    *out_ << "    mov rbp, rsp\n";

    if (stackSize_ > 0) {
        *out_ << "    sub rsp, " << stackSize_ << "\n";
    }

    gpIndex = 0;
    fpIndex = 0;

    for (const Parameter& parameter : function.parameters) {
        int offset = localOffset(parameter.localIndex);

        if (parameter.type == Type::Float) {
            *out_ << "    movsd [rbp - " << offset << "], "
                  << fpArgRegs[fpIndex] << "\n";
            fpIndex++;
        } else {
            *out_ << "    mov [rbp - " << offset << "], "
                  << gpArgRegs[gpIndex] << "\n";
            gpIndex++;
        }
    }

    emitBlock(*function.body);

    if (function.returnType == Type::Float) {
        *out_ << "    xorpd xmm0, xmm0\n";
    } else {
        *out_ << "    mov rax, 0\n";
    }

    *out_ << currentEndLabel_ << ":\n";
    *out_ << "    leave\n";
    *out_ << "    ret\n\n";
}

void AsmGenerator::emitBlock(const BlockStmt& block) {
    for (const auto& statement : block.statements) {
        emitStmt(*statement);
    }
}

void AsmGenerator::emitStmt(const Stmt& stmt) {
    if (dynamic_cast<const EmptyStmt*>(&stmt)) {
        return;
    }

    if (auto* s = dynamic_cast<const BlockStmt*>(&stmt)) {
        emitBlock(*s);
        return;
    }

    if (auto* s = dynamic_cast<const VarDeclStmt*>(&stmt)) {
        emitVarDeclStmt(*s);
        return;
    }

    if (auto* s = dynamic_cast<const AssignStmt*>(&stmt)) {
        emitAssignStmt(*s);
        return;
    }

    if (auto* s = dynamic_cast<const ArrayAssignStmt*>(&stmt)) {
        emitArrayAssignStmt(*s);
        return;
    }

    if (auto* s = dynamic_cast<const FieldAssignStmt*>(&stmt)) {
        emitFieldAssignStmt(*s);
        return;
    }

    if (auto* s = dynamic_cast<const IfStmt*>(&stmt)) {
        emitIfStmt(*s);
        return;
    }

    if (auto* s = dynamic_cast<const WhileStmt*>(&stmt)) {
        emitWhileStmt(*s);
        return;
    }

    if (auto* s = dynamic_cast<const ForStmt*>(&stmt)) {
        emitForStmt(*s);
        return;
    }

    if (auto* s = dynamic_cast<const ReturnStmt*>(&stmt)) {
        emitReturnStmt(*s);
        return;
    }

    if (auto* s = dynamic_cast<const PrintStmt*>(&stmt)) {
        emitPrintStmt(*s);
        return;
    }

    if (auto* s = dynamic_cast<const ExprStmt*>(&stmt)) {
        emitExprStmt(*s);
        return;
    }

    if (auto* s = dynamic_cast<const BreakStmt*>(&stmt)) {
        emitBreakStmt(*s);
        return;
    }

    if (auto* s = dynamic_cast<const ContinueStmt*>(&stmt)) {
        emitContinueStmt(*s);
        return;
    }

    failVoid(stmt.loc, "this statement");
    return;
}

void AsmGenerator::emitStructLiteralToMemory(const StructLiteralExpr& expr,
                                             int baseOffset,
                                             SourceLocation location) {
    (void)location;

    std::vector<const StructLiteralField*> orderedFields;
    orderedFields.reserve(expr.fields.size());

    for (const auto& field : expr.fields) {
        if (field.fieldIndex < 0) {
            failVoid(field.loc, "struct literal without semantic field index");
    return;
        }

        orderedFields.push_back(&field);
    }

    std::sort(
        orderedFields.begin(),
        orderedFields.end(),
        [](const StructLiteralField* left, const StructLiteralField* right) {
            return left->fieldIndex < right->fieldIndex;
        }
    );

    for (const StructLiteralField* field : orderedFields) {
        if (field->value->inferredType != Type::Int &&
            field->value->inferredType != Type::UInt &&
            field->value->inferredType != Type::Bool &&
            field->value->inferredType != Type::String) {
            failVoid(field->value->loc, "non-int32/bool/string struct field in asm backend");
    return;
        }

        emitExpr(*field->value);

        int offset = baseOffset + field->fieldIndex * 8;
        *out_ << "    mov [rbp - " << offset << "], rax\n";
    }
}

void AsmGenerator::emitVarDeclStmt(const VarDeclStmt& stmt) {
    if (stmt.type == Type::Struct) {
        int baseOffset = structBaseOffset(stmt.localIndex, stmt.loc);
        int count = structFieldCount(stmt.structName, stmt.loc);

        if (stmt.initializer) {
            auto* literal = dynamic_cast<StructLiteralExpr*>(stmt.initializer.get());

            if (!literal) {
                failVoid(stmt.initializer->loc, "non-literal struct initializer in asm backend");
    return;
            }

            emitStructLiteralToMemory(*literal, baseOffset, stmt.loc);
        } else {
            for (int i = 0; i < count; ++i) {
                *out_ << "    mov qword [rbp - " << (baseOffset + i * 8) << "], 0\n";
            }
        }

        *out_ << "    lea rax, [rbp - " << baseOffset << "]\n";
        *out_ << "    mov [rbp - " << localOffset(stmt.localIndex) << "], rax\n";
        return;
    }

    if (stmt.type == Type::IntArray) {
        int baseOffset = arrayBaseOffset(stmt.localIndex, stmt.loc);
        int size = arraySize(stmt.localIndex, stmt.loc);

        if (stmt.initializer) {
            auto* arrayLiteral = dynamic_cast<ArrayLiteralExpr*>(stmt.initializer.get());

            if (!arrayLiteral) {
                failVoid(stmt.initializer->loc, "non-literal array initializer in asm backend");
    return;
            }

            if (static_cast<int>(arrayLiteral->elements.size()) != size) {
                failVoid(stmt.initializer->loc, "array initializer size mismatch in asm backend");
    return;
            }

            for (int i = 0; i < size; ++i) {
                const Expr& element = *arrayLiteral->elements[i];

                if (element.inferredType != Type::Int && element.inferredType != Type::Bool) {
                    failVoid(element.loc, "non-int32 array element in asm backend");
    return;
                }

                emitExpr(element);

                *out_ << "    mov [rbp - " << (baseOffset + i * 8) << "], rax\n";
            }
        } else {
            for (int i = 0; i < size; ++i) {
                *out_ << "    mov qword [rbp - " << (baseOffset + i * 8) << "], 0\n";
            }
        }

        *out_ << "    lea rax, [rbp - " << baseOffset << "]\n";
        *out_ << "    mov [rbp - " << localOffset(stmt.localIndex) << "], rax\n";
        return;
    }

    if (stmt.type != Type::Int &&
        stmt.type != Type::UInt &&
        stmt.type != Type::Bool &&
        stmt.type != Type::String &&
        stmt.type != Type::Float) {
        failVoid(stmt.loc, "non-int32/uint/bool/string/float64 local variables");
    return;
    }

    if (stmt.localIndex < 0) {
        failVoid(stmt.loc, "variable without local index");
    return;
    }

    if (stmt.initializer) {
        emitExpr(*stmt.initializer);
    } else {
        if (stmt.type == Type::String) {
            std::string label = emitStringLiteralData("");
            *out_ << "    lea rax, [rel " << label << "]\n";
        } else if (stmt.type == Type::Float) {
            *out_ << "    xorpd xmm0, xmm0\n";
        } else {
            *out_ << "    mov rax, 0\n";
        }
    }

    if (stmt.type == Type::Float) {
        *out_ << "    movsd [rbp - " << localOffset(stmt.localIndex) << "], xmm0\n";
    } else {
        *out_ << "    mov [rbp - " << localOffset(stmt.localIndex) << "], rax\n";
    }
}

void AsmGenerator::emitAssignStmt(const AssignStmt& stmt) {
    if (stmt.localIndex < 0) {
        failVoid(stmt.loc, "assignment without local index");
    return;
    }

    emitExpr(*stmt.value);

    if (stmt.value->inferredType == Type::Float) {
        *out_ << "    movsd [rbp - " << localOffset(stmt.localIndex) << "], xmm0\n";
    } else {
        *out_ << "    mov [rbp - " << localOffset(stmt.localIndex) << "], rax\n";
    }
}

void AsmGenerator::emitArrayAssignStmt(const ArrayAssignStmt& stmt) {
    if (stmt.localIndex < 0) {
        failVoid(stmt.loc, "array assignment without local index");
    return;
    }

    int baseOffset = arrayBaseOffset(stmt.localIndex, stmt.loc);
    int size = arraySize(stmt.localIndex, stmt.loc);

    emitExpr(*stmt.index);
    *out_ << "    mov rbx, rax\n";

    emitArrayBoundsCheck("rbx", size, stmt.index->loc);

    emitExpr(*stmt.value);

    *out_ << "    mov rcx, rbx\n";
    *out_ << "    imul rcx, 8\n";
    *out_ << "    lea rdx, [rbp - " << baseOffset << "]\n";
    *out_ << "    sub rdx, rcx\n";
    *out_ << "    mov [rdx], rax\n";
}

void AsmGenerator::emitFieldAssignStmt(const FieldAssignStmt& stmt) {
    if (stmt.localIndex < 0 || stmt.fieldIndex < 0) {
        failVoid(stmt.loc, "field assignment without semantic indexes");
    return;
    }

    int baseOffset = structBaseOffset(stmt.localIndex, stmt.loc);

    emitExpr(*stmt.value);

    int offset = baseOffset + stmt.fieldIndex * 8;
    *out_ << "    mov [rbp - " << offset << "], rax\n";
}

void AsmGenerator::emitIfStmt(const IfStmt& stmt) {
    std::string elseLabel = makeLabel("if_else");
    std::string endLabel = makeLabel("if_end");

    emitExpr(*stmt.condition);

    *out_ << "    cmp rax, 0\n";
    *out_ << "    je " << elseLabel << "\n";

    emitStmt(*stmt.thenBranch);

    *out_ << "    jmp " << endLabel << "\n";
    *out_ << elseLabel << ":\n";

    if (stmt.elseBranch) {
        emitStmt(*stmt.elseBranch);
    }

    *out_ << endLabel << ":\n";
}

void AsmGenerator::emitWhileStmt(const WhileStmt& stmt) {
    std::string startLabel = makeLabel("while_start");
    std::string endLabel = makeLabel("while_end");

    *out_ << startLabel << ":\n";

    emitExpr(*stmt.condition);

    *out_ << "    cmp rax, 0\n";
    *out_ << "    je " << endLabel << "\n";

    loopStack_.push_back(LoopContext{endLabel, startLabel});

    emitStmt(*stmt.body);

    loopStack_.pop_back();

    *out_ << "    jmp " << startLabel << "\n";
    *out_ << endLabel << ":\n";
}

void AsmGenerator::emitForStmt(const ForStmt& stmt) {
    if (stmt.initializer) {
        emitStmt(*stmt.initializer);
    }

    std::string startLabel = makeLabel("for_start");
    std::string updateLabel = makeLabel("for_update");
    std::string endLabel = makeLabel("for_end");

    *out_ << startLabel << ":\n";

    emitExpr(*stmt.condition);

    *out_ << "    cmp rax, 0\n";
    *out_ << "    je " << endLabel << "\n";

    loopStack_.push_back(LoopContext{endLabel, updateLabel});

    emitStmt(*stmt.body);

    loopStack_.pop_back();

    *out_ << updateLabel << ":\n";

    if (stmt.update) {
        emitStmt(*stmt.update);
    }

    *out_ << "    jmp " << startLabel << "\n";
    *out_ << endLabel << ":\n";
}

void AsmGenerator::emitBreakStmt(const BreakStmt& stmt) {
    if (loopStack_.empty()) {
        failVoid(stmt.loc, "break outside loop in asm backend");
    return;
    }

    *out_ << "    jmp " << loopStack_.back().breakLabel << "\n";
}

void AsmGenerator::emitContinueStmt(const ContinueStmt& stmt) {
    if (loopStack_.empty()) {
        failVoid(stmt.loc, "continue outside loop in asm backend");
    return;
    }

    *out_ << "    jmp " << loopStack_.back().continueLabel << "\n";
}

void AsmGenerator::emitReturnStmt(const ReturnStmt& stmt) {
    if (stmt.value) {
        emitExpr(*stmt.value);
    } else {
        *out_ << "    mov rax, 0\n";
    }

    *out_ << "    jmp " << currentEndLabel_ << "\n";
}

void AsmGenerator::emitPrintStmt(const PrintStmt& stmt) {
    for (const auto& argument : stmt.arguments) {
        const Expr& arg = *argument;

        if (arg.inferredType == Type::Int || arg.inferredType == Type::Bool) {
            emitExpr(arg);

            *out_ << "    mov rsi, rax\n";
            *out_ << "    lea rdi, [rel fmt_int]\n";
            *out_ << "    xor eax, eax\n";
            *out_ << "    call printf\n";
            continue;
        }

        if (arg.inferredType == Type::UInt) {
            emitExpr(arg);

            *out_ << "    mov rsi, rax\n";
            *out_ << "    lea rdi, [rel fmt_uint]\n";
            *out_ << "    xor eax, eax\n";
            *out_ << "    call printf\n";
            continue;
        }

        if (arg.inferredType == Type::Float) {
            emitExpr(arg);

            *out_ << "    lea rdi, [rel fmt_float]\n";
            *out_ << "    mov eax, 1\n";
            *out_ << "    call printf\n";
            continue;
        }

        if (arg.inferredType == Type::String) {
            emitExpr(arg);

            *out_ << "    mov rsi, rax\n";
            *out_ << "    lea rdi, [rel fmt_str]\n";
            *out_ << "    xor eax, eax\n";
            *out_ << "    call printf\n";
            continue;
        }

        failVoid(arg.loc, "print of this value type in asm backend");
    return;
    }

    *out_ << "    lea rdi, [rel fmt_nl]\n";
    *out_ << "    xor eax, eax\n";
    *out_ << "    call printf\n";
}

void AsmGenerator::emitExprStmt(const ExprStmt& stmt) {
    emitExpr(*stmt.expression);
}

void AsmGenerator::emitExpr(const Expr& expr) {
    if (auto* e = dynamic_cast<const IntLiteralExpr*>(&expr)) {
        emitIntLiteralExpr(*e);
        return;
    }

    if (auto* e = dynamic_cast<const FloatLiteralExpr*>(&expr)) {
        emitFloatLiteralExpr(*e);
        return;
    }

    if (auto* e = dynamic_cast<const BoolLiteralExpr*>(&expr)) {
        emitBoolLiteralExpr(*e);
        return;
    }

    if (auto* e = dynamic_cast<const StringLiteralExpr*>(&expr)) {
        emitStringLiteralExpr(*e);
        return;
    }

    if (auto* e = dynamic_cast<const VariableExpr*>(&expr)) {
        emitVariableExpr(*e);
        return;
    }

    if (auto* e = dynamic_cast<const IndexExpr*>(&expr)) {
        emitIndexExpr(*e);
        return;
    }

    if (auto* e = dynamic_cast<const FieldAccessExpr*>(&expr)) {
        emitFieldAccessExpr(*e);
        return;
    }

    if (auto* e = dynamic_cast<const CallExpr*>(&expr)) {
        emitCallExpr(*e);
        return;
    }

    if (auto* e = dynamic_cast<const BinaryExpr*>(&expr)) {
        emitBinaryExpr(*e);
        return;
    }

    if (auto* e = dynamic_cast<const UnaryExpr*>(&expr)) {
        emitUnaryExpr(*e);
        return;
    }

    if (auto* e = dynamic_cast<const CastExpr*>(&expr)) {
        emitCastExpr(*e);
        return;
    }

    failVoid(expr.loc, "this expression");
    return;
}

void AsmGenerator::emitIntLiteralExpr(const IntLiteralExpr& expr) {
    *out_ << "    mov rax, " << expr.value << "\n";
}

void AsmGenerator::emitFloatLiteralExpr(const FloatLiteralExpr& expr) {
    std::string label = emitFloatLiteralData(expr.value);

    *out_ << "    movsd xmm0, [rel " << label << "]\n";
}

void AsmGenerator::emitBoolLiteralExpr(const BoolLiteralExpr& expr) {
    *out_ << "    mov rax, " << (expr.value ? 1 : 0) << "\n";
}

void AsmGenerator::emitStringLiteralExpr(const StringLiteralExpr& expr) {
    std::string label = emitStringLiteralData(expr.value);

    *out_ << "    lea rax, [rel " << label << "]\n";
}

void AsmGenerator::emitVariableExpr(const VariableExpr& expr) {
    if (expr.localIndex < 0) {
        failVoid(expr.loc, "variable without local index");
    return;
    }

    if (expr.inferredType == Type::Float) {
        *out_ << "    movsd xmm0, [rbp - " << localOffset(expr.localIndex) << "]\n";
    } else {
        *out_ << "    mov rax, [rbp - " << localOffset(expr.localIndex) << "]\n";
    }
}

void AsmGenerator::emitIndexExpr(const IndexExpr& expr) {
    if (expr.localIndex < 0) {
        failVoid(expr.loc, "array index expression without local index");
    return;
    }

    int baseOffset = arrayBaseOffset(expr.localIndex, expr.loc);
    int size = arraySize(expr.localIndex, expr.loc);

    emitExpr(*expr.index);

    *out_ << "    mov rbx, rax\n";

    emitArrayBoundsCheck("rbx", size, expr.index->loc);

    *out_ << "    mov rcx, rbx\n";
    *out_ << "    imul rcx, 8\n";
    *out_ << "    lea rdx, [rbp - " << baseOffset << "]\n";
    *out_ << "    sub rdx, rcx\n";
    *out_ << "    mov rax, [rdx]\n";
}

void AsmGenerator::emitFieldAccessExpr(const FieldAccessExpr& expr) {
    if (expr.localIndex < 0 || expr.fieldIndex < 0) {
        failVoid(expr.loc, "field access without semantic indexes");
    return;
    }

    int baseOffset = structBaseOffset(expr.localIndex, expr.loc);
    int offset = baseOffset + expr.fieldIndex * 8;

    *out_ << "    mov rax, [rbp - " << offset << "]\n";
}

void AsmGenerator::emitCallExpr(const CallExpr& expr) {
    static const std::vector<std::string> gpArgRegs = {
        "rdi", "rsi", "rdx", "rcx", "r8", "r9"
    };

    static const std::vector<std::string> fpArgRegs = {
        "xmm0", "xmm1", "xmm2", "xmm3",
        "xmm4", "xmm5", "xmm6", "xmm7"
    };

    if (expr.callee == "len") {
        // len for int32 arrays
        if (expr.arguments.size() != 1) {
            failVoid(expr.loc, "len with wrong argument count in asm backend");
    return;
        }

        auto* variable = dynamic_cast<VariableExpr*>(expr.arguments[0].get());

        if (!variable || variable->inferredType != Type::IntArray) {
            failVoid(expr.arguments[0]->loc, "len for non-array value in asm backend");
    return;
        }

        int size = arraySize(variable->localIndex, variable->loc);

        *out_ << "    mov rax, " << size << "\n";
        return;
    }

    if (expr.callee == "assert") {
        if (expr.arguments.size() != 1 && expr.arguments.size() != 2) {
            failVoid(expr.loc, "assert with wrong argument count in asm backend");
    return;
        }

        if (expr.arguments[0]->inferredType != Type::Bool) {
            failVoid(expr.arguments[0]->loc, "assert condition that is not bool in asm backend");
    return;
        }

        std::string message = "assertion failed";

        if (expr.arguments.size() == 2) {
            auto* literal = dynamic_cast<StringLiteralExpr*>(expr.arguments[1].get());

            if (!literal) {
                failVoid(expr.arguments[1]->loc, "non-literal assert message in asm backend");
    return;
            }

            message = literal->value;
        }

        std::string failLabel = makeLabel("assert_fail");
        std::string endLabel = makeLabel("assert_end");
        std::string messageLabel = emitStringLiteralData(message);

        emitExpr(*expr.arguments[0]);

        *out_ << "    cmp rax, 0\n";
        *out_ << "    je " << failLabel << "\n";
        *out_ << "    jmp " << endLabel << "\n";
        *out_ << failLabel << ":\n";
        *out_ << "    lea rsi, [rel " << messageLabel << "]\n";
        *out_ << "    lea rdi, [rel fmt_str]\n";
        *out_ << "    xor eax, eax\n";
        *out_ << "    call printf\n";
        *out_ << "    lea rdi, [rel fmt_nl]\n";
        *out_ << "    xor eax, eax\n";
        *out_ << "    call printf\n";
        *out_ << "    mov rdi, 1\n";
        *out_ << "    call exit\n";
        *out_ << endLabel << ":\n";
        return;
    }

    if (expr.callee == "input" ||
        expr.callee == "exit" ||
        expr.callee == "panic") {
        failVoid(expr.loc, "built-in function '" + expr.callee + "' in asm backend");
    return;
    }

    struct ArgPlacement {
        bool isFloat = false;
        int regIndex = 0;
    };

    std::vector<ArgPlacement> placements;
    placements.reserve(expr.arguments.size());

    int gpIndex = 0;
    int fpIndex = 0;

    for (const auto& argument : expr.arguments) {
        Type type = argument->inferredType;

        if (type == Type::Float) {
            if (fpIndex >= static_cast<int>(fpArgRegs.size())) {
                failVoid(argument->loc, "function call with more than 8 float64 arguments");
    return;
            }

            placements.push_back(ArgPlacement{true, fpIndex});
            fpIndex++;
            continue;
        }

        if (type == Type::Int ||
            type == Type::UInt ||
            type == Type::Bool ||
            type == Type::String) {
            if (gpIndex >= static_cast<int>(gpArgRegs.size())) {
                failVoid(argument->loc, "function call with more than 6 integer/string arguments");
    return;
            }

            placements.push_back(ArgPlacement{false, gpIndex});
            gpIndex++;
            continue;
        }

        failVoid(argument->loc, "non-int32/uint/bool/string/float64 function argument");
    return;
    }

    for (std::size_t i = 0; i < expr.arguments.size(); ++i) {
        const Expr& argument = *expr.arguments[i];

        emitExpr(argument);

        if (placements[i].isFloat) {
            *out_ << "    sub rsp, 8\n";
            *out_ << "    movsd [rsp], xmm0\n";
        } else {
            *out_ << "    push rax\n";
        }
    }

    for (int i = static_cast<int>(expr.arguments.size()) - 1; i >= 0; --i) {
        if (placements[i].isFloat) {
            *out_ << "    movsd " << fpArgRegs[placements[i].regIndex]
                  << ", [rsp]\n";
            *out_ << "    add rsp, 8\n";
        } else {
            *out_ << "    pop " << gpArgRegs[placements[i].regIndex] << "\n";
        }
    }

    *out_ << "    call " << mangleFunctionName(expr.callee) << "\n";
}

void AsmGenerator::emitBinaryExpr(const BinaryExpr& expr) {
    if (expr.left->inferredType == Type::String ||
        expr.right->inferredType == Type::String) {
        failVoid(expr.loc, "string binary operators in asm backend");
    return;
    }

    if (expr.left->inferredType == Type::Float ||
        expr.right->inferredType == Type::Float) {
        if (expr.left->inferredType != Type::Float ||
            expr.right->inferredType != Type::Float) {
            failVoid(expr.loc, "mixed int/float binary expression in asm backend");
    return;
        }

        emitExpr(*expr.left);

        *out_ << "    sub rsp, 8\n";
        *out_ << "    movsd [rsp], xmm0\n";

        emitExpr(*expr.right);

        *out_ << "    movsd xmm1, xmm0\n";
        *out_ << "    movsd xmm0, [rsp]\n";
        *out_ << "    add rsp, 8\n";

        switch (expr.op) {
            case BinaryOp::BitAnd:
            case BinaryOp::BitOr:
            case BinaryOp::BitXor:
            case BinaryOp::ShiftLeft:
            case BinaryOp::ShiftRight:
                failVoid(expr.loc, "x86-64 asm backend does not support bitwise operations yet");
                return;


            case BinaryOp::Add:
                *out_ << "    addsd xmm0, xmm1\n";
                return;

            case BinaryOp::Sub:
                *out_ << "    subsd xmm0, xmm1\n";
                return;

            case BinaryOp::Mul:
                *out_ << "    mulsd xmm0, xmm1\n";
                return;

            case BinaryOp::Div:
                *out_ << "    divsd xmm0, xmm1\n";
                return;

            case BinaryOp::Less:
                *out_ << "    ucomisd xmm0, xmm1\n";
                *out_ << "    setb al\n";
                *out_ << "    movzx rax, al\n";
                return;

            case BinaryOp::Greater:
                *out_ << "    ucomisd xmm0, xmm1\n";
                *out_ << "    seta al\n";
                *out_ << "    movzx rax, al\n";
                return;

            case BinaryOp::LessEqual:
                *out_ << "    ucomisd xmm0, xmm1\n";
                *out_ << "    setbe al\n";
                *out_ << "    movzx rax, al\n";
                return;

            case BinaryOp::GreaterEqual:
                *out_ << "    ucomisd xmm0, xmm1\n";
                *out_ << "    setae al\n";
                *out_ << "    movzx rax, al\n";
                return;

            case BinaryOp::Equal:
                *out_ << "    ucomisd xmm0, xmm1\n";
                *out_ << "    sete al\n";
                *out_ << "    movzx rax, al\n";
                return;

            case BinaryOp::NotEqual:
                *out_ << "    ucomisd xmm0, xmm1\n";
                *out_ << "    setne al\n";
                *out_ << "    movzx rax, al\n";
                return;

            case BinaryOp::Mod:
            case BinaryOp::And:
            case BinaryOp::Or:
                failVoid(expr.loc, "this float64 binary operator in asm backend");
    return;
        }
    }

    if (expr.left->inferredType == Type::UInt ||
        expr.right->inferredType == Type::UInt) {
        if (expr.left->inferredType != Type::UInt ||
            expr.right->inferredType != Type::UInt) {
            failVoid(expr.loc, "mixed int/uint binary expression in asm backend");
    return;
        }

        emitExpr(*expr.left);
        *out_ << "    push rax\n";

        emitExpr(*expr.right);
        *out_ << "    pop rbx\n";

        switch (expr.op) {
            case BinaryOp::BitAnd:
                *out_ << "    and rbx, rax\n";
                *out_ << "    mov rax, rbx\n";
                return;

            case BinaryOp::BitOr:
                *out_ << "    or rbx, rax\n";
                *out_ << "    mov rax, rbx\n";
                return;

            case BinaryOp::BitXor:
                *out_ << "    xor rbx, rax\n";
                *out_ << "    mov rax, rbx\n";
                return;

            case BinaryOp::ShiftLeft:
                *out_ << "    cmp rax, 0\n";
                *out_ << "    jl ml_shift_error\n";
                *out_ << "    cmp rax, 32\n";
                *out_ << "    jge ml_shift_error\n";
                *out_ << "    mov rcx, rax\n";
                *out_ << "    mov rax, rbx\n";
                *out_ << "    shl rax, cl\n";
                return;

            case BinaryOp::ShiftRight:
                *out_ << "    cmp rax, 0\n";
                *out_ << "    jl ml_shift_error\n";
                *out_ << "    cmp rax, 32\n";
                *out_ << "    jge ml_shift_error\n";
                *out_ << "    mov rcx, rax\n";
                *out_ << "    mov rax, rbx\n";
                *out_ << "    shr rax, cl\n";
                return;


            case BinaryOp::Add:
                *out_ << "    add rax, rbx\n";
                return;

            case BinaryOp::Sub:
                *out_ << "    sub rbx, rax\n";
                *out_ << "    mov rax, rbx\n";
                return;

            case BinaryOp::Mul:
                *out_ << "    imul rax, rbx\n";
                return;

            case BinaryOp::Div:
                *out_ << "    mov rcx, rax\n";
                *out_ << "    mov rax, rbx\n";
                *out_ << "    xor rdx, rdx\n";
                *out_ << "    div rcx\n";
                return;

            case BinaryOp::Mod:
                *out_ << "    mov rcx, rax\n";
                *out_ << "    mov rax, rbx\n";
                *out_ << "    xor rdx, rdx\n";
                *out_ << "    div rcx\n";
                *out_ << "    mov rax, rdx\n";
                return;

            case BinaryOp::Less:
                *out_ << "    cmp rbx, rax\n";
                *out_ << "    setb al\n";
                *out_ << "    movzx rax, al\n";
                return;

            case BinaryOp::Greater:
                *out_ << "    cmp rbx, rax\n";
                *out_ << "    seta al\n";
                *out_ << "    movzx rax, al\n";
                return;

            case BinaryOp::LessEqual:
                *out_ << "    cmp rbx, rax\n";
                *out_ << "    setbe al\n";
                *out_ << "    movzx rax, al\n";
                return;

            case BinaryOp::GreaterEqual:
                *out_ << "    cmp rbx, rax\n";
                *out_ << "    setae al\n";
                *out_ << "    movzx rax, al\n";
                return;

            case BinaryOp::Equal:
                *out_ << "    cmp rbx, rax\n";
                *out_ << "    sete al\n";
                *out_ << "    movzx rax, al\n";
                return;

            case BinaryOp::NotEqual:
                *out_ << "    cmp rbx, rax\n";
                *out_ << "    setne al\n";
                *out_ << "    movzx rax, al\n";
                return;

            case BinaryOp::And:
            case BinaryOp::Or:
                failVoid(expr.loc, "logical operator for uint in asm backend");
    return;
        }
    }

    if ((expr.left->inferredType != Type::Int && expr.left->inferredType != Type::Bool) ||
        (expr.right->inferredType != Type::Int && expr.right->inferredType != Type::Bool)) {
        failVoid(expr.loc, "non-int32/bool binary expression in asm backend");
    return;
    }

    emitExpr(*expr.left);
    *out_ << "    push rax\n";

    emitExpr(*expr.right);
    *out_ << "    pop rbx\n";

    switch (expr.op) {
            case BinaryOp::BitAnd:
                *out_ << "    and rbx, rax\n";
                *out_ << "    mov rax, rbx\n";
                return;

            case BinaryOp::BitOr:
                *out_ << "    or rbx, rax\n";
                *out_ << "    mov rax, rbx\n";
                return;

            case BinaryOp::BitXor:
                *out_ << "    xor rbx, rax\n";
                *out_ << "    mov rax, rbx\n";
                return;

            case BinaryOp::ShiftLeft:
                *out_ << "    cmp rax, 0\n";
                *out_ << "    jl ml_shift_error\n";
                *out_ << "    cmp rax, 32\n";
                *out_ << "    jge ml_shift_error\n";
                *out_ << "    mov rcx, rax\n";
                *out_ << "    mov rax, rbx\n";
                *out_ << "    shl rax, cl\n";
                return;

            case BinaryOp::ShiftRight:
                *out_ << "    cmp rax, 0\n";
                *out_ << "    jl ml_shift_error\n";
                *out_ << "    cmp rax, 32\n";
                *out_ << "    jge ml_shift_error\n";
                *out_ << "    mov rcx, rax\n";
                *out_ << "    mov rax, rbx\n";
                *out_ << "    sar rax, cl\n";
                return;


        case BinaryOp::Add:
            *out_ << "    add rax, rbx\n";
            return;

        case BinaryOp::Sub:
            *out_ << "    sub rbx, rax\n";
            *out_ << "    mov rax, rbx\n";
            return;

        case BinaryOp::Mul:
            *out_ << "    imul rax, rbx\n";
            return;

        case BinaryOp::Div:
            *out_ << "    mov rcx, rax\n";
            *out_ << "    mov rax, rbx\n";
            *out_ << "    cqo\n";
            *out_ << "    idiv rcx\n";
            return;

        case BinaryOp::Mod:
            *out_ << "    mov rcx, rax\n";
            *out_ << "    mov rax, rbx\n";
            *out_ << "    cqo\n";
            *out_ << "    idiv rcx\n";
            *out_ << "    mov rax, rdx\n";
            return;

        case BinaryOp::Less:
            *out_ << "    cmp rbx, rax\n";
            *out_ << "    setl al\n";
            *out_ << "    movzx rax, al\n";
            return;

        case BinaryOp::Greater:
            *out_ << "    cmp rbx, rax\n";
            *out_ << "    setg al\n";
            *out_ << "    movzx rax, al\n";
            return;

        case BinaryOp::LessEqual:
            *out_ << "    cmp rbx, rax\n";
            *out_ << "    setle al\n";
            *out_ << "    movzx rax, al\n";
            return;

        case BinaryOp::GreaterEqual:
            *out_ << "    cmp rbx, rax\n";
            *out_ << "    setge al\n";
            *out_ << "    movzx rax, al\n";
            return;

        case BinaryOp::Equal:
            *out_ << "    cmp rbx, rax\n";
            *out_ << "    sete al\n";
            *out_ << "    movzx rax, al\n";
            return;

        case BinaryOp::NotEqual:
            *out_ << "    cmp rbx, rax\n";
            *out_ << "    setne al\n";
            *out_ << "    movzx rax, al\n";
            return;

        case BinaryOp::And:
            *out_ << "    cmp rbx, 0\n";
            *out_ << "    setne bl\n";
            *out_ << "    cmp rax, 0\n";
            *out_ << "    setne al\n";
            *out_ << "    and al, bl\n";
            *out_ << "    movzx rax, al\n";
            return;

        case BinaryOp::Or:
            *out_ << "    cmp rbx, 0\n";
            *out_ << "    setne bl\n";
            *out_ << "    cmp rax, 0\n";
            *out_ << "    setne al\n";
            *out_ << "    or al, bl\n";
            *out_ << "    movzx rax, al\n";
            return;
    }

    failVoid(expr.loc, "unknown binary operator");
    return;
}

void AsmGenerator::emitUnaryExpr(const UnaryExpr& expr) {
    emitExpr(*expr.operand);

    switch (expr.op) {
        case UnaryOp::BitNot:
            if (expr.operand->inferredType != Type::Int &&
                expr.operand->inferredType != Type::UInt) {
                failVoid(expr.loc, "bitwise not for non-int/uint in asm backend");
                return;
            }

            *out_ << "    not rax\n";
            return;


        case UnaryOp::Negate:
            if (expr.operand->inferredType == Type::Float) {
                *out_ << "    xorpd xmm1, xmm1\n";
                *out_ << "    subsd xmm1, xmm0\n";
                *out_ << "    movapd xmm0, xmm1\n";
            } else {
                *out_ << "    neg rax\n";
            }
            return;

        case UnaryOp::Not:
            *out_ << "    cmp rax, 0\n";
            *out_ << "    sete al\n";
            *out_ << "    movzx rax, al\n";
            return;
    }

    failVoid(expr.loc, "unknown unary operator");
    return;
}

void AsmGenerator::emitCastExpr(const CastExpr& expr) {
    Type sourceType = expr.expression->inferredType;

    if (expr.targetType != Type::Int &&
        expr.targetType != Type::UInt &&
        expr.targetType != Type::Bool &&
        expr.targetType != Type::Float) {
        failVoid(expr.loc, "cast target type in asm backend");
    return;
    }

    if (sourceType != Type::Int &&
        sourceType != Type::UInt &&
        sourceType != Type::Bool &&
        sourceType != Type::Float) {
        failVoid(expr.loc, "cast source type in asm backend");
    return;
    }

    emitExpr(*expr.expression);

    if (sourceType == expr.targetType) {
        return;
    }

    if (expr.targetType == Type::Float) {
        if (sourceType == Type::Int ||
            sourceType == Type::UInt ||
            sourceType == Type::Bool) {
            *out_ << "    cvtsi2sd xmm0, rax\n";
            return;
        }
    }

    if (expr.targetType == Type::Int) {
        if (sourceType == Type::Float) {
            *out_ << "    cvttsd2si rax, xmm0\n";
            return;
        }

        if (sourceType == Type::UInt || sourceType == Type::Bool) {
            return;
        }
    }

    if (expr.targetType == Type::UInt) {
        if (sourceType == Type::Float) {
            *out_ << "    cvttsd2si rax, xmm0\n";
            return;
        }

        if (sourceType == Type::Int || sourceType == Type::Bool) {
            return;
        }
    }

    if (expr.targetType == Type::Bool) {
        if (sourceType == Type::Float) {
            *out_ << "    xorpd xmm1, xmm1\n";
            *out_ << "    ucomisd xmm0, xmm1\n";
            *out_ << "    setne al\n";
            *out_ << "    movzx rax, al\n";
            return;
        }

        if (sourceType == Type::Int ||
            sourceType == Type::UInt) {
            *out_ << "    cmp rax, 0\n";
            *out_ << "    setne al\n";
            *out_ << "    movzx rax, al\n";
            return;
        }
    }

    failVoid(expr.loc, "this cast in asm backend");
    return;
}

} 
