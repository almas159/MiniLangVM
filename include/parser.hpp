#pragma once
#include "ast.hpp"
#include "lexer.hpp"
#include "token.hpp"
#include <memory>

class Parser {
public:
    explicit Parser(Lexer& lexer);

    std::unique_ptr<Program> parseProgram();

private:
    Lexer& lexer_;
    Token current_;

    void advance();
    bool check(TokenType type) const;
    bool match(TokenType type);

    Token expect(TokenType type, const std::string& message);

    bool isTypeToken(TokenType type) const;
    Type parseType();

    std::unique_ptr<MainFunction> parseMainFunction();
    std::unique_ptr<BlockStmt> parseBlock();

    std::unique_ptr<Stmt> parseStatement();
    std::unique_ptr<Stmt> parseVarDeclStatement();
    std::unique_ptr<Stmt> parseAssignmentStatement();
    std::unique_ptr<Stmt> parseIfStatement();
    std::unique_ptr<Stmt> parseWhileStatement();
    std::unique_ptr<Stmt> parseReturnStatement();
    std::unique_ptr<Stmt> parsePrintStatement();

    std::unique_ptr<Expr> parseExpression();
    std::unique_ptr<Expr> parseLogicOr();
    std::unique_ptr<Expr> parseLogicAnd();
    std::unique_ptr<Expr> parseEquality();
    std::unique_ptr<Expr> parseComparison();
    std::unique_ptr<Expr> parseTerm();
    std::unique_ptr<Expr> parseFactor();
    std::unique_ptr<Expr> parseUnary();
    std::unique_ptr<Expr> parsePrimary();
};
