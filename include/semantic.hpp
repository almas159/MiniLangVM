#pragma once
#include "ast.hpp"
#include <string>
#include <unordered_map>
#include <vector>

class SemanticAnalyzer {
public:
    void analyze(Program& program);

    int localCount() const {
        return nextLocalIndex_;
    }

private:
    struct Symbol {
        Type type = Type::Unknown;
        int localIndex = -1;
        SourceLocation location;
    };

    std::vector<std::unordered_map<std::string, Symbol>> scopes_;

    int nextLocalIndex_ = 0;
    bool hasReturn_ = false;

    void beginScope();
    void endScope();

    void declareVariable(const std::string& name,
                         Type type,
                         int localIndex,
                         SourceLocation location);

    Symbol* findVariable(const std::string& name);

    void analyzeMainFunction(MainFunction& function);
    void analyzeBlock(BlockStmt& block);

    void analyzeStmt(Stmt& stmt);
    void analyzeVarDeclStmt(VarDeclStmt& stmt);
    void analyzeAssignStmt(AssignStmt& stmt);
    void analyzeIfStmt(IfStmt& stmt);
    void analyzeWhileStmt(WhileStmt& stmt);
    void analyzeReturnStmt(ReturnStmt& stmt);
    void analyzePrintStmt(PrintStmt& stmt);

    Type analyzeExpr(Expr& expr);
    Type analyzeIntLiteralExpr(IntLiteralExpr& expr);
    Type analyzeBoolLiteralExpr(BoolLiteralExpr& expr);
    Type analyzeStringLiteralExpr(StringLiteralExpr& expr);
    Type analyzeVariableExpr(VariableExpr& expr);
    Type analyzeBinaryExpr(BinaryExpr& expr);
    Type analyzeUnaryExpr(UnaryExpr& expr);

    bool sameType(Type left, Type right) const;
};
