#pragma once
#include "ast.hpp"
#include "bytecode.hpp"

class BytecodeGenerator {
public:
    Bytecode generate(Program& program, int localCount);

private:
    Bytecode bytecode_;

    int currentAddress() const;

    int emit(const Instruction& instruction);
    int emitJump(OpCode op, SourceLocation location);
    void patchJump(int instructionIndex, int targetAddress);

    void generateMainFunction(MainFunction& function);
    void generateBlock(BlockStmt& block);

    void generateStmt(Stmt& stmt);
    void generateVarDeclStmt(VarDeclStmt& stmt);
    void generateAssignStmt(AssignStmt& stmt);
    void generateIfStmt(IfStmt& stmt);
    void generateWhileStmt(WhileStmt& stmt);
    void generateReturnStmt(ReturnStmt& stmt);
    void generatePrintStmt(PrintStmt& stmt);

    void generateExpr(Expr& expr);
    void generateIntLiteralExpr(IntLiteralExpr& expr);
    void generateBoolLiteralExpr(BoolLiteralExpr& expr);
    void generateStringLiteralExpr(StringLiteralExpr& expr);
    void generateVariableExpr(VariableExpr& expr);
    void generateBinaryExpr(BinaryExpr& expr);
    void generateUnaryExpr(UnaryExpr& expr);

    void emitDefaultValue(Type type, SourceLocation location);
};
