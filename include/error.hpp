#pragma once
#include "source_location.hpp"
#include <stdexcept>
#include <string>

class MiniLangError : public std::runtime_error {
public:
    MiniLangError(SourceLocation location, const std::string& message)
        : std::runtime_error(message), location_(location) {}

    const SourceLocation& location() const {
        return location_;
    }

private:
    SourceLocation location_;
};

class LexicalError : public MiniLangError {
public:
    LexicalError(SourceLocation location, const std::string& message)
        : MiniLangError(location, "Lexical error at " + toString(location) + ": " + message) {}
};

class SyntaxError : public MiniLangError {
public:
    SyntaxError(SourceLocation location, const std::string& message)
        : MiniLangError(location, "Syntax error at " + toString(location) + ": " + message) {}
};

class SemanticError : public MiniLangError {
public:
    SemanticError(SourceLocation location, const std::string& message)
        : MiniLangError(location, "Semantic error at " + toString(location) + ": " + message) {}
};

class RuntimeError : public MiniLangError {
public:
    RuntimeError(SourceLocation location, const std::string& message)
        : MiniLangError(location, "Runtime error at " + toString(location) + ": " + message) {}
};
