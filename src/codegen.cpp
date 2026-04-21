#include "codegen.hpp"
#include "error.hpp"
#include <string>

Bytecode BytecodeGenerator::generate(Program& program, int localCount) {
    bytecode_ = Bytecode{};
    bytecode_.localCount = localCount;

    if (!program.mainFunction) {
        throw SemanticError(program.loc, "cannot generate bytecode: missing main function");
    }

    generateMainFunction(*program.mainFunction);

    return bytecode_;
}

int BytecodeGenerator::currentAddress() const {
    return static_cast<int>(bytecode_.code.size());
}

int BytecodeGenerator::emit(const Instruction& instruction) {
    bytecode_.code.push_back(instruction);
    return static_cast<int>(bytecode_.code.size()) - 1;
}

int BytecodeGenerator::emitJump(OpCode op, SourceLocation location) {
    return emit(Instruction(op, -1, location));
}

void BytecodeGenerator::patchJump(int instructionIndex, int targetAddress) {
    if (instructionIndex < 0 ||
        instructionIndex >= static_cast<int>(bytecode_.code.size())) {
        throw RuntimeError(SourceLocation(), "invalid jump patch index");
    }

    bytecode_.code[instructionIndex].intArg = targetAddress;
}

void BytecodeGenerator::generateMainFunction(MainFunction& function) {
    generateBlock(*function.body);
}

void BytecodeGenerator::generateBlock(BlockStmt& block) {
    for (auto& statement : block.statements) {
        generateStmt(*statement);
    }
}

void BytecodeGenerator::generateStmt(Stmt& stmt) {
    if (auto* s = dynamic_cast<BlockStmt*>(&stmt)) {
        generateBlock(*s);
        return;
    }

    if (auto* s = dynamic_cast<VarDeclStmt*>(&stmt)) {
        generateVarDeclStmt(*s);
        return;
    }

    if (auto* s = dynamic_cast<AssignStmt*>(&stmt)) {
        generateAssignStmt(*s);
        return;
    }

    if (auto* s = dynamic_cast<IfStmt*>(&stmt)) {
        generateIfStmt(*s);
        return;
    }

    if (auto* s = dynamic_cast<WhileStmt*>(&stmt)) {
        generateWhileStmt(*s);
        return;
    }

    if (auto* s = dynamic_cast<ReturnStmt*>(&stmt)) {
        generateReturnStmt(*s);
        return;
    }

    if (auto* s = dynamic_cast<PrintStmt*>(&stmt)) {
        generatePrintStmt(*s);
        return;
    }

    throw SemanticError(stmt.loc, "cannot generate bytecode for unknown statement");
}

void BytecodeGenerator::generateVarDeclStmt(VarDeclStmt& stmt) {
    if (stmt.localIndex < 0) {
        throw SemanticError(stmt.loc, "internal error: variable has no local index");
    }

    if (stmt.initializer) {
        generateExpr(*stmt.initializer);
    } else {
        emitDefaultValue(stmt.type, stmt.loc);
    }

    emit(Instruction(OpCode::Store, stmt.localIndex, stmt.loc));
}

void BytecodeGenerator::generateAssignStmt(AssignStmt& stmt) {
    if (stmt.localIndex < 0) {
        throw SemanticError(stmt.loc, "internal error: assignment target has no local index");
    }

    generateExpr(*stmt.value);

    emit(Instruction(OpCode::Store, stmt.localIndex, stmt.loc));
}

void BytecodeGenerator::generateIfStmt(IfStmt& stmt) {
    generateExpr(*stmt.condition);

    int jumpToElseOrEnd = emitJump(OpCode::JumpIfFalse, stmt.loc);

    generateStmt(*stmt.thenBranch);

    if (stmt.elseBranch) {
        int jumpToEnd = emitJump(OpCode::Jump, stmt.loc);

        int elseAddress = currentAddress();
        patchJump(jumpToElseOrEnd, elseAddress);

        generateStmt(*stmt.elseBranch);

        int endAddress = currentAddress();
        patchJump(jumpToEnd, endAddress);
    } else {
        int endAddress = currentAddress();
        patchJump(jumpToElseOrEnd, endAddress);
    }
}

void BytecodeGenerator::generateWhileStmt(WhileStmt& stmt) {
    int startAddress = currentAddress();

    generateExpr(*stmt.condition);

    int jumpToEnd = emitJump(OpCode::JumpIfFalse, stmt.loc);

    generateStmt(*stmt.body);

    emit(Instruction(OpCode::Jump, startAddress, stmt.loc));

    int endAddress = currentAddress();
    patchJump(jumpToEnd, endAddress);
}

void BytecodeGenerator::generateReturnStmt(ReturnStmt& stmt) {
    generateExpr(*stmt.value);
    emit(Instruction(OpCode::Ret, stmt.loc));
}

void BytecodeGenerator::generatePrintStmt(PrintStmt& stmt) {
    for (auto& argument : stmt.arguments) {
        generateExpr(*argument);
    }

    emit(Instruction(
        OpCode::Print,
        static_cast<int>(stmt.arguments.size()),
        stmt.loc
    ));
}

void BytecodeGenerator::generateExpr(Expr& expr) {
    if (auto* e = dynamic_cast<IntLiteralExpr*>(&expr)) {
        generateIntLiteralExpr(*e);
        return;
    }

    if (auto* e = dynamic_cast<BoolLiteralExpr*>(&expr)) {
        generateBoolLiteralExpr(*e);
        return;
    }

    if (auto* e = dynamic_cast<StringLiteralExpr*>(&expr)) {
        generateStringLiteralExpr(*e);
        return;
    }

    if (auto* e = dynamic_cast<VariableExpr*>(&expr)) {
        generateVariableExpr(*e);
        return;
    }

    if (auto* e = dynamic_cast<BinaryExpr*>(&expr)) {
        generateBinaryExpr(*e);
        return;
    }

    if (auto* e = dynamic_cast<UnaryExpr*>(&expr)) {
        generateUnaryExpr(*e);
        return;
    }

    throw SemanticError(expr.loc, "cannot generate bytecode for unknown expression");
}

void BytecodeGenerator::generateIntLiteralExpr(IntLiteralExpr& expr) {
    emit(Instruction(OpCode::PushInt, expr.value, expr.loc));
}

void BytecodeGenerator::generateBoolLiteralExpr(BoolLiteralExpr& expr) {
    emit(Instruction(OpCode::PushBool, expr.value ? 1 : 0, expr.loc));
}

void BytecodeGenerator::generateStringLiteralExpr(StringLiteralExpr& expr) {
    emit(Instruction(OpCode::PushString, expr.value, expr.loc));
}

void BytecodeGenerator::generateVariableExpr(VariableExpr& expr) {
    if (expr.localIndex < 0) {
        throw SemanticError(expr.loc, "internal error: variable has no local index");
    }

    emit(Instruction(OpCode::Load, expr.localIndex, expr.loc));
}

void BytecodeGenerator::generateBinaryExpr(BinaryExpr& expr) {
    // ленивое вычисление &&
    if (expr.op == BinaryOp::And) {
        generateExpr(*expr.left);

        int jumpToFalse = emitJump(OpCode::JumpIfFalse, expr.loc);

        generateExpr(*expr.right);

        int jumpToEnd = emitJump(OpCode::Jump, expr.loc);

        int falseAddress = currentAddress();
        patchJump(jumpToFalse, falseAddress);

        emit(Instruction(OpCode::PushBool, 0, expr.loc));

        int endAddress = currentAddress();
        patchJump(jumpToEnd, endAddress);

        return;
    }

    // ленивое вычисление ||
    if (expr.op == BinaryOp::Or) {
        generateExpr(*expr.left);

        int jumpToTrue = emitJump(OpCode::JumpIfTrue, expr.loc);

        generateExpr(*expr.right);

        int jumpToEnd = emitJump(OpCode::Jump, expr.loc);

        int trueAddress = currentAddress();
        patchJump(jumpToTrue, trueAddress);

        emit(Instruction(OpCode::PushBool, 1, expr.loc));

        int endAddress = currentAddress();
        patchJump(jumpToEnd, endAddress);

        return;
    }

    generateExpr(*expr.left);
    generateExpr(*expr.right);

    switch (expr.op) {
        case BinaryOp::Add:
            emit(Instruction(OpCode::Add, expr.loc));
            return;

        case BinaryOp::Sub:
            emit(Instruction(OpCode::Sub, expr.loc));
            return;

        case BinaryOp::Mul:
            emit(Instruction(OpCode::Mul, expr.loc));
            return;

        case BinaryOp::Div:
            emit(Instruction(OpCode::Div, expr.loc));
            return;

        case BinaryOp::Mod:
            emit(Instruction(OpCode::Mod, expr.loc));
            return;

        case BinaryOp::Less:
            emit(Instruction(OpCode::Less, expr.loc));
            return;

        case BinaryOp::Greater:
            emit(Instruction(OpCode::Greater, expr.loc));
            return;

        case BinaryOp::LessEqual:
            emit(Instruction(OpCode::LessEqual, expr.loc));
            return;

        case BinaryOp::GreaterEqual:
            emit(Instruction(OpCode::GreaterEqual, expr.loc));
            return;

        case BinaryOp::Equal:
            emit(Instruction(OpCode::Equal, expr.loc));
            return;

        case BinaryOp::NotEqual:
            emit(Instruction(OpCode::NotEqual, expr.loc));
            return;

        case BinaryOp::And:
        case BinaryOp::Or:
            break;
    }

    throw SemanticError(expr.loc, "unknown binary operator in bytecode generator");
}

void BytecodeGenerator::generateUnaryExpr(UnaryExpr& expr) {
    generateExpr(*expr.operand);

    switch (expr.op) {
        case UnaryOp::Negate:
            emit(Instruction(OpCode::Neg, expr.loc));
            return;

        case UnaryOp::Not:
            emit(Instruction(OpCode::Not, expr.loc));
            return;
    }

    throw SemanticError(expr.loc, "unknown unary operator in bytecode generator");
}

void BytecodeGenerator::emitDefaultValue(Type type, SourceLocation location) {
    switch (type) {
        case Type::Int:
            emit(Instruction(OpCode::PushInt, 0, location));
            return;

        case Type::Bool:
            emit(Instruction(OpCode::PushBool, 0, location));
            return;

        case Type::String:
            emit(Instruction(OpCode::PushString, "", location));
            return;

        case Type::Void:
        case Type::Unknown:
            throw SemanticError(location, "cannot emit default value for type " + typeToString(type));
    }
}
