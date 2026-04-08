#pragma once
#include "token.hpp"
#include <string>

class Lexer {
public:
    explicit Lexer(std::string source);

    Token nextToken();

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

    void skipWhitespaceAndComments();

    Token makeToken(TokenType type, const std::string& lexeme, SourceLocation location) const;

    Token identifierOrKeyword();
    Token number();
    Token stringLiteral();

    bool match(char expected);
};
