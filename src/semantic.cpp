#include "semantic.hpp"
#include "error.hpp"
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace minilang {

static Type arrayElementTypeFromEncodedName(const std::string& encoded) {
    if (encoded.empty()) {
        return Type::Int;
    }

    if (encoded == "__array:int:") {
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

static std::string arrayElementStructNameFromEncodedName(const std::string& encoded) {
    const std::string prefix = "__array:struct:";

    if (encoded.rfind(prefix, 0) == 0) {
        return encoded.substr(prefix.size());
    }

    return "";
}

static std::string encodeArrayElementTypeNameSemantic(Type type, const std::string& typeName) {
    switch (type) {
        case Type::Int:
            return "__array:int:";

        case Type::UInt:
            return "__array:uint:";

        case Type::Float:
            return "__array:float64:";

        case Type::Bool:
            return "__array:bool:";

        case Type::String:
            return "__array:string:";

        case Type::Struct:
            return "__array:struct:" + typeName;

        default:
            return "__array:unknown:";
    }
}

static std::string arrayElementTypeToString(const std::string& encoded) {
    Type type = arrayElementTypeFromEncodedName(encoded);

    if (type == Type::Struct) {
        return arrayElementStructNameFromEncodedName(encoded);
    }

    return typeToString(type);
}

static bool sameConcreteType(Type expectedType,
                             const std::string& expectedName,
                             Type actualType,
                             const std::string& actualName) {
    if (expectedType != actualType) {
        return false;
    }

    if (expectedType == Type::Struct || expectedType == Type::Generic ||
        expectedType == Type::IntArray) {
        return expectedName == actualName;
    }

    return true;
}


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

    if (mainIt->second.size() != 1) {
        failVoid(mainIt->second.front().location, "main function cannot be overloaded");
        return;
    }

    const FunctionSymbol& mainFunction = mainIt->second.front();

    if (mainFunction.returnType != Type::Int) {
        failVoid(mainFunction.location, "main must return int");
        return;
    }

    if (!mainFunction.parameterTypes.empty()) {
        failVoid(mainFunction.location, "main must not have parameters");
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
                if (field.arraySize <= 0) {
                    failVoid(field.loc, "array field size must be positive");
                    return;
                }

                Type elementType = arrayElementTypeFromEncodedName(field.typeName);
                std::string elementStructName =
                    arrayElementStructNameFromEncodedName(field.typeName);

                if (elementType == Type::Unknown) {
                    failVoid(field.loc, "unknown array field element type");
                    return;
                }

                if (elementType == Type::Struct &&
                    structs_.find(elementStructName) == structs_.end() &&
                    elementStructName != structDecl->name) {
                    failVoid(field.loc,
                        "unknown struct type '" + elementStructName + "'");
                    return;
                }
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
            fieldSymbol.arraySize = field.arraySize;

            symbol.fields.push_back(fieldSymbol);
        }

        structs_[structDecl->name] = symbol;
    }
}

void SemanticAnalyzer::collectFunctionDeclarations(Program& program) {
    int index = 0;

    auto typeNameMatters = [](Type type) {
        return type == Type::Struct ||
               type == Type::Generic ||
               type == Type::IntArray;
    };

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

        auto& overloads = functions_[function->name];

        for (const FunctionSymbol& existing : overloads) {
            if (existing.parameterTypes.size() != symbol.parameterTypes.size()) {
                continue;
            }

            bool sameSignature = true;

            for (std::size_t i = 0; i < symbol.parameterTypes.size(); ++i) {
                if (existing.parameterTypes[i] != symbol.parameterTypes[i]) {
                    sameSignature = false;
                    break;
                }

                if (typeNameMatters(symbol.parameterTypes[i]) &&
                    existing.parameterTypeNames[i] != symbol.parameterTypeNames[i]) {
                    sameSignature = false;
                    break;
                }
            }

            if (sameSignature) {
                failVoid(function->loc,
                    "function '" + function->name +
                    "' with the same parameter types is already declared");
                return;
            }
        }

        function->functionIndex = index;
        overloads.push_back(symbol);

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

        Type expectedElementType = arrayElementTypeFromEncodedName(stmt.structName);
        std::string expectedElementName = arrayElementStructNameFromEncodedName(stmt.structName);

        if (expectedElementType == Type::Unknown) {
            failVoid(stmt.loc, "unknown array element type");
            return;
        }

        if (!stmt.initializer) {
            if (expectedElementType != Type::Int) {
                failVoid(stmt.loc,
                    "array variable '" + stmt.name +
                    "' requires initializer for element type " +
                    arrayElementTypeToString(stmt.structName));
                return;
            }

            int localIndex = nextLocalIndex_++;
            stmt.localIndex = localIndex;
            declareVariable(stmt.name, stmt.type, localIndex, stmt.loc,
                            stmt.arraySize, stmt.structName, stmt.isMutable);
            return;
        }

        auto* arrayLiteral = dynamic_cast<ArrayLiteralExpr*>(stmt.initializer.get());

        if (!arrayLiteral) {
            failVoid(stmt.initializer->loc,
                "array variable '" + stmt.name + "' requires array literal initializer");
            return;
        }

        if (static_cast<int>(arrayLiteral->elements.size()) != stmt.arraySize) {
            failVoid(stmt.initializer->loc,
                "array initializer size mismatch for '" + stmt.name +
                "': expected " + std::to_string(stmt.arraySize) +
                ", got " + std::to_string(arrayLiteral->elements.size()));
            return;
        }

        for (auto& element : arrayLiteral->elements) {
            Type actualType = analyzeExpr(*element);
            std::string actualName;

            if (actualType == Type::Struct || actualType == Type::Generic) {
                actualName = element->inferredStructName;
            }

            if (expectedElementType == Type::UInt && actualType == Type::Int) {
                auto* literal = dynamic_cast<IntLiteralExpr*>(element.get());

                if (literal && literal->value >= 0) {
                    element->inferredType = Type::UInt;
                    actualType = Type::UInt;
                }
            }

            if (!sameConcreteType(expectedElementType, expectedElementName,
                                  actualType, actualName)) {
                failVoid(element->loc,
                    "array element must be " +
                    arrayElementTypeToString(stmt.structName) +
                    ", got " + typeToString(actualType));
                return;
            }
        }

        arrayLiteral->inferredType = Type::IntArray;
        arrayLiteral->inferredArraySize = stmt.arraySize;
        arrayLiteral->inferredStructName = stmt.structName;

        int localIndex = nextLocalIndex_++;
        stmt.localIndex = localIndex;

        declareVariable(stmt.name, stmt.type, localIndex, stmt.loc,
                        stmt.arraySize, stmt.structName, stmt.isMutable);
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
            stmt.structName = stmt.initializer->inferredStructName;
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
    Type expectedElementType = Type::Unknown;
    std::string expectedElementName;
    std::string encodedElementType;

    if (stmt.arrayExpr) {
        if (auto* fieldAccess = dynamic_cast<FieldAccessExpr*>(stmt.arrayExpr.get())) {
            Symbol* owner = findVariable(fieldAccess->objectName);

            if (!owner) {
                failVoid(stmt.loc,
                    "variable '" + fieldAccess->objectName + "' is not declared");
                return;
            }

            if (!owner->isMutable) {
                failVoid(stmt.loc,
                    "cannot modify immutable struct '" + fieldAccess->objectName + "'");
                return;
            }
        }

        Type arrayType = analyzeExpr(*stmt.arrayExpr);

        if (arrayType != Type::IntArray) {
            failVoid(stmt.arrayExpr->loc,
                "index assignment target must be array, got " + typeToString(arrayType));
            return;
        }

        encodedElementType = stmt.arrayExpr->inferredStructName;
    } else {
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

        stmt.localIndex = symbol->localIndex;
        encodedElementType = symbol->structName;
    }

    Type indexType = analyzeExpr(*stmt.index);

    if (indexType != Type::Int) {
        failVoid(stmt.index->loc,
            "array index must be int, got " + typeToString(indexType));
        return;
    }

    expectedElementType = arrayElementTypeFromEncodedName(encodedElementType);
    expectedElementName = arrayElementStructNameFromEncodedName(encodedElementType);

    Type valueType = analyzeExpr(*stmt.value);
    std::string valueName;

    if (valueType == Type::Struct || valueType == Type::Generic) {
        valueName = stmt.value->inferredStructName;
    }

    if (expectedElementType == Type::UInt && valueType == Type::Int) {
        auto* literal = dynamic_cast<IntLiteralExpr*>(stmt.value.get());

        if (literal && literal->value >= 0) {
            stmt.value->inferredType = Type::UInt;
            valueType = Type::UInt;
        }
    }

    if (!sameConcreteType(expectedElementType, expectedElementName,
                          valueType, valueName)) {
        failVoid(stmt.value->loc,
            "array element must be " +
            arrayElementTypeToString(encodedElementType) +
            ", got " + typeToString(valueType));
        return;
    }
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

    if (fieldSymbol->type == Type::IntArray) {
        auto* arrayLiteral = dynamic_cast<ArrayLiteralExpr*>(stmt.value.get());

        if (arrayLiteral) {
            if (static_cast<int>(arrayLiteral->elements.size()) != fieldSymbol->arraySize) {
                failVoid(stmt.value->loc,
                    "field '" + stmt.fieldName +
                    "' array size mismatch: expected " +
                    std::to_string(fieldSymbol->arraySize) +
                    ", got " +
                    std::to_string(arrayLiteral->elements.size()));
                return;
            }

            Type expectedElementType =
                arrayElementTypeFromEncodedName(fieldSymbol->typeName);
            std::string expectedElementName =
                arrayElementStructNameFromEncodedName(fieldSymbol->typeName);

            for (auto& element : arrayLiteral->elements) {
                Type actualType = analyzeExpr(*element);
                std::string actualName;

                if (actualType == Type::Struct || actualType == Type::Generic) {
                    actualName = element->inferredStructName;
                }

                if (expectedElementType == Type::UInt && actualType == Type::Int) {
                    auto* literal = dynamic_cast<IntLiteralExpr*>(element.get());

                    if (literal && literal->value >= 0) {
                        element->inferredType = Type::UInt;
                        actualType = Type::UInt;
                    }
                }

                if (!sameConcreteType(expectedElementType, expectedElementName,
                                      actualType, actualName)) {
                    failVoid(element->loc,
                        "field '" + stmt.fieldName +
                        "' array element must be " +
                        arrayElementTypeToString(fieldSymbol->typeName) +
                        ", got " + typeToString(actualType));
                    return;
                }
            }

            arrayLiteral->inferredType = Type::IntArray;
            arrayLiteral->inferredArraySize = fieldSymbol->arraySize;
            arrayLiteral->inferredStructName = fieldSymbol->typeName;
        } else {
            Type valueType = analyzeExpr(*stmt.value);

            if (valueType != Type::IntArray) {
                failVoid(stmt.value->loc,
                    "field '" + stmt.fieldName +
                    "' must be array, got " + typeToString(valueType));
                return;
            }

            if (stmt.value->inferredArraySize != fieldSymbol->arraySize) {
                failVoid(stmt.value->loc,
                    "field '" + stmt.fieldName +
                    "' array size mismatch: expected " +
                    std::to_string(fieldSymbol->arraySize) +
                    ", got " +
                    std::to_string(stmt.value->inferredArraySize));
                return;
            }

            if (stmt.value->inferredStructName != fieldSymbol->typeName) {
                failVoid(stmt.value->loc,
                    "field '" + stmt.fieldName +
                    "' array element type mismatch");
                return;
            }
        }

        stmt.localIndex = symbol->localIndex;
        stmt.fieldIndex = fieldIndex;
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

    Type firstType = analyzeExpr(*expr.elements[0]);
    std::string firstName;

    if (firstType == Type::Struct || firstType == Type::Generic) {
        firstName = expr.elements[0]->inferredStructName;
    }

    if (firstType == Type::Void || firstType == Type::Unknown ||
        firstType == Type::IntArray) {
        return fail<Type>(expr.elements[0]->loc,
            "invalid array element type " + typeToString(firstType));
    }

    for (std::size_t i = 1; i < expr.elements.size(); ++i) {
        Type elementType = analyzeExpr(*expr.elements[i]);
        std::string elementName;

        if (elementType == Type::Struct || elementType == Type::Generic) {
            elementName = expr.elements[i]->inferredStructName;
        }

        if (!sameConcreteType(firstType, firstName, elementType, elementName)) {
            return fail<Type>(expr.elements[i]->loc,
                "array literal has mixed element types: expected " +
                typeToString(firstType) + ", got " + typeToString(elementType));
        }
    }

    expr.inferredType = Type::IntArray;
    expr.inferredArraySize = static_cast<int>(expr.elements.size());
    expr.inferredStructName = encodeArrayElementTypeNameSemantic(firstType, firstName);

    return expr.inferredType;
}

Type SemanticAnalyzer::analyzeIndexExpr(IndexExpr& expr) {
    std::string encodedElementType;

    if (expr.arrayExpr) {
        Type arrayType = analyzeExpr(*expr.arrayExpr);

        if (arrayType != Type::IntArray) {
            return fail<Type>(expr.arrayExpr->loc,
                "indexing target must be array, got " + typeToString(arrayType));
        }

        encodedElementType = expr.arrayExpr->inferredStructName;
        expr.inferredArraySize = expr.arrayExpr->inferredArraySize;
    } else {
        Symbol* symbol = findVariable(expr.arrayName);

        if (!symbol) {
            return fail<Type>(expr.loc,
                "array '" + expr.arrayName + "' is not declared");
        }

        if (symbol->type != Type::IntArray) {
            return fail<Type>(expr.loc,
                "variable '" + expr.arrayName + "' is not an array");
        }

        expr.localIndex = symbol->localIndex;
        expr.inferredArraySize = symbol->arraySize;
        encodedElementType = symbol->structName;
    }

    Type indexType = analyzeExpr(*expr.index);

    if (indexType != Type::Int) {
        return fail<Type>(expr.index->loc,
            "array index must be int, got " + typeToString(indexType));
    }

    Type elementType = arrayElementTypeFromEncodedName(encodedElementType);

    if (elementType == Type::Unknown) {
        return fail<Type>(expr.loc, "unknown array element type");
    }

    expr.inferredType = elementType;

    if (elementType == Type::Struct) {
        expr.inferredStructName =
            arrayElementStructNameFromEncodedName(encodedElementType);
    }

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

        if (expectedField.type == Type::IntArray) {
            auto* arrayLiteral =
                dynamic_cast<ArrayLiteralExpr*>(literalField.value.get());

            if (!arrayLiteral) {
                Type valueType = analyzeExpr(*literalField.value);

                if (valueType != Type::IntArray) {
                    return fail<Type>(literalField.value->loc,
                        "field '" + literalField.name +
                        "' must be array, got " + typeToString(valueType));
                }

                if (literalField.value->inferredArraySize != expectedField.arraySize) {
                    return fail<Type>(literalField.value->loc,
                        "field '" + literalField.name +
                        "' array size mismatch: expected " +
                        std::to_string(expectedField.arraySize) +
                        ", got " +
                        std::to_string(literalField.value->inferredArraySize));
                }

                if (literalField.value->inferredStructName != expectedField.typeName) {
                    return fail<Type>(literalField.value->loc,
                        "field '" + literalField.name +
                        "' array element type mismatch");
                }

                continue;
            }

            if (static_cast<int>(arrayLiteral->elements.size()) != expectedField.arraySize) {
                return fail<Type>(literalField.value->loc,
                    "field '" + literalField.name +
                    "' array size mismatch: expected " +
                    std::to_string(expectedField.arraySize) +
                    ", got " + std::to_string(arrayLiteral->elements.size()));
            }

            Type expectedElementType =
                arrayElementTypeFromEncodedName(expectedField.typeName);
            std::string expectedElementName =
                arrayElementStructNameFromEncodedName(expectedField.typeName);

            for (auto& element : arrayLiteral->elements) {
                Type actualType = analyzeExpr(*element);
                std::string actualName;

                if (actualType == Type::Struct || actualType == Type::Generic) {
                    actualName = element->inferredStructName;
                }

                if (expectedElementType == Type::UInt && actualType == Type::Int) {
                    auto* literal = dynamic_cast<IntLiteralExpr*>(element.get());

                    if (literal && literal->value >= 0) {
                        element->inferredType = Type::UInt;
                        actualType = Type::UInt;
                    }
                }

                if (!sameConcreteType(expectedElementType, expectedElementName,
                                      actualType, actualName)) {
                    return fail<Type>(element->loc,
                        "field '" + literalField.name +
                        "' array element must be " +
                        arrayElementTypeToString(expectedField.typeName) +
                        ", got " + typeToString(actualType));
                }
            }

            arrayLiteral->inferredType = Type::IntArray;
            arrayLiteral->inferredArraySize = expectedField.arraySize;
            arrayLiteral->inferredStructName = expectedField.typeName;
            continue;
        }

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

            if (field.type == Type::IntArray) {
                expr.inferredArraySize = field.arraySize;
                expr.inferredStructName = field.typeName;
            }

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
        expr.inferredStructName = symbol->structName;
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
                "len argument must be string or array, got " +
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

    std::vector<Type> argumentTypes;
    std::vector<std::string> argumentTypeNames;

    argumentTypes.reserve(expr.arguments.size());
    argumentTypeNames.reserve(expr.arguments.size());

    for (auto& argument : expr.arguments) {
        Type argumentType = analyzeExpr(*argument);
        argumentTypes.push_back(argumentType);

        if (argumentType == Type::Struct ||
            argumentType == Type::Generic ||
            argumentType == Type::IntArray) {
            argumentTypeNames.push_back(argument->inferredStructName);
        } else {
            argumentTypeNames.push_back("");
        }
    }

    struct GenericBinding {
        Type type = Type::Unknown;
        std::string typeName;
    };

    struct CandidateMatch {
        FunctionSymbol* function = nullptr;
        std::unordered_map<std::string, GenericBinding> genericBindings;
        std::vector<Type> targetTypes;
        std::vector<std::string> targetTypeNames;
        Type returnType = Type::Unknown;
        std::string returnTypeName;
        int score = 0;
    };

    auto typeNameMatters = [](Type type) {
        return type == Type::Struct ||
               type == Type::Generic ||
               type == Type::IntArray;
    };

    auto sameConcreteType =
        [&](Type leftType, const std::string& leftName,
            Type rightType, const std::string& rightName) {
            if (!sameType(leftType, rightType)) {
                return false;
            }

            if (typeNameMatters(leftType) || typeNameMatters(rightType)) {
                return leftName == rightName;
            }

            return true;
        };

    auto tryCandidate =
        [&](FunctionSymbol& function, bool allowImplicit)
            -> std::optional<CandidateMatch> {
            if (expr.arguments.size() != function.parameterTypes.size()) {
                return std::nullopt;
            }

            CandidateMatch match;
            match.function = &function;
            match.score = function.typeParameters.empty() ? 0 : 1;

            if (!expr.typeArguments.empty()) {
                if (function.typeParameters.empty()) {
                    return std::nullopt;
                }

                if (expr.typeArguments.size() != function.typeParameters.size()) {
                    return std::nullopt;
                }

                for (std::size_t i = 0; i < expr.typeArguments.size(); ++i) {
                    Type explicitType = expr.typeArguments[i];
                    std::string explicitTypeName = expr.typeArgumentNames[i];

                    if (explicitType == Type::Void ||
                        explicitType == Type::Unknown ||
                        explicitType == Type::Generic) {
                        return std::nullopt;
                    }

                    match.genericBindings[function.typeParameters[i]] =
                        GenericBinding{explicitType, explicitTypeName};
                }
            }

            match.targetTypes.resize(argumentTypes.size(), Type::Unknown);
            match.targetTypeNames.resize(argumentTypes.size());

            for (std::size_t i = 0; i < argumentTypes.size(); ++i) {
                Type argumentType = argumentTypes[i];
                const std::string& argumentTypeName = argumentTypeNames[i];

                Type parameterType = function.parameterTypes[i];
                std::string parameterTypeName = function.parameterTypeNames[i];

                if (parameterType == Type::Generic) {
                    const std::string& genericName = parameterTypeName;

                    auto existing = match.genericBindings.find(genericName);

                    if (existing == match.genericBindings.end()) {
                        match.genericBindings[genericName] =
                            GenericBinding{argumentType, argumentTypeName};

                        parameterType = argumentType;
                        parameterTypeName = argumentTypeName;
                    } else {
                        parameterType = existing->second.type;
                        parameterTypeName = existing->second.typeName;
                    }
                }

                if (sameConcreteType(parameterType,
                                     parameterTypeName,
                                     argumentType,
                                     argumentTypeName)) {
                    match.targetTypes[i] = parameterType;
                    match.targetTypeNames[i] = parameterTypeName;
                    continue;
                }

                if (allowImplicit &&
                    !typeNameMatters(parameterType) &&
                    canImplicitlyConvert(argumentType, parameterType)) {
                    match.targetTypes[i] = parameterType;
                    match.targetTypeNames[i] = parameterTypeName;
                    match.score += 10;
                    continue;
                }

                return std::nullopt;
            }

            if (function.returnType == Type::Generic) {
                auto binding = match.genericBindings.find(function.returnTypeName);

                if (binding == match.genericBindings.end()) {
                    return std::nullopt;
                }

                match.returnType = binding->second.type;
                match.returnTypeName = binding->second.typeName;
            } else {
                match.returnType = function.returnType;
                match.returnTypeName = function.returnTypeName;
            }

            return match;
        };

    std::optional<CandidateMatch> best;
    bool ambiguous = false;

    auto chooseCandidate = [&](bool allowImplicit) {
        best.reset();
        ambiguous = false;

        for (FunctionSymbol& function : found->second) {
            auto current = tryCandidate(function, allowImplicit);

            if (!current) {
                continue;
            }

            if (!best || current->score < best->score) {
                best = current;
                ambiguous = false;
                continue;
            }

            if (current->score == best->score) {
                ambiguous = true;
            }
        }
    };

    chooseCandidate(false);

    if (!best) {
        chooseCandidate(true);
    }

    if (!best) {
        return fail<Type>(expr.loc,
            "no matching overload for function '" + expr.callee + "'");
    }

    if (ambiguous) {
        return fail<Type>(expr.loc,
            "ambiguous overload call to function '" + expr.callee + "'");
    }

    expr.functionIndex = best->function->functionIndex;
    expr.argumentTargetTypes = best->targetTypes;
    expr.argumentTargetTypeNames = best->targetTypeNames;

    expr.inferredType = best->returnType;

    if (expr.inferredType == Type::Struct ||
        expr.inferredType == Type::Generic ||
        expr.inferredType == Type::IntArray) {
        expr.inferredStructName = best->returnTypeName;
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


int minilang::SemanticAnalyzer::implicitConversionCost(minilang::Type from, minilang::Type to) const {
    if (from == to) {
        return 0;
    }

    auto isNumeric = [](minilang::Type t) {
        return t == minilang::Type::Int ||
               t == minilang::Type::UInt ||
               t == minilang::Type::Float;
    };

    /*
     * A.3.1:
     * Неявные преобразования используются только при выборе перегрузки.
     * Обычные бинарные операции остаются строгими.
     */
    if (isNumeric(from) && isNumeric(to)) {
        return 1;
    }

    return -1;
}

bool minilang::SemanticAnalyzer::canImplicitlyConvert(minilang::Type from, minilang::Type to) const {
    return implicitConversionCost(from, to) >= 0;
}

