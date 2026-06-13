#pragma once
#include "ast.hpp"
#include "error.hpp"

#include <string>
#include <optional>
#include <type_traits>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace minilang {

class SemanticAnalyzer {
public:
    Result<void> analyzeExpected(Program& program);
    void analyze(Program& program);

    int localCount() const {
        return nextLocalIndex_;
    }

private:
    struct Symbol {
        Type type = Type::Unknown;
        int localIndex = -1;
        int arraySize = -1;
        std::string structName;
        SourceLocation location;
        bool isMutable = true;
    };

    struct StructFieldSymbol {
        Type type = Type::Unknown;
        std::string typeName;
        std::string name;
        SourceLocation location;
        int arraySize = -1;
    };

    struct StructSymbol {
        std::string name;
        std::vector<StructFieldSymbol> fields;
        SourceLocation location;
    };

    struct FunctionSymbol {
        Type returnType = Type::Unknown;
        std::string returnTypeName;
        std::vector<Type> parameterTypes;
        std::vector<std::string> parameterTypeNames;
        std::vector<std::string> typeParameters;
        int functionIndex = -1;
        FunctionDecl* declaration = nullptr;
        SourceLocation location;
    };

    std::vector<std::unordered_map<std::string, Symbol>> scopes_;
    std::unordered_map<std::string, FunctionSymbol> functions_;
    std::unordered_map<std::string, StructSymbol> structs_;

    int nextLocalIndex_ = 0;

    Type currentReturnType_ = Type::Unknown;
    std::string currentReturnTypeName_;
    std::string currentFunctionName_;
    bool currentFunctionHasReturn_ = false;
    std::unordered_set<std::string> currentTypeParameters_;

    int loopDepth_ = 0;
    int switchDepth_ = 0;
    std::optional<Diagnostic> diagnostic_;

    void beginScope();
    void endScope();

    void declareVariable(const std::string& name,
                         Type type,
                         int localIndex,
                         SourceLocation location,
                         int arraySize = -1,
                         const std::string& structName = "",
                         bool isMutable = true);

    void setDiagnostic(SourceLocation location, const std::string& message);
    void failVoid(SourceLocation location, const std::string& message);

    template <typename T>
    T fail(SourceLocation location, const std::string& message) {
        setDiagnostic(location, message);

        if constexpr (std::is_same_v<T, Type>) {
            return Type::Unknown;
        } else if constexpr (std::is_pointer_v<T>) {
            return nullptr;
        } else if constexpr (std::is_same_v<T, bool>) {
            return false;
        } else if constexpr (std::is_same_v<T, int>) {
            return 0;
        } else {
            return T{};
        }
    }

    Symbol* findVariable(const std::string& name);

    void collectStructDeclarations(Program& program);
    void collectFunctionDeclarations(Program& program);
    void analyzeFunction(FunctionDecl& function);

    void analyzeBlock(BlockStmt& block);

    void analyzeStmt(Stmt& stmt);
    void analyzeExprStmt(ExprStmt& stmt);
    void analyzeVarDeclStmt(VarDeclStmt& stmt);
    void analyzeAssignStmt(AssignStmt& stmt);
    void analyzeArrayAssignStmt(ArrayAssignStmt& stmt);
    void analyzeFieldAssignStmt(FieldAssignStmt& stmt);
    void analyzeIfStmt(IfStmt& stmt);
    void analyzeWhileStmt(WhileStmt& stmt);
    void analyzeForStmt(ForStmt& stmt);
    void analyzeSwitchStmt(SwitchStmt& stmt);
    void analyzeReturnStmt(ReturnStmt& stmt);
    void analyzeBreakStmt(BreakStmt& stmt);
    void analyzeContinueStmt(ContinueStmt& stmt);
    void analyzePrintStmt(PrintStmt& stmt);

    Type analyzeExpr(Expr& expr);
    Type analyzeArrayLiteralExpr(ArrayLiteralExpr& expr);
    Type analyzeIndexExpr(IndexExpr& expr);
    Type analyzeStructLiteralExpr(StructLiteralExpr& expr);
    Type analyzeFieldAccessExpr(FieldAccessExpr& expr);
    Type analyzeIntLiteralExpr(IntLiteralExpr& expr);
    Type analyzeFloatLiteralExpr(FloatLiteralExpr& expr);
    Type analyzeBoolLiteralExpr(BoolLiteralExpr& expr);
    Type analyzeStringLiteralExpr(StringLiteralExpr& expr);
    Type analyzeVariableExpr(VariableExpr& expr);
    Type analyzeCallExpr(CallExpr& expr);
    Type analyzeCastExpr(CastExpr& expr);
    Type analyzeBinaryExpr(BinaryExpr& expr);
    Type analyzeUnaryExpr(UnaryExpr& expr);

    bool sameType(Type left, Type right) const;
    bool canAssign(Type target, Type source) const;
};

} 
