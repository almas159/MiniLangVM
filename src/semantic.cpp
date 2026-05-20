#include "semantic.hpp"
#include "error.hpp"
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace minilang {

void SemanticAnalyzer::setDiagnostic(SourceLocation location, const std::string& message) {
    if (diagnostic_) {
        return;
    }

    diagnostic_ = Diagnostic(location, message);
}

void SemanticAnalyzer::failVoid(SourceLocation location, const std::string& message) {
    setDiagnostic(location, message);
}

Result<void> SemanticAnalyzer::analyzeExpected(Program& program) {
    analyze(program);

    if (diagnostic_) {
        return std::unexpected(*diagnostic_);
    }

    return {};
}

void SemanticAnalyzer::analyze(Program& program) {
    scopes_.clear();
    functions_.clear();
    structs_.clear();

    collectStructDeclarations(program);
    collectFunctionDeclarations(program);

    auto mainIt = functions_.find("main");

    if (mainIt == functions_.end()) {
        failVoid(program.loc, "program does not contain main function");
    return;
    }

    if (mainIt->second.returnType != Type::Int) {
        failVoid(mainIt->second.location, "main must return int");
    return;
    }

    if (!mainIt->second.parameterTypes.empty()) {
        failVoid(mainIt->second.location, "main must not have parameters");
    return;
    }

    for (auto& function : program.functions) {
        analyzeFunction(*function);
    }
}

void SemanticAnalyzer::collectStructDeclarations(Program& program) {
    for (auto& structDecl : program.structs) {
        if (structs_.find(structDecl->name) != structs_.end()) {
            failVoid(structDecl->loc,
                "struct '" + structDecl->name + "' is already declared");
    return;
        }

        StructSymbol symbol;
        symbol.name = structDecl->name;
        symbol.location = structDecl->loc;

        std::unordered_set<std::string> fieldNames;

        for (const StructFieldDecl& field : structDecl->fields) {
            if (field.type == Type::Void || field.type == Type::Unknown) {
                failVoid(field.loc, "invalid struct field type");
    return;
            }

            if (field.type == Type::IntArray) {
                failVoid(field.loc,
                    "array fields in structs are not supported yet");
    return;
            }

            if (field.type == Type::Struct &&
                structs_.find(field.typeName) == structs_.end() &&
                field.typeName != structDecl->name) {
                failVoid(field.loc,
                    "unknown struct type '" + field.typeName + "'");
    return;
            }

            if (fieldNames.find(field.name) != fieldNames.end()) {
                failVoid(field.loc,
                    "field '" + field.name + "' is already declared in struct '" +
                    structDecl->name + "'");
    return;
            }

            fieldNames.insert(field.name);

            StructFieldSymbol fieldSymbol;
            fieldSymbol.type = field.type;
            fieldSymbol.typeName = field.typeName;
            fieldSymbol.name = field.name;
            fieldSymbol.location = field.loc;

            symbol.fields.push_back(fieldSymbol);
        }

        structs_[structDecl->name] = symbol;
    }
}

void SemanticAnalyzer::collectFunctionDeclarations(Program& program) {
    int index = 0;

    for (auto& function : program.functions) {
        std::unordered_set<std::string> typeParameterSet;

        for (const std::string& typeParameter : function->typeParameters) {
            if (typeParameterSet.find(typeParameter) != typeParameterSet.end()) {
                failVoid(function->loc,
                    "generic type parameter '" + typeParameter +
                    "' is already declared");
                return;
            }

            typeParameterSet.insert(typeParameter);
        }

        if (function->name == "main" && !function->typeParameters.empty()) {
            failVoid(function->loc, "main function cannot be generic");
            return;
        }

        if (function->returnType == Type::Unknown) {
            failVoid(function->loc, "invalid function return type");
            return;
        }

        if (function->returnType == Type::Generic &&
            typeParameterSet.find(function->returnTypeName) == typeParameterSet.end()) {
            failVoid(function->loc,
                "unknown generic return type '" + function->returnTypeName + "'");
            return;
        }

        if (functions_.find(function->name) != functions_.end()) {
            failVoid(function->loc,
                "function '" + function->name + "' is already declared");
            return;
        }

        FunctionSymbol symbol;
        symbol.returnType = function->returnType;
        symbol.returnTypeName = function->returnTypeName;
        symbol.typeParameters = function->typeParameters;
        symbol.functionIndex = index;
        symbol.declaration = function.get();
        symbol.location = function->loc;

        for (const Parameter& parameter : function->parameters) {
            if (parameter.type == Type::Unknown || parameter.type == Type::Void) {
                failVoid(parameter.loc, "invalid parameter type");
                return;
            }

            if (parameter.type == Type::Generic &&
                typeParameterSet.find(parameter.typeName) == typeParameterSet.end()) {
                failVoid(parameter.loc,
                    "unknown generic parameter type '" + parameter.typeName + "'");
                return;
            }

            symbol.parameterTypes.push_back(parameter.type);
            symbol.parameterTypeNames.push_back(parameter.typeName);
        }

        function->functionIndex = index;
        functions_[function->name] = symbol;

        index++;
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
                                       SourceLocation location,
                                       int arraySize,
                                       const std::string& structName,
                                       bool isMutable) {
    if (scopes_.empty()) {
        beginScope();
    }

    auto& currentScope = scopes_.back();

    if (currentScope.find(name) != currentScope.end()) {
        failVoid(location,
            "variable '" + name + "' is already declared in this scope");
    return;
    }

    currentScope[name] = Symbol{type, localIndex, arraySize, structName, location, isMutable};
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

void SemanticAnalyzer::analyzeFunction(FunctionDecl& function) {
    scopes_.clear();

    nextLocalIndex_ = 0;
    currentReturnType_ = function.returnType;
    currentReturnTypeName_ = function.returnTypeName;
    currentFunctionName_ = function.name;
    currentFunctionHasReturn_ = false;

    currentTypeParameters_.clear();

    for (const std::string& typeParameter : function.typeParameters) {
        currentTypeParameters_.insert(typeParameter);
    }

    loopDepth_ = 0;
    switchDepth_ = 0;

    beginScope();

    for (Parameter& parameter : function.parameters) {
        int localIndex = nextLocalIndex_++;
        parameter.localIndex = localIndex;

        declareVariable(
            parameter.name,
            parameter.type,
            localIndex,
            parameter.loc,
            -1,
            parameter.typeName
        );
    }

    analyzeBlock(*function.body);

    endScope();

    if (currentReturnType_ != Type::Void && !currentFunctionHasReturn_) {
        failVoid(function.loc,
            "function '" + function.name + "' must contain return statement");
    return;
    }

    function.localCount = nextLocalIndex_;
}

void SemanticAnalyzer::analyzeBlock(BlockStmt& block) {
    beginScope();

    for (auto& statement : block.statements) {
        analyzeStmt(*statement);
    }

    endScope();
}

void SemanticAnalyzer::analyzeStmt(Stmt& stmt) {
    if (dynamic_cast<EmptyStmt*>(&stmt)) {
        return;
    }

    if (auto* s = dynamic_cast<ExprStmt*>(&stmt)) {
        analyzeExprStmt(*s);
        return;
    }

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

    if (auto* s = dynamic_cast<ArrayAssignStmt*>(&stmt)) {
        analyzeArrayAssignStmt(*s);
        return;
    }

    if (auto* s = dynamic_cast<FieldAssignStmt*>(&stmt)) {
        analyzeFieldAssignStmt(*s);
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

    if (auto* s = dynamic_cast<ForStmt*>(&stmt)) {
        analyzeForStmt(*s);
        return;
    }

    if (auto* s = dynamic_cast<SwitchStmt*>(&stmt)) {
        analyzeSwitchStmt(*s);
        return;
    }

    if (auto* s = dynamic_cast<ReturnStmt*>(&stmt)) {
        analyzeReturnStmt(*s);
        return;
    }

    if (auto* s = dynamic_cast<BreakStmt*>(&stmt)) {
        analyzeBreakStmt(*s);
        return;
    }

    if (auto* s = dynamic_cast<ContinueStmt*>(&stmt)) {
        analyzeContinueStmt(*s);
        return;
    }

    if (auto* s = dynamic_cast<PrintStmt*>(&stmt)) {
        analyzePrintStmt(*s);
        return;
    }

    failVoid(stmt.loc, "unknown statement");
    return;
}

void SemanticAnalyzer::analyzeExprStmt(ExprStmt& stmt) {
    analyzeExpr(*stmt.expression);
}

void SemanticAnalyzer::analyzeVarDeclStmt(VarDeclStmt& stmt) {
    if (stmt.type == Type::Void) {
        failVoid(stmt.loc, "invalid variable type");
    return;
    }

    if (!scopes_.empty()) {
        auto& currentScope = scopes_.back();

        if (currentScope.find(stmt.name) != currentScope.end()) {
            failVoid(stmt.loc,
                "variable '" + stmt.name + "' is already declared in this scope");
    return;
        }
    }

    if (stmt.type == Type::Struct) {
        if (stmt.structName.empty() ||
            structs_.find(stmt.structName) == structs_.end()) {
            failVoid(stmt.loc,
                "unknown struct type '" + stmt.structName + "'");
    return;
        }

        if (!stmt.initializer) {
            failVoid(stmt.loc,
                "struct variable '" + stmt.name + "' requires initializer");
    return;
        }

        Type initializerType = analyzeExpr(*stmt.initializer);

        if (initializerType != Type::Struct ||
            stmt.initializer->inferredStructName != stmt.structName) {
            failVoid(stmt.initializer->loc,
                "cannot initialize struct '" + stmt.structName +
                "' variable '" + stmt.name + "' with " +
                typeToString(initializerType) + " value");
    return;
        }

        int localIndex = nextLocalIndex_++;
        stmt.localIndex = localIndex;

        declareVariable(stmt.name, stmt.type, localIndex, stmt.loc, -1, stmt.structName, stmt.isMutable);
        return;
    }

    if (stmt.type == Type::IntArray) {
        if (stmt.arraySize <= 0) {
            failVoid(stmt.loc, "array size must be positive");
    return;
        }

        if (stmt.initializer) {
            Type initializerType = analyzeExpr(*stmt.initializer);

            if (initializerType != Type::IntArray) {
                failVoid(stmt.initializer->loc,
                    "cannot initialize int32[] variable '" + stmt.name +
                    "' with " + typeToString(initializerType) + " value");
    return;
            }

            if (stmt.initializer->inferredArraySize != stmt.arraySize) {
                failVoid(stmt.initializer->loc,
                    "array initializer size mismatch for '" + stmt.name +
                    "': expected " + std::to_string(stmt.arraySize) +
                    ", got " + std::to_string(stmt.initializer->inferredArraySize));
    return;
            }
        }

        int localIndex = nextLocalIndex_++;
        stmt.localIndex = localIndex;

        declareVariable(stmt.name, stmt.type, localIndex, stmt.loc, stmt.arraySize, stmt.structName, stmt.isMutable);
        return;
    }

    if (stmt.type == Type::Unknown) {
        if (!stmt.initializer) {
            failVoid(stmt.loc,
                "var declaration requires initializer");
    return;
        }

        Type initializerType = analyzeExpr(*stmt.initializer);

        if (initializerType == Type::Unknown || initializerType == Type::Void) {
            failVoid(stmt.initializer->loc,
                "cannot infer type for variable '" + stmt.name + "'");
    return;
        }

        stmt.type = initializerType;

        if (initializerType == Type::IntArray) {
            stmt.arraySize = stmt.initializer->inferredArraySize;
        }

        if (initializerType == Type::Struct) {
            stmt.structName = stmt.initializer->inferredStructName;
        }
    } else if (stmt.initializer) {
        Type initializerType = analyzeExpr(*stmt.initializer);

        if (stmt.type == Type::UInt && initializerType == Type::Int) {
            auto* literal = dynamic_cast<IntLiteralExpr*>(stmt.initializer.get());

            if (literal && literal->value >= 0) {
                stmt.initializer->inferredType = Type::UInt;
            } else {
                failVoid(stmt.initializer->loc,
                    "cannot initialize uint variable '" + stmt.name +
                    "' with int value without cast<uint>");
                return;
            }
        } else if (!sameType(stmt.type, initializerType)) {
            failVoid(stmt.initializer->loc,
                "cannot initialize " + typeToString(stmt.type) +
                " variable '" + stmt.name + "' with " +
                typeToString(initializerType) + " value");
            return;
        }
    }

    int localIndex = nextLocalIndex_++;
    stmt.localIndex = localIndex;

    declareVariable(stmt.name, stmt.type, localIndex, stmt.loc, stmt.arraySize, stmt.structName, stmt.isMutable);
}

void SemanticAnalyzer::analyzeAssignStmt(AssignStmt& stmt) {
    Symbol* symbol = findVariable(stmt.name);

    if (!symbol) {
        failVoid(stmt.loc,
            "variable '" + stmt.name + "' is not declared");
    return;
    }

    if (!symbol->isMutable) {
        failVoid(stmt.loc,
            "cannot assign to immutable variable '" + stmt.name + "'");
    return;
    }

    Type valueType = analyzeExpr(*stmt.value);

    if (symbol->type == Type::Struct) {
        if (valueType != Type::Struct ||
            stmt.value->inferredStructName != symbol->structName) {
            failVoid(stmt.value->loc,
                "cannot assign struct '" + stmt.value->inferredStructName +
                "' to struct '" + symbol->structName +
                "' variable '" + stmt.name + "'");
    return;
        }

        stmt.localIndex = symbol->localIndex;
        return;
    }

    if (symbol->type == Type::UInt && valueType == Type::Int) {
        auto* literal = dynamic_cast<IntLiteralExpr*>(stmt.value.get());

        if (literal && literal->value >= 0) {
            stmt.value->inferredType = Type::UInt;
        } else {
            failVoid(stmt.value->loc,
                "cannot assign int value to uint variable '" + stmt.name +
                "' without cast<uint>");
            return;
        }
    } else if (!sameType(symbol->type, valueType)) {
        failVoid(stmt.value->loc,
            "cannot assign " + typeToString(valueType) +
            " to " + typeToString(symbol->type) +
            " variable '" + stmt.name + "'");
        return;
    }

    stmt.localIndex = symbol->localIndex;
}

void SemanticAnalyzer::analyzeArrayAssignStmt(ArrayAssignStmt& stmt) {
    Symbol* symbol = findVariable(stmt.name);

    if (!symbol) {
        failVoid(stmt.loc,
            "array '" + stmt.name + "' is not declared");
    return;
    }

    if (!symbol->isMutable) {
        failVoid(stmt.loc,
            "cannot modify immutable array '" + stmt.name + "'");
    return;
    }

    if (symbol->type != Type::IntArray) {
        failVoid(stmt.loc,
            "variable '" + stmt.name + "' is not an array");
    return;
    }

    Type indexType = analyzeExpr(*stmt.index);

    if (indexType != Type::Int) {
        failVoid(stmt.index->loc,
            "array index must be int, got " + typeToString(indexType));
    return;
    }

    Type valueType = analyzeExpr(*stmt.value);

    if (valueType != Type::Int) {
        failVoid(stmt.value->loc,
            "int32[] element must be int, got " + typeToString(valueType));
    return;
    }

    stmt.localIndex = symbol->localIndex;
}

void SemanticAnalyzer::analyzeFieldAssignStmt(FieldAssignStmt& stmt) {
    Symbol* symbol = findVariable(stmt.objectName);

    if (!symbol) {
        failVoid(stmt.loc,
            "variable '" + stmt.objectName + "' is not declared");
    return;
    }

    if (!symbol->isMutable) {
        failVoid(stmt.loc,
            "cannot modify immutable struct '" + stmt.objectName + "'");
    return;
    }

    if (symbol->type != Type::Struct) {
        failVoid(stmt.loc,
            "variable '" + stmt.objectName + "' is not a struct");
    return;
    }

    auto structIt = structs_.find(symbol->structName);

    if (structIt == structs_.end()) {
        failVoid(stmt.loc,
            "unknown struct type '" + symbol->structName + "'");
    return;
    }

    const StructSymbol& structSymbol = structIt->second;

    int fieldIndex = -1;
    const StructFieldSymbol* fieldSymbol = nullptr;

    for (std::size_t i = 0; i < structSymbol.fields.size(); ++i) {
        if (structSymbol.fields[i].name == stmt.fieldName) {
            fieldIndex = static_cast<int>(i);
            fieldSymbol = &structSymbol.fields[i];
            break;
        }
    }

    if (!fieldSymbol) {
        failVoid(stmt.loc,
            "struct '" + symbol->structName +
            "' has no field '" + stmt.fieldName + "'");
    return;
    }

    Type valueType = analyzeExpr(*stmt.value);

    if (fieldSymbol->type == Type::Struct) {
        if (valueType != Type::Struct ||
            stmt.value->inferredStructName != fieldSymbol->typeName) {
            failVoid(stmt.value->loc,
                "field '" + stmt.fieldName +
                "' must be struct '" + fieldSymbol->typeName + "'");
    return;
        }
    } else if (valueType != fieldSymbol->type) {
        failVoid(stmt.value->loc,
            "field '" + stmt.fieldName + "' must be " +
            typeToString(fieldSymbol->type) + ", got " +
            typeToString(valueType));
    return;
    }

    stmt.localIndex = symbol->localIndex;
    stmt.fieldIndex = fieldIndex;
}

void SemanticAnalyzer::analyzeIfStmt(IfStmt& stmt) {
    Type conditionType = analyzeExpr(*stmt.condition);

    if (conditionType != Type::Bool) {
        failVoid(stmt.condition->loc,
            "condition of if must be bool, got " + typeToString(conditionType));
    return;
    }

    analyzeStmt(*stmt.thenBranch);

    if (stmt.elseBranch) {
        analyzeStmt(*stmt.elseBranch);
    }
}

void SemanticAnalyzer::analyzeWhileStmt(WhileStmt& stmt) {
    Type conditionType = analyzeExpr(*stmt.condition);

    if (conditionType != Type::Bool) {
        failVoid(stmt.condition->loc,
            "condition of while must be bool, got " + typeToString(conditionType));
    return;
    }

    loopDepth_++;
    analyzeStmt(*stmt.body);
    loopDepth_--;
}

void SemanticAnalyzer::analyzeForStmt(ForStmt& stmt) {
    beginScope();

    if (stmt.initializer) {
        analyzeStmt(*stmt.initializer);
    }

    Type conditionType = analyzeExpr(*stmt.condition);

    if (conditionType != Type::Bool) {
        failVoid(stmt.condition->loc,
            "condition of for must be bool, got " + typeToString(conditionType));
    return;
    }

    loopDepth_++;

    analyzeStmt(*stmt.body);

    if (stmt.update) {
        analyzeStmt(*stmt.update);
    }

    loopDepth_--;

    endScope();
}

void SemanticAnalyzer::analyzeSwitchStmt(SwitchStmt& stmt) {
    Type expressionType = analyzeExpr(*stmt.expression);

    if (expressionType != Type::Int) {
        failVoid(stmt.expression->loc,
            "switch expression must be int, got " + typeToString(expressionType));
    return;
    }

    beginScope();
    switchDepth_++;

    for (auto& switchCase : stmt.cases) {
        for (auto& statement : switchCase.statements) {
            analyzeStmt(*statement);
        }
    }

    if (stmt.hasDefault) {
        for (auto& statement : stmt.defaultStatements) {
            analyzeStmt(*statement);
        }
    }

    switchDepth_--;
    endScope();
}

void SemanticAnalyzer::analyzeReturnStmt(ReturnStmt& stmt) {
    if (currentReturnType_ == Type::Void) {
        if (stmt.value) {
            Type valueType = analyzeExpr(*stmt.value);

            failVoid(stmt.value->loc,
                "function '" + currentFunctionName_ +
                "' must not return a value, got " + typeToString(valueType));
            return;
        }

        currentFunctionHasReturn_ = true;
        return;
    }

    if (!stmt.value) {
        failVoid(stmt.loc,
            "function '" + currentFunctionName_ + "' must return " +
            typeToString(currentReturnType_));
        return;
    }

    Type valueType = analyzeExpr(*stmt.value);

    if (currentReturnType_ == Type::Generic) {
        if (valueType != Type::Generic ||
            stmt.value->inferredStructName != currentReturnTypeName_) {
            failVoid(stmt.value->loc,
                "function '" + currentFunctionName_ +
                "' must return generic type '" + currentReturnTypeName_ + "'");
            return;
        }

        currentFunctionHasReturn_ = true;
        return;
    }

    if (currentReturnType_ == Type::Struct) {
        if (valueType != Type::Struct ||
            stmt.value->inferredStructName != currentReturnTypeName_) {
            failVoid(stmt.value->loc,
                "function '" + currentFunctionName_ + "' must return struct '" +
                currentReturnTypeName_ + "'");
            return;
        }

        currentFunctionHasReturn_ = true;
        return;
    }

    if (currentReturnType_ == Type::UInt && valueType == Type::Int) {
        auto* literal = dynamic_cast<IntLiteralExpr*>(stmt.value.get());

        if (literal && literal->value >= 0) {
            stmt.value->inferredType = Type::UInt;
        } else {
            failVoid(stmt.value->loc,
                "function '" + currentFunctionName_ +
                "' must return uint, got int without cast<uint>");
            return;
        }
    } else if (!sameType(currentReturnType_, valueType)) {
        failVoid(stmt.value->loc,
            "function '" + currentFunctionName_ + "' must return " +
            typeToString(currentReturnType_) + ", got " + typeToString(valueType));
        return;
    }

    currentFunctionHasReturn_ = true;
}

void SemanticAnalyzer::analyzeBreakStmt(BreakStmt& stmt) {
    if (loopDepth_ <= 0 && switchDepth_ <= 0) {
        failVoid(stmt.loc, "'break' can be used only inside a loop or switch");
    return;
    }
}

void SemanticAnalyzer::analyzeContinueStmt(ContinueStmt& stmt) {
    if (loopDepth_ <= 0) {
        failVoid(stmt.loc, "'continue' can be used only inside a loop");
    return;
    }
}

void SemanticAnalyzer::analyzePrintStmt(PrintStmt& stmt) {
    for (auto& argument : stmt.arguments) {
        Type argType = analyzeExpr(*argument);

        if (argType != Type::Int &&
            argType != Type::UInt &&
            argType != Type::IntArray &&
            argType != Type::Struct &&
            argType != Type::Float &&
            argType != Type::Bool &&
            argType != Type::String) {
            failVoid(argument->loc,
                "print does not support value of type " + typeToString(argType));
    return;
        }
    }
}

Type SemanticAnalyzer::analyzeExpr(Expr& expr) {
    if (auto* e = dynamic_cast<ArrayLiteralExpr*>(&expr)) {
        return analyzeArrayLiteralExpr(*e);
    }

    if (auto* e = dynamic_cast<IndexExpr*>(&expr)) {
        return analyzeIndexExpr(*e);
    }

    if (auto* e = dynamic_cast<StructLiteralExpr*>(&expr)) {
        return analyzeStructLiteralExpr(*e);
    }

    if (auto* e = dynamic_cast<FieldAccessExpr*>(&expr)) {
        return analyzeFieldAccessExpr(*e);
    }

    if (auto* e = dynamic_cast<IntLiteralExpr*>(&expr)) {
        return analyzeIntLiteralExpr(*e);
    }

    if (auto* e = dynamic_cast<FloatLiteralExpr*>(&expr)) {
        return analyzeFloatLiteralExpr(*e);
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

    if (auto* e = dynamic_cast<CallExpr*>(&expr)) {
        return analyzeCallExpr(*e);
    }

    if (auto* e = dynamic_cast<CastExpr*>(&expr)) {
        return analyzeCastExpr(*e);
    }

    if (auto* e = dynamic_cast<BinaryExpr*>(&expr)) {
        return analyzeBinaryExpr(*e);
    }

    if (auto* e = dynamic_cast<UnaryExpr*>(&expr)) {
        return analyzeUnaryExpr(*e);
    }

    return fail<Type>(expr.loc, "unknown expression");
}

Type SemanticAnalyzer::analyzeArrayLiteralExpr(ArrayLiteralExpr& expr) {
    if (expr.elements.empty()) {
        return fail<Type>(expr.loc, "empty array literal cannot infer element type");
    }

    for (auto& element : expr.elements) {
        Type elementType = analyzeExpr(*element);

        if (elementType != Type::Int) {
            return fail<Type>(element->loc,
                "int32[] literal element must be int, got " + typeToString(elementType));
        }
    }

    expr.inferredType = Type::IntArray;
    expr.inferredArraySize = static_cast<int>(expr.elements.size());

    return expr.inferredType;
}

Type SemanticAnalyzer::analyzeIndexExpr(IndexExpr& expr) {
    Symbol* symbol = findVariable(expr.arrayName);

    if (!symbol) {
        return fail<Type>(expr.loc,
            "array '" + expr.arrayName + "' is not declared");
    }

    if (symbol->type != Type::IntArray) {
        return fail<Type>(expr.loc,
            "variable '" + expr.arrayName + "' is not an array");
    }

    Type indexType = analyzeExpr(*expr.index);

    if (indexType != Type::Int) {
        return fail<Type>(expr.index->loc,
            "array index must be int, got " + typeToString(indexType));
    }

    expr.localIndex = symbol->localIndex;
    expr.inferredType = Type::Int;

    return expr.inferredType;
}

Type SemanticAnalyzer::analyzeStructLiteralExpr(StructLiteralExpr& expr) {
    auto structIt = structs_.find(expr.structName);

    if (structIt == structs_.end()) {
        return fail<Type>(expr.loc,
            "unknown struct type '" + expr.structName + "'");
    }

    const StructSymbol& structSymbol = structIt->second;

    if (expr.fields.size() != structSymbol.fields.size()) {
        return fail<Type>(expr.loc,
            "struct literal for '" + expr.structName + "' expects " +
            std::to_string(structSymbol.fields.size()) +
            " fields, got " + std::to_string(expr.fields.size()));
    }

    std::vector<bool> seen(structSymbol.fields.size(), false);

    for (StructLiteralField& literalField : expr.fields) {
        int fieldIndex = -1;

        for (std::size_t i = 0; i < structSymbol.fields.size(); ++i) {
            if (structSymbol.fields[i].name == literalField.name) {
                fieldIndex = static_cast<int>(i);
                break;
            }
        }

        if (fieldIndex < 0) {
            return fail<Type>(literalField.loc,
                "struct '" + expr.structName +
                "' has no field '" + literalField.name + "'");
        }

        if (seen[fieldIndex]) {
            return fail<Type>(literalField.loc,
                "field '" + literalField.name +
                "' is already initialized");
        }

        seen[fieldIndex] = true;
        literalField.fieldIndex = fieldIndex;

        const StructFieldSymbol& expectedField = structSymbol.fields[fieldIndex];

        Type valueType = analyzeExpr(*literalField.value);

        if (expectedField.type == Type::Struct) {
            if (valueType != Type::Struct ||
                literalField.value->inferredStructName != expectedField.typeName) {
                return fail<Type>(literalField.value->loc,
                    "field '" + literalField.name +
                    "' must be struct '" + expectedField.typeName + "'");
            }
        } else if (valueType != expectedField.type) {
            return fail<Type>(literalField.value->loc,
                "field '" + literalField.name + "' must be " +
                typeToString(expectedField.type) + ", got " +
                typeToString(valueType));
        }
    }

    for (std::size_t i = 0; i < seen.size(); ++i) {
        if (!seen[i]) {
            return fail<Type>(expr.loc,
                "field '" + structSymbol.fields[i].name +
                "' is not initialized");
        }
    }

    expr.inferredType = Type::Struct;
    expr.inferredStructName = expr.structName;

    return expr.inferredType;
}

Type SemanticAnalyzer::analyzeFieldAccessExpr(FieldAccessExpr& expr) {
    Symbol* symbol = findVariable(expr.objectName);

    if (!symbol) {
        return fail<Type>(expr.loc,
            "variable '" + expr.objectName + "' is not declared");
    }

    if (symbol->type != Type::Struct) {
        return fail<Type>(expr.loc,
            "variable '" + expr.objectName + "' is not a struct");
    }

    auto structIt = structs_.find(symbol->structName);

    if (structIt == structs_.end()) {
        return fail<Type>(expr.loc,
            "unknown struct type '" + symbol->structName + "'");
    }

    const StructSymbol& structSymbol = structIt->second;

    for (std::size_t i = 0; i < structSymbol.fields.size(); ++i) {
        const StructFieldSymbol& field = structSymbol.fields[i];

        if (field.name == expr.fieldName) {
            expr.localIndex = symbol->localIndex;
            expr.fieldIndex = static_cast<int>(i);
            expr.inferredType = field.type;

            if (field.type == Type::Struct) {
                expr.inferredStructName = field.typeName;
            }

            return expr.inferredType;
        }
    }

    return fail<Type>(expr.loc,
        "struct '" + symbol->structName +
        "' has no field '" + expr.fieldName + "'");
}

Type SemanticAnalyzer::analyzeIntLiteralExpr(IntLiteralExpr& expr) {
    expr.inferredType = Type::Int;
    return expr.inferredType;
}

Type SemanticAnalyzer::analyzeFloatLiteralExpr(FloatLiteralExpr& expr) {
    expr.inferredType = Type::Float;
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
        return fail<Type>(expr.loc,
            "variable '" + expr.name + "' is not declared");
    }

    expr.localIndex = symbol->localIndex;
    expr.inferredType = symbol->type;

    if (symbol->type == Type::IntArray) {
        expr.inferredArraySize = symbol->arraySize;
    }

    if (symbol->type == Type::Struct || symbol->type == Type::Generic) {
        expr.inferredStructName = symbol->structName;
    }

    return expr.inferredType;
}

Type SemanticAnalyzer::analyzeCallExpr(CallExpr& expr) {
    if (expr.callee == "toInt") {
        if (expr.arguments.size() != 1) {
            return fail<Type>(expr.loc, "toInt expects 1 argument");
        }

        Type argumentType = analyzeExpr(*expr.arguments[0]);

        if (argumentType != Type::String) {
            return fail<Type>(
                expr.arguments[0]->loc,
                "toInt argument must be string, got " + typeToString(argumentType)
            );
        }

        expr.functionIndex = -1;
        expr.inferredType = Type::Int;
        return expr.inferredType;
    }


    if (expr.callee == "sizeof") {
        if (expr.arguments.size() != 1) {
            return fail<Type>(expr.loc, "sizeof expects 1 argument");
        }

        Type argumentType = analyzeExpr(*expr.arguments[0]);

        if (argumentType == Type::Void || argumentType == Type::Unknown) {
            return fail<Type>(
                expr.arguments[0]->loc,
                "sizeof cannot be applied to " + typeToString(argumentType)
            );
        }

        expr.functionIndex = -1;
        expr.inferredType = Type::Int;
        return expr.inferredType;
    }

    if (expr.callee == "typeid" || expr.callee == "typeof") {
        if (expr.arguments.size() != 1) {
            return fail<Type>(
                expr.loc,
                expr.callee + " expects 1 argument"
            );
        }

        Type argumentType = analyzeExpr(*expr.arguments[0]);

        if (argumentType == Type::Void || argumentType == Type::Unknown) {
            return fail<Type>(
                expr.arguments[0]->loc,
                expr.callee + " cannot be applied to " + typeToString(argumentType)
            );
        }

        expr.functionIndex = -1;
        expr.inferredType = Type::String;
        return expr.inferredType;
    }

    if (expr.callee == "input") {
        if (!expr.arguments.empty()) {
            return fail<Type>(expr.loc, "input expects 0 arguments");
        }

        expr.functionIndex = -1;
        expr.inferredType = Type::String;
        return expr.inferredType;
    }

    if (expr.callee == "assert") {
        if (expr.arguments.size() != 1 && expr.arguments.size() != 2) {
            return fail<Type>(expr.loc, "assert expects 1 or 2 arguments");
        }

        Type conditionType = analyzeExpr(*expr.arguments[0]);

        if (conditionType != Type::Bool) {
            return fail<Type>(expr.arguments[0]->loc,
                "assert condition must be bool, got " +
                typeToString(conditionType));
        }

        if (expr.arguments.size() == 2) {
            Type messageType = analyzeExpr(*expr.arguments[1]);

            if (messageType != Type::String) {
                return fail<Type>(expr.arguments[1]->loc,
                    "assert message must be string, got " +
                    typeToString(messageType));
            }
        }

        expr.functionIndex = -1;
        expr.inferredType = Type::Void;
        return expr.inferredType;
    }

    if (expr.callee == "len") {
        if (expr.arguments.size() != 1) {
            return fail<Type>(expr.loc, "len expects 1 argument");
        }

        Type argumentType = analyzeExpr(*expr.arguments[0]);

        if (argumentType != Type::String &&
            argumentType != Type::IntArray) {
            return fail<Type>(expr.arguments[0]->loc,
                "len argument must be string or int32[], got " +
                typeToString(argumentType));
        }

        expr.functionIndex = -1;
        expr.inferredType = Type::Int;
        return expr.inferredType;
    }

    if (expr.callee == "exit") {
        if (expr.arguments.size() != 1) {
            return fail<Type>(expr.loc, "exit expects 1 argument");
        }

        Type codeType = analyzeExpr(*expr.arguments[0]);

        if (codeType != Type::Int) {
            return fail<Type>(expr.arguments[0]->loc,
                "exit argument must be int, got " + typeToString(codeType));
        }

        expr.functionIndex = -1;
        expr.inferredType = Type::Void;
        return expr.inferredType;
    }

    if (expr.callee == "panic") {
        if (expr.arguments.size() != 1) {
            return fail<Type>(expr.loc, "panic expects 1 argument");
        }

        Type messageType = analyzeExpr(*expr.arguments[0]);

        if (messageType != Type::String) {
            return fail<Type>(expr.arguments[0]->loc,
                "panic argument must be string, got " + typeToString(messageType));
        }

        expr.functionIndex = -1;
        expr.inferredType = Type::Void;
        return expr.inferredType;
    }

    auto found = functions_.find(expr.callee);

    if (found == functions_.end()) {
        return fail<Type>(expr.loc,
            "function '" + expr.callee + "' is not declared");
    }

    FunctionSymbol& function = found->second;

    if (expr.arguments.size() != function.parameterTypes.size()) {
        return fail<Type>(expr.loc,
            "function '" + expr.callee + "' expects " +
            std::to_string(function.parameterTypes.size()) +
            " arguments, got " +
            std::to_string(expr.arguments.size()));
    }

    struct GenericBinding {
        Type type = Type::Unknown;
        std::string typeName;
    };

    std::unordered_map<std::string, GenericBinding> genericBindings;

    if (!expr.typeArguments.empty()) {
        if (function.typeParameters.empty()) {
            return fail<Type>(
                expr.loc,
                "function '" + expr.callee + "' is not generic"
            );
        }

        if (expr.typeArguments.size() != function.typeParameters.size()) {
            return fail<Type>(
                expr.loc,
                "function '" + expr.callee + "' expects " +
                std::to_string(function.typeParameters.size()) +
                " generic type arguments, got " +
                std::to_string(expr.typeArguments.size())
            );
        }

        for (std::size_t i = 0; i < expr.typeArguments.size(); ++i) {
            Type explicitType = expr.typeArguments[i];
            std::string explicitTypeName = expr.typeArgumentNames[i];

            if (explicitType == Type::Void ||
                explicitType == Type::Unknown ||
                explicitType == Type::Generic) {
                return fail<Type>(
                    expr.loc,
                    "invalid generic type argument"
                );
            }

            genericBindings[function.typeParameters[i]] =
                GenericBinding{explicitType, explicitTypeName};
        }
    }

    for (std::size_t i = 0; i < expr.arguments.size(); ++i) {
        Type argumentType = analyzeExpr(*expr.arguments[i]);
        Type parameterType = function.parameterTypes[i];

        if (parameterType == Type::Generic) {
            const std::string& genericName = function.parameterTypeNames[i];

            auto existing = genericBindings.find(genericName);

            std::string argumentTypeName;

            if (argumentType == Type::Struct || argumentType == Type::Generic) {
                argumentTypeName = expr.arguments[i]->inferredStructName;
            }

            if (existing == genericBindings.end()) {
                genericBindings[genericName] = GenericBinding{argumentType, argumentTypeName};
            } else {
                bool sameBinding = existing->second.type == argumentType;

                if ((argumentType == Type::Struct || argumentType == Type::Generic) &&
                    existing->second.typeName != argumentTypeName) {
                    sameBinding = false;
                }

                if (!sameBinding) {
                    return fail<Type>(expr.arguments[i]->loc,
                        "generic type '" + genericName +
                        "' has inconsistent argument types in call to '" +
                        expr.callee + "'");
                }
            }

            continue;
        }

        if (parameterType == Type::Struct) {
            if (argumentType != Type::Struct ||
                expr.arguments[i]->inferredStructName != function.parameterTypeNames[i]) {
                return fail<Type>(expr.arguments[i]->loc,
                    "argument " + std::to_string(i + 1) +
                    " of function '" + expr.callee + "' must be struct '" +
                    function.parameterTypeNames[i] + "'");
            }

            continue;
        }

        if (!(sameType(parameterType, argumentType) ||
              (parameterType == Type::UInt && argumentType == Type::Int))) {
            return fail<Type>(expr.arguments[i]->loc,
                "argument " + std::to_string(i + 1) +
                " of function '" + expr.callee + "' must be " +
                typeToString(parameterType) + ", got " +
                typeToString(argumentType));
        }
    }

    expr.functionIndex = function.functionIndex;

    if (function.returnType == Type::Generic) {
        auto binding = genericBindings.find(function.returnTypeName);

        if (binding == genericBindings.end()) {
            return fail<Type>(expr.loc,
                "cannot infer generic return type '" +
                function.returnTypeName + "' in call to '" + expr.callee + "'");
        }

        expr.inferredType = binding->second.type;

        if (expr.inferredType == Type::Struct || expr.inferredType == Type::Generic) {
            expr.inferredStructName = binding->second.typeName;
        }

        return expr.inferredType;
    }

    expr.inferredType = function.returnType;

    if (function.returnType == Type::Struct) {
        expr.inferredStructName = function.returnTypeName;
    }

    return expr.inferredType;
}

Type SemanticAnalyzer::analyzeCastExpr(CastExpr& expr) {
    Type sourceType = analyzeExpr(*expr.expression);

    if (expr.targetType != Type::Int &&
        expr.targetType != Type::UInt &&
        expr.targetType != Type::Float &&
        expr.targetType != Type::Bool) {
        return fail<Type>(expr.loc,
            "unsupported cast target type " + typeToString(expr.targetType));
    }

    if (sourceType != Type::Int &&
        sourceType != Type::UInt &&
        sourceType != Type::Float &&
        sourceType != Type::Bool) {
        return fail<Type>(expr.expression->loc,
            "unsupported cast source type " + typeToString(sourceType));
    }

    expr.inferredType = expr.targetType;
    return expr.inferredType;
}

static bool isNonNegativeIntLiteralForUInt(Expr& expr) {
    auto* literal = dynamic_cast<IntLiteralExpr*>(&expr);

    if (!literal) {
        return false;
    }

    return literal->value >= 0;
}

Type SemanticAnalyzer::analyzeBinaryExpr(BinaryExpr& expr) {
    Type leftType = analyzeExpr(*expr.left);
    Type rightType = analyzeExpr(*expr.right);

    if (leftType == Type::UInt &&
        rightType == Type::Int &&
        isNonNegativeIntLiteralForUInt(*expr.right)) {
        rightType = Type::UInt;
        expr.right->inferredType = Type::UInt;
    }

    if (leftType == Type::Int &&
        rightType == Type::UInt &&
        isNonNegativeIntLiteralForUInt(*expr.left)) {
        leftType = Type::UInt;
        expr.left->inferredType = Type::UInt;
    }

    switch (expr.op) {
        case BinaryOp::BitAnd:
        case BinaryOp::BitOr:
        case BinaryOp::BitXor: {
            if (leftType != rightType) {
                return fail<Type>(
                    expr.loc,
                    "bitwise operator '" + binaryOpToString(expr.op) +
                    "' requires operands of the same type"
                );
            }

            if (leftType != Type::Int && leftType != Type::UInt) {
                return fail<Type>(
                    expr.loc,
                    "bitwise operator '" + binaryOpToString(expr.op) +
                    "' can be applied only to int or uint"
                );
            }

            expr.inferredType = leftType;
            return expr.inferredType;
        }

        case BinaryOp::ShiftLeft:
        case BinaryOp::ShiftRight: {
            if (leftType != Type::Int && leftType != Type::UInt) {
                return fail<Type>(
                    expr.loc,
                    "shift operator '" + binaryOpToString(expr.op) +
                    "' left operand must be int or uint"
                );
            }

            if (rightType != Type::Int && rightType != Type::UInt) {
                return fail<Type>(
                    expr.loc,
                    "shift operator '" + binaryOpToString(expr.op) +
                    "' right operand must be int or uint"
                );
            }

            expr.inferredType = leftType;
            return expr.inferredType;
        }


        case BinaryOp::Add:
            if (leftType == Type::Int && rightType == Type::Int) {
                expr.inferredType = Type::Int;
                return expr.inferredType;
            }

            if (leftType == Type::UInt && rightType == Type::UInt) {
                expr.inferredType = Type::UInt;
                return expr.inferredType;
            }

            if (leftType == Type::Float && rightType == Type::Float) {
                expr.inferredType = Type::Float;
                return expr.inferredType;
            }

            if (leftType == Type::String && rightType == Type::String) {
                expr.inferredType = Type::String;
                return expr.inferredType;
            }

            return fail<Type>(expr.loc,
                "operator '+' cannot be applied to " +
                typeToString(leftType) + " and " + typeToString(rightType));

        case BinaryOp::Sub:
        case BinaryOp::Mul:
        case BinaryOp::Div:
        case BinaryOp::Mod:
            if (leftType == Type::Int && rightType == Type::Int) {
                expr.inferredType = Type::Int;
                return expr.inferredType;
            }

            if (leftType == Type::UInt && rightType == Type::UInt) {
                expr.inferredType = Type::UInt;
                return expr.inferredType;
            }

            if (leftType == Type::Float &&
                rightType == Type::Float &&
                expr.op != BinaryOp::Mod) {
                expr.inferredType = Type::Float;
                return expr.inferredType;
            }

            return fail<Type>(expr.loc,
                "arithmetic operator '" + binaryOpToString(expr.op) +
                "' cannot be applied to " +
                typeToString(leftType) + " and " + typeToString(rightType));

        case BinaryOp::Less:
        case BinaryOp::Greater:
        case BinaryOp::LessEqual:
        case BinaryOp::GreaterEqual:
            if (leftType == Type::Int && rightType == Type::Int) {
                expr.inferredType = Type::Bool;
                return expr.inferredType;
            }

            if (leftType == Type::UInt && rightType == Type::UInt) {
                expr.inferredType = Type::Bool;
                return expr.inferredType;
            }

            if (leftType == Type::Float && rightType == Type::Float) {
                expr.inferredType = Type::Bool;
                return expr.inferredType;
            }

            return fail<Type>(expr.loc,
                "comparison operator '" + binaryOpToString(expr.op) +
                "' cannot be applied to " +
                typeToString(leftType) + " and " + typeToString(rightType));

        case BinaryOp::Equal:
        case BinaryOp::NotEqual:
            if (sameType(leftType, rightType) &&
                (leftType == Type::Int ||
                 leftType == Type::UInt ||
                 leftType == Type::Float ||
                 leftType == Type::Bool ||
                 leftType == Type::String)) {
                expr.inferredType = Type::Bool;
                return expr.inferredType;
            }

            if (leftType == Type::IntArray && rightType == Type::IntArray) {
                if (expr.left->inferredArraySize != expr.right->inferredArraySize) {
                    return fail<Type>(expr.loc,
                        "cannot compare arrays of different sizes");
                }

                expr.inferredType = Type::Bool;
                return expr.inferredType;
            }

            if (leftType == Type::Struct && rightType == Type::Struct) {
                if (expr.left->inferredStructName != expr.right->inferredStructName) {
                    return fail<Type>(expr.loc,
                        "cannot compare structs of different types");
                }

                expr.inferredType = Type::Bool;
                return expr.inferredType;
            }

            return fail<Type>(expr.loc,
                "equality operator '" + binaryOpToString(expr.op) +
                "' cannot be applied to " +
                typeToString(leftType) + " and " + typeToString(rightType));

        case BinaryOp::And:
        case BinaryOp::Or:
            if (leftType == Type::Bool && rightType == Type::Bool) {
                expr.inferredType = Type::Bool;
                return expr.inferredType;
            }

            return fail<Type>(expr.loc,
                "logical operator '" + binaryOpToString(expr.op) +
                "' cannot be applied to " +
                typeToString(leftType) + " and " + typeToString(rightType));
    }

    return fail<Type>(expr.loc, "unknown binary operator");
}

Type SemanticAnalyzer::analyzeUnaryExpr(UnaryExpr& expr) {
    Type operandType = analyzeExpr(*expr.operand);

    switch (expr.op) {
        case UnaryOp::BitNot:
            if (operandType != Type::Int && operandType != Type::UInt) {
                return fail<Type>(
                    expr.loc,
                    "bitwise not operator '~' can be applied only to int or uint"
                );
            }

            expr.inferredType = operandType;
            return expr.inferredType;


        case UnaryOp::Negate:
            if (operandType == Type::Int) {
                expr.inferredType = Type::Int;
                return expr.inferredType;
            }

            if (operandType == Type::Float) {
                expr.inferredType = Type::Float;
                return expr.inferredType;
            }

            return fail<Type>(expr.loc,
                "unary '-' cannot be applied to " + typeToString(operandType));

        case UnaryOp::Not:
            if (operandType == Type::Bool) {
                expr.inferredType = Type::Bool;
                return expr.inferredType;
            }

            return fail<Type>(expr.loc,
                "unary '!' cannot be applied to " + typeToString(operandType));
    }

    return fail<Type>(expr.loc, "unknown unary operator");
}

bool SemanticAnalyzer::sameType(Type left, Type right) const {
    return left == right;
}

} 
