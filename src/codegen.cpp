#include "codegen.hpp"
#include "error.hpp"

#include <algorithm>
#include <string>

namespace minilang {

static Type arrayElementTypeFromEncodedNameCodegen(const std::string& encoded) {
    if (encoded.empty() || encoded == "__array:int:") {
        return Type::Int;
    }

    if (encoded == "__array:uint:") {
        return Type::UInt;
    }

    if (encoded == "__array:float64:") {
        return Type::Float;
    }

    if (encoded == "__array:bool:") {
        return Type::Bool;
    }

    if (encoded == "__array:string:") {
        return Type::String;
    }

    if (encoded.rfind("__array:struct:", 0) == 0) {
        return Type::Struct;
    }

    return Type::Unknown;
}

static int arrayElementSizeOfCodegen(Type type) {
    switch (type) {
        case Type::Bool:
            return 1;

        case Type::Int:
        case Type::UInt:
            return 4;

        case Type::Float:
            return 8;

        case Type::String:
        case Type::Struct:
        case Type::Generic:
        case Type::IntArray:
            return 8;

        case Type::Void:
        case Type::Unknown:
            return 0;
    }

    return 0;
}

static std::string arrayTypeNameCodegen(const std::string& encoded) {
    if (encoded.empty() || encoded == "__array:int:") {
        return "int[]";
    }

    if (encoded == "__array:uint:") {
        return "uint[]";
    }

    if (encoded == "__array:float64:") {
        return "float64[]";
    }

    if (encoded == "__array:bool:") {
        return "bool[]";
    }

    if (encoded == "__array:string:") {
        return "string[]";
    }

    const std::string prefix = "__array:struct:";

    if (encoded.rfind(prefix, 0) == 0) {
        return encoded.substr(prefix.size()) + "[]";
    }

    return "array";
}


void BytecodeGenerator::setDiagnostic(SourceLocation location, const std::string& message) {
    if (diagnostic_) {
        return;
    }

    diagnostic_ = Diagnostic(location, message);
}

void BytecodeGenerator::failVoid(SourceLocation location, const std::string& message) {
    setDiagnostic(location, message);
}

static std::string metaTypeName(const Expr& expr) {
    if (expr.inferredType == Type::IntArray) {
        return arrayTypeNameCodegen(expr.inferredStructName);
    }

    if ((expr.inferredType == Type::Struct || expr.inferredType == Type::Generic) &&
        !expr.inferredStructName.empty()) {
        return expr.inferredStructName;
    }

    return typeToString(expr.inferredType);
}

static int metaSizeOf(const Expr& expr) {
    switch (expr.inferredType) {
        case Type::Int:
            return 4;

        case Type::UInt:
            return 4;

        case Type::Float:
            return 8;

        case Type::Bool:
            return 1;

        case Type::String:
            return 8;

        case Type::IntArray:
            if (expr.inferredArraySize > 0) {
                Type elementType = arrayElementTypeFromEncodedNameCodegen(expr.inferredStructName);
                int elementSize = arrayElementSizeOfCodegen(elementType);

                if (elementSize > 0) {
                    return expr.inferredArraySize * elementSize;
                }
            }

            return 8;

        case Type::Struct:
            return 8;

        case Type::Generic:
            return 8;

        case Type::Void:
        case Type::Unknown:
            return 0;
    }

    return 0;
}

Result<Bytecode> BytecodeGenerator::generateExpected(Program& program) {
    diagnostic_.reset();

    Bytecode result = generate(program);

    if (diagnostic_) {
        return std::unexpected(*diagnostic_);
    }

    return result;
}

Bytecode BytecodeGenerator::generate(Program& program) {
    bytecode_ = Bytecode{};
    loopStack_.clear();
    switchBreakStack_.clear();

    prepareFunctionTable(program);

    for (auto& function : program.functions) {
        generateFunction(*function);
    }

    if (bytecode_.entryFunction < 0) {
        return fail<Bytecode>(program.loc, "cannot generate bytecode: missing main function");
    }

    return bytecode_;
}

void BytecodeGenerator::prepareFunctionTable(Program& program) {
    bytecode_.functions.clear();

    for (auto& function : program.functions) {
        FunctionInfo info;
        info.name = function->name;
        info.returnType = function->returnType;
        info.localCount = function->localCount;
        info.address = -1;

        for (const Parameter& parameter : function->parameters) {
            info.parameterTypes.push_back(parameter.type);
        }

        bytecode_.functions.push_back(info);

        if (function->name == "main") {
            bytecode_.entryFunction = function->functionIndex;
        }
    }
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
        failVoid(SourceLocation(), "invalid jump patch index");
    return;
    }

    bytecode_.code[instructionIndex].intArg = targetAddress;
}

void BytecodeGenerator::generateFunction(FunctionDecl& function) {
    if (function.functionIndex < 0 ||
        function.functionIndex >= static_cast<int>(bytecode_.functions.size())) {
        failVoid(function.loc, "internal error: invalid function index");
    return;
    }

    bytecode_.functions[function.functionIndex].address = currentAddress();

    generateBlock(*function.body);


    emitDefaultValue(function.returnType, -1, function.loc);
    emit(Instruction(OpCode::Ret, function.loc));
}

void BytecodeGenerator::generateBlock(BlockStmt& block) {
    for (auto& statement : block.statements) {
        generateStmt(*statement);
    }
}

void BytecodeGenerator::generateStmt(Stmt& stmt) {
    if (dynamic_cast<EmptyStmt*>(&stmt)) {
        return;
    }

    if (auto* s = dynamic_cast<ExprStmt*>(&stmt)) {
        generateExprStmt(*s);
        return;
    }

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

    if (auto* s = dynamic_cast<ArrayAssignStmt*>(&stmt)) {
        generateArrayAssignStmt(*s);
        return;
    }

    if (auto* s = dynamic_cast<FieldAssignStmt*>(&stmt)) {
        generateFieldAssignStmt(*s);
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

    if (auto* s = dynamic_cast<ForStmt*>(&stmt)) {
        generateForStmt(*s);
        return;
    }

    if (auto* s = dynamic_cast<SwitchStmt*>(&stmt)) {
        generateSwitchStmt(*s);
        return;
    }

    if (auto* s = dynamic_cast<ReturnStmt*>(&stmt)) {
        generateReturnStmt(*s);
        return;
    }

    if (auto* s = dynamic_cast<BreakStmt*>(&stmt)) {
        generateBreakStmt(*s);
        return;
    }

    if (auto* s = dynamic_cast<ContinueStmt*>(&stmt)) {
        generateContinueStmt(*s);
        return;
    }

    if (auto* s = dynamic_cast<PrintStmt*>(&stmt)) {
        generatePrintStmt(*s);
        return;
    }

    failVoid(stmt.loc, "cannot generate bytecode for unknown statement");
    return;
}

void BytecodeGenerator::generateExprStmt(ExprStmt& stmt) {
    generateExpr(*stmt.expression);

    if (stmt.expression->inferredType != Type::Void) {
        emit(Instruction(OpCode::Pop, stmt.loc));
    }
}

void BytecodeGenerator::generateVarDeclStmt(VarDeclStmt& stmt) {
    if (stmt.localIndex < 0) {
        failVoid(stmt.loc, "internal error: variable has no local index");
    return;
    }

    if (stmt.initializer) {
        generateExpr(*stmt.initializer);
    } else {
        emitDefaultValue(stmt.type, stmt.arraySize, stmt.loc);
    }

    emit(Instruction(OpCode::Store, stmt.localIndex, stmt.loc));
}

void BytecodeGenerator::generateAssignStmt(AssignStmt& stmt) {
    if (stmt.localIndex < 0) {
        failVoid(stmt.loc, "internal error: assignment target has no local index");
    return;
    }

    generateExpr(*stmt.value);
    emit(Instruction(OpCode::Store, stmt.localIndex, stmt.loc));
}

void BytecodeGenerator::generateArrayAssignStmt(ArrayAssignStmt& stmt) {
    if (stmt.arrayExpr) {
        generateExpr(*stmt.arrayExpr);
        generateExpr(*stmt.index);
        generateExpr(*stmt.value);

        emit(Instruction(OpCode::StoreIndex, -1, stmt.loc));
        return;
    }

    if (stmt.localIndex < 0) {
        failVoid(stmt.loc, "internal error: array assignment target has no local index");
        return;
    }

    generateExpr(*stmt.index);
    generateExpr(*stmt.value);

    emit(Instruction(OpCode::StoreIndex, stmt.localIndex, stmt.loc));
}

void BytecodeGenerator::generateFieldAssignStmt(FieldAssignStmt& stmt) {
    if (stmt.localIndex < 0 || stmt.fieldIndex < 0) {
        failVoid(stmt.loc,
            "internal error: field assignment has invalid indexes");
    return;
    }

    generateExpr(*stmt.value);

    emit(Instruction(
        OpCode::StoreField,
        stmt.localIndex,
        stmt.fieldIndex,
        stmt.loc
    ));
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

    LoopContext context;
    context.continueTarget = startAddress;
    loopStack_.push_back(context);

    generateStmt(*stmt.body);

    emit(Instruction(OpCode::Jump, startAddress, stmt.loc));

    int endAddress = currentAddress();

    patchJump(jumpToEnd, endAddress);

    for (int jumpIndex : loopStack_.back().breakJumps) {
        patchJump(jumpIndex, endAddress);
    }

    for (int jumpIndex : loopStack_.back().continueJumps) {
        patchJump(jumpIndex, loopStack_.back().continueTarget);
    }

    loopStack_.pop_back();
}

void BytecodeGenerator::generateForStmt(ForStmt& stmt) {
    if (stmt.initializer) {
        generateStmt(*stmt.initializer);
    }

    int conditionAddress = currentAddress();

    generateExpr(*stmt.condition);

    int jumpToEnd = emitJump(OpCode::JumpIfFalse, stmt.loc);

    LoopContext context;
    context.continueTarget = -1;
    loopStack_.push_back(context);

    generateStmt(*stmt.body);

    int updateAddress = currentAddress();

    for (int jumpIndex : loopStack_.back().continueJumps) {
        patchJump(jumpIndex, updateAddress);
    }

    if (stmt.update) {
        generateStmt(*stmt.update);
    }

    emit(Instruction(OpCode::Jump, conditionAddress, stmt.loc));

    int endAddress = currentAddress();

    patchJump(jumpToEnd, endAddress);

    for (int jumpIndex : loopStack_.back().breakJumps) {
        patchJump(jumpIndex, endAddress);
    }

    loopStack_.pop_back();
}

void BytecodeGenerator::generateSwitchStmt(SwitchStmt& stmt) {
    std::vector<int> jumpToCase;

    for (auto& switchCase : stmt.cases) {
        generateExpr(*stmt.expression);
        emit(Instruction(OpCode::PushInt, switchCase.value, switchCase.loc));
        emit(Instruction(OpCode::Equal, switchCase.loc));

        int jumpIndex = emitJump(OpCode::JumpIfTrue, switchCase.loc);
        jumpToCase.push_back(jumpIndex);
    }

    int jumpToDefaultOrEnd = emitJump(OpCode::Jump, stmt.loc);

    switchBreakStack_.push_back({});

    for (std::size_t i = 0; i < stmt.cases.size(); ++i) {
        int caseAddress = currentAddress();
        patchJump(jumpToCase[i], caseAddress);

        for (auto& statement : stmt.cases[i].statements) {
            generateStmt(*statement);
        }
    }

    if (stmt.hasDefault) {
        int defaultAddress = currentAddress();
        patchJump(jumpToDefaultOrEnd, defaultAddress);

        for (auto& statement : stmt.defaultStatements) {
            generateStmt(*statement);
        }
    } else {
        int endAddress = currentAddress();
        patchJump(jumpToDefaultOrEnd, endAddress);
    }

    int endAddress = currentAddress();

    for (int jumpIndex : switchBreakStack_.back()) {
        patchJump(jumpIndex, endAddress);
    }

    switchBreakStack_.pop_back();
}

void BytecodeGenerator::generateBreakStmt(BreakStmt& stmt) {
    if (!switchBreakStack_.empty()) {
        int jumpIndex = emitJump(OpCode::Jump, stmt.loc);
        switchBreakStack_.back().push_back(jumpIndex);
        return;
    }

    if (loopStack_.empty()) {
        failVoid(stmt.loc, "internal error: 'break' outside loop");
    return;
    }

    int jumpIndex = emitJump(OpCode::Jump, stmt.loc);
    loopStack_.back().breakJumps.push_back(jumpIndex);
}

void BytecodeGenerator::generateContinueStmt(ContinueStmt& stmt) {
    if (loopStack_.empty()) {
        failVoid(stmt.loc, "internal error: 'continue' outside loop");
    return;
    }

    int jumpIndex = emitJump(OpCode::Jump, stmt.loc);
    loopStack_.back().continueJumps.push_back(jumpIndex);
}

void BytecodeGenerator::generateReturnStmt(ReturnStmt& stmt) {
    if (stmt.value) {
        generateExpr(*stmt.value);
    } else {
        emit(Instruction(OpCode::PushVoid, stmt.loc));
    }

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
    if (auto* e = dynamic_cast<ArrayLiteralExpr*>(&expr)) {
        generateArrayLiteralExpr(*e);
        return;
    }

    if (auto* e = dynamic_cast<IndexExpr*>(&expr)) {
        generateIndexExpr(*e);
        return;
    }

    if (auto* e = dynamic_cast<StructLiteralExpr*>(&expr)) {
        generateStructLiteralExpr(*e);
        return;
    }

    if (auto* e = dynamic_cast<FieldAccessExpr*>(&expr)) {
        generateFieldAccessExpr(*e);
        return;
    }

    if (auto* e = dynamic_cast<IntLiteralExpr*>(&expr)) {
        generateIntLiteralExpr(*e);
        return;
    }

    if (auto* e = dynamic_cast<FloatLiteralExpr*>(&expr)) {
        generateFloatLiteralExpr(*e);
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

    if (auto* e = dynamic_cast<CallExpr*>(&expr)) {
        generateCallExpr(*e);
        return;
    }

    if (auto* e = dynamic_cast<CastExpr*>(&expr)) {
        generateCastExpr(*e);
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

    failVoid(expr.loc, "cannot generate bytecode for unknown expression");
    return;
}

void BytecodeGenerator::generateArrayLiteralExpr(ArrayLiteralExpr& expr) {
    for (auto& element : expr.elements) {
        generateExpr(*element);
    }

    emit(Instruction(
        OpCode::MakeArray,
        static_cast<int>(expr.elements.size()),
        expr.loc
    ));
}

void BytecodeGenerator::generateIndexExpr(IndexExpr& expr) {
    if (expr.arrayExpr) {
        generateExpr(*expr.arrayExpr);
        generateExpr(*expr.index);
        emit(Instruction(OpCode::LoadIndex, -1, expr.loc));
        return;
    }

    if (expr.localIndex < 0) {
        failVoid(expr.loc, "internal error: array access has no local index");
        return;
    }

    generateExpr(*expr.index);

    emit(Instruction(OpCode::LoadIndex, expr.localIndex, expr.loc));
}

void BytecodeGenerator::generateStructLiteralExpr(StructLiteralExpr& expr) {
    std::vector<StructLiteralField*> orderedFields;

    orderedFields.reserve(expr.fields.size());

    for (auto& field : expr.fields) {
        orderedFields.push_back(&field);
    }

    std::sort(
        orderedFields.begin(),
        orderedFields.end(),
        [](const StructLiteralField* left, const StructLiteralField* right) {
            return left->fieldIndex < right->fieldIndex;
        }
    );

    for (StructLiteralField* field : orderedFields) {
        generateExpr(*field->value);
    }

    Instruction instruction(
        OpCode::MakeStruct,
        static_cast<int>(orderedFields.size()),
        expr.loc
    );

    instruction.stringArg = expr.structName;

    emit(instruction);
}

void BytecodeGenerator::generateFieldAccessExpr(FieldAccessExpr& expr) {
    if (expr.localIndex < 0 || expr.fieldIndex < 0) {
        failVoid(expr.loc,
            "internal error: field access has invalid indexes");
    return;
    }

    emit(Instruction(
        OpCode::LoadField,
        expr.localIndex,
        expr.fieldIndex,
        expr.loc
    ));
}

void BytecodeGenerator::generateIntLiteralExpr(IntLiteralExpr& expr) {
    if (expr.inferredType == Type::UInt) {
        emit(Instruction(OpCode::PushUInt, expr.value, expr.loc));
        return;
    }

    emit(Instruction(OpCode::PushInt, expr.value, expr.loc));
}

void BytecodeGenerator::generateFloatLiteralExpr(FloatLiteralExpr& expr) {
    emit(Instruction(OpCode::PushFloat, expr.value, expr.loc));
}

void BytecodeGenerator::generateBoolLiteralExpr(BoolLiteralExpr& expr) {
    emit(Instruction(OpCode::PushBool, expr.value ? 1 : 0, expr.loc));
}

void BytecodeGenerator::generateStringLiteralExpr(StringLiteralExpr& expr) {
    emit(Instruction(OpCode::PushString, expr.value, expr.loc));
}

void BytecodeGenerator::generateVariableExpr(VariableExpr& expr) {
    if (expr.localIndex < 0) {
        failVoid(expr.loc, "internal error: variable has no local index");
    return;
    }

    emit(Instruction(OpCode::Load, expr.localIndex, expr.loc));
}

void BytecodeGenerator::generateCallExpr(CallExpr& expr) {
    if (expr.callee == "toInt") {
        generateExpr(*expr.arguments[0]);
        emit(Instruction(OpCode::ToInt, expr.loc));
        return;
    }


    if (expr.callee == "sizeof") {
        emit(Instruction(OpCode::PushInt, metaSizeOf(*expr.arguments[0]), expr.loc));
        return;
    }

    if (expr.callee == "typeid" || expr.callee == "typeof") {
        emit(Instruction(OpCode::PushString, metaTypeName(*expr.arguments[0]), expr.loc));
        return;
    }


    if (expr.callee == "input") {
        emit(Instruction(OpCode::Input, expr.loc));
        return;
    }

    if (expr.callee == "assert") {
        generateExpr(*expr.arguments[0]);

        std::string message = "assertion failed";

        if (expr.arguments.size() == 2) {
            auto* literal = dynamic_cast<StringLiteralExpr*>(expr.arguments[1].get());

            if (!literal) {
                failVoid(expr.arguments[1]->loc,
                    "assert message must be a string literal in bytecode generator");
    return;
            }

            message = literal->value;
        }

        emit(Instruction(OpCode::Assert, message, expr.loc));
        return;
    }

    if (expr.callee == "len") {
        generateExpr(*expr.arguments[0]);
        emit(Instruction(OpCode::Len, expr.loc));
        return;
    }

    if (expr.callee == "exit") {
        generateExpr(*expr.arguments[0]);
        emit(Instruction(OpCode::Exit, expr.loc));
        return;
    }

    if (expr.callee == "panic") {
        generateExpr(*expr.arguments[0]);
        emit(Instruction(OpCode::Panic, expr.loc));
        return;
    }

    if (expr.functionIndex < 0) {
        failVoid(expr.loc, "internal error: function call has no function index");
    return;
    }

    for (std::size_t i = 0; i < expr.arguments.size(); ++i) {
        generateExpr(*expr.arguments[i]);

        if (i < expr.argumentTargetTypes.size()) {
            Type sourceType = expr.arguments[i]->inferredType;
            Type targetType = expr.argumentTargetTypes[i];

            if (sourceType != targetType) {
                switch (targetType) {
                    case Type::Int:
                        emit(Instruction(OpCode::CastInt, expr.arguments[i]->loc));
                        break;

                    case Type::UInt:
                        emit(Instruction(OpCode::CastUInt, expr.arguments[i]->loc));
                        break;

                    case Type::Float:
                        emit(Instruction(OpCode::CastFloat, expr.arguments[i]->loc));
                        break;

                    case Type::Bool:
                        emit(Instruction(OpCode::CastBool, expr.arguments[i]->loc));
                        break;

                    case Type::Struct:
                    case Type::IntArray:
                    case Type::String:
                    case Type::Void:
                    case Type::Generic:
                    case Type::Unknown:
                        failVoid(expr.arguments[i]->loc,
                            "unsupported implicit argument conversion in bytecode generator");
                        return;
                }
            }
        }
    }

    emit(Instruction(OpCode::Call, expr.functionIndex, expr.loc));
}

void BytecodeGenerator::generateCastExpr(CastExpr& expr) {
    generateExpr(*expr.expression);

    Type sourceType = expr.expression->inferredType;

    if (sourceType == expr.targetType) {
        return;
    }

    switch (expr.targetType) {
        case Type::Int:
            emit(Instruction(OpCode::CastInt, expr.loc));
            return;

        case Type::UInt:
            emit(Instruction(OpCode::CastUInt, expr.loc));
            return;

        case Type::Float:
            emit(Instruction(OpCode::CastFloat, expr.loc));
            return;

        case Type::Bool:
            emit(Instruction(OpCode::CastBool, expr.loc));
            return;

        case Type::Generic:
        case Type::IntArray:
        case Type::Struct:
        case Type::String:
        case Type::Void:
        case Type::Unknown:
            failVoid(expr.loc,
                "cannot generate cast to " + typeToString(expr.targetType));
    return;
    }
}

void BytecodeGenerator::generateBinaryExpr(BinaryExpr& expr) {
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
        case BinaryOp::BitAnd:
            emit(Instruction(OpCode::BitAnd, expr.loc));
            return;

        case BinaryOp::BitOr:
            emit(Instruction(OpCode::BitOr, expr.loc));
            return;

        case BinaryOp::BitXor:
            emit(Instruction(OpCode::BitXor, expr.loc));
            return;

        case BinaryOp::ShiftLeft:
            emit(Instruction(OpCode::ShiftLeft, expr.loc));
            return;

        case BinaryOp::ShiftRight:
            emit(Instruction(OpCode::ShiftRight, expr.loc));
            return;


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

    failVoid(expr.loc, "unknown binary operator in bytecode generator");
    return;
}

void BytecodeGenerator::generateUnaryExpr(UnaryExpr& expr) {
    generateExpr(*expr.operand);

    switch (expr.op) {
        case UnaryOp::BitNot:
            emit(Instruction(OpCode::BitNot, expr.loc));
            return;


        case UnaryOp::Negate:
            emit(Instruction(OpCode::Neg, expr.loc));
            return;

        case UnaryOp::Not:
            emit(Instruction(OpCode::Not, expr.loc));
            return;
    }

    failVoid(expr.loc, "unknown unary operator in bytecode generator");
    return;
}

void BytecodeGenerator::emitDefaultValue(Type type, int arraySize, SourceLocation location) {
    switch (type) {
        case Type::Int:
            emit(Instruction(OpCode::PushInt, 0, location));
            return;

        case Type::UInt:
            emit(Instruction(OpCode::PushUInt, 0, location));
            return;

        case Type::IntArray:
            if (arraySize <= 0) {
                failVoid(location, "invalid array size for default value");
    return;
            }

            for (int i = 0; i < arraySize; ++i) {
                emit(Instruction(OpCode::PushInt, 0, location));
            }

            emit(Instruction(OpCode::MakeArray, arraySize, location));
            return;

        case Type::Float:
            emit(Instruction(OpCode::PushFloat, 0.0, location));
            return;

        case Type::Bool:
            emit(Instruction(OpCode::PushBool, 0, location));
            return;

        case Type::String:
            emit(Instruction(OpCode::PushString, "", location));
            return;

        case Type::Struct:
            emit(Instruction(OpCode::PushVoid, location));
            return;

        case Type::Generic:
            emit(Instruction(OpCode::PushVoid, location));
            return;

        case Type::Void:
            emit(Instruction(OpCode::PushVoid, location));
            return;

        case Type::Unknown:
            failVoid(location, "cannot emit default value for type " + typeToString(type));
    return;
    }
}

} 
