#pragma once
#include <string>

namespace minilang {

struct SourceLocation {
    int line = 1;
    int column = 1;

    SourceLocation() = default;

    SourceLocation(int line, int column)
        : line(line), column(column) {}
};

inline std::string toString(const SourceLocation& loc) {
    return "line " + std::to_string(loc.line) +
           ", column " + std::to_string(loc.column);
}

} 
