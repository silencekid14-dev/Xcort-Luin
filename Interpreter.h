#ifndef LUIN_INTERPRETER_H
#define LUIN_INTERPRETER_H

#include "AST.h"
#include <unordered_map>
#include <variant>
#include <memory>
#include <vector>
#include <string>
#include <iostream>
#include <exception>

namespace luin {

// Forward declarations
struct ValueArray;
struct ClassInstance;
struct Class;
struct Function;

// Runtime value
using Value = std::variant<
    std::string,
    int,
    double,
    bool,
    std::shared_ptr<ValueArray>,
    std::shared_ptr<ClassInstance>,
    std::shared_ptr<Class>,
    std::shared_ptr<Function>
>;

struct ValueArray {
    std::vector<Value> elements;
    ValueArray() = default;
    explicit ValueArray(std::vector<Value> elems) : elements(std::move(elems)) {}
};

struct ClassInstance {
    std::shared_ptr<Class> klass;
    std::unordered_map<std::string, Value> fields;
};

struct Class {
    std::string name;
    std::unordered_map<std::string, FnStmt*> methods;  // raw pointers to AST
};

struct Function {
    FnStmt* def;                            
    std::shared_ptr<ClassInstance> closure; 

    // Add "= nullptr" to the first parameter here:
    Function(FnStmt* d = nullptr, std::shared_ptr<ClassInstance> clos = nullptr)
        : def(d), closure(clos) {}
};

class SloopException : public std::exception {
public:
    const char* what() const noexcept override { return "Sloop break"; }
};

class ReturnException : public std::exception {
public:
    Value value;
    explicit ReturnException(Value val) : value(std::move(val)) {}
};

class Interpreter {
public:
    Interpreter() = default;
    void interpret(const Program& program);

private:
    void execute(const Stmt& stmt);
    void executeBlock(const BlockStmt& block);
    void executeShowStmt(const ShowStmt& stmt);
    void executeAssignmentStmt(const AssignmentStmt& stmt);
    void executeAskStmt(const AskStmt& stmt);
    void executeIfStmt(const IfStmt& stmt);
    void executeWhileStmt(const WhileStmt& stmt);
    void executeForStmt(const ForStmt& stmt);
    void executeLoopStmt(const LoopStmt& stmt);
    void executeRtnStmt(const RtnStmt& stmt);
    void executeFnStmt(const FnStmt& stmt);
    void executeClassStmt(const ClassStmt& stmt);

    Value evaluate(const Expr& expr);
    Value evaluateStringLiteral(const StringLiteral& expr);
    Value evaluateFStringLiteral(const FStringLiteral& expr);
    Value evaluateNumberLiteral(const NumberLiteral& expr);
    Value evaluateFloatLiteral(const FloatLiteral& expr);
    Value evaluateBoolLiteral(const BoolLiteral& expr);
    Value evaluateVariableExpr(const VariableExpr& expr);
    Value evaluateSelfExpr(const SelfExpr& expr);
    Value evaluateMemberAccessExpr(const MemberAccessExpr& expr);
    Value evaluateCallExpr(const CallExpr& expr);
    Value evaluateArrayLiteral(const ArrayLiteral& expr);
    Value evaluateBinaryOp(const BinaryOpExpr& expr);
    Value evaluateUnaryMinusExpr(const UnaryMinusExpr& expr);

    std::string valueToString(const Value& value) const;
    bool isTruthy(const Value& value) const;
    bool valuesEqual(const Value& a, const Value& b) const;
    static double toNumeric(const Value& value);

    // Looks up a variable across the whole scope stack (innermost first).
    // Returns nullptr if it isn't defined anywhere.
    Value* findVariable(const std::string& name);

    // Environment: stack of scopes (global + function locals)
    std::vector<std::unordered_map<std::string, Value>> m_envStack;
    // Global class storage (to find Class objects by name)
    std::unordered_map<std::string, std::shared_ptr<Class>> m_classes;
};

} // namespace luin

#endif // LUIN_INTERPRETER_H