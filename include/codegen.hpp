#pragma once
#include <optional>
#include <type_traits>
#include <string>

#include "ast.hpp"
#include "bytecode.hpp"
#include "error.hpp"
#include <vector>

namespace minilang {

class BytecodeGenerator {
public:
    Result<Bytecode> generateExpected(Program& program);
    Bytecode generate(Program& program);

private:
    std::optional<Diagnostic> diagnostic_;

    void setDiagnostic(SourceLocation location, const std::string& message);
    void failVoid(SourceLocation location, const std::string& message);

    template <typename T>
    T fail(SourceLocation location, const std::string& message) {
        setDiagnostic(location, message);

        if constexpr (std::is_same_v<T, Bytecode>) {
            return Bytecode{};
        } else if constexpr (std::is_same_v<T, int>) {
            return 0;
        } else if constexpr (std::is_same_v<T, std::string>) {
            return "";
        } else {
            return T{};
        }
    }

    Bytecode bytecode_;

    struct LoopContext {
        int continueTarget = -1;
        std::vector<int> breakJumps;
        std::vector<int> continueJumps;
    };

    std::vector<LoopContext> loopStack_;
    std::vector<std::vector<int>> switchBreakStack_;

    int currentAddress() const;

    int emit(const Instruction& instruction);
    int emitJump(OpCode op, SourceLocation location);
    void patchJump(int instructionIndex, int targetAddress);

    void prepareFunctionTable(Program& program);

    void generateFunction(FunctionDecl& function);
    void generateBlock(BlockStmt& block);

    void generateStmt(Stmt& stmt);
    void generateExprStmt(ExprStmt& stmt);
    void generateVarDeclStmt(VarDeclStmt& stmt);
    void generateAssignStmt(AssignStmt& stmt);
    void generateArrayAssignStmt(ArrayAssignStmt& stmt);
    void generateFieldAssignStmt(FieldAssignStmt& stmt);
    void generateIfStmt(IfStmt& stmt);
    void generateWhileStmt(WhileStmt& stmt);
    void generateForStmt(ForStmt& stmt);
    void generateSwitchStmt(SwitchStmt& stmt);
    void generateReturnStmt(ReturnStmt& stmt);
    void generateBreakStmt(BreakStmt& stmt);
    void generateContinueStmt(ContinueStmt& stmt);
    void generatePrintStmt(PrintStmt& stmt);

    void generateExpr(Expr& expr);
    void generateArrayLiteralExpr(ArrayLiteralExpr& expr);
    void generateIndexExpr(IndexExpr& expr);
    void generateStructLiteralExpr(StructLiteralExpr& expr);
    void generateFieldAccessExpr(FieldAccessExpr& expr);
    void generateIntLiteralExpr(IntLiteralExpr& expr);
    void generateFloatLiteralExpr(FloatLiteralExpr& expr);
    void generateBoolLiteralExpr(BoolLiteralExpr& expr);
    void generateStringLiteralExpr(StringLiteralExpr& expr);
    void generateVariableExpr(VariableExpr& expr);
    void generateCallExpr(CallExpr& expr);
    void generateCastExpr(CastExpr& expr);
    void generateBinaryExpr(BinaryExpr& expr);
    void generateUnaryExpr(UnaryExpr& expr);

    void emitDefaultValue(Type type, int arraySize, SourceLocation location);
    void emitImplicitCast(Type sourceType, Type targetType, SourceLocation location);
};

} 
