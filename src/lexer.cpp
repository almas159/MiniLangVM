#include "lexer.hpp"
#include "error.hpp"
#include <cctype>
#include <limits>
#include <string>
#include <utility>

Lexer::Lexer(std::string source)
    : source_(std::move(source)) {}

bool Lexer::isAtEnd() const {
    return pos_ >= source_.size();
}

char Lexer::peek() const {
    if (isAtEnd()) {
        return '\0';
    }

    return source_[pos_];
}

char Lexer::peekNext() const {
    if (pos_ + 1 >= source_.size()) {
        return '\0';
    }

    return source_[pos_ + 1];
}

char Lexer::advance() {
    if (isAtEnd()) {
        return '\0';
    }

    char c = source_[pos_++];

    if (c == '\n') {
        line_++;
        column_ = 1;
    } else {
        column_++;
    }

    return c;
}

SourceLocation Lexer::currentLocation() const {
    return SourceLocation(line_, column_);
}

bool Lexer::match(char expected) {
    if (isAtEnd()) {
        return false;
    }

    if (source_[pos_] != expected) {
        return false;
    }

    advance();
    return true;
}

Token Lexer::makeToken(TokenType type,
                       const std::string& lexeme,
                       SourceLocation location) const {
    return Token(type, lexeme, location);
}

void Lexer::skipWhitespaceAndComments() {
    while (!isAtEnd()) {
        char c = peek();

        if (c == ' ' || c == '\r' || c == '\t' || c == '\n') {
            advance();
            continue;
        }

        if (c == '/' && peekNext() == '/') {
            advance(); // /
            advance(); // /

            while (!isAtEnd() && peek() != '\n') {
                advance();
            }

            continue;
        }

        if (c == '/' && peekNext() == '*') {
            SourceLocation commentStart = currentLocation();

            advance(); // /
            advance(); // *

            bool closed = false;

            while (!isAtEnd()) {
                if (peek() == '*' && peekNext() == '/') {
                    advance(); // *
                    advance(); // /
                    closed = true;
                    break;
                }

                advance();
            }

            if (!closed) {
                throw LexicalError(commentStart, "unterminated block comment");
            }

            continue;
        }

        break;
    }
}

Token Lexer::identifierOrKeyword() {
    SourceLocation startLocation = currentLocation();
    std::size_t start = pos_;

    while (std::isalnum(static_cast<unsigned char>(peek())) || peek() == '_') {
        advance();
    }

    std::string text = source_.substr(start, pos_ - start);

    if (text == "int") {
        return Token(TokenType::KwInt, text, startLocation);
    }

    if (text == "bool") {
        return Token(TokenType::KwBool, text, startLocation);
    }

    if (text == "string") {
        return Token(TokenType::KwString, text, startLocation);
    }

    if (text == "if") {
        return Token(TokenType::KwIf, text, startLocation);
    }

    if (text == "else") {
        return Token(TokenType::KwElse, text, startLocation);
    }

    if (text == "while") {
        return Token(TokenType::KwWhile, text, startLocation);
    }

    if (text == "return") {
        return Token(TokenType::KwReturn, text, startLocation);
    }

    if (text == "print") {
        return Token(TokenType::KwPrint, text, startLocation);
    }

    if (text == "true") {
        return Token(TokenType::KwTrue, text, startLocation);
    }

    if (text == "false") {
        return Token(TokenType::KwFalse, text, startLocation);
    }

    if (text == "main") {
        return Token(TokenType::KwMain, text, startLocation);
    }

    return Token(TokenType::Identifier, text, startLocation);
}

Token Lexer::number() {
    SourceLocation startLocation = currentLocation();
    std::size_t start = pos_;

    long long value = 0;

    while (std::isdigit(static_cast<unsigned char>(peek()))) {
        int digit = advance() - '0';

        value = value * 10 + digit;

        if (value > std::numeric_limits<int>::max()) {
            throw LexicalError(startLocation, "integer literal is too large");
        }
    }

    std::string text = source_.substr(start, pos_ - start);

    return Token(TokenType::IntLiteral, text, startLocation, static_cast<int>(value));
}

Token Lexer::stringLiteral() {
    SourceLocation startLocation = currentLocation();

    advance(); // открывающая кавычка "

    std::string value;

    while (!isAtEnd() && peek() != '"') {
        if (peek() == '\n') {
            throw LexicalError(startLocation, "unterminated string literal");
        }

        value.push_back(advance());
    }

    if (isAtEnd()) {
        throw LexicalError(startLocation, "unterminated string literal");
    }

    advance(); // закрывающая кавычка "

    std::string lexeme = "\"" + value + "\"";

    return Token(TokenType::StringLiteral, lexeme, startLocation, value);
}

Token Lexer::nextToken() {
    skipWhitespaceAndComments();

    SourceLocation location = currentLocation();

    if (isAtEnd()) {
        return Token(TokenType::EndOfFile, "", location);
    }

    char c = peek();

    if (std::isalpha(static_cast<unsigned char>(c)) || c == '_') {
        return identifierOrKeyword();
    }

    if (std::isdigit(static_cast<unsigned char>(c))) {
        return number();
    }

    if (c == '"') {
        return stringLiteral();
    }

    advance();

    switch (c) {
        case '+':
            return makeToken(TokenType::Plus, "+", location);

        case '-':
            return makeToken(TokenType::Minus, "-", location);

        case '*':
            return makeToken(TokenType::Star, "*", location);

        case '/':
            return makeToken(TokenType::Slash, "/", location);

        case '%':
            return makeToken(TokenType::Percent, "%", location);

        case '=':
            if (match('=')) {
                return makeToken(TokenType::Equal, "==", location);
            }
            return makeToken(TokenType::Assign, "=", location);

        case '!':
            if (match('=')) {
                return makeToken(TokenType::NotEqual, "!=", location);
            }
            return makeToken(TokenType::Bang, "!", location);

        case '<':
            if (match('=')) {
                return makeToken(TokenType::LessEqual, "<=", location);
            }
            return makeToken(TokenType::Less, "<", location);

        case '>':
            if (match('=')) {
                return makeToken(TokenType::GreaterEqual, ">=", location);
            }
            return makeToken(TokenType::Greater, ">", location);

        case '&':
            if (match('&')) {
                return makeToken(TokenType::AndAnd, "&&", location);
            }
            throw LexicalError(location, "unexpected character '&'. Did you mean '&&'?");

        case '|':
            if (match('|')) {
                return makeToken(TokenType::OrOr, "||", location);
            }
            throw LexicalError(location, "unexpected character '|'. Did you mean '||'?");

        case '(':
            return makeToken(TokenType::LParen, "(", location);

        case ')':
            return makeToken(TokenType::RParen, ")", location);

        case '{':
            return makeToken(TokenType::LBrace, "{", location);

        case '}':
            return makeToken(TokenType::RBrace, "}", location);

        case ',':
            return makeToken(TokenType::Comma, ",", location);

        case ';':
            return makeToken(TokenType::Semicolon, ";", location);

        default:
            throw LexicalError(location, std::string("unexpected character '") + c + "'");
    }
}
