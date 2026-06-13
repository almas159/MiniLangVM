#pragma once
#include "ast.hpp"
#include <iosfwd>

namespace minilang {

class AstPrinter {
public:
    void print(const Program& program, std::ostream& out);

private:
    int indent_ = 0;

    void writeIndent(std::ostream& out) const;
    void writeLine(std::ostream& out, const std::string& text) const;

    void printFunction(const FunctionDecl& function, std::ostream& out);
    void printBlock(const BlockStmt& block, std::ostream& out);

    void printStmt(const Stmt& stmt, std::ostream& out);
    void printVarDeclStmt(const VarDeclStmt& stmt, std::ostream& out);
    void printAssignStmt(const AssignStmt& stmt, std::ostream& out);
    void printArrayAssignStmt(const ArrayAssignStmt& stmt, std::ostream& out);
    void printIfStmt(const IfStmt& stmt, std::ostream& out);
    void printWhileStmt(const WhileStmt& stmt, std::ostream& out);
    void printForStmt(const ForStmt& stmt, std::ostream& out);
    void printSwitchStmt(const SwitchStmt& stmt, std::ostream& out);
    void printReturnStmt(const ReturnStmt& stmt, std::ostream& out);
    void printPrintStmt(const PrintStmt& stmt, std::ostream& out);
    void printExprStmt(const ExprStmt& stmt, std::ostream& out);

    void printExpr(const Expr& expr, std::ostream& out);
    void printArrayLiteralExpr(const ArrayLiteralExpr& expr, std::ostream& out);
    void printIndexExpr(const IndexExpr& expr, std::ostream& out);
    void printBinaryExpr(const BinaryExpr& expr, std::ostream& out);
    void printUnaryExpr(const UnaryExpr& expr, std::ostream& out);
    void printCallExpr(const CallExpr& expr, std::ostream& out);
};

} 
