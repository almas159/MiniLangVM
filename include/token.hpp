#pragma once
#include "source_location.hpp"
#include <string>
#include <utility>

namespace minilang {

enum class TokenType {
    KwInt,
    KwInt32,
    KwUInt,
    KwUInt32,
    KwFloat32,
    KwFloat64,
    KwBool,
    KwString,
    KwVoid,
    KwUnit,
    KwType,
    KwStruct,
    KwNamespace,
    KwVar,
    KwLet,
    KwIf,
    KwElse,
    KwWhile,
    KwSwitch,
    KwCase,
    KwDefault,
    KwFor,
    KwReturn,
    KwBreak,
    KwContinue,
    KwCast,
    KwPrint,
    KwTrue,
    KwFalse,
    KwMain,

    // identifiers and literals
    Identifier,
    IntLiteral,
    FloatLiteral,
    StringLiteral,

    // operators
    Plus,          // +
    PlusPlus,      // ++
    PlusAssign,    // +=
    Minus,         // -
    MinusMinus,    // --
    MinusAssign,   // -=
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

    
    LParen,        // (
    RParen,        // )
    LBrace,        // {
    RBrace,        // }
    LBracket,      // [
    RBracket,      // ]
    Comma,         // ,
    Semicolon,     // ;
    Colon,         // :
    DoubleColon,   // ::
    Dot,           // .

    BitAnd,
    BitOr,
    BitXor,
    BitNot,
    ShiftLeft,
    ShiftRight,
    EndOfFile
};

struct Token {
    TokenType type;
    std::string lexeme;
    SourceLocation location;

    int intValue = 0;
    double floatValue = 0.0;
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
          double floatValue)
        : type(type),
          lexeme(std::move(lexeme)),
          location(location),
          floatValue(floatValue) {}

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
        case TokenType::KwInt32: return "KwInt32";
        case TokenType::KwUInt: return "KwUInt";
        case TokenType::KwUInt32: return "KwUInt32";
        case TokenType::KwFloat32: return "KwFloat32";
        case TokenType::KwFloat64: return "KwFloat64";
        case TokenType::KwBool: return "KwBool";
        case TokenType::KwString: return "KwString";
        case TokenType::KwVoid: return "KwVoid";
        case TokenType::KwUnit: return "KwUnit";
        case TokenType::KwType: return "KwType";
        case TokenType::KwStruct: return "KwStruct";
        case TokenType::KwNamespace: return "KwNamespace";
        case TokenType::KwVar: return "KwVar";
        case TokenType::KwLet: return "KwLet";
        case TokenType::KwIf: return "KwIf";
        case TokenType::KwElse: return "KwElse";
        case TokenType::KwWhile: return "KwWhile";
        case TokenType::KwSwitch: return "KwSwitch";
        case TokenType::KwCase: return "KwCase";
        case TokenType::KwDefault: return "KwDefault";
        case TokenType::KwFor: return "KwFor";
        case TokenType::KwReturn: return "KwReturn";
        case TokenType::KwBreak: return "KwBreak";
        case TokenType::KwContinue: return "KwContinue";
        case TokenType::KwCast: return "KwCast";
        case TokenType::KwPrint: return "KwPrint";
        case TokenType::KwTrue: return "KwTrue";
        case TokenType::KwFalse: return "KwFalse";
        case TokenType::KwMain: return "KwMain";

        case TokenType::Identifier: return "Identifier";
        case TokenType::IntLiteral: return "IntLiteral";
        case TokenType::FloatLiteral: return "FloatLiteral";
        case TokenType::StringLiteral: return "StringLiteral";

        case TokenType::Plus: return "Plus";
        case TokenType::PlusPlus: return "PlusPlus";
        case TokenType::PlusAssign: return "PlusAssign";
        case TokenType::Minus: return "Minus";
        case TokenType::MinusMinus: return "MinusMinus";
        case TokenType::MinusAssign: return "MinusAssign";
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
        case TokenType::BitAnd: return "BitAnd";
        case TokenType::BitOr: return "BitOr";
        case TokenType::BitXor: return "BitXor";
        case TokenType::BitNot: return "BitNot";
        case TokenType::ShiftLeft: return "ShiftLeft";
        case TokenType::ShiftRight: return "ShiftRight";
        case TokenType::OrOr: return "OrOr";
        case TokenType::Bang: return "Bang";

        case TokenType::LParen: return "LParen";
        case TokenType::RParen: return "RParen";
        case TokenType::LBrace: return "LBrace";
        case TokenType::RBrace: return "RBrace";
        case TokenType::LBracket: return "LBracket";
        case TokenType::RBracket: return "RBracket";
        case TokenType::Comma: return "Comma";
        case TokenType::Semicolon: return "Semicolon";
        case TokenType::Colon: return "Colon";
        case TokenType::DoubleColon: return "DoubleColon";
        case TokenType::Dot: return "Dot";

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

} 
