#pragma once

#include "source_location.hpp"
#include <exception>
#include <expected>
#include <string>
#include <utility>

namespace minilang {

struct Diagnostic {
    SourceLocation location;
    std::string message;

    Diagnostic() = default;

    Diagnostic(SourceLocation location, std::string message)
        : location(location),
          message(std::move(message)) {}
};

template <typename T>
using Result = std::expected<T, Diagnostic>;

class MiniLangError : public std::exception {
public:
    MiniLangError(std::string kind,
                  SourceLocation location,
                  std::string message)
        : kind_(std::move(kind)),
          location_(location),
          message_(std::move(message)) {
        whatMessage_ =
            kind_ + " error at line " +
            std::to_string(location_.line) +
            ", column " +
            std::to_string(location_.column) +
            ": " + message_;
    }

    const char* what() const noexcept override {
        return whatMessage_.c_str();
    }

    const std::string& kind() const {
        return kind_;
    }

    SourceLocation location() const {
        return location_;
    }

    const std::string& message() const {
        return message_;
    }

private:
    std::string kind_;
    SourceLocation location_;
    std::string message_;
    std::string whatMessage_;
};

class LexicalError : public MiniLangError {
public:
    LexicalError(SourceLocation location, std::string message)
        : MiniLangError("lexical", location, std::move(message)) {}
};

class SyntaxError : public MiniLangError {
public:
    SyntaxError(SourceLocation location, std::string message)
        : MiniLangError("syntax", location, std::move(message)) {}
};

class SemanticError : public MiniLangError {
public:
    SemanticError(SourceLocation location, std::string message)
        : MiniLangError("semantic", location, std::move(message)) {}
};

class RuntimeError : public MiniLangError {
public:
    RuntimeError(SourceLocation location, std::string message)
        : MiniLangError("runtime", location, std::move(message)) {}
};

} 
