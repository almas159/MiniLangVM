#pragma once

#include "source_location.hpp"
#include <memory>
#include <string>
#include <utility>
#include <vector>

namespace minilang {

enum class Type {
    Int,
    UInt,
    IntArray,
    Float,
    Struct,
    Bool,
    String,
    Generic,
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
    BitAnd,
    BitOr,
    BitXor,
    ShiftLeft,
    ShiftRight,
    Equal,
    NotEqual,

    And,
    Or
};

enum class UnaryOp {
    Negate,
    BitNot,
    Not
};

std::string typeToString(Type type);
std::string binaryOpToString(BinaryOp op);
std::string unaryOpToString(UnaryOp op);

struct AstNode {
    SourceLocation loc;

    explicit AstNode(SourceLocation location = {})
        : loc(location) {}

    virtual ~AstNode() = default;
};

struct Expr : AstNode {
    Type inferredType = Type::Unknown;
    int inferredArraySize = -1;
    std::string inferredStructName;

    explicit Expr(SourceLocation location = {})
        : AstNode(location) {}

    virtual ~Expr() = default;
};

struct Stmt : AstNode {
    explicit Stmt(SourceLocation location = {})
        : AstNode(location) {}

    virtual ~Stmt() = default;
};

struct BlockStmt;

struct Parameter {
    Type type = Type::Unknown;
    std::string name;
    SourceLocation loc;
    int localIndex = -1;
    std::string typeName;

    Parameter(Type type, std::string name, SourceLocation location = {})
        : type(type),
          name(std::move(name)),
          loc(location) {}
};

struct StructFieldDecl {
    Type type = Type::Unknown;
    std::string typeName;
    std::string name;
    SourceLocation loc;
    int arraySize = -1;

    StructFieldDecl(Type type,
                    std::string typeName,
                    std::string name,
                    SourceLocation location = {},
                    int arraySize = -1)
        : type(type),
          typeName(std::move(typeName)),
          name(std::move(name)),
          loc(location),
          arraySize(arraySize) {}
};

struct StructDecl : AstNode {
    std::string name;
    std::vector<StructFieldDecl> fields;

    StructDecl(std::string name,
               std::vector<StructFieldDecl> fields,
               SourceLocation location = {})
        : AstNode(location),
          name(std::move(name)),
          fields(std::move(fields)) {}
};

struct TypeAliasDecl : AstNode {
    std::string name;
    Type targetType = Type::Unknown;
    int arraySize = -1;

    TypeAliasDecl(std::string name,
                  Type targetType,
                  int arraySize,
                  SourceLocation location = {})
        : AstNode(location),
          name(std::move(name)),
          targetType(targetType),
          arraySize(arraySize) {}
};

struct FunctionDecl : AstNode {
    Type returnType = Type::Unknown;
    std::string returnTypeName;
    std::string name;
    std::vector<std::string> typeParameters;
    std::vector<Parameter> parameters;
    std::unique_ptr<BlockStmt> body;

    int functionIndex = -1;
    int localCount = 0;

    FunctionDecl(Type returnType,
                 std::string name,
                 std::vector<Parameter> parameters,
                 std::unique_ptr<BlockStmt> body,
                 SourceLocation location = {})
        : AstNode(location),
          returnType(returnType),
          name(std::move(name)),
          parameters(std::move(parameters)),
          body(std::move(body)) {}
};

struct Program : AstNode {
    std::vector<std::unique_ptr<StructDecl>> structs;
    std::vector<std::unique_ptr<TypeAliasDecl>> aliases;
    std::vector<std::unique_ptr<FunctionDecl>> functions;

    explicit Program(SourceLocation location = {})
        : AstNode(location) {}
};

struct BlockStmt : Stmt {
    std::vector<std::unique_ptr<Stmt>> statements;

    explicit BlockStmt(SourceLocation location = {})
        : Stmt(location) {}
};

struct EmptyStmt : Stmt {
    explicit EmptyStmt(SourceLocation location = {})
        : Stmt(location) {}
};

struct VarDeclStmt : Stmt {
    Type type;
    std::string name;
    std::unique_ptr<Expr> initializer;
    int localIndex = -1;
    int arraySize = -1;
    std::string structName;
    bool isMutable = true;

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
    Type targetType = Type::Unknown;

    AssignStmt(std::string name,
               std::unique_ptr<Expr> value,
               SourceLocation location = {})
        : Stmt(location),
          name(std::move(name)),
          value(std::move(value)) {}
};

struct ArrayAssignStmt : Stmt {
    std::string name;
    std::unique_ptr<Expr> arrayExpr;
    std::unique_ptr<Expr> index;
    std::unique_ptr<Expr> value;
    int localIndex = -1;

    ArrayAssignStmt(std::string name,
                    std::unique_ptr<Expr> index,
                    std::unique_ptr<Expr> value,
                    SourceLocation location = {})
        : Stmt(location),
          name(std::move(name)),
          index(std::move(index)),
          value(std::move(value)) {}

    ArrayAssignStmt(std::unique_ptr<Expr> arrayExpr,
                    std::unique_ptr<Expr> index,
                    std::unique_ptr<Expr> value,
                    SourceLocation location = {})
        : Stmt(location),
          arrayExpr(std::move(arrayExpr)),
          index(std::move(index)),
          value(std::move(value)) {}
};

struct FieldAssignStmt : Stmt {
    std::string objectName;
    std::string fieldName;
    std::unique_ptr<Expr> value;
    int localIndex = -1;
    int fieldIndex = -1;

    FieldAssignStmt(std::string objectName,
                    std::string fieldName,
                    std::unique_ptr<Expr> value,
                    SourceLocation location = {})
        : Stmt(location),
          objectName(std::move(objectName)),
          fieldName(std::move(fieldName)),
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

struct ForStmt : Stmt {
    std::unique_ptr<Stmt> initializer;
    std::unique_ptr<Expr> condition;
    std::unique_ptr<Stmt> update;
    std::unique_ptr<Stmt> body;

    ForStmt(std::unique_ptr<Stmt> initializer,
            std::unique_ptr<Expr> condition,
            std::unique_ptr<Stmt> update,
            std::unique_ptr<Stmt> body,
            SourceLocation location = {})
        : Stmt(location),
          initializer(std::move(initializer)),
          condition(std::move(condition)),
          update(std::move(update)),
          body(std::move(body)) {}
};

struct SwitchCase {
    int value;
    SourceLocation loc;
    std::vector<std::unique_ptr<Stmt>> statements;

    SwitchCase(int value, SourceLocation location = {})
        : value(value), loc(location) {}

    SwitchCase(SwitchCase&&) noexcept = default;
    SwitchCase& operator=(SwitchCase&&) noexcept = default;

    SwitchCase(const SwitchCase&) = delete;
    SwitchCase& operator=(const SwitchCase&) = delete;
};

struct SwitchStmt : Stmt {
    std::unique_ptr<Expr> expression;
    std::vector<SwitchCase> cases;
    std::vector<std::unique_ptr<Stmt>> defaultStatements;
    bool hasDefault = false;

    SwitchStmt(std::unique_ptr<Expr> expression,
               SourceLocation location = {})
        : Stmt(location),
          expression(std::move(expression)) {}
};

struct ReturnStmt : Stmt {
    std::unique_ptr<Expr> value;
    Type targetType = Type::Unknown;

    ReturnStmt(std::unique_ptr<Expr> value,
               SourceLocation location = {})
        : Stmt(location),
          value(std::move(value)) {}
};

struct BreakStmt : Stmt {
    explicit BreakStmt(SourceLocation location = {})
        : Stmt(location) {}
};

struct ContinueStmt : Stmt {
    explicit ContinueStmt(SourceLocation location = {})
        : Stmt(location) {}
};

struct PrintStmt : Stmt {
    std::vector<std::unique_ptr<Expr>> arguments;

    explicit PrintStmt(SourceLocation location = {})
        : Stmt(location) {}
};

struct ExprStmt : Stmt {
    std::unique_ptr<Expr> expression;

    ExprStmt(std::unique_ptr<Expr> expression,
             SourceLocation location = {})
        : Stmt(location),
          expression(std::move(expression)) {}
};

struct ArrayLiteralExpr : Expr {
    std::vector<std::unique_ptr<Expr>> elements;

    explicit ArrayLiteralExpr(SourceLocation location = {})
        : Expr(location) {}
};

struct StructLiteralField {
    std::string name;
    std::unique_ptr<Expr> value;
    SourceLocation loc;
    int fieldIndex = -1;

    StructLiteralField(std::string name,
                       std::unique_ptr<Expr> value,
                       SourceLocation location = {})
        : name(std::move(name)),
          value(std::move(value)),
          loc(location) {}

    StructLiteralField(StructLiteralField&&) noexcept = default;
    StructLiteralField& operator=(StructLiteralField&&) noexcept = default;

    StructLiteralField(const StructLiteralField&) = delete;
    StructLiteralField& operator=(const StructLiteralField&) = delete;
};

struct StructLiteralExpr : Expr {
    std::string structName;
    std::vector<StructLiteralField> fields;

    StructLiteralExpr(std::string structName,
                      SourceLocation location = {})
        : Expr(location),
          structName(std::move(structName)) {}
};

struct FieldAccessExpr : Expr {
    std::string objectName;
    std::string fieldName;
    int localIndex = -1;
    int fieldIndex = -1;

    FieldAccessExpr(std::string objectName,
                    std::string fieldName,
                    SourceLocation location = {})
        : Expr(location),
          objectName(std::move(objectName)),
          fieldName(std::move(fieldName)) {}
};

struct IntLiteralExpr : Expr {
    int value;

    IntLiteralExpr(int value, SourceLocation location = {})
        : Expr(location),
          value(value) {}
};

struct FloatLiteralExpr : Expr {
    double value;

    FloatLiteralExpr(double value, SourceLocation location = {})
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

struct CallExpr : Expr {
    std::string callee;
    std::vector<Type> typeArguments;
    std::vector<std::string> typeArgumentNames;
    std::vector<std::unique_ptr<Expr>> arguments;
    std::vector<Type> argumentTargetTypes;
    std::vector<std::string> argumentTargetTypeNames;
    int functionIndex = -1;

    CallExpr(std::string callee,
             std::vector<std::unique_ptr<Expr>> arguments,
             SourceLocation location = {})
        : Expr(location),
          callee(std::move(callee)),
          arguments(std::move(arguments)) {}
};

struct IndexExpr : Expr {
    std::string arrayName;
    std::unique_ptr<Expr> arrayExpr;
    std::unique_ptr<Expr> index;
    int localIndex = -1;

    IndexExpr(std::string arrayName,
              std::unique_ptr<Expr> index,
              SourceLocation location = {})
        : Expr(location),
          arrayName(std::move(arrayName)),
          index(std::move(index)) {}

    IndexExpr(std::unique_ptr<Expr> arrayExpr,
              std::unique_ptr<Expr> index,
              SourceLocation location = {})
        : Expr(location),
          arrayExpr(std::move(arrayExpr)),
          index(std::move(index)) {}
};

struct CastExpr : Expr {
    Type targetType = Type::Unknown;
    int targetArraySize = -1;
    std::unique_ptr<Expr> expression;

    CastExpr(Type targetType,
             int targetArraySize,
             std::unique_ptr<Expr> expression,
             SourceLocation location = {})
        : Expr(location),
          targetType(targetType),
          targetArraySize(targetArraySize),
          expression(std::move(expression)) {}
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

} 
