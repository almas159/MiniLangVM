#include "parser.hpp"
#include "error.hpp"
#include <memory>
#include <string>

Parser::Parser(Lexer& lexer)
    : lexer_(lexer),
      current_(lexer_.nextToken()) {}

void Parser::advance() {
    current_ = lexer_.nextToken();
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
        throw SyntaxError(current_.location, message + ", got " + tokenToString(current_));
    }

    Token token = current_;
    advance();
    return token;
}

bool Parser::isTypeToken(TokenType type) const {
    return type == TokenType::KwInt ||
           type == TokenType::KwBool ||
           type == TokenType::KwString;
}

Type Parser::parseType() {
    if (match(TokenType::KwInt)) {
        return Type::Int;
    }

    if (match(TokenType::KwBool)) {
        return Type::Bool;
    }

    if (match(TokenType::KwString)) {
        return Type::String;
    }

    throw SyntaxError(current_.location, "expected type");
}

std::unique_ptr<Program> Parser::parseProgram() {
    SourceLocation loc = current_.location;

    auto mainFunction = parseMainFunction();

    expect(TokenType::EndOfFile, "expected end of file");

    return std::make_unique<Program>(std::move(mainFunction), loc);
}

std::unique_ptr<MainFunction> Parser::parseMainFunction() {
    SourceLocation loc = current_.location;

    expect(TokenType::KwInt, "expected 'int' before main function");
    expect(TokenType::KwMain, "expected 'main'");
    expect(TokenType::LParen, "expected '(' after main");
    expect(TokenType::RParen, "expected ')' after '('");

    auto body = parseBlock();

    return std::make_unique<MainFunction>(std::move(body), loc);
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
    if (isTypeToken(current_.type)) {
        auto stmt = parseVarDeclStatement();
        expect(TokenType::Semicolon, "expected ';' after variable declaration");
        return stmt;
    }

    if (check(TokenType::Identifier)) {
        auto stmt = parseAssignmentStatement();
        expect(TokenType::Semicolon, "expected ';' after assignment");
        return stmt;
    }

    if (check(TokenType::KwIf)) {
        return parseIfStatement();
    }

    if (check(TokenType::KwWhile)) {
        return parseWhileStatement();
    }

    if (check(TokenType::KwReturn)) {
        auto stmt = parseReturnStatement();
        expect(TokenType::Semicolon, "expected ';' after return");
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

    throw SyntaxError(current_.location, "expected statement");
}

std::unique_ptr<Stmt> Parser::parseVarDeclStatement() {
    SourceLocation loc = current_.location;

    Type type = parseType();

    Token nameToken = expect(TokenType::Identifier, "expected variable name");

    std::unique_ptr<Expr> initializer = nullptr;

    if (match(TokenType::Assign)) {
        initializer = parseExpression();
    }

    return std::make_unique<VarDeclStmt>(
        type,
        nameToken.lexeme,
        std::move(initializer),
        loc
    );
}

std::unique_ptr<Stmt> Parser::parseAssignmentStatement() {
    Token nameToken = expect(TokenType::Identifier, "expected variable name");

    expect(TokenType::Assign, "expected '=' after variable name");

    auto value = parseExpression();

    return std::make_unique<AssignStmt>(
        nameToken.lexeme,
        std::move(value),
        nameToken.location
    );
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

std::unique_ptr<Stmt> Parser::parseReturnStatement() {
    SourceLocation loc = current_.location;

    expect(TokenType::KwReturn, "expected 'return'");

    auto value = parseExpression();

    return std::make_unique<ReturnStmt>(
        std::move(value),
        loc
    );
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
    auto expr = parseEquality();

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
    auto expr = parseTerm();

    while (check(TokenType::Less) ||
           check(TokenType::Greater) ||
           check(TokenType::LessEqual) ||
           check(TokenType::GreaterEqual)) {
        Token opToken = current_;
        advance();

        auto right = parseTerm();

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
                throw SyntaxError(opToken.location, "invalid comparison operator");
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
                throw SyntaxError(opToken.location, "invalid factor operator");
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

    return parsePrimary();
}

std::unique_ptr<Expr> Parser::parsePrimary() {
    if (check(TokenType::IntLiteral)) {
        Token token = current_;
        advance();

        return std::make_unique<IntLiteralExpr>(
            token.intValue,
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

    throw SyntaxError(current_.location, "expected expression");
}
