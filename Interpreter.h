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
#include <functional>

namespace luin {

// Forward declarations
struct ValueArray;
struct ClassInstance;
struct Class;
struct Function;
struct Module;
struct NativeFunction;

// Runtime value
using Value = std::variant<
    std::string,
    int,
    double,
    bool,
    std::shared_ptr<ValueArray>,
    std::shared_ptr<ClassInstance>,
    std::shared_ptr<Class>,
    std::shared_ptr<Function>,
    std::shared_ptr<Module>,
    std::shared_ptr<NativeFunction>
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

// A callable implemented in C++ rather than in Luin (e.g. math.sqrt).
// Modules like "math" expose their functions as these.
struct NativeFunction {
    std::string name;
    std::function<Value(std::vector<Value>&)> fn;
};

// A native namespace loaded via `import <name>`, e.g. `import math` then
// `math.sqrt(x)` / `math.pi`. Members are either NativeFunction callables
// or plain constant Values (like math.pi).
struct Module {
    std::string name;
    std::unordered_map<std::string, Value> members;
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
    void executeTryStmt(const TryStmt& stmt);
    void executeImportStmt(const ImportStmt& stmt);

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

    // Builtin get():
    //   get(e)                        -> returns the message of the last error
    //                                    caught by a try { ... } block (string,
    //                                    or "" if none)
    //   get(value, "file.ext")        -> WRITE: writes value's text form to
    //                                    that file, creating it if needed
    //   get("file.ext", varName)      -> READ: reads that file's contents into
    //                                    the (possibly new) variable varName.
    //                                    Distinguished from WRITE by the first
    //                                    argument being a literal string/f-string
    //                                    and the second a bare variable name.
    Value evaluateGetBuiltin(const CallExpr& expr);

    // Builtin del(): deletes a file or folder (recursively).
    //   del("name.ext")             -> deletes name.ext from the program's folder
    //   del("name.ext", "C:\\path") -> deletes <path>/name.ext
    //   del("folder_name")          -> deletes a folder (no extension in the name)
    Value evaluateDelBuiltin(const CallExpr& expr);

    // Builtin crt(): creates a file or folder.
    //   crt("name.ext")             -> creates an empty name.ext in the program's folder
    //   crt("name.ext", "C:\\path") -> creates it at <path>/name.ext
    //   crt("folder_name")          -> creates a folder (name has no extension)
    Value evaluateCrtBuiltin(const CallExpr& expr);

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

    // Message of the most recent error caught by a try { ... } block,
    // readable afterwards via get(e). m_hasError distinguishes "no error
    // caught yet" from "an error with an empty message".
    std::string m_lastError;
    bool m_hasError = false;
};

} // namespace luin

#endif // LUIN_INTERPRETER_H