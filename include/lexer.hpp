#pragma once

#include "error.hpp"
#include "token.hpp"
#include <string>


namespace minilang {

class Lexer {
public:
    explicit Lexer(std::string source);

    Result<Token> nextTokenExpected();

private:
    std::string source_;
    std::size_t pos_ = 0;
    int line_ = 1;
    int column_ = 1;

    bool isAtEnd() const;

    char peek() const;
    char peekNext() const;
    char advance();

    SourceLocation currentLocation() const;

    Result<void> skipWhitespaceAndComments();

    Token makeToken(TokenType type, const std::string& lexeme, SourceLocation location) const;

    Result<Token> identifierOrKeyword();
    Result<Token> number();
    Result<Token> stringLiteral();

    bool match(char expected);

    Result<Token> lexicalError(SourceLocation location, const std::string& message) const;
    Result<void> lexicalVoidError(SourceLocation location, const std::string& message) const;
};

} 
