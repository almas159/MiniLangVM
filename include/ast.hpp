#pragma once
#include "source_location.hpp"
#include <memory>
#include <string>
#include <utility>
#include <vector>

enum class Type {
    Int,
    Bool,
    String,
    Void,
    Unknown
};

enum class BinaryOp {
    Add,
    Sub,
    Mul,
    Div,
    Mod,

    Less,
    Greater,
    LessEqual,
    GreaterEqual,
    Equal,
    NotEqual,

    And,
    Or
};

enum class UnaryOp {
    Negate,
    Not
};

std::string typeToString(Type type);
std::string binaryOpToString(BinaryOp op);
std::string unaryOpToString(UnaryOp op);

//базовые AST-узлы

struct AstNode {
    SourceLocation loc;

    explicit AstNode(SourceLocation location = {})
        : loc(location) {}

    virtual ~AstNode() = default;
};

struct Expr : AstNode {
    Type inferredType = Type::Unknown;

    explicit Expr(SourceLocation location = {})
        : AstNode(location) {}

    virtual ~Expr() = default;
};

struct Stmt : AstNode {
    explicit Stmt(SourceLocation location = {})
        : AstNode(location) {}

    virtual ~Stmt() = default;
};

// forward declarations
struct MainFunction;
struct BlockStmt;

// program
struct Program : AstNode {
    std::unique_ptr<MainFunction> mainFunction;

    Program(std::unique_ptr<MainFunction> mainFunction,
            SourceLocation location = {})
        : AstNode(location),
          mainFunction(std::move(mainFunction)) {}
};

struct MainFunction : AstNode {
    Type returnType = Type::Int;
    std::unique_ptr<BlockStmt> body;

    MainFunction(std::unique_ptr<BlockStmt> body,
                 SourceLocation location = {})
        : AstNode(location),
          body(std::move(body)) {}
};

// statements

struct BlockStmt : Stmt {
    std::vector<std::unique_ptr<Stmt>> statements;

    explicit BlockStmt(SourceLocation location = {})
        : Stmt(location) {}
};

struct VarDeclStmt : Stmt {
    Type type;
    std::string name;
    std::unique_ptr<Expr> initializer;
    int localIndex = -1;

    VarDeclStmt(Type type,
                std::string name,
                std::unique_ptr<Expr> initializer,
                SourceLocation location = {})
        : Stmt(location),
          type(type),
          name(std::move(name)),
          initializer(std::move(initializer)) {}
};

struct AssignStmt : Stmt {
    std::string name;
    std::unique_ptr<Expr> value;
    int localIndex = -1;

    AssignStmt(std::string name,
               std::unique_ptr<Expr> value,
               SourceLocation location = {})
        : Stmt(location),
          name(std::move(name)),
          value(std::move(value)) {}
};

struct IfStmt : Stmt {
    std::unique_ptr<Expr> condition;
    std::unique_ptr<Stmt> thenBranch;
    std::unique_ptr<Stmt> elseBranch;

    IfStmt(std::unique_ptr<Expr> condition,
           std::unique_ptr<Stmt> thenBranch,
           std::unique_ptr<Stmt> elseBranch,
           SourceLocation location = {})
        : Stmt(location),
          condition(std::move(condition)),
          thenBranch(std::move(thenBranch)),
          elseBranch(std::move(elseBranch)) {}
};

struct WhileStmt : Stmt {
    std::unique_ptr<Expr> condition;
    std::unique_ptr<Stmt> body;

    WhileStmt(std::unique_ptr<Expr> condition,
              std::unique_ptr<Stmt> body,
              SourceLocation location = {})
        : Stmt(location),
          condition(std::move(condition)),
          body(std::move(body)) {}
};

struct ReturnStmt : Stmt {
    std::unique_ptr<Expr> value;

    ReturnStmt(std::unique_ptr<Expr> value,
               SourceLocation location = {})
        : Stmt(location),
          value(std::move(value)) {}
};

struct PrintStmt : Stmt {
    std::vector<std::unique_ptr<Expr>> arguments;

    explicit PrintStmt(SourceLocation location = {})
        : Stmt(location) {}
};

// expressions

struct IntLiteralExpr : Expr {
    int value;

    IntLiteralExpr(int value, SourceLocation location = {})
        : Expr(location),
          value(value) {}
};

struct BoolLiteralExpr : Expr {
    bool value;

    BoolLiteralExpr(bool value, SourceLocation location = {})
        : Expr(location),
          value(value) {}
};

struct StringLiteralExpr : Expr {
    std::string value;

    StringLiteralExpr(std::string value, SourceLocation location = {})
        : Expr(location),
          value(std::move(value)) {}
};

struct VariableExpr : Expr {
    std::string name;
    int localIndex = -1;

    VariableExpr(std::string name, SourceLocation location = {})
        : Expr(location),
          name(std::move(name)) {}
};

struct BinaryExpr : Expr {
    BinaryOp op;
    std::unique_ptr<Expr> left;
    std::unique_ptr<Expr> right;

    BinaryExpr(BinaryOp op,
               std::unique_ptr<Expr> left,
               std::unique_ptr<Expr> right,
               SourceLocation location = {})
        : Expr(location),
          op(op),
          left(std::move(left)),
          right(std::move(right)) {}
};

struct UnaryExpr : Expr {
    UnaryOp op;
    std::unique_ptr<Expr> operand;

    UnaryExpr(UnaryOp op,
              std::unique_ptr<Expr> operand,
              SourceLocation location = {})
        : Expr(location),
          op(op),
          operand(std::move(operand)) {}
};
