#pragma once
#include "ast.hpp"
#include "error.hpp"
#include "lexer.hpp"
#include "token.hpp"

#include <memory>
#include <optional>
#include <type_traits>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace minilang {

class Parser {
public:
    explicit Parser(Lexer& lexer);

    Result<std::unique_ptr<Program>> parseProgramExpected();
    std::unique_ptr<Program> parseProgram();

private:
    Lexer& lexer_;
    Token current_;
    std::vector<Token> bufferedTokens_;
    std::optional<Diagnostic> diagnostic_;
    int lastParsedArraySize_ = -1;

    struct TypeAliasInfo {
        Type type = Type::Unknown;
        int arraySize = -1;
    std::string typeName;
};

    std::unordered_map<std::string, TypeAliasInfo> typeAliases_;
    std::unordered_set<std::string> structTypeNames_;
    std::string lastParsedTypeName_;
    std::string currentNamespace_;

    void advance();
    const Token& peekToken(std::size_t offset);
    bool looksLikeExplicitGenericCall() const;

    bool check(TokenType type) const;
    bool match(TokenType type);

    Token expect(TokenType type, const std::string& message);
    void setDiagnostic(SourceLocation location, const std::string& message);
    void failVoid(SourceLocation location, const std::string& message);

    template <typename T>
    T fail(SourceLocation location, const std::string& message) {
        setDiagnostic(location, message);

        if constexpr (std::is_same_v<T, Token>) {
            return Token(TokenType::EndOfFile, "", location);
        } else if constexpr (std::is_same_v<T, Type>) {
            return Type::Unknown;
        } else {
            return T{};
        }
    }


    bool isTypeToken(TokenType type) const;
    Type parseType();

    void parseNamespaceDecl(Program& program);
    std::unique_ptr<StructDecl> parseStructDecl();
    std::vector<StructFieldDecl> parseStructFields();
    std::unique_ptr<TypeAliasDecl> parseTypeAliasDecl();
    std::string parseFunctionName();
    std::unique_ptr<FunctionDecl> parseFunctionDecl();
    std::vector<Parameter> parseParameters();

    std::unique_ptr<BlockStmt> parseBlock();

    std::unique_ptr<Stmt> parseStatement();
    std::unique_ptr<Stmt> parseIdentifierStatement();
    std::unique_ptr<Stmt> parseVarDeclStatement();
    std::unique_ptr<Stmt> parseAssignmentStatement();
    std::unique_ptr<Stmt> parseIfStatement();
    std::unique_ptr<Stmt> parseWhileStatement();
    std::unique_ptr<Stmt> parseForStatement();
    std::unique_ptr<Stmt> parseForInitializer();
    std::unique_ptr<Stmt> parseForUpdate();
    std::unique_ptr<Stmt> parseSwitchStatement();
    std::unique_ptr<Stmt> parseReturnStatement();
    std::unique_ptr<Stmt> parseBreakStatement();
    std::unique_ptr<Stmt> parseContinueStatement();
    std::unique_ptr<Stmt> parsePrintStatement();

    std::unique_ptr<Expr> parseExpression();
    std::unique_ptr<Expr> parseLogicOr();
    std::unique_ptr<Expr> parseLogicAnd();
    std::unique_ptr<Expr> parseBitOr();
    std::unique_ptr<Expr> parseBitXor();
    std::unique_ptr<Expr> parseBitAnd();
    std::unique_ptr<Expr> parseEquality();
    std::unique_ptr<Expr> parseComparison();
    std::unique_ptr<Expr> parseShift();
    std::unique_ptr<Expr> parseTerm();
    std::unique_ptr<Expr> parseFactor();
    std::unique_ptr<Expr> parseUnary();
    std::unique_ptr<Expr> parseCastExpression();
    std::unique_ptr<Expr> parsePrimary();

    std::unique_ptr<Expr> parseCallExpression(Token nameToken);
    std::unique_ptr<Expr> parseQualifiedCallExpression(Token namespaceToken);
    std::unique_ptr<Expr> parseStructLiteral(Token nameToken);
};

} 
