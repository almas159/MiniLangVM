#pragma once
#include <optional>
#include <type_traits>
#include <string>
#include "ast.hpp"
#include "error.hpp"

#include <ostream>
#include <string>
#include <unordered_map>
#include <vector>

namespace minilang {

class AsmGenerator {
public:
    Result<void> generateExpected(const Program& program, const std::string& outputPath);
    void generate(const Program& program, const std::string& outputPath);

private:
    mutable std::optional<Diagnostic> diagnostic_;

    void setDiagnostic(SourceLocation location, const std::string& message) const;
    void failVoid(SourceLocation location, const std::string& message) const;

    template <typename T>
    T fail(SourceLocation location, const std::string& message) const {
        setDiagnostic(location, message);

        if constexpr (std::is_same_v<T, int>) {
            return 0;
        } else if constexpr (std::is_same_v<T, std::string>) {
            return "";
        } else {
            return T{};
        }
    }

    std::ostream* out_ = nullptr;

    int labelCounter_ = 0;
    int stackSize_ = 0;

    std::unordered_map<int, int> arrayBaseOffsets_;
    std::unordered_map<int, int> arraySizes_;

    std::unordered_map<std::string, int> structFieldCounts_;
    std::unordered_map<int, int> structBaseOffsets_;
    std::unordered_map<int, std::string> structLocalTypes_;

    std::string currentEndLabel_;

    struct LoopContext {
        std::string breakLabel;
        std::string continueLabel;
    };

    std::vector<LoopContext> loopStack_;

    std::string makeLabel(const std::string& base);
    std::string mangleFunctionName(const std::string& name) const;
    std::string emitStringLiteralData(const std::string& value);
    std::string emitFloatLiteralData(double value);

    int localOffset(int localIndex) const;
    int arrayBaseOffset(int localIndex, SourceLocation location) const;
    int arraySize(int localIndex, SourceLocation location) const;

    int structBaseOffset(int localIndex, SourceLocation location) const;
    int structFieldCount(const std::string& structName, SourceLocation location) const;

    void collectStructDeclarations(const Program& program);

    void collectArrayLocalsInBlock(const BlockStmt& block, int& nextOffset);
    void collectArrayLocalsInStmt(const Stmt& stmt, int& nextOffset);

    void emitArrayBoundsCheck(const std::string& indexReg,
                              int size,
                              SourceLocation location);

    void emitFunction(const FunctionDecl& function);
    void emitBlock(const BlockStmt& block);

    void emitStmt(const Stmt& stmt);
    void emitVarDeclStmt(const VarDeclStmt& stmt);
    void emitAssignStmt(const AssignStmt& stmt);
    void emitArrayAssignStmt(const ArrayAssignStmt& stmt);
    void emitFieldAssignStmt(const FieldAssignStmt& stmt);
    void emitIfStmt(const IfStmt& stmt);
    void emitWhileStmt(const WhileStmt& stmt);
    void emitForStmt(const ForStmt& stmt);
    void emitReturnStmt(const ReturnStmt& stmt);
    void emitBreakStmt(const BreakStmt& stmt);
    void emitContinueStmt(const ContinueStmt& stmt);
    void emitPrintStmt(const PrintStmt& stmt);
    void emitExprStmt(const ExprStmt& stmt);

    void emitStructLiteralToMemory(const StructLiteralExpr& expr,
                                   int baseOffset,
                                   SourceLocation location);

    void emitExpr(const Expr& expr);
    void emitIntLiteralExpr(const IntLiteralExpr& expr);
    void emitFloatLiteralExpr(const FloatLiteralExpr& expr);
    void emitBoolLiteralExpr(const BoolLiteralExpr& expr);
    void emitStringLiteralExpr(const StringLiteralExpr& expr);
    void emitVariableExpr(const VariableExpr& expr);
    void emitIndexExpr(const IndexExpr& expr);
    void emitFieldAccessExpr(const FieldAccessExpr& expr);
    void emitCallExpr(const CallExpr& expr);
    void emitBinaryExpr(const BinaryExpr& expr);
    void emitUnaryExpr(const UnaryExpr& expr);
    void emitCastExpr(const CastExpr& expr);

    void unsupported(SourceLocation location, const std::string& feature) const;
};
} 