#pragma once
#include "source_location.hpp"
#include <string>
#include <utility>

enum class TokenType {
    // Keywords
    KwInt,
    KwBool,
    KwString,
    KwIf,
    KwElse,
    KwWhile,
    KwReturn,
    KwPrint,
    KwTrue,
    KwFalse,
    KwMain,

    // Identifiers and literals
    Identifier,
    IntLiteral,
    StringLiteral,

    // Operators
    Plus,          // +
    Minus,         // -
    Star,          // *
    Slash,         // /
    Percent,       // %

    Assign,        // =
    Equal,         // ==
    NotEqual,      // !=
    Less,          // <
    Greater,       // >
    LessEqual,     // <=
    GreaterEqual,  // >=

    AndAnd,        // &&
    OrOr,          // ||
    Bang,          // !

    // Delimiters
    LParen,        // (
    RParen,        // )
    LBrace,        // {
    RBrace,        // }
    Comma,         // ,
    Semicolon,     // ;

    EndOfFile
};

struct Token {
    TokenType type;
    std::string lexeme;
    SourceLocation location;

    int intValue = 0;
    std::string stringValue;

    Token(TokenType type,
          std::string lexeme,
          SourceLocation location)
        : type(type),
          lexeme(std::move(lexeme)),
          location(location) {}

    Token(TokenType type,
          std::string lexeme,
          SourceLocation location,
          int intValue)
        : type(type),
          lexeme(std::move(lexeme)),
          location(location),
          intValue(intValue) {}

    Token(TokenType type,
          std::string lexeme,
          SourceLocation location,
          std::string stringValue)
        : type(type),
          lexeme(std::move(lexeme)),
          location(location),
          stringValue(std::move(stringValue)) {}
};

inline std::string tokenTypeToString(TokenType type) {
    switch (type) {
        case TokenType::KwInt: return "KwInt";
        case TokenType::KwBool: return "KwBool";
        case TokenType::KwString: return "KwString";
        case TokenType::KwIf: return "KwIf";
        case TokenType::KwElse: return "KwElse";
        case TokenType::KwWhile: return "KwWhile";
        case TokenType::KwReturn: return "KwReturn";
        case TokenType::KwPrint: return "KwPrint";
        case TokenType::KwTrue: return "KwTrue";
        case TokenType::KwFalse: return "KwFalse";
        case TokenType::KwMain: return "KwMain";

        case TokenType::Identifier: return "Identifier";
        case TokenType::IntLiteral: return "IntLiteral";
        case TokenType::StringLiteral: return "StringLiteral";

        case TokenType::Plus: return "Plus";
        case TokenType::Minus: return "Minus";
        case TokenType::Star: return "Star";
        case TokenType::Slash: return "Slash";
        case TokenType::Percent: return "Percent";

        case TokenType::Assign: return "Assign";
        case TokenType::Equal: return "Equal";
        case TokenType::NotEqual: return "NotEqual";
        case TokenType::Less: return "Less";
        case TokenType::Greater: return "Greater";
        case TokenType::LessEqual: return "LessEqual";
        case TokenType::GreaterEqual: return "GreaterEqual";

        case TokenType::AndAnd: return "AndAnd";
        case TokenType::OrOr: return "OrOr";
        case TokenType::Bang: return "Bang";

        case TokenType::LParen: return "LParen";
        case TokenType::RParen: return "RParen";
        case TokenType::LBrace: return "LBrace";
        case TokenType::RBrace: return "RBrace";
        case TokenType::Comma: return "Comma";
        case TokenType::Semicolon: return "Semicolon";

        case TokenType::EndOfFile: return "EndOfFile";
    }

    return "Unknown";
}

inline std::string tokenToString(const Token& token) {
    std::string result = tokenTypeToString(token.type);

    if (!token.lexeme.empty()) {
        result += "('" + token.lexeme + "')";
    }

    result += " at " + toString(token.location);

    return result;
}
