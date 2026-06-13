#include "ast_printer.hpp"
#include <iostream>
#include <sstream>
#include <string>

namespace minilang {

void AstPrinter::print(const Program& program, std::ostream& out) {
    indent_ = 0;

    writeLine(out, "Program");
    indent_++;

    for (const auto& structDecl : program.structs) {
        writeLine(out, "StructDecl name=" + structDecl->name);

        indent_++;

        for (const StructFieldDecl& field : structDecl->fields) {
            std::string line =
                "Field name=" + field.name +
                " type=" + typeToString(field.type);

            if (!field.typeName.empty()) {
                line += " typeName=" + field.typeName;
            }

            writeLine(out, line);
        }

        indent_--;
    }

    for (const auto& alias : program.aliases) {
        std::string line =
            "TypeAliasDecl name=" + alias->name +
            " target=" + typeToString(alias->targetType);

        if (alias->arraySize > 0) {
            line += " arraySize=" + std::to_string(alias->arraySize);
        }

        writeLine(out, line);
    }

    for (const auto& function : program.functions) {
        printFunction(*function, out);
    }

    indent_--;
}

void AstPrinter::writeIndent(std::ostream& out) const {
    for (int i = 0; i < indent_; ++i) {
        out << "  ";
    }
}

void AstPrinter::writeLine(std::ostream& out, const std::string& text) const {
    writeIndent(out);
    out << text << '\n';
}

void AstPrinter::printFunction(const FunctionDecl& function, std::ostream& out) {
    writeLine(
        out,
        "FunctionDecl name=" + function.name +
        " returnType=" + typeToString(function.returnType) +
        " localCount=" + std::to_string(function.localCount)
    );

    indent_++;

    writeLine(out, "Parameters");
    indent_++;

    for (const Parameter& parameter : function.parameters) {
        writeLine(
            out,
            "Parameter name=" + parameter.name +
            " type=" + typeToString(parameter.type) +
            " localIndex=" + std::to_string(parameter.localIndex)
        );
    }

    indent_--;

    writeLine(out, "Body");
    indent_++;
    printBlock(*function.body, out);
    indent_--;

    indent_--;
}

void AstPrinter::printBlock(const BlockStmt& block, std::ostream& out) {
    writeLine(out, "BlockStmt");

    indent_++;

    for (const auto& statement : block.statements) {
        printStmt(*statement, out);
    }

    indent_--;
}

void AstPrinter::printStmt(const Stmt& stmt, std::ostream& out) {
    if (dynamic_cast<const EmptyStmt*>(&stmt)) {
        writeLine(out, "EmptyStmt");
        return;
    }

    if (auto* s = dynamic_cast<const ExprStmt*>(&stmt)) {
        printExprStmt(*s, out);
        return;
    }

    if (auto* s = dynamic_cast<const BlockStmt*>(&stmt)) {
        printBlock(*s, out);
        return;
    }

    if (auto* s = dynamic_cast<const VarDeclStmt*>(&stmt)) {
        printVarDeclStmt(*s, out);
        return;
    }

    if (auto* s = dynamic_cast<const AssignStmt*>(&stmt)) {
        printAssignStmt(*s, out);
        return;
    }

    if (auto* s = dynamic_cast<const ArrayAssignStmt*>(&stmt)) {
        printArrayAssignStmt(*s, out);
        return;
    }

    if (auto* s = dynamic_cast<const IfStmt*>(&stmt)) {
        printIfStmt(*s, out);
        return;
    }

    if (auto* s = dynamic_cast<const WhileStmt*>(&stmt)) {
        printWhileStmt(*s, out);
        return;
    }

    if (auto* s = dynamic_cast<const ForStmt*>(&stmt)) {
        printForStmt(*s, out);
        return;
    }

    if (auto* s = dynamic_cast<const SwitchStmt*>(&stmt)) {
        printSwitchStmt(*s, out);
        return;
    }

    if (auto* s = dynamic_cast<const ReturnStmt*>(&stmt)) {
        printReturnStmt(*s, out);
        return;
    }

    if (dynamic_cast<const BreakStmt*>(&stmt)) {
        writeLine(out, "BreakStmt");
        return;
    }

    if (dynamic_cast<const ContinueStmt*>(&stmt)) {
        writeLine(out, "ContinueStmt");
        return;
    }

    if (auto* s = dynamic_cast<const PrintStmt*>(&stmt)) {
        printPrintStmt(*s, out);
        return;
    }

    writeLine(out, "UnknownStmt");
}

void AstPrinter::printVarDeclStmt(const VarDeclStmt& stmt, std::ostream& out) {
    std::string line =
        "VarDeclStmt name=" + stmt.name +
        " type=" + typeToString(stmt.type) +
        " localIndex=" + std::to_string(stmt.localIndex);

    if (stmt.arraySize > 0) {
        line += " arraySize=" + std::to_string(stmt.arraySize);
    }

    writeLine(out, line);

    if (stmt.initializer) {
        indent_++;
        writeLine(out, "Initializer");
        indent_++;
        printExpr(*stmt.initializer, out);
        indent_ -= 2;
    }
}

void AstPrinter::printAssignStmt(const AssignStmt& stmt, std::ostream& out) {
    writeLine(
        out,
        "AssignStmt name=" + stmt.name +
        " localIndex=" + std::to_string(stmt.localIndex)
    );

    indent_++;
    printExpr(*stmt.value, out);
    indent_--;
}

void AstPrinter::printArrayAssignStmt(const ArrayAssignStmt& stmt, std::ostream& out) {
    writeLine(
        out,
        "ArrayAssignStmt name=" + stmt.name +
        " localIndex=" + std::to_string(stmt.localIndex)
    );

    indent_++;

    writeLine(out, "Index");
    indent_++;
    printExpr(*stmt.index, out);
    indent_--;

    writeLine(out, "Value");
    indent_++;
    printExpr(*stmt.value, out);
    indent_--;

    indent_--;
}

void AstPrinter::printIfStmt(const IfStmt& stmt, std::ostream& out) {
    writeLine(out, "IfStmt");

    indent_++;

    writeLine(out, "Condition");
    indent_++;
    printExpr(*stmt.condition, out);
    indent_--;

    writeLine(out, "Then");
    indent_++;
    printStmt(*stmt.thenBranch, out);
    indent_--;

    if (stmt.elseBranch) {
        writeLine(out, "Else");
        indent_++;
        printStmt(*stmt.elseBranch, out);
        indent_--;
    }

    indent_--;
}

void AstPrinter::printWhileStmt(const WhileStmt& stmt, std::ostream& out) {
    writeLine(out, "WhileStmt");

    indent_++;

    writeLine(out, "Condition");
    indent_++;
    printExpr(*stmt.condition, out);
    indent_--;

    writeLine(out, "Body");
    indent_++;
    printStmt(*stmt.body, out);
    indent_--;

    indent_--;
}

void AstPrinter::printForStmt(const ForStmt& stmt, std::ostream& out) {
    writeLine(out, "ForStmt");

    indent_++;

    if (stmt.initializer) {
        writeLine(out, "Initializer");
        indent_++;
        printStmt(*stmt.initializer, out);
        indent_--;
    }

    writeLine(out, "Condition");
    indent_++;
    printExpr(*stmt.condition, out);
    indent_--;

    if (stmt.update) {
        writeLine(out, "Update");
        indent_++;
        printStmt(*stmt.update, out);
        indent_--;
    }

    writeLine(out, "Body");
    indent_++;
    printStmt(*stmt.body, out);
    indent_--;

    indent_--;
}

void AstPrinter::printSwitchStmt(const SwitchStmt& stmt, std::ostream& out) {
    writeLine(out, "SwitchStmt");

    indent_++;

    writeLine(out, "Expression");
    indent_++;
    printExpr(*stmt.expression, out);
    indent_--;

    for (const SwitchCase& switchCase : stmt.cases) {
        writeLine(out, "Case value=" + std::to_string(switchCase.value));

        indent_++;

        for (const auto& statement : switchCase.statements) {
            printStmt(*statement, out);
        }

        indent_--;
    }

    if (stmt.hasDefault) {
        writeLine(out, "Default");

        indent_++;

        for (const auto& statement : stmt.defaultStatements) {
            printStmt(*statement, out);
        }

        indent_--;
    }

    indent_--;
}

void AstPrinter::printReturnStmt(const ReturnStmt& stmt, std::ostream& out) {
    writeLine(out, "ReturnStmt");

    if (stmt.value) {
        indent_++;
        printExpr(*stmt.value, out);
        indent_--;
    }
}

void AstPrinter::printPrintStmt(const PrintStmt& stmt, std::ostream& out) {
    writeLine(out, "PrintStmt");

    indent_++;

    for (const auto& argument : stmt.arguments) {
        printExpr(*argument, out);
    }

    indent_--;
}

void AstPrinter::printExprStmt(const ExprStmt& stmt, std::ostream& out) {
    writeLine(out, "ExprStmt");

    indent_++;
    printExpr(*stmt.expression, out);
    indent_--;
}

void AstPrinter::printExpr(const Expr& expr, std::ostream& out) {
    if (auto* e = dynamic_cast<const ArrayLiteralExpr*>(&expr)) {
        printArrayLiteralExpr(*e, out);
        return;
    }

    if (auto* e = dynamic_cast<const IndexExpr*>(&expr)) {
        printIndexExpr(*e, out);
        return;
    }

    if (auto* e = dynamic_cast<const StructLiteralExpr*>(&expr)) {
        writeLine(
            out,
            "StructLiteralExpr name=" + e->structName +
            " type=" + typeToString(e->inferredType)
        );

        indent_++;

        for (const StructLiteralField& field : e->fields) {
            writeLine(
                out,
                "FieldInit name=" + field.name +
                " fieldIndex=" + std::to_string(field.fieldIndex)
            );

            indent_++;
            printExpr(*field.value, out);
            indent_--;
        }

        indent_--;
        return;
    }

    if (auto* e = dynamic_cast<const FieldAccessExpr*>(&expr)) {
        writeLine(
            out,
            "FieldAccessExpr object=" + e->objectName +
            " field=" + e->fieldName +
            " localIndex=" + std::to_string(e->localIndex) +
            " fieldIndex=" + std::to_string(e->fieldIndex) +
            " type=" + typeToString(e->inferredType)
        );
        return;
    }

    if (auto* e = dynamic_cast<const IntLiteralExpr*>(&expr)) {
        writeLine(out, "IntLiteralExpr value=" + std::to_string(e->value));
        return;
    }

    if (auto* e = dynamic_cast<const FloatLiteralExpr*>(&expr)) {
        std::ostringstream ss;
        ss << e->value;
        writeLine(out, "FloatLiteralExpr value=" + ss.str());
        return;
    }

    if (auto* e = dynamic_cast<const BoolLiteralExpr*>(&expr)) {
        writeLine(out, std::string("BoolLiteralExpr value=") + (e->value ? "true" : "false"));
        return;
    }

    if (auto* e = dynamic_cast<const StringLiteralExpr*>(&expr)) {
        writeLine(out, "StringLiteralExpr value=\"" + e->value + "\"");
        return;
    }

    if (auto* e = dynamic_cast<const VariableExpr*>(&expr)) {
        writeLine(
            out,
            "VariableExpr name=" + e->name +
            " localIndex=" + std::to_string(e->localIndex) +
            " type=" + typeToString(e->inferredType)
        );
        return;
    }

    if (auto* e = dynamic_cast<const CallExpr*>(&expr)) {
        printCallExpr(*e, out);
        return;
    }

    if (auto* e = dynamic_cast<const BinaryExpr*>(&expr)) {
        printBinaryExpr(*e, out);
        return;
    }

    if (auto* e = dynamic_cast<const UnaryExpr*>(&expr)) {
        printUnaryExpr(*e, out);
        return;
    }

    writeLine(out, "UnknownExpr");
}

void AstPrinter::printArrayLiteralExpr(const ArrayLiteralExpr& expr, std::ostream& out) {
    writeLine(
        out,
        "ArrayLiteralExpr size=" + std::to_string(expr.elements.size()) +
        " type=" + typeToString(expr.inferredType)
    );

    indent_++;

    for (const auto& element : expr.elements) {
        printExpr(*element, out);
    }

    indent_--;
}

void AstPrinter::printIndexExpr(const IndexExpr& expr, std::ostream& out) {
    writeLine(
        out,
        "IndexExpr array=" + expr.arrayName +
        " localIndex=" + std::to_string(expr.localIndex)
    );

    indent_++;
    printExpr(*expr.index, out);
    indent_--;
}

void AstPrinter::printBinaryExpr(const BinaryExpr& expr, std::ostream& out) {
    writeLine(
        out,
        "BinaryExpr op=" + binaryOpToString(expr.op) +
        " type=" + typeToString(expr.inferredType)
    );

    indent_++;
    printExpr(*expr.left, out);
    printExpr(*expr.right, out);
    indent_--;
}

void AstPrinter::printUnaryExpr(const UnaryExpr& expr, std::ostream& out) {
    writeLine(
        out,
        "UnaryExpr op=" + unaryOpToString(expr.op) +
        " type=" + typeToString(expr.inferredType)
    );

    indent_++;
    printExpr(*expr.operand, out);
    indent_--;
}

void AstPrinter::printCallExpr(const CallExpr& expr, std::ostream& out) {
    writeLine(
        out,
        "CallExpr callee=" + expr.callee +
        " functionIndex=" + std::to_string(expr.functionIndex) +
        " type=" + typeToString(expr.inferredType)
    );

    indent_++;

    for (const auto& argument : expr.arguments) {
        printExpr(*argument, out);
    }

    indent_--;
}

} 
