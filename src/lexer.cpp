#include "lexer.hpp"
#include <cctype>
#include <limits>
#include <string>
#include <utility>


namespace minilang {

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

Result<Token> Lexer::lexicalError(SourceLocation location, const std::string& message) const {
    return std::unexpected(Diagnostic(location, message));
}

Result<void> Lexer::lexicalVoidError(SourceLocation location, const std::string& message) const {
    return std::unexpected(Diagnostic(location, message));
}

Result<void> Lexer::skipWhitespaceAndComments() {
    while (!isAtEnd()) {
        char c = peek();

        if (c == ' ' || c == '\r' || c == '\t' || c == '\n') {
            advance();
            continue;
        }

        if (c == '/' && peekNext() == '/') {
            advance();
            advance();

            while (!isAtEnd() && peek() != '\n') {
                advance();
            }

            continue;
        }

        if (c == '/' && peekNext() == '*') {
            SourceLocation commentStart = currentLocation();

            advance();
            advance();

            bool closed = false;

            while (!isAtEnd()) {
                if (peek() == '*' && peekNext() == '/') {
                    advance();
                    advance();
                    closed = true;
                    break;
                }

                advance();
            }

            if (!closed) {
                return lexicalVoidError(commentStart, "unterminated block comment");
            }

            continue;
        }

        break;
    }

    return {};
}

Result<Token> Lexer::identifierOrKeyword() {
    SourceLocation startLocation = currentLocation();
    std::size_t start = pos_;

    while (std::isalnum(static_cast<unsigned char>(peek())) || peek() == '_') {
        advance();
    }

    std::string text = source_.substr(start, pos_ - start);

    if (text == "int") {
        return Token(TokenType::KwInt, text, startLocation);
    }

    if (text == "int32") {
        return Token(TokenType::KwInt32, text, startLocation);
    }

    if (text == "uint") {
        return Token(TokenType::KwUInt, text, startLocation);
    }

    if (text == "uint32") {
        return Token(TokenType::KwUInt32, text, startLocation);
    }

    if (text == "float32") {
        return Token(TokenType::KwFloat32, text, startLocation);
    }

    if (text == "float64") {
        return Token(TokenType::KwFloat64, text, startLocation);
    }

    if (text == "bool") {
        return Token(TokenType::KwBool, text, startLocation);
    }

    if (text == "string") {
        return Token(TokenType::KwString, text, startLocation);
    }

    if (text == "void") {
        return Token(TokenType::KwVoid, text, startLocation);
    }

    if (text == "unit") {
        return Token(TokenType::KwUnit, text, startLocation);
    }

    if (text == "type") {
        return Token(TokenType::KwType, text, startLocation);
    }

    if (text == "struct") {
        return Token(TokenType::KwStruct, text, startLocation);
    }

    if (text == "namespace") {
        return Token(TokenType::KwNamespace, text, startLocation);
    }

    if (text == "var") {
        return Token(TokenType::KwVar, text, startLocation);
    }

    if (text == "let") {
        return Token(TokenType::KwLet, text, startLocation);
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

    if (text == "switch") {
        return Token(TokenType::KwSwitch, text, startLocation);
    }

    if (text == "case") {
        return Token(TokenType::KwCase, text, startLocation);
    }

    if (text == "default") {
        return Token(TokenType::KwDefault, text, startLocation);
    }

    if (text == "for") {
        return Token(TokenType::KwFor, text, startLocation);
    }

    if (text == "return") {
        return Token(TokenType::KwReturn, text, startLocation);
    }

    if (text == "break") {
        return Token(TokenType::KwBreak, text, startLocation);
    }

    if (text == "continue") {
        return Token(TokenType::KwContinue, text, startLocation);
    }

    if (text == "cast") {
        return Token(TokenType::KwCast, text, startLocation);
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

    if (text == "inf") {
        return Token(
            TokenType::FloatLiteral,
            text,
            startLocation,
            std::numeric_limits<double>::infinity()
        );
    }

    if (text == "NaN" || text == "nan") {
        return Token(
            TokenType::FloatLiteral,
            text,
            startLocation,
            std::numeric_limits<double>::quiet_NaN()
        );
    }

    return Token(TokenType::Identifier, text, startLocation);
}

Result<Token> Lexer::number() {
    SourceLocation startLocation = currentLocation();
    std::string text;

    if (peek() == '0' && (peekNext() == 'x' || peekNext() == 'X')) {
        advance();
        advance();

        std::string digits;

        while (std::isxdigit(static_cast<unsigned char>(peek()))) {
            digits.push_back(advance());
        }

        if (digits.empty()) {
            return lexicalError(startLocation, "expected hexadecimal digits after 0x");
        }

        if (std::isalnum(static_cast<unsigned char>(peek())) || peek() == '_') {
            return lexicalError(startLocation, "invalid character in hexadecimal integer literal");
        }

        long long value = 0;

        try {
            value = std::stoll(digits, nullptr, 16);
        } catch (...) {
            return lexicalError(startLocation, "hexadecimal integer literal is too large");
        }

        if (value > std::numeric_limits<int>::max()) {
            return lexicalError(startLocation, "hexadecimal integer literal is out of int32 range");
        }

        return Token(
            TokenType::IntLiteral,
            std::to_string(value),
            startLocation,
            static_cast<int>(value)
        );
    }

    if (peek() == '0' && (peekNext() == 'b' || peekNext() == 'B')) {
        advance();
        advance();

        long long value = 0;
        bool hasDigit = false;

        while (peek() == '0' || peek() == '1') {
            hasDigit = true;
            char digit = advance();

            value = value * 2 + (digit - '0');

            if (value > std::numeric_limits<int>::max()) {
                return lexicalError(startLocation, "binary integer literal is out of int32 range");
            }
        }

        if (!hasDigit) {
            return lexicalError(startLocation, "expected binary digits after 0b");
        }

        if (std::isalnum(static_cast<unsigned char>(peek())) || peek() == '_') {
            return lexicalError(startLocation, "invalid character in binary integer literal");
        }

        return Token(
            TokenType::IntLiteral,
            std::to_string(value),
            startLocation,
            static_cast<int>(value)
        );
    }

    bool isFloat = false;

    while (std::isdigit(static_cast<unsigned char>(peek()))) {
        text.push_back(advance());
    }

    if (peek() == '.') {
        isFloat = true;
        text.push_back(advance());

        if (!std::isdigit(static_cast<unsigned char>(peek()))) {
            return lexicalError(startLocation, "expected digit after '.'");
        }

        while (std::isdigit(static_cast<unsigned char>(peek()))) {
            text.push_back(advance());
        }
    }

    if (peek() == 'e' || peek() == 'E') {
        isFloat = true;
        text.push_back(advance());

        if (peek() == '+' || peek() == '-') {
            text.push_back(advance());
        }

        if (!std::isdigit(static_cast<unsigned char>(peek()))) {
            return lexicalError(startLocation, "expected digit in exponent");
        }

        while (std::isdigit(static_cast<unsigned char>(peek()))) {
            text.push_back(advance());
        }
    }

    if (isFloat) {
        double value = 0.0;

        try {
            value = std::stod(text);
        } catch (...) {
            return lexicalError(startLocation, "floating-point literal is invalid");
        }

        return Token(TokenType::FloatLiteral, text, startLocation, value);
    }

    long long intValue = 0;

    try {
        intValue = std::stoll(text);
    } catch (...) {
        return lexicalError(startLocation, "integer literal is too large");
    }

    if (intValue > std::numeric_limits<int>::max()) {
        return lexicalError(startLocation, "integer literal is out of int32 range");
    }

    return Token(
        TokenType::IntLiteral,
        text,
        startLocation,
        static_cast<int>(intValue)
    );
}

Result<Token> Lexer::stringLiteral() {
    SourceLocation startLocation = currentLocation();

    advance();

    std::string value;
    std::string lexeme = "\"";

    while (!isAtEnd() && peek() != '"') {
        if (peek() == '\n') {
            return lexicalError(startLocation, "unterminated string literal");
        }

        if (peek() == '\\') {
            lexeme.push_back(advance());

            if (isAtEnd()) {
                return lexicalError(startLocation, "unterminated escape sequence");
            }

            char escaped = advance();
            lexeme.push_back(escaped);

            switch (escaped) {
                case 'n':
                    value.push_back('\n');
                    break;

                case 't':
                    value.push_back('\t');
                    break;

                case '"':
                    value.push_back('"');
                    break;

                case '\\':
                    value.push_back('\\');
                    break;

                default:
                    return lexicalError(
                        currentLocation(),
                        std::string("unknown escape sequence \\") + escaped
                    );
            }

            continue;
        }

        char ch = advance();
        lexeme.push_back(ch);
        value.push_back(ch);
    }

    if (isAtEnd()) {
        return lexicalError(startLocation, "unterminated string literal");
    }

    lexeme.push_back(advance());

    return Token(TokenType::StringLiteral, lexeme, startLocation, value);
}

Result<Token> Lexer::nextTokenExpected() {
    Result<void> skipped = skipWhitespaceAndComments();

    if (!skipped) {
        return std::unexpected(skipped.error());
    }

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
            if (match('+')) {
                return makeToken(TokenType::PlusPlus, "++", location);
            }

            if (match('=')) {
                return makeToken(TokenType::PlusAssign, "+=", location);
            }

            return makeToken(TokenType::Plus, "+", location);

        case '-':
            if (match('-')) {
                return makeToken(TokenType::MinusMinus, "--", location);
            }

            if (match('=')) {
                return makeToken(TokenType::MinusAssign, "-=", location);
            }

            return makeToken(TokenType::Minus, "-", location);

        case '*':
            return makeToken(TokenType::Star, "*", location);

        case '/':
            return makeToken(TokenType::Slash, "/", location);

        case '%':
            return makeToken(TokenType::Percent, "%", location);

        case '^':
            return makeToken(TokenType::BitXor, "^", location);

        case '~':
            return makeToken(TokenType::BitNot, "~", location);

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
            if (match('<')) {
                return makeToken(TokenType::ShiftLeft, "<<", location);
            }

            if (match('=')) {
                return makeToken(TokenType::LessEqual, "<=", location);
            }

            return makeToken(TokenType::Less, "<", location);

        case '>':
            if (match('>')) {
                return makeToken(TokenType::ShiftRight, ">>", location);
            }

            if (match('=')) {
                return makeToken(TokenType::GreaterEqual, ">=", location);
            }

            return makeToken(TokenType::Greater, ">", location);

        case '&':
            if (match('&')) {
                return makeToken(TokenType::AndAnd, "&&", location);
            }

            return makeToken(TokenType::BitAnd, "&", location);

        case '|':
            if (match('|')) {
                return makeToken(TokenType::OrOr, "||", location);
            }

            return makeToken(TokenType::BitOr, "|", location);

        case '(':
            return makeToken(TokenType::LParen, "(", location);

        case ')':
            return makeToken(TokenType::RParen, ")", location);

        case '{':
            return makeToken(TokenType::LBrace, "{", location);

        case '}':
            return makeToken(TokenType::RBrace, "}", location);

        case '[':
            return makeToken(TokenType::LBracket, "[", location);

        case ']':
            return makeToken(TokenType::RBracket, "]", location);

        case ',':
            return makeToken(TokenType::Comma, ",", location);

        case ';':
            return makeToken(TokenType::Semicolon, ";", location);

        case ':':
            if (match(':')) {
                return makeToken(TokenType::DoubleColon, "::", location);
            }

            return makeToken(TokenType::Colon, ":", location);

        case '.':
            return makeToken(TokenType::Dot, ".", location);

        default:
            return lexicalError(location, std::string("unexpected character '") + c + "'");
    }
}

} 
