#ifndef LUIN_AST_H
#define LUIN_AST_H

#include <memory>
#include <string>
#include <vector>

namespace luin {

// ---------- Expressions ----------
class Expr {
public:
    virtual ~Expr() = default;
};

class StringLiteral : public Expr {
public:
    std::string value;
    explicit StringLiteral(std::string value) : value(std::move(value)) {}
};

// An f-string is split at parse time into a sequence of segments:
// either literal text, or a fully parsed expression to evaluate and
// stringify at runtime (e.g. {factorial(5)}, {a + b}, {hero.isAlive()}).
class FStringLiteral : public Expr {
public:
    struct Segment {
        std::string literal;         // used when expr == nullptr
        std::unique_ptr<Expr> expr;  // used when non-null (literal is ignored)
    };
    std::vector<Segment> segments;
    FStringLiteral() = default;
    explicit FStringLiteral(std::vector<Segment> segs) : segments(std::move(segs)) {}
};

class NumberLiteral : public Expr {
public:
    int value;
    explicit NumberLiteral(int value) : value(value) {}
};

class FloatLiteral : public Expr {
public:
    double value;
    explicit FloatLiteral(double value) : value(value) {}
};

class BoolLiteral : public Expr {
public:
    bool value;
    explicit BoolLiteral(bool value) : value(value) {}
};

class VariableExpr : public Expr {
public:
    std::string name;
    explicit VariableExpr(std::string name) : name(std::move(name)) {}
};

class SelfExpr : public Expr {};

class MemberAccessExpr : public Expr {
public:
    std::unique_ptr<Expr> object;
    std::string member;
    MemberAccessExpr(std::unique_ptr<Expr> object, std::string member)
        : object(std::move(object)), member(std::move(member)) {}
};

class CallExpr : public Expr {
public:
    std::unique_ptr<Expr> callee;
    std::vector<std::unique_ptr<Expr>> arguments;
    CallExpr(std::unique_ptr<Expr> callee, std::vector<std::unique_ptr<Expr>> arguments)
        : callee(std::move(callee)), arguments(std::move(arguments)) {}
};

class ArrayLiteral : public Expr {
public:
    std::vector<std::unique_ptr<Expr>> elements;
    ArrayLiteral() = default;
    explicit ArrayLiteral(std::vector<std::unique_ptr<Expr>> elements)
        : elements(std::move(elements)) {}
};

enum class BinaryOp {
    PLUS, MINUS, STAR, SLASH, PERCENT,
    EQUAL_EQUAL, BANG_EQUAL, GREATER, GREATER_EQUAL, LESS, LESS_EQUAL
};

class BinaryOpExpr : public Expr {
public:
    BinaryOp op;
    std::unique_ptr<Expr> left, right;
    BinaryOpExpr(BinaryOp op, std::unique_ptr<Expr> left, std::unique_ptr<Expr> right)
        : op(op), left(std::move(left)), right(std::move(right)) {}
};

// Unary minus, e.g. -5, -age, -(a + b)
class UnaryMinusExpr : public Expr {
public:
    std::unique_ptr<Expr> operand;
    explicit UnaryMinusExpr(std::unique_ptr<Expr> operand) : operand(std::move(operand)) {}
};

// ---------- Statements ----------
class Stmt {
public:
    virtual ~Stmt() = default;
};

class ExprStmt : public Stmt {
public:
    std::unique_ptr<Expr> expression;
    explicit ExprStmt(std::unique_ptr<Expr> expr) : expression(std::move(expr)) {}
};

class MemberAssignmentStmt : public Stmt {
public:
    std::unique_ptr<Expr> object;
    std::string member;
    std::unique_ptr<Expr> value;
    MemberAssignmentStmt(std::unique_ptr<Expr> obj, std::string mem, std::unique_ptr<Expr> val)
        : object(std::move(obj)), member(std::move(mem)), value(std::move(val)) {}
};

class ShowStmt : public Stmt {
public:
    std::unique_ptr<Expr> expression;
    explicit ShowStmt(std::unique_ptr<Expr> e) : expression(std::move(e)) {}
};

class AssignmentStmt : public Stmt {
public:
    std::string name;
    std::unique_ptr<Expr> value;
    AssignmentStmt(std::string n, std::unique_ptr<Expr> v)
        : name(std::move(n)), value(std::move(v)) {}
};

class AskStmt : public Stmt {
public:
    std::unique_ptr<Expr> prompt;
    std::vector<std::string> variables;
    AskStmt(std::unique_ptr<Expr> p, std::vector<std::string> v)
        : prompt(std::move(p)), variables(std::move(v)) {}
};

class IfStmt : public Stmt {
public:
    std::unique_ptr<Expr> condition;
    std::unique_ptr<Stmt> thenBranch;
    std::unique_ptr<Stmt> elseBranch;   // another IfStmt or a final statement

    IfStmt(std::unique_ptr<Expr> cond, std::unique_ptr<Stmt> thenB,
           std::unique_ptr<Stmt> elseB = nullptr)
        : condition(std::move(cond)), thenBranch(std::move(thenB)),
          elseBranch(std::move(elseB)) {}
};

class BlockStmt : public Stmt {
public:
    std::vector<std::unique_ptr<Stmt>> statements;
    BlockStmt() = default;
    explicit BlockStmt(std::vector<std::unique_ptr<Stmt>> stmts)
        : statements(std::move(stmts)) {}
};

class WhileStmt : public Stmt {
public:
    std::unique_ptr<Expr> condition;
    std::unique_ptr<BlockStmt> body;
    WhileStmt(std::unique_ptr<Expr> cond, std::unique_ptr<BlockStmt> b)
        : condition(std::move(cond)), body(std::move(b)) {}
};

class ForStmt : public Stmt {
public:
    std::string iterator;
    std::unique_ptr<Expr> iterable;
    std::unique_ptr<BlockStmt> body;
    ForStmt(std::string iter, std::unique_ptr<Expr> iterable,
            std::unique_ptr<BlockStmt> b)
        : iterator(std::move(iter)), iterable(std::move(iterable)),
          body(std::move(b)) {}
};

class LoopStmt : public Stmt {
public:
    std::unique_ptr<BlockStmt> body;
    explicit LoopStmt(std::unique_ptr<BlockStmt> b) : body(std::move(b)) {}
};

class SloopStmt : public Stmt {};

// try { ... } — runs body and, if a runtime error occurs anywhere inside it,
// catches it instead of letting the program crash. The error message becomes
// available afterwards through the builtin call get(e). Control-flow
// exceptions (rtn / sloop) are NOT treated as errors and pass through.
class TryStmt : public Stmt {
public:
    std::unique_ptr<BlockStmt> body;
    explicit TryStmt(std::unique_ptr<BlockStmt> b) : body(std::move(b)) {}
};

class RtnStmt : public Stmt {
public:
    std::unique_ptr<Expr> value;   // nullptr if void
    explicit RtnStmt(std::unique_ptr<Expr> v = nullptr) : value(std::move(v)) {}
};

class FnStmt : public Stmt {
public:
    std::string name;
    std::vector<std::string> params;
    std::unique_ptr<BlockStmt> body;
    FnStmt(std::string name, std::vector<std::string> params,
           std::unique_ptr<BlockStmt> body)
        : name(std::move(name)), params(std::move(params)),
          body(std::move(body)) {}
};

class ClassStmt : public Stmt {
public:
    std::string name;
    std::vector<std::unique_ptr<FnStmt>> methods;
    ClassStmt(std::string name, std::vector<std::unique_ptr<FnStmt>> methods)
        : name(std::move(name)), methods(std::move(methods)) {}
};

// ---------- Program root ----------
class Program : public Stmt {
public:
    std::vector<std::unique_ptr<Stmt>> statements;
    Program() = default;
    explicit Program(std::vector<std::unique_ptr<Stmt>> stmts)
        : statements(std::move(stmts)) {}
};

} // namespace luin

#endif // LUIN_AST_H