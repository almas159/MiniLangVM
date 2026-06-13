#include "parser.hpp"
#include "error.hpp"
#include <memory>
#include <string>
#include <utility>
#include <vector>

namespace minilang {

static std::string encodeArrayElementTypeName(Type type, const std::string& typeName) {
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


Parser::Parser(Lexer& lexer)
    : lexer_(lexer),
      current_(TokenType::EndOfFile, "", SourceLocation()) {
    advance();
}

void Parser::advance() {
    if (diagnostic_) {
        return;
    }

    if (!bufferedTokens_.empty()) {
        current_ = bufferedTokens_.front();
        bufferedTokens_.erase(bufferedTokens_.begin());
        return;
    }

    Result<Token> tokenResult = lexer_.nextTokenExpected();

    if (!tokenResult) {
        setDiagnostic(tokenResult.error().location, tokenResult.error().message);
        return;
    }

    current_ = *tokenResult;
}


const Token& Parser::peekToken(std::size_t offset) {
    if (offset == 0) {
        return current_;
    }

    while (!diagnostic_ && bufferedTokens_.size() < offset) {
        Result<Token> tokenResult = lexer_.nextTokenExpected();

        if (!tokenResult) {
            setDiagnostic(tokenResult.error().location, tokenResult.error().message);
            break;
        }

        bufferedTokens_.push_back(*tokenResult);
    }

    if (bufferedTokens_.empty()) {
        return current_;
    }

    if (offset - 1 >= bufferedTokens_.size()) {
        return bufferedTokens_.back();
    }

    return bufferedTokens_[offset - 1];
}

static bool isExplicitGenericTypeToken(TokenType type) {
    switch (type) {
        case TokenType::KwInt:
        case TokenType::KwInt32:
        case TokenType::KwUInt:
        case TokenType::KwUInt32:
        case TokenType::KwFloat32:
        case TokenType::KwFloat64:
        case TokenType::KwBool:
        case TokenType::KwString:
        case TokenType::KwVoid:
        case TokenType::KwUnit:
        case TokenType::Identifier:
            return true;

        default:
            return false;
    }
}

bool Parser::looksLikeExplicitGenericCall() const {
    if (current_.type != TokenType::Less) {
        return false;
    }

    Parser* self = const_cast<Parser*>(this);

    std::size_t offset = 1;
    bool expectType = true;

    while (true) {
        const Token& token = self->peekToken(offset);

        if (self->diagnostic_) {
            return false;
        }

        if (expectType) {
            if (!isExplicitGenericTypeToken(token.type)) {
                return false;
            }

            expectType = false;
            offset++;
            continue;
        }

        if (token.type == TokenType::Comma) {
            expectType = true;
            offset++;
            continue;
        }

        if (token.type == TokenType::Greater) {
            const Token& afterGreater = self->peekToken(offset + 1);
            return afterGreater.type == TokenType::LParen;
        }

        return false;
    }
}


void Parser::setDiagnostic(SourceLocation location, const std::string& message) {
    if (diagnostic_) {
        return;
    }

    diagnostic_ = Diagnostic(location, message);
    current_ = Token(TokenType::EndOfFile, "", location);
}

void Parser::failVoid(SourceLocation location, const std::string& message) {
    setDiagnostic(location, message);
}

bool Parser::check(TokenType type) const {
    return current_.type == type;
}

bool Parser::match(TokenType type) {
    if (!check(type)) {
        return false;
    }

    advance();
    return true;
}

Token Parser::expect(TokenType type, const std::string& message) {
    if (!check(type)) {
        return fail<Token>(current_.location, message + ", got " + tokenToString(current_));
    }

    Token token = current_;
    advance();
    return token;
}

bool Parser::isTypeToken(TokenType type) const {
    return type == TokenType::KwInt ||
           type == TokenType::KwInt32 ||
           type == TokenType::KwUInt ||
           type == TokenType::KwUInt32 ||
           type == TokenType::KwFloat32 ||
           type == TokenType::KwFloat64 ||
           type == TokenType::KwBool ||
           type == TokenType::KwString ||
           type == TokenType::KwVoid ||
           type == TokenType::KwUnit ||
           type == TokenType::KwVar ||
           (type == TokenType::Identifier &&
            (typeAliases_.find(current_.lexeme) != typeAliases_.end() ||
             structTypeNames_.find(current_.lexeme) != structTypeNames_.end()));
}

Type Parser::parseType() {
    lastParsedArraySize_ = -1;
    lastParsedTypeName_.clear();

    Type baseType = Type::Unknown;
    std::string baseTypeName;

    if (match(TokenType::KwInt) || match(TokenType::KwInt32)) {
        baseType = Type::Int;
    } else if (match(TokenType::KwUInt) || match(TokenType::KwUInt32)) {
        baseType = Type::UInt;
    } else if (match(TokenType::KwFloat32) || match(TokenType::KwFloat64)) {
        baseType = Type::Float;
    } else if (match(TokenType::KwBool)) {
        baseType = Type::Bool;
    } else if (match(TokenType::KwString)) {
        baseType = Type::String;
    } else if (match(TokenType::KwVoid) || match(TokenType::KwUnit)) {
        baseType = Type::Void;
    } else if (match(TokenType::KwVar)) {
        baseType = Type::Unknown;
    } else if (check(TokenType::Identifier)) {
        auto aliasFound = typeAliases_.find(current_.lexeme);

        if (aliasFound != typeAliases_.end()) {
            baseType = aliasFound->second.type;
            lastParsedArraySize_ = aliasFound->second.arraySize;
            lastParsedTypeName_ = aliasFound->second.typeName;
            advance();
            return baseType;
        }

        if (structTypeNames_.find(current_.lexeme) != structTypeNames_.end()) {
            baseType = Type::Struct;
            baseTypeName = current_.lexeme;
            advance();
        } else {
            baseType = Type::Generic;
            baseTypeName = current_.lexeme;
            advance();
        }
    } else {
        return fail<Type>(current_.location, "expected type");
    }

    if (match(TokenType::LBracket)) {
        if (baseType == Type::Void || baseType == Type::Unknown ||
            baseType == Type::Generic || baseType == Type::IntArray) {
            return fail<Type>(current_.location, "invalid array element type");
        }

        Token sizeToken = expect(TokenType::IntLiteral, "expected array size");
        expect(TokenType::RBracket, "expected ']' after array size");

        if (sizeToken.intValue <= 0) {
            return fail<Type>(sizeToken.location, "array size must be positive");
        }

        lastParsedArraySize_ = sizeToken.intValue;
        lastParsedTypeName_ = encodeArrayElementTypeName(baseType, baseTypeName);
        return Type::IntArray;
    }

    if (baseType == Type::Struct || baseType == Type::Generic) {
        lastParsedTypeName_ = baseTypeName;
    }

    return baseType;
}

Result<std::unique_ptr<Program>> Parser::parseProgramExpected() {
    if (diagnostic_) {
        return std::unexpected(*diagnostic_);
    }

    std::unique_ptr<Program> program = parseProgram();

    if (diagnostic_) {
        return std::unexpected(*diagnostic_);
    }

    return program;
}

std::unique_ptr<Program> Parser::parseProgram() {
    SourceLocation loc = current_.location;

    auto program = std::make_unique<Program>(loc);

    while (!check(TokenType::EndOfFile)) {
        if (check(TokenType::KwNamespace)) {
            parseNamespaceDecl(*program);
        } else if (check(TokenType::KwStruct)) {
            program->structs.push_back(parseStructDecl());
        } else if (check(TokenType::KwType)) {
            program->aliases.push_back(parseTypeAliasDecl());
        } else {
            program->functions.push_back(parseFunctionDecl());
        }
    }

    expect(TokenType::EndOfFile, "expected end of file");

    return program;
}

void Parser::parseNamespaceDecl(Program& program) {
    expect(TokenType::KwNamespace, "expected 'namespace'");

    Token nameToken = expect(TokenType::Identifier, "expected namespace name");

    expect(TokenType::LBrace, "expected '{' after namespace name");

    std::string previousNamespace = currentNamespace_;

    if (previousNamespace.empty()) {
        currentNamespace_ = nameToken.lexeme;
    } else {
        currentNamespace_ = previousNamespace + "::" + nameToken.lexeme;
    }

    while (!check(TokenType::RBrace) && !check(TokenType::EndOfFile)) {
        if (check(TokenType::KwNamespace)) {
            parseNamespaceDecl(program);
            continue;
        }

        if (check(TokenType::KwStruct) || check(TokenType::KwType)) {
            failVoid(current_.location,
                "struct and type declarations inside namespace are not supported yet");
    return;
        }

        program.functions.push_back(parseFunctionDecl());
    }

    expect(TokenType::RBrace, "expected '}' after namespace body");

    currentNamespace_ = previousNamespace;
}

std::unique_ptr<StructDecl> Parser::parseStructDecl() {
    SourceLocation loc = current_.location;

    expect(TokenType::KwStruct, "expected 'struct'");

    Token nameToken = expect(TokenType::Identifier, "expected struct name");

    if (structTypeNames_.find(nameToken.lexeme) != structTypeNames_.end()) {
        return fail<std::unique_ptr<StructDecl>>(nameToken.location,
            "struct '" + nameToken.lexeme + "' is already declared");
    }

    structTypeNames_.insert(nameToken.lexeme);

    expect(TokenType::LBrace, "expected '{' after struct name");

    std::vector<StructFieldDecl> fields = parseStructFields();

    expect(TokenType::RBrace, "expected '}' after struct declaration");

    return std::make_unique<StructDecl>(
        nameToken.lexeme,
        std::move(fields),
        loc
    );
}

std::vector<StructFieldDecl> Parser::parseStructFields() {
    std::vector<StructFieldDecl> fields;

    while (!check(TokenType::RBrace) && !check(TokenType::EndOfFile)) {
        SourceLocation loc = current_.location;

        Type fieldType = parseType();
        std::string fieldTypeName = lastParsedTypeName_;
        int fieldArraySize = lastParsedArraySize_;

        if (fieldType == Type::Void || fieldType == Type::Unknown) {
            return fail<std::vector<StructFieldDecl>>(loc, "invalid struct field type");
        }

        Token nameToken = expect(TokenType::Identifier, "expected field name");

        expect(TokenType::Semicolon, "expected ';' after struct field");

        fields.emplace_back(
            fieldType,
            fieldTypeName,
            nameToken.lexeme,
            loc,
            fieldArraySize
        );
    }

    return fields;
}

std::unique_ptr<TypeAliasDecl> Parser::parseTypeAliasDecl() {
    SourceLocation loc = current_.location;

    expect(TokenType::KwType, "expected 'type'");

    Token nameToken = expect(TokenType::Identifier, "expected type alias name");

    if (typeAliases_.find(nameToken.lexeme) != typeAliases_.end()) {
        return fail<std::unique_ptr<TypeAliasDecl>>(nameToken.location,
            "type alias '" + nameToken.lexeme + "' is already declared");
    }

    expect(TokenType::Assign, "expected '=' after type alias name");

    Type targetType = parseType();
    int arraySize = lastParsedArraySize_;
    std::string targetTypeName = lastParsedTypeName_;

    if (targetType == Type::Unknown || targetType == Type::Void) {
        return fail<std::unique_ptr<TypeAliasDecl>>(loc, "invalid type alias target");
    }

    expect(TokenType::Semicolon, "expected ';' after type alias declaration");

    typeAliases_[nameToken.lexeme] = TypeAliasInfo{targetType, arraySize, targetTypeName};

    return std::make_unique<TypeAliasDecl>(
        nameToken.lexeme,
        targetType,
        arraySize,
        loc
    );
}

std::string Parser::parseFunctionName() {
    if (check(TokenType::Identifier)) {
        std::string name = current_.lexeme;
        advance();
        return name;
    }

    if (check(TokenType::KwMain)) {
        std::string name = current_.lexeme;
        advance();
        return name;
    }

    return fail<std::string>(current_.location, "expected function name");
}

std::unique_ptr<FunctionDecl> Parser::parseFunctionDecl() {
    SourceLocation loc = current_.location;

    Type returnType = parseType();
    std::string returnTypeName = lastParsedTypeName_;

    if (returnType == Type::Unknown) {
        return fail<std::unique_ptr<FunctionDecl>>(loc, "'var' cannot be used as function return type");
    }

    std::string name = parseFunctionName();

    std::vector<std::string> typeParameters;

    if (match(TokenType::Less)) {
        while (true) {
            Token typeParameterToken = expect(
                TokenType::Identifier,
                "expected generic type parameter name"
            );

            typeParameters.push_back(typeParameterToken.lexeme);

            if (!match(TokenType::Comma)) {
                break;
            }
        }

        expect(TokenType::Greater, "expected '>' after generic type parameters");
    }

    if (!currentNamespace_.empty()) {
        name = currentNamespace_ + "::" + name;
    }

    expect(TokenType::LParen, "expected '(' after function name");

    std::vector<Parameter> parameters = parseParameters();

    expect(TokenType::RParen, "expected ')' after function parameters");

    auto body = parseBlock();

    auto function = std::make_unique<FunctionDecl>(
        returnType,
        name,
        std::move(parameters),
        std::move(body),
        loc
    );

    function->returnTypeName = returnTypeName;
    function->typeParameters = std::move(typeParameters);

    return function;
}

std::vector<Parameter> Parser::parseParameters() {
    std::vector<Parameter> parameters;

    if (check(TokenType::RParen)) {
        return parameters;
    }

    while (true) {
        SourceLocation loc = current_.location;

        Type type = parseType();
        std::string typeName = lastParsedTypeName_;

        if (type == Type::Unknown) {
            return fail<std::vector<Parameter>>(loc, "'var' cannot be used as parameter type");
        }

        if (type == Type::Void) {
            return fail<std::vector<Parameter>>(loc, "'void' cannot be used as parameter type");
        }

        Token nameToken = expect(TokenType::Identifier, "expected parameter name");

        parameters.emplace_back(type, nameToken.lexeme, loc);
        parameters.back().typeName = typeName;

        if (!match(TokenType::Comma)) {
            break;
        }
    }

    return parameters;
}

std::unique_ptr<BlockStmt> Parser::parseBlock() {
    SourceLocation loc = current_.location;

    expect(TokenType::LBrace, "expected '{' to start block");

    auto block = std::make_unique<BlockStmt>(loc);

    while (!check(TokenType::RBrace) && !check(TokenType::EndOfFile)) {
        block->statements.push_back(parseStatement());
    }

    expect(TokenType::RBrace, "expected '}' to close block");

    return block;
}

std::unique_ptr<Stmt> Parser::parseStatement() {
    if (check(TokenType::Semicolon)) {
        SourceLocation loc = current_.location;
        advance();
        return std::make_unique<EmptyStmt>(loc);
    }

    if (check(TokenType::KwLet) || isTypeToken(current_.type)) {
        auto stmt = parseVarDeclStatement();
        expect(TokenType::Semicolon, "expected ';' after variable declaration");
        return stmt;
    }

    if (check(TokenType::Identifier)) {
        auto stmt = parseIdentifierStatement();
        expect(TokenType::Semicolon, "expected ';' after identifier statement");
        return stmt;
    }

    if (check(TokenType::KwIf)) {
        return parseIfStatement();
    }

    if (check(TokenType::KwWhile)) {
        return parseWhileStatement();
    }

    if (check(TokenType::KwFor)) {
        return parseForStatement();
    }

    if (check(TokenType::KwSwitch)) {
        return parseSwitchStatement();
    }

    if (check(TokenType::KwReturn)) {
        auto stmt = parseReturnStatement();
        expect(TokenType::Semicolon, "expected ';' after return");
        return stmt;
    }

    if (check(TokenType::KwBreak)) {
        auto stmt = parseBreakStatement();
        expect(TokenType::Semicolon, "expected ';' after break");
        return stmt;
    }

    if (check(TokenType::KwContinue)) {
        auto stmt = parseContinueStatement();
        expect(TokenType::Semicolon, "expected ';' after continue");
        return stmt;
    }

    if (check(TokenType::KwPrint)) {
        auto stmt = parsePrintStatement();
        expect(TokenType::Semicolon, "expected ';' after print");
        return stmt;
    }

    if (check(TokenType::LBrace)) {
        return parseBlock();
    }

    return fail<std::unique_ptr<Stmt>>(current_.location, "expected statement");
}

std::unique_ptr<Stmt> Parser::parseIdentifierStatement() {
    Token nameToken = expect(TokenType::Identifier, "expected identifier");

    if (check(TokenType::DoubleColon)) {
        auto expression = parseQualifiedCallExpression(nameToken);

        return std::make_unique<ExprStmt>(
            std::move(expression),
            nameToken.location
        );
    }

    if (check(TokenType::LParen)) {
        auto expression = parseCallExpression(nameToken);

        return std::make_unique<ExprStmt>(
            std::move(expression),
            nameToken.location
        );
    }

    if (match(TokenType::LBracket)) {
        auto index = parseExpression();

        expect(TokenType::RBracket, "expected ']' after array index");
        expect(TokenType::Assign, "expected '=' after array element");

        auto value = parseExpression();

        return std::make_unique<ArrayAssignStmt>(
            nameToken.lexeme,
            std::move(index),
            std::move(value),
            nameToken.location
        );
    }

    if (match(TokenType::Dot)) {
        Token fieldToken = expect(TokenType::Identifier, "expected field name after '.'");

        if (match(TokenType::LBracket)) {
            auto arrayExpr = std::make_unique<FieldAccessExpr>(
                nameToken.lexeme,
                fieldToken.lexeme,
                nameToken.location
            );

            auto index = parseExpression();

            expect(TokenType::RBracket, "expected ']' after array index");
            expect(TokenType::Assign, "expected '=' after array element");

            auto value = parseExpression();

            return std::make_unique<ArrayAssignStmt>(
                std::move(arrayExpr),
                std::move(index),
                std::move(value),
                nameToken.location
            );
        }

        expect(TokenType::Assign, "expected '=' after struct field");

        auto value = parseExpression();

        return std::make_unique<FieldAssignStmt>(
            nameToken.lexeme,
            fieldToken.lexeme,
            std::move(value),
            nameToken.location
        );
    }

    if (match(TokenType::Assign)) {
        auto value = parseExpression();

        return std::make_unique<AssignStmt>(
            nameToken.lexeme,
            std::move(value),
            nameToken.location
        );
    }

    if (match(TokenType::PlusAssign)) {
        auto right = parseExpression();

        auto left = std::make_unique<VariableExpr>(
            nameToken.lexeme,
            nameToken.location
        );

        auto value = std::make_unique<BinaryExpr>(
            BinaryOp::Add,
            std::move(left),
            std::move(right),
            nameToken.location
        );

        return std::make_unique<AssignStmt>(
            nameToken.lexeme,
            std::move(value),
            nameToken.location
        );
    }

    if (match(TokenType::MinusAssign)) {
        auto right = parseExpression();

        auto left = std::make_unique<VariableExpr>(
            nameToken.lexeme,
            nameToken.location
        );

        auto value = std::make_unique<BinaryExpr>(
            BinaryOp::Sub,
            std::move(left),
            std::move(right),
            nameToken.location
        );

        return std::make_unique<AssignStmt>(
            nameToken.lexeme,
            std::move(value),
            nameToken.location
        );
    }

    if (match(TokenType::PlusPlus)) {
        auto left = std::make_unique<VariableExpr>(
            nameToken.lexeme,
            nameToken.location
        );

        auto one = std::make_unique<IntLiteralExpr>(
            1,
            nameToken.location
        );

        auto value = std::make_unique<BinaryExpr>(
            BinaryOp::Add,
            std::move(left),
            std::move(one),
            nameToken.location
        );

        return std::make_unique<AssignStmt>(
            nameToken.lexeme,
            std::move(value),
            nameToken.location
        );
    }

    if (match(TokenType::MinusMinus)) {
        auto left = std::make_unique<VariableExpr>(
            nameToken.lexeme,
            nameToken.location
        );

        auto one = std::make_unique<IntLiteralExpr>(
            1,
            nameToken.location
        );

        auto value = std::make_unique<BinaryExpr>(
            BinaryOp::Sub,
            std::move(left),
            std::move(one),
            nameToken.location
        );

        return std::make_unique<AssignStmt>(
            nameToken.lexeme,
            std::move(value),
            nameToken.location
        );
    }

    return fail<std::unique_ptr<Stmt>>(current_.location,
        "expected assignment operator or function call after identifier");
}

std::unique_ptr<Stmt> Parser::parseVarDeclStatement() {
    SourceLocation loc = current_.location;
    bool isMutable = true;

    if (match(TokenType::KwLet)) {
        isMutable = false;

        // Форма из ТЗ:
        // let x = 10;
        // Тип выводится по initializer, как у var, но переменная immutable.
        if (check(TokenType::Identifier) && peekToken(1).type == TokenType::Assign) {
            Token nameToken = expect(TokenType::Identifier, "expected variable name");
            expect(TokenType::Assign, "expected '=' after let variable name");

            auto initializer = parseExpression();

            auto stmt = std::make_unique<VarDeclStmt>(
                Type::Unknown,
                nameToken.lexeme,
                std::move(initializer),
                loc
            );

            stmt->isMutable = false;
            return stmt;
        }
    }

    Type type = parseType();
    int arraySize = lastParsedArraySize_;
    std::string structName = lastParsedTypeName_;

    Token nameToken = expect(TokenType::Identifier, "expected variable name");

    std::unique_ptr<Expr> initializer = nullptr;

    if (match(TokenType::Assign)) {
        initializer = parseExpression();
    }

    auto stmt = std::make_unique<VarDeclStmt>(
        type,
        nameToken.lexeme,
        std::move(initializer),
        loc
    );

    stmt->arraySize = arraySize;
    stmt->structName = structName;
    stmt->isMutable = isMutable;

    return stmt;
}

std::unique_ptr<Stmt> Parser::parseAssignmentStatement() {
    Token nameToken = expect(TokenType::Identifier, "expected variable name");

    if (match(TokenType::Assign)) {
        auto value = parseExpression();

        return std::make_unique<AssignStmt>(
            nameToken.lexeme,
            std::move(value),
            nameToken.location
        );
    }

    if (match(TokenType::PlusAssign)) {
        auto right = parseExpression();

        auto left = std::make_unique<VariableExpr>(
            nameToken.lexeme,
            nameToken.location
        );

        auto value = std::make_unique<BinaryExpr>(
            BinaryOp::Add,
            std::move(left),
            std::move(right),
            nameToken.location
        );

        return std::make_unique<AssignStmt>(
            nameToken.lexeme,
            std::move(value),
            nameToken.location
        );
    }

    if (match(TokenType::MinusAssign)) {
        auto right = parseExpression();

        auto left = std::make_unique<VariableExpr>(
            nameToken.lexeme,
            nameToken.location
        );

        auto value = std::make_unique<BinaryExpr>(
            BinaryOp::Sub,
            std::move(left),
            std::move(right),
            nameToken.location
        );

        return std::make_unique<AssignStmt>(
            nameToken.lexeme,
            std::move(value),
            nameToken.location
        );
    }

    if (match(TokenType::PlusPlus)) {
        auto left = std::make_unique<VariableExpr>(
            nameToken.lexeme,
            nameToken.location
        );

        auto one = std::make_unique<IntLiteralExpr>(
            1,
            nameToken.location
        );

        auto value = std::make_unique<BinaryExpr>(
            BinaryOp::Add,
            std::move(left),
            std::move(one),
            nameToken.location
        );

        return std::make_unique<AssignStmt>(
            nameToken.lexeme,
            std::move(value),
            nameToken.location
        );
    }

    if (match(TokenType::MinusMinus)) {
        auto left = std::make_unique<VariableExpr>(
            nameToken.lexeme,
            nameToken.location
        );

        auto one = std::make_unique<IntLiteralExpr>(
            1,
            nameToken.location
        );

        auto value = std::make_unique<BinaryExpr>(
            BinaryOp::Sub,
            std::move(left),
            std::move(one),
            nameToken.location
        );

        return std::make_unique<AssignStmt>(
            nameToken.lexeme,
            std::move(value),
            nameToken.location
        );
    }

    return fail<std::unique_ptr<Stmt>>(current_.location,
        "expected assignment operator after variable name");
}

std::unique_ptr<Stmt> Parser::parseIfStatement() {
    SourceLocation loc = current_.location;

    expect(TokenType::KwIf, "expected 'if'");
    expect(TokenType::LParen, "expected '(' after 'if'");

    auto condition = parseExpression();

    expect(TokenType::RParen, "expected ')' after if condition");

    auto thenBranch = parseStatement();

    std::unique_ptr<Stmt> elseBranch = nullptr;

    if (match(TokenType::KwElse)) {
        elseBranch = parseStatement();
    }

    return std::make_unique<IfStmt>(
        std::move(condition),
        std::move(thenBranch),
        std::move(elseBranch),
        loc
    );
}

std::unique_ptr<Stmt> Parser::parseWhileStatement() {
    SourceLocation loc = current_.location;

    expect(TokenType::KwWhile, "expected 'while'");
    expect(TokenType::LParen, "expected '(' after 'while'");

    auto condition = parseExpression();

    expect(TokenType::RParen, "expected ')' after while condition");

    auto body = parseStatement();

    return std::make_unique<WhileStmt>(
        std::move(condition),
        std::move(body),
        loc
    );
}

std::unique_ptr<Stmt> Parser::parseForStatement() {
    SourceLocation loc = current_.location;

    expect(TokenType::KwFor, "expected 'for'");
    expect(TokenType::LParen, "expected '(' after 'for'");

    std::unique_ptr<Stmt> initializer = nullptr;

    if (!check(TokenType::Semicolon)) {
        initializer = parseForInitializer();
    }

    expect(TokenType::Semicolon, "expected ';' after for initializer");

    auto condition = parseExpression();

    expect(TokenType::Semicolon, "expected ';' after for condition");

    std::unique_ptr<Stmt> update = nullptr;

    if (!check(TokenType::RParen)) {
        update = parseForUpdate();
    }

    expect(TokenType::RParen, "expected ')' after for update");

    auto body = parseStatement();

    return std::make_unique<ForStmt>(
        std::move(initializer),
        std::move(condition),
        std::move(update),
        std::move(body),
        loc
    );
}

std::unique_ptr<Stmt> Parser::parseForInitializer() {
    if (check(TokenType::KwLet) || isTypeToken(current_.type)) {
        return parseVarDeclStatement();
    }

    if (check(TokenType::Identifier)) {
        return parseAssignmentStatement();
    }

    return fail<std::unique_ptr<Stmt>>(current_.location, "expected for initializer");
}

std::unique_ptr<Stmt> Parser::parseForUpdate() {
    if (check(TokenType::Identifier)) {
        return parseAssignmentStatement();
    }

    return fail<std::unique_ptr<Stmt>>(current_.location, "expected for update assignment");
}

std::unique_ptr<Stmt> Parser::parseSwitchStatement() {
    SourceLocation loc = current_.location;

    expect(TokenType::KwSwitch, "expected 'switch'");
    expect(TokenType::LParen, "expected '(' after switch");

    auto expression = parseExpression();

    expect(TokenType::RParen, "expected ')' after switch expression");
    expect(TokenType::LBrace, "expected '{' after switch condition");

    auto switchStmt = std::make_unique<SwitchStmt>(std::move(expression), loc);

    while (!check(TokenType::RBrace) && !check(TokenType::EndOfFile)) {
        if (match(TokenType::KwCase)) {
            Token valueToken = expect(TokenType::IntLiteral, "expected integer literal after case");

            expect(TokenType::Colon, "expected ':' after case value");

            SwitchCase currentCase(valueToken.intValue, valueToken.location);

            while (!check(TokenType::KwCase) &&
                   !check(TokenType::KwDefault) &&
                   !check(TokenType::RBrace) &&
                   !check(TokenType::EndOfFile)) {
                currentCase.statements.push_back(parseStatement());
            }

            switchStmt->cases.push_back(std::move(currentCase));
            continue;
        }

        if (match(TokenType::KwDefault)) {
            if (switchStmt->hasDefault) {
                return fail<std::unique_ptr<Stmt>>(current_.location, "duplicate default label in switch");
            }

            expect(TokenType::Colon, "expected ':' after default");

            switchStmt->hasDefault = true;

            while (!check(TokenType::KwCase) &&
                   !check(TokenType::KwDefault) &&
                   !check(TokenType::RBrace) &&
                   !check(TokenType::EndOfFile)) {
                switchStmt->defaultStatements.push_back(parseStatement());
            }

            if (check(TokenType::KwCase)) {
                return fail<std::unique_ptr<Stmt>>(current_.location,
                    "case after default is not supported; put default at the end");
            }

            continue;
        }

        return fail<std::unique_ptr<Stmt>>(current_.location, "expected 'case', 'default' or '}' in switch");
    }

    expect(TokenType::RBrace, "expected '}' after switch body");

    return switchStmt;
}

std::unique_ptr<Stmt> Parser::parseReturnStatement() {
    SourceLocation loc = current_.location;

    expect(TokenType::KwReturn, "expected 'return'");

    if (check(TokenType::Semicolon)) {
        return std::make_unique<ReturnStmt>(
            nullptr,
            loc
        );
    }

    auto value = parseExpression();

    return std::make_unique<ReturnStmt>(
        std::move(value),
        loc
    );
}

std::unique_ptr<Stmt> Parser::parseBreakStatement() {
    SourceLocation loc = current_.location;

    expect(TokenType::KwBreak, "expected 'break'");

    return std::make_unique<BreakStmt>(loc);
}

std::unique_ptr<Stmt> Parser::parseContinueStatement() {
    SourceLocation loc = current_.location;

    expect(TokenType::KwContinue, "expected 'continue'");

    return std::make_unique<ContinueStmt>(loc);
}

std::unique_ptr<Stmt> Parser::parsePrintStatement() {
    SourceLocation loc = current_.location;

    expect(TokenType::KwPrint, "expected 'print'");
    expect(TokenType::LParen, "expected '(' after 'print'");

    auto stmt = std::make_unique<PrintStmt>(loc);

    if (!check(TokenType::RParen)) {
        stmt->arguments.push_back(parseExpression());

        while (match(TokenType::Comma)) {
            stmt->arguments.push_back(parseExpression());
        }
    }

    expect(TokenType::RParen, "expected ')' after print arguments");

    return stmt;
}

std::unique_ptr<Expr> Parser::parseExpression() {
    return parseLogicOr();
}

std::unique_ptr<Expr> Parser::parseLogicOr() {
    auto expr = parseLogicAnd();

    while (check(TokenType::OrOr)) {
        SourceLocation loc = current_.location;
        advance();

        auto right = parseLogicAnd();

        expr = std::make_unique<BinaryExpr>(
            BinaryOp::Or,
            std::move(expr),
            std::move(right),
            loc
        );
    }

    return expr;
}

std::unique_ptr<Expr> Parser::parseLogicAnd() {
    auto expr = parseBitOr();

    while (check(TokenType::AndAnd)) {
        SourceLocation loc = current_.location;
        advance();

        auto right = parseEquality();

        expr = std::make_unique<BinaryExpr>(
            BinaryOp::And,
            std::move(expr),
            std::move(right),
            loc
        );
    }

    return expr;
}

std::unique_ptr<Expr> Parser::parseBitOr() {
    auto expr = parseBitXor();

    while (check(TokenType::BitOr)) {
        Token opToken = current_;
        advance();

        auto right = parseBitXor();

        expr = std::make_unique<BinaryExpr>(
            BinaryOp::BitOr,
            std::move(expr),
            std::move(right),
            opToken.location
        );
    }

    return expr;
}

std::unique_ptr<Expr> Parser::parseBitXor() {
    auto expr = parseBitAnd();

    while (check(TokenType::BitXor)) {
        Token opToken = current_;
        advance();

        auto right = parseBitAnd();

        expr = std::make_unique<BinaryExpr>(
            BinaryOp::BitXor,
            std::move(expr),
            std::move(right),
            opToken.location
        );
    }

    return expr;
}

std::unique_ptr<Expr> Parser::parseBitAnd() {
    auto expr = parseEquality();

    while (check(TokenType::BitAnd)) {
        Token opToken = current_;
        advance();

        auto right = parseEquality();

        expr = std::make_unique<BinaryExpr>(
            BinaryOp::BitAnd,
            std::move(expr),
            std::move(right),
            opToken.location
        );
    }

    return expr;
}

std::unique_ptr<Expr> Parser::parseEquality() {
    auto expr = parseComparison();

    while (check(TokenType::Equal) || check(TokenType::NotEqual)) {
        Token opToken = current_;
        advance();

        auto right = parseComparison();

        BinaryOp op;

        if (opToken.type == TokenType::Equal) {
            op = BinaryOp::Equal;
        } else {
            op = BinaryOp::NotEqual;
        }

        expr = std::make_unique<BinaryExpr>(
            op,
            std::move(expr),
            std::move(right),
            opToken.location
        );
    }

    return expr;
}

std::unique_ptr<Expr> Parser::parseComparison() {
    auto expr = parseShift();

    while (check(TokenType::Less) ||
           check(TokenType::Greater) ||
           check(TokenType::LessEqual) ||
           check(TokenType::GreaterEqual)) {
        Token opToken = current_;
        advance();

        auto right = parseShift();

        BinaryOp op;

        switch (opToken.type) {
            case TokenType::Less:
                op = BinaryOp::Less;
                break;

            case TokenType::Greater:
                op = BinaryOp::Greater;
                break;

            case TokenType::LessEqual:
                op = BinaryOp::LessEqual;
                break;

            case TokenType::GreaterEqual:
                op = BinaryOp::GreaterEqual;
                break;

            default:
                return fail<std::unique_ptr<Expr>>(opToken.location, "invalid comparison operator");
        }

        expr = std::make_unique<BinaryExpr>(
            op,
            std::move(expr),
            std::move(right),
            opToken.location
        );
    }

    return expr;
}

std::unique_ptr<Expr> Parser::parseShift() {
    auto expr = parseTerm();

    while (check(TokenType::ShiftLeft) || check(TokenType::ShiftRight)) {
        Token opToken = current_;
        advance();

        auto right = parseTerm();

        BinaryOp op;

        if (opToken.type == TokenType::ShiftLeft) {
            op = BinaryOp::ShiftLeft;
        } else {
            op = BinaryOp::ShiftRight;
        }

        expr = std::make_unique<BinaryExpr>(
            op,
            std::move(expr),
            std::move(right),
            opToken.location
        );
    }

    return expr;
}

std::unique_ptr<Expr> Parser::parseTerm() {
    auto expr = parseFactor();

    while (check(TokenType::Plus) || check(TokenType::Minus)) {
        Token opToken = current_;
        advance();

        auto right = parseFactor();

        BinaryOp op;

        if (opToken.type == TokenType::Plus) {
            op = BinaryOp::Add;
        } else {
            op = BinaryOp::Sub;
        }

        expr = std::make_unique<BinaryExpr>(
            op,
            std::move(expr),
            std::move(right),
            opToken.location
        );
    }

    return expr;
}

std::unique_ptr<Expr> Parser::parseFactor() {
    auto expr = parseUnary();

    while (check(TokenType::Star) ||
           check(TokenType::Slash) ||
           check(TokenType::Percent)) {
        Token opToken = current_;
        advance();

        auto right = parseUnary();

        BinaryOp op;

        switch (opToken.type) {
            case TokenType::Star:
                op = BinaryOp::Mul;
                break;

            case TokenType::Slash:
                op = BinaryOp::Div;
                break;

            case TokenType::Percent:
                op = BinaryOp::Mod;
                break;

            default:
                return fail<std::unique_ptr<Expr>>(opToken.location, "invalid factor operator");
        }

        expr = std::make_unique<BinaryExpr>(
            op,
            std::move(expr),
            std::move(right),
            opToken.location
        );
    }

    return expr;
}

std::unique_ptr<Expr> Parser::parseUnary() {
    if (check(TokenType::KwCast)) {
        return parseCastExpression();
    }

    if (check(TokenType::Bang)) {
        Token opToken = current_;
        advance();

        auto operand = parseUnary();

        return std::make_unique<UnaryExpr>(
            UnaryOp::Not,
            std::move(operand),
            opToken.location
        );
    }

    if (check(TokenType::Minus)) {
        Token opToken = current_;
        advance();

        auto operand = parseUnary();

        return std::make_unique<UnaryExpr>(
            UnaryOp::Negate,
            std::move(operand),
            opToken.location
        );
    }

    if (check(TokenType::BitNot)) {
        Token opToken = current_;
        advance();

        auto operand = parseUnary();

        return std::make_unique<UnaryExpr>(
            UnaryOp::BitNot,
            std::move(operand),
            opToken.location
        );
    }

    return parsePrimary();
}

std::unique_ptr<Expr> Parser::parseQualifiedCallExpression(Token namespaceToken) {
    std::string qualifiedName = namespaceToken.lexeme;

    while (match(TokenType::DoubleColon)) {
        Token memberToken = expect(TokenType::Identifier, "expected name after '::'");
        qualifiedName += "::" + memberToken.lexeme;
    }

    Token qualifiedToken = namespaceToken;
    qualifiedToken.lexeme = qualifiedName;

    if (!looksLikeExplicitGenericCall() && !check(TokenType::LParen)) {
        return fail<std::unique_ptr<Expr>>(current_.location,
            "expected '(' after qualified function name");
    }

    return parseCallExpression(qualifiedToken);
}

std::unique_ptr<Expr> Parser::parseCallExpression(Token nameToken) {
    std::vector<Type> typeArguments;
    std::vector<std::string> typeArgumentNames;

    if (match(TokenType::Less)) {
        while (true) {
            Type typeArgument = parseType();
            typeArguments.push_back(typeArgument);
            typeArgumentNames.push_back(lastParsedTypeName_);

            if (!match(TokenType::Comma)) {
                break;
            }
        }

        expect(TokenType::Greater, "expected '>' after generic type arguments");
    }

    expect(TokenType::LParen, "expected '(' in function call");

    std::vector<std::unique_ptr<Expr>> arguments;

    if (!check(TokenType::RParen)) {
        arguments.push_back(parseExpression());

        while (match(TokenType::Comma)) {
            arguments.push_back(parseExpression());
        }
    }

    expect(TokenType::RParen, "expected ')' after function call arguments");

    auto call = std::make_unique<CallExpr>(
        nameToken.lexeme,
        std::move(arguments),
        nameToken.location
    );

    call->typeArguments = std::move(typeArguments);
    call->typeArgumentNames = std::move(typeArgumentNames);

    return call;
}

std::unique_ptr<Expr> Parser::parseCastExpression() {
    SourceLocation loc = current_.location;

    expect(TokenType::KwCast, "expected 'cast'");
    expect(TokenType::Less, "expected '<' after cast");

    Type targetType = parseType();
    int targetArraySize = lastParsedArraySize_;

    expect(TokenType::Greater, "expected '>' after cast target type");
    expect(TokenType::LParen, "expected '(' after cast target type");

    auto expression = parseExpression();

    expect(TokenType::RParen, "expected ')' after cast expression");

    return std::make_unique<CastExpr>(
        targetType,
        targetArraySize,
        std::move(expression),
        loc
    );
}

std::unique_ptr<Expr> Parser::parseStructLiteral(Token nameToken) {
    expect(TokenType::LBrace, "expected '{' after struct type name");

    auto expr = std::make_unique<StructLiteralExpr>(
        nameToken.lexeme,
        nameToken.location
    );

    if (!check(TokenType::RBrace)) {
        while (true) {
            Token fieldToken = expect(TokenType::Identifier, "expected struct field name");

            expect(TokenType::Colon, "expected ':' after struct field name");

            auto value = parseExpression();

            expr->fields.emplace_back(
                fieldToken.lexeme,
                std::move(value),
                fieldToken.location
            );

            if (!match(TokenType::Comma)) {
                break;
            }
        }
    }

    expect(TokenType::RBrace, "expected '}' after struct literal");

    return expr;
}

std::unique_ptr<Expr> Parser::parsePrimary() {
    if (check(TokenType::LBracket)) {
        SourceLocation loc = current_.location;
        advance();

        auto array = std::make_unique<ArrayLiteralExpr>(loc);

        if (!check(TokenType::RBracket)) {
            array->elements.push_back(parseExpression());

            while (match(TokenType::Comma)) {
                array->elements.push_back(parseExpression());
            }
        }

        expect(TokenType::RBracket, "expected ']' after array literal");

        return array;
    }

    if (check(TokenType::IntLiteral)) {
        Token token = current_;
        advance();

        return std::make_unique<IntLiteralExpr>(
            token.intValue,
            token.location
        );
    }

    if (check(TokenType::FloatLiteral)) {
        Token token = current_;
        advance();

        return std::make_unique<FloatLiteralExpr>(
            token.floatValue,
            token.location
        );
    }

    if (check(TokenType::StringLiteral)) {
        Token token = current_;
        advance();

        return std::make_unique<StringLiteralExpr>(
            token.stringValue,
            token.location
        );
    }

    if (check(TokenType::KwTrue)) {
        Token token = current_;
        advance();

        return std::make_unique<BoolLiteralExpr>(
            true,
            token.location
        );
    }

    if (check(TokenType::KwFalse)) {
        Token token = current_;
        advance();

        return std::make_unique<BoolLiteralExpr>(
            false,
            token.location
        );
    }

    if (check(TokenType::Identifier)) {
        Token token = current_;
        advance();

        if (structTypeNames_.find(token.lexeme) != structTypeNames_.end() &&
            check(TokenType::LBrace)) {
            return parseStructLiteral(token);
        }

        if (check(TokenType::DoubleColon)) {
            return parseQualifiedCallExpression(token);
        }

        if (looksLikeExplicitGenericCall()) {
            return parseCallExpression(token);
        }

        if (check(TokenType::LParen)) {
            return parseCallExpression(token);
        }

        if (match(TokenType::LBracket)) {
            auto index = parseExpression();

            expect(TokenType::RBracket, "expected ']' after array index");

            return std::make_unique<IndexExpr>(
                token.lexeme,
                std::move(index),
                token.location
            );
        }

        if (match(TokenType::Dot)) {
            Token fieldToken = expect(TokenType::Identifier, "expected field name after '.'");

            auto fieldExpr = std::make_unique<FieldAccessExpr>(
                token.lexeme,
                fieldToken.lexeme,
                token.location
            );

            if (match(TokenType::LBracket)) {
                auto index = parseExpression();

                expect(TokenType::RBracket, "expected ']' after array index");

                return std::make_unique<IndexExpr>(
                    std::move(fieldExpr),
                    std::move(index),
                    token.location
                );
            }

            return fieldExpr;
        }

        return std::make_unique<VariableExpr>(
            token.lexeme,
            token.location
        );
    }

    if (match(TokenType::LParen)) {
        auto expr = parseExpression();

        expect(TokenType::RParen, "expected ')' after expression");

        return expr;
    }

    return fail<std::unique_ptr<Expr>>(current_.location, "expected expression");
}

} 
