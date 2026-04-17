#include "semantic.hpp"
#include "error.hpp"
#include <string>

void SemanticAnalyzer::analyze(Program& program) {
    scopes_.clear();
    nextLocalIndex_ = 0;
    hasReturn_ = false;

    if (!program.mainFunction) {
        throw SemanticError(program.loc, "program does not contain main function");
    }

    analyzeMainFunction(*program.mainFunction);

    if (!hasReturn_) {
        throw SemanticError(program.loc, "main must contain return statement");
    }
}

void SemanticAnalyzer::beginScope() {
    scopes_.push_back({});
}

void SemanticAnalyzer::endScope() {
    scopes_.pop_back();
}

void SemanticAnalyzer::declareVariable(const std::string& name,
                                       Type type,
                                       int localIndex,
                                       SourceLocation location) {
    if (scopes_.empty()) {
        beginScope();
    }

    auto& currentScope = scopes_.back();

    if (currentScope.find(name) != currentScope.end()) {
        throw SemanticError(
            location,
            "variable '" + name + "' is already declared in this scope"
        );
    }

    currentScope[name] = Symbol{type, localIndex, location};
}

SemanticAnalyzer::Symbol* SemanticAnalyzer::findVariable(const std::string& name) {
    for (auto it = scopes_.rbegin(); it != scopes_.rend(); ++it) {
        auto found = it->find(name);

        if (found != it->end()) {
            return &found->second;
        }
    }

    return nullptr;
}

void SemanticAnalyzer::analyzeMainFunction(MainFunction& function) {
    if (function.returnType != Type::Int) {
        throw SemanticError(function.loc, "main must return int");
    }

    analyzeBlock(*function.body);
}

void SemanticAnalyzer::analyzeBlock(BlockStmt& block) {
    beginScope();

    for (auto& statement : block.statements) {
        analyzeStmt(*statement);
    }

    endScope();
}

void SemanticAnalyzer::analyzeStmt(Stmt& stmt) {
    if (auto* s = dynamic_cast<BlockStmt*>(&stmt)) {
        analyzeBlock(*s);
        return;
    }

    if (auto* s = dynamic_cast<VarDeclStmt*>(&stmt)) {
        analyzeVarDeclStmt(*s);
        return;
    }

    if (auto* s = dynamic_cast<AssignStmt*>(&stmt)) {
        analyzeAssignStmt(*s);
        return;
    }

    if (auto* s = dynamic_cast<IfStmt*>(&stmt)) {
        analyzeIfStmt(*s);
        return;
    }

    if (auto* s = dynamic_cast<WhileStmt*>(&stmt)) {
        analyzeWhileStmt(*s);
        return;
    }

    if (auto* s = dynamic_cast<ReturnStmt*>(&stmt)) {
        analyzeReturnStmt(*s);
        return;
    }

    if (auto* s = dynamic_cast<PrintStmt*>(&stmt)) {
        analyzePrintStmt(*s);
        return;
    }

    throw SemanticError(stmt.loc, "unknown statement");
}

void SemanticAnalyzer::analyzeVarDeclStmt(VarDeclStmt& stmt) {
    if (stmt.type == Type::Void || stmt.type == Type::Unknown) {
        throw SemanticError(stmt.loc, "invalid variable type");
    }

    if (!scopes_.empty()) {
        auto& currentScope = scopes_.back();

        if (currentScope.find(stmt.name) != currentScope.end()) {
            throw SemanticError(
                stmt.loc,
                "variable '" + stmt.name + "' is already declared in this scope"
            );
        }
    }

    if (stmt.initializer) {
        Type initializerType = analyzeExpr(*stmt.initializer);

        if (!sameType(stmt.type, initializerType)) {
            throw SemanticError(
                stmt.initializer->loc,
                "cannot initialize " + typeToString(stmt.type) +
                " variable '" + stmt.name + "' with " +
                typeToString(initializerType) + " value"
            );
        }
    }

    int localIndex = nextLocalIndex_++;
    stmt.localIndex = localIndex;

    declareVariable(stmt.name, stmt.type, localIndex, stmt.loc);
}

void SemanticAnalyzer::analyzeAssignStmt(AssignStmt& stmt) {
    Symbol* symbol = findVariable(stmt.name);

    if (!symbol) {
        throw SemanticError(
            stmt.loc,
            "variable '" + stmt.name + "' is not declared"
        );
    }

    Type valueType = analyzeExpr(*stmt.value);

    if (!sameType(symbol->type, valueType)) {
        throw SemanticError(
            stmt.value->loc,
            "cannot assign " + typeToString(valueType) +
            " to " + typeToString(symbol->type) +
            " variable '" + stmt.name + "'"
        );
    }

    stmt.localIndex = symbol->localIndex;
}

void SemanticAnalyzer::analyzeIfStmt(IfStmt& stmt) {
    Type conditionType = analyzeExpr(*stmt.condition);

    if (conditionType != Type::Bool) {
        throw SemanticError(
            stmt.condition->loc,
            "condition of if must be bool, got " + typeToString(conditionType)
        );
    }

    analyzeStmt(*stmt.thenBranch);

    if (stmt.elseBranch) {
        analyzeStmt(*stmt.elseBranch);
    }
}

void SemanticAnalyzer::analyzeWhileStmt(WhileStmt& stmt) {
    Type conditionType = analyzeExpr(*stmt.condition);

    if (conditionType != Type::Bool) {
        throw SemanticError(
            stmt.condition->loc,
            "condition of while must be bool, got " + typeToString(conditionType)
        );
    }

    analyzeStmt(*stmt.body);
}

void SemanticAnalyzer::analyzeReturnStmt(ReturnStmt& stmt) {
    Type valueType = analyzeExpr(*stmt.value);

    if (valueType != Type::Int) {
        throw SemanticError(
            stmt.value->loc,
            "main must return int, got " + typeToString(valueType)
        );
    }

    hasReturn_ = true;
}

void SemanticAnalyzer::analyzePrintStmt(PrintStmt& stmt) {
    for (auto& argument : stmt.arguments) {
        Type argType = analyzeExpr(*argument);

        if (argType != Type::Int &&
            argType != Type::Bool &&
            argType != Type::String) {
            throw SemanticError(
                argument->loc,
                "print does not support value of type " + typeToString(argType)
            );
        }
    }
}

Type SemanticAnalyzer::analyzeExpr(Expr& expr) {
    if (auto* e = dynamic_cast<IntLiteralExpr*>(&expr)) {
        return analyzeIntLiteralExpr(*e);
    }

    if (auto* e = dynamic_cast<BoolLiteralExpr*>(&expr)) {
        return analyzeBoolLiteralExpr(*e);
    }

    if (auto* e = dynamic_cast<StringLiteralExpr*>(&expr)) {
        return analyzeStringLiteralExpr(*e);
    }

    if (auto* e = dynamic_cast<VariableExpr*>(&expr)) {
        return analyzeVariableExpr(*e);
    }

    if (auto* e = dynamic_cast<BinaryExpr*>(&expr)) {
        return analyzeBinaryExpr(*e);
    }

    if (auto* e = dynamic_cast<UnaryExpr*>(&expr)) {
        return analyzeUnaryExpr(*e);
    }

    throw SemanticError(expr.loc, "unknown expression");
}

Type SemanticAnalyzer::analyzeIntLiteralExpr(IntLiteralExpr& expr) {
    expr.inferredType = Type::Int;
    return expr.inferredType;
}

Type SemanticAnalyzer::analyzeBoolLiteralExpr(BoolLiteralExpr& expr) {
    expr.inferredType = Type::Bool;
    return expr.inferredType;
}

Type SemanticAnalyzer::analyzeStringLiteralExpr(StringLiteralExpr& expr) {
    expr.inferredType = Type::String;
    return expr.inferredType;
}

Type SemanticAnalyzer::analyzeVariableExpr(VariableExpr& expr) {
    Symbol* symbol = findVariable(expr.name);

    if (!symbol) {
        throw SemanticError(
            expr.loc,
            "variable '" + expr.name + "' is not declared"
        );
    }

    expr.localIndex = symbol->localIndex;
    expr.inferredType = symbol->type;

    return expr.inferredType;
}

Type SemanticAnalyzer::analyzeBinaryExpr(BinaryExpr& expr) {
    Type leftType = analyzeExpr(*expr.left);
    Type rightType = analyzeExpr(*expr.right);

    switch (expr.op) {
        case BinaryOp::Add:
            if (leftType == Type::Int && rightType == Type::Int) {
                expr.inferredType = Type::Int;
                return expr.inferredType;
            }

            if (leftType == Type::String && rightType == Type::String) {
                expr.inferredType = Type::String;
                return expr.inferredType;
            }

            throw SemanticError(
                expr.loc,
                "operator '+' cannot be applied to " +
                typeToString(leftType) + " and " + typeToString(rightType)
            );

        case BinaryOp::Sub:
        case BinaryOp::Mul:
        case BinaryOp::Div:
        case BinaryOp::Mod:
            if (leftType == Type::Int && rightType == Type::Int) {
                expr.inferredType = Type::Int;
                return expr.inferredType;
            }

            throw SemanticError(
                expr.loc,
                "arithmetic operator '" + binaryOpToString(expr.op) +
                "' cannot be applied to " +
                typeToString(leftType) + " and " + typeToString(rightType)
            );

        case BinaryOp::Less:
        case BinaryOp::Greater:
        case BinaryOp::LessEqual:
        case BinaryOp::GreaterEqual:
            if (leftType == Type::Int && rightType == Type::Int) {
                expr.inferredType = Type::Bool;
                return expr.inferredType;
            }

            throw SemanticError(
                expr.loc,
                "comparison operator '" + binaryOpToString(expr.op) +
                "' cannot be applied to " +
                typeToString(leftType) + " and " + typeToString(rightType)
            );

        case BinaryOp::Equal:
        case BinaryOp::NotEqual:
            if (sameType(leftType, rightType) &&
                (leftType == Type::Int ||
                 leftType == Type::Bool ||
                 leftType == Type::String)) {
                expr.inferredType = Type::Bool;
                return expr.inferredType;
            }

            throw SemanticError(
                expr.loc,
                "equality operator '" + binaryOpToString(expr.op) +
                "' cannot be applied to " +
                typeToString(leftType) + " and " + typeToString(rightType)
            );

        case BinaryOp::And:
        case BinaryOp::Or:
            if (leftType == Type::Bool && rightType == Type::Bool) {
                expr.inferredType = Type::Bool;
                return expr.inferredType;
            }

            throw SemanticError(
                expr.loc,
                "logical operator '" + binaryOpToString(expr.op) +
                "' cannot be applied to " +
                typeToString(leftType) + " and " + typeToString(rightType)
            );
    }

    throw SemanticError(expr.loc, "unknown binary operator");
}

Type SemanticAnalyzer::analyzeUnaryExpr(UnaryExpr& expr) {
    Type operandType = analyzeExpr(*expr.operand);

    switch (expr.op) {
        case UnaryOp::Negate:
            if (operandType == Type::Int) {
                expr.inferredType = Type::Int;
                return expr.inferredType;
            }

            throw SemanticError(
                expr.loc,
                "unary '-' cannot be applied to " + typeToString(operandType)
            );

        case UnaryOp::Not:
            if (operandType == Type::Bool) {
                expr.inferredType = Type::Bool;
                return expr.inferredType;
            }

            throw SemanticError(
                expr.loc,
                "unary '!' cannot be applied to " + typeToString(operandType)
            );
    }

    throw SemanticError(expr.loc, "unknown unary operator");
}

bool SemanticAnalyzer::sameType(Type left, Type right) const {
    return left == right;
}
