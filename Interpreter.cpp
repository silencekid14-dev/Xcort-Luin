#include "Interpreter.h"
#include <stdexcept>
#include <sstream>
#include <cctype>
#include <fstream>

namespace luin {

void Interpreter::interpret(const Program& program) {
    m_envStack.push_back({});   // global scope
    for (const auto& stmt : program.statements)
        execute(*stmt);
}
void Interpreter::execute(const Stmt& stmt) {
    if (auto* s = dynamic_cast<const ShowStmt*>(&stmt)) {
        executeShowStmt(*s);
    } else if (auto* a = dynamic_cast<const AssignmentStmt*>(&stmt)) {
        executeAssignmentStmt(*a);
    } else if (auto* ask = dynamic_cast<const AskStmt*>(&stmt)) {
        executeAskStmt(*ask);
    } else if (auto* ifStmt = dynamic_cast<const IfStmt*>(&stmt)) {
        executeIfStmt(*ifStmt);
    } else if (auto* whileStmt = dynamic_cast<const WhileStmt*>(&stmt)) {
        executeWhileStmt(*whileStmt);
    } else if (auto* forStmt = dynamic_cast<const ForStmt*>(&stmt)) {
        executeForStmt(*forStmt);
    } else if (auto* loopStmt = dynamic_cast<const LoopStmt*>(&stmt)) {
        executeLoopStmt(*loopStmt);
    } else if (dynamic_cast<const SloopStmt*>(&stmt)) {
        throw SloopException();
    } else if (auto* rtn = dynamic_cast<const RtnStmt*>(&stmt)) {
        executeRtnStmt(*rtn);
    } else if (auto* fn = dynamic_cast<const FnStmt*>(&stmt)) {
        executeFnStmt(*fn);
    } else if (auto* cls = dynamic_cast<const ClassStmt*>(&stmt)) {
        executeClassStmt(*cls);
    } else if (auto* tryStmt = dynamic_cast<const TryStmt*>(&stmt)) {
        executeTryStmt(*tryStmt);
    } else if (auto* exprStmt = dynamic_cast<const ExprStmt*>(&stmt)) {
        // Run the function/method expression and discard the return value
        evaluate(*exprStmt->expression);
    } else if (auto* memAssign = dynamic_cast<const MemberAssignmentStmt*>(&stmt)) {
        // 1. Resolve which object instance is targeted
        Value objVal = evaluate(*memAssign->object);
        if (!std::holds_alternative<std::shared_ptr<ClassInstance>>(objVal)) {
            throw std::runtime_error("Only class instances can contain mutable fields.");
        }
        auto instance = std::get<std::shared_ptr<ClassInstance>>(objVal);
        
        // 2. Evaluate the value and save it to the object's instance fields
        Value computedValue = evaluate(*memAssign->value);
        instance->fields[memAssign->member] = computedValue;
    } else { // <-- Notice how everything pairs up properly here
        throw std::runtime_error("Unknown statement type in interpreter.");
    }
}

void Interpreter::executeBlock(const BlockStmt& block) {
    for (const auto& stmt : block.statements)
        execute(*stmt);
}

Value* Interpreter::findVariable(const std::string& name) {
    for (auto it = m_envStack.rbegin(); it != m_envStack.rend(); ++it) {
        auto found = it->find(name);
        if (found != it->end()) return &found->second;
    }
    return nullptr;
}

void Interpreter::executeShowStmt(const ShowStmt& stmt) {
    Value val = evaluate(*stmt.expression);
    std::cout << valueToString(val) << std::endl;
}

void Interpreter::executeAssignmentStmt(const AssignmentStmt& stmt) {
    Value val = evaluate(*stmt.value);
    // If the variable already exists anywhere in the visible scope chain,
    // update it in place. Otherwise create it in the current (innermost) scope.
    // This matches the lookup rule used for reads (evaluateVariableExpr),
    // preventing a write from silently forking a same-named outer variable.
    if (Value* existing = findVariable(stmt.name))
        *existing = val;
    else
        m_envStack.back()[stmt.name] = val;
}

void Interpreter::executeAskStmt(const AskStmt& stmt) {
    Value promptVal = evaluate(*stmt.prompt);
    if (!std::holds_alternative<std::string>(promptVal))
        throw std::runtime_error("Prompt in ask() must be a string");
    std::string promptStr = std::get<std::string>(promptVal);
    std::cout << promptStr << std::flush;

    std::string input;
    std::getline(std::cin, input);

    std::vector<std::string> tokens;
    std::istringstream iss(input);
    std::string tok;
    while (iss >> tok) tokens.push_back(tok);

    if (tokens.size() != stmt.variables.size())
        throw std::runtime_error("ask: expected " + std::to_string(stmt.variables.size()) +
                                 " inputs, got " + std::to_string(tokens.size()));

    for (size_t i = 0; i < stmt.variables.size(); ++i) {
        // Search the whole scope chain (same rule as reads/assignments) instead
        // of only the innermost frame, so ask() can update outer/global variables.
        Value* found = findVariable(stmt.variables[i]);
        if (!found)
            throw std::runtime_error("Undefined variable '" + stmt.variables[i] + "' in ask");
        Value& old = *found;
        const std::string& token = tokens[i];

        if (std::holds_alternative<int>(old)) {
            try {
                size_t pos;
                int val = std::stoi(token, &pos);
                if (pos != token.size()) throw std::invalid_argument("");
                old = val;
            } catch (...) {
                throw std::runtime_error("Cannot put '" + token + "' into integer variable");
            }
        } else if (std::holds_alternative<double>(old)) {
            try {
                size_t pos;
                double val = std::stod(token, &pos);
                if (pos != token.size()) throw std::invalid_argument("");
                old = val;
            } catch (...) {
                throw std::runtime_error("Cannot put '" + token + "' into float variable");
            }
        } else if (std::holds_alternative<bool>(old)) {
            if (token == "true") old = true;
            else if (token == "false") old = false;
            else throw std::runtime_error("Cannot put '" + token + "' into boolean (true/false only)");
        } else {
            old = token; // string
        }
    }
}

void Interpreter::executeIfStmt(const IfStmt& stmt) {
    if (isTruthy(evaluate(*stmt.condition)))
        execute(*stmt.thenBranch);
    else if (stmt.elseBranch)
        execute(*stmt.elseBranch);
}

void Interpreter::executeWhileStmt(const WhileStmt& stmt) {
    while (isTruthy(evaluate(*stmt.condition))) {
        try {
            executeBlock(*stmt.body);
        } catch (const SloopException&) {
            break;
        } catch (const ReturnException&) {
            throw; // propagate
        }
    }
}

void Interpreter::executeForStmt(const ForStmt& stmt) {
    Value iterableVal = evaluate(*stmt.iterable);
    if (!std::holds_alternative<std::shared_ptr<ValueArray>>(iterableVal))
        throw std::runtime_error("For loop iterable must be an array");
    auto arrPtr = std::get<std::shared_ptr<ValueArray>>(iterableVal);
    for (const auto& elem : arrPtr->elements) {
        m_envStack.back()[stmt.iterator] = elem;
        try {
            executeBlock(*stmt.body);
        } catch (const SloopException&) {
            break;
        } catch (const ReturnException&) {
            throw;
        }
    }
}

void Interpreter::executeLoopStmt(const LoopStmt& stmt) {
    while (true) {
        try {
            executeBlock(*stmt.body);
        } catch (const SloopException&) {
            break;
        } catch (const ReturnException&) {
            throw;
        }
    }
}

void Interpreter::executeTryStmt(const TryStmt& stmt) {
    try {
        executeBlock(*stmt.body);
        // Ran clean: clear any earlier error so get(e) doesn't report stale info.
        m_lastError.clear();
        m_hasError = false;
    } catch (const ReturnException&) {
        throw;  // control flow (rtn), not a real error — must propagate
    } catch (const SloopException&) {
        throw;  // control flow (sloop break), not a real error — must propagate
    } catch (const std::exception& e) {
        // A genuine runtime error: remember its message for get(e) and swallow
        // it here so the program keeps running instead of crashing.
        m_lastError = e.what();
        m_hasError = true;
    }
}

void Interpreter::executeRtnStmt(const RtnStmt& stmt) {
    if (stmt.value)
        throw ReturnException(evaluate(*stmt.value));
    else
        throw ReturnException(Value(std::string("")));  // void return
}

void Interpreter::executeFnStmt(const FnStmt& stmt) {
    auto func = std::make_shared<Function>();
    func->def = const_cast<FnStmt*>(&stmt);
    m_envStack.back()[stmt.name] = func;
}

void Interpreter::executeClassStmt(const ClassStmt& stmt) {
    auto cls = std::make_shared<Class>();
    cls->name = stmt.name;
    for (const auto& method : stmt.methods)
        cls->methods[method->name] = method.get();  // raw pointer (safe as long as program lives)
    m_envStack.back()[stmt.name] = cls;
    m_classes[stmt.name] = cls;
}

// ---------- Expression evaluation ----------

Value Interpreter::evaluate(const Expr& expr) {
    if (auto* s = dynamic_cast<const StringLiteral*>(&expr))
        return evaluateStringLiteral(*s);
    if (auto* f = dynamic_cast<const FStringLiteral*>(&expr))
        return evaluateFStringLiteral(*f);
    if (auto* n = dynamic_cast<const NumberLiteral*>(&expr))
        return evaluateNumberLiteral(*n);
    if (auto* fl = dynamic_cast<const FloatLiteral*>(&expr))
        return evaluateFloatLiteral(*fl);
    if (auto* b = dynamic_cast<const BoolLiteral*>(&expr))
        return evaluateBoolLiteral(*b);
    if (auto* v = dynamic_cast<const VariableExpr*>(&expr))
        return evaluateVariableExpr(*v);
    if (auto* self = dynamic_cast<const SelfExpr*>(&expr))
        return evaluateSelfExpr(*self);
    if (auto* mem = dynamic_cast<const MemberAccessExpr*>(&expr))
        return evaluateMemberAccessExpr(*mem);
    if (auto* call = dynamic_cast<const CallExpr*>(&expr))
        return evaluateCallExpr(*call);
    if (auto* arr = dynamic_cast<const ArrayLiteral*>(&expr))
        return evaluateArrayLiteral(*arr);
    if (auto* bin = dynamic_cast<const BinaryOpExpr*>(&expr))
        return evaluateBinaryOp(*bin);
    if (auto* un = dynamic_cast<const UnaryMinusExpr*>(&expr))
        return evaluateUnaryMinusExpr(*un);
    throw std::runtime_error("Unknown expression type in interpreter.");
}

Value Interpreter::evaluateStringLiteral(const StringLiteral& expr) {
    return expr.value;
}

Value Interpreter::evaluateFStringLiteral(const FStringLiteral& expr) {
    std::ostringstream result;
    for (const auto& segment : expr.segments) {
        if (segment.expr) {
            // Any expression is supported here: variables, member access,
            // arithmetic, unary minus, and function/method calls like
            // {factorial(5)} or {hero.isAlive()}.
            result << valueToString(evaluate(*segment.expr));
        } else {
            result << segment.literal;
        }
    }
    return result.str();
}

Value Interpreter::evaluateNumberLiteral(const NumberLiteral& expr) {
    return expr.value;
}

Value Interpreter::evaluateFloatLiteral(const FloatLiteral& expr) {
    return expr.value;
}

Value Interpreter::evaluateBoolLiteral(const BoolLiteral& expr) {
    return expr.value;
}

Value Interpreter::evaluateVariableExpr(const VariableExpr& expr) {
    if (Value* found = findVariable(expr.name)) return *found;
    throw std::runtime_error("Undefined variable '" + expr.name + "'");
}

Value Interpreter::evaluateSelfExpr(const SelfExpr& expr) {
    (void)expr;
    if (Value* found = findVariable("self")) return *found;
    throw std::runtime_error("'self' used outside of a method");
}

Value Interpreter::evaluateMemberAccessExpr(const MemberAccessExpr& expr) {
    Value obj = evaluate(*expr.object);
    if (std::holds_alternative<std::shared_ptr<ClassInstance>>(obj)) {
        auto inst = std::get<std::shared_ptr<ClassInstance>>(obj);
        
        // 1. Check if it's a field variable (e.g., p.hp)
        auto it = inst->fields.find(expr.member);
        if (it != inst->fields.end()) return it->second;
        
        // 2. Check if it's a class method (e.g., p.takeDamage)
        if (inst->klass && inst->klass->methods.find(expr.member) != inst->klass->methods.end()) {
            luin::FnStmt* methodStmt = inst->klass->methods[expr.member];
            
            // Wrap it into a callable Function object bound to this specific instance
            auto methodCallable = std::make_shared<Function>(methodStmt, inst); 
            return methodCallable;
        }
        
        throw std::runtime_error("Field or Method '" + expr.member + "' not found");
    }
    throw std::runtime_error("Only class instances have fields and methods");
}
Value Interpreter::evaluateGetBuiltin(const CallExpr& expr) {
    const auto& args = expr.arguments;

    if (args.size() == 1) {
        // get(e) — only the literal identifier 'e' is accepted here; it reads
        // the message from the most recent error caught by a try { ... } block.
        auto* var = dynamic_cast<const VariableExpr*>(args[0].get());
        if (!var || var->name != "e")
            throw std::runtime_error(
                "get() with a single argument must be written as get(e), "
                "to read the last error caught by a try block");
        return m_hasError ? m_lastError : std::string("");
    }

    if (args.size() == 2) {
        // get(value, "filename.ext") — writes value's text form to that file,
        // creating the file (with whatever extension is given) if it doesn't exist.
        Value data = evaluate(*args[0]);
        Value fileVal = evaluate(*args[1]);
        if (!std::holds_alternative<std::string>(fileVal))
            throw std::runtime_error("get(value, filename): filename must be a string");
        const std::string& filename = std::get<std::string>(fileVal);

        std::ofstream out(filename, std::ios::out | std::ios::trunc);
        if (!out.is_open())
            throw std::runtime_error("get(): could not create or open file '" + filename + "'");
        out << valueToString(data);
        out.close();

        return std::string("");  // void-style return, like rtn with no value
    }

    throw std::runtime_error(
        "get() expects either 1 argument (get(e)) or 2 arguments "
        "(get(value, \"filename\"))");
}

Value Interpreter::evaluateCallExpr(const CallExpr& expr) {
    // 'get' is a reserved builtin, handled before normal variable/function
    // resolution so the user never has to (and can't) define their own 'get'.
    if (auto* calleeVar = dynamic_cast<const VariableExpr*>(expr.callee.get())) {
        if (calleeVar->name == "get")
            return evaluateGetBuiltin(expr);
    }

    Value callee = evaluate(*expr.callee);
    std::vector<Value> args;
    for (const auto& arg : expr.arguments)
        args.push_back(evaluate(*arg));

    if (std::holds_alternative<std::shared_ptr<Function>>(callee)) {
        auto func = std::get<std::shared_ptr<Function>>(callee);
        FnStmt* def = func->def;
        if (args.size() != def->params.size())
            throw std::runtime_error("Function '" + def->name + "' expects " +
                                     std::to_string(def->params.size()) + " arguments, got " +
                                     std::to_string(args.size()));
        
        m_envStack.push_back({});
        auto& newEnv = m_envStack.back();

        // ==========================================
        // >>> START OF THE FIX <<<
        // If this function is a method bound to an instance, 
        // make 'self' available inside its execution context.
        if (func->closure != nullptr) {
            newEnv["self"] = func->closure;
        }
        // >>> END OF THE FIX <<<
        // ==========================================

        for (size_t i = 0; i < def->params.size(); ++i)
            newEnv[def->params[i]] = args[i];
        try {
            executeBlock(*def->body);
            m_envStack.pop_back();
            return Value(std::string("")); // void
        } catch (const ReturnException& ret) {
            m_envStack.pop_back();
            return ret.value;
        }
    }
    else if (std::holds_alternative<std::shared_ptr<Class>>(callee)) {
        auto cls = std::get<std::shared_ptr<Class>>(callee);
        auto initIt = cls->methods.find("init");
        if (initIt == cls->methods.end())
            throw std::runtime_error("Class '" + cls->name + "' has no init method");
        FnStmt* init = initIt->second;
        auto inst = std::make_shared<ClassInstance>();
        inst->klass = cls;
        m_envStack.push_back({});
        m_envStack.back()["self"] = inst;
        if (args.size() != init->params.size())
            throw std::runtime_error("init expects " + std::to_string(init->params.size()) +
                                     " arguments, got " + std::to_string(args.size()));
        for (size_t i = 0; i < init->params.size(); ++i)
            m_envStack.back()[init->params[i]] = args[i];
        try {
            executeBlock(*init->body);
            m_envStack.pop_back();
            return inst;
        } catch (const ReturnException&) {
            m_envStack.pop_back();
            return inst; // ignore return value from init
        }
    }
    throw std::runtime_error("Cannot call a non-function value");
}

Value Interpreter::evaluateUnaryMinusExpr(const UnaryMinusExpr& expr) {
    Value v = evaluate(*expr.operand);
    if (std::holds_alternative<int>(v)) return -std::get<int>(v);
    if (std::holds_alternative<double>(v)) return -std::get<double>(v);
    throw std::runtime_error("Unary '-' requires a numeric operand");
}

Value Interpreter::evaluateArrayLiteral(const ArrayLiteral& expr) {
    auto arr = std::make_shared<ValueArray>();
    for (const auto& elem : expr.elements)
        arr->elements.push_back(evaluate(*elem));
    return arr;
}

double Interpreter::toNumeric(const Value& value) {
    if (std::holds_alternative<int>(value))    return std::get<int>(value);
    if (std::holds_alternative<double>(value)) return std::get<double>(value);
    throw std::runtime_error("Numeric value expected");
}

Value Interpreter::evaluateBinaryOp(const BinaryOpExpr& expr) {
    Value leftVal = evaluate(*expr.left);
    Value rightVal = evaluate(*expr.right);

    switch (expr.op) {
        case BinaryOp::EQUAL_EQUAL:
            return valuesEqual(leftVal, rightVal);
        case BinaryOp::BANG_EQUAL:
            return !valuesEqual(leftVal, rightVal);
        case BinaryOp::GREATER:
        case BinaryOp::GREATER_EQUAL:
        case BinaryOp::LESS:
        case BinaryOp::LESS_EQUAL: {
            double l = toNumeric(leftVal), r = toNumeric(rightVal);
            switch (expr.op) {
                case BinaryOp::GREATER:       return l > r;
                case BinaryOp::GREATER_EQUAL: return l >= r;
                case BinaryOp::LESS:          return l < r;
                case BinaryOp::LESS_EQUAL:    return l <= r;
                default: break;
            }
        }
        default: break;
    }

    // Arithmetic
    bool leftInt = std::holds_alternative<int>(leftVal);
    bool rightInt = std::holds_alternative<int>(rightVal);
    bool leftDouble = std::holds_alternative<double>(leftVal);
    bool rightDouble = std::holds_alternative<double>(rightVal);

    if (!(leftInt || leftDouble) || !(rightInt || rightDouble))
        throw std::runtime_error("Arithmetic requires numeric operands");

    double l = toNumeric(leftVal), r = toNumeric(rightVal);

    switch (expr.op) {
        case BinaryOp::PLUS:
            if (leftInt && rightInt) return std::get<int>(leftVal) + std::get<int>(rightVal);
            else return l + r;
        case BinaryOp::MINUS:
            if (leftInt && rightInt) return std::get<int>(leftVal) - std::get<int>(rightVal);
            else return l - r;
        case BinaryOp::STAR:
            if (leftInt && rightInt) return std::get<int>(leftVal) * std::get<int>(rightVal);
            else return l * r;
        case BinaryOp::SLASH:
            if (r == 0) throw std::runtime_error("Division by zero");
            return l / r;
        case BinaryOp::PERCENT:
            if (leftInt && rightInt) {
                int ri = std::get<int>(rightVal);
                if (ri == 0) throw std::runtime_error("Modulo by zero");
                return std::get<int>(leftVal) % ri;
            } else throw std::runtime_error("Modulus (%) requires integer operands");
        default:
            throw std::runtime_error("Unknown binary operator");
    }
}

bool Interpreter::valuesEqual(const Value& a, const Value& b) const {
    bool aNum = std::holds_alternative<int>(a) || std::holds_alternative<double>(a);
    bool bNum = std::holds_alternative<int>(b) || std::holds_alternative<double>(b);

    // Numbers compare by numeric value regardless of int vs double (e.g. 9 == 9.0).
    if (aNum && bNum) return toNumeric(a) == toNumeric(b);

    // Different underlying types (and not both numeric) are never equal.
    if (a.index() != b.index()) return false;

    // Arrays behave as value types: compare element-by-element.
    if (std::holds_alternative<std::shared_ptr<ValueArray>>(a)) {
        auto arrA = std::get<std::shared_ptr<ValueArray>>(a);
        auto arrB = std::get<std::shared_ptr<ValueArray>>(b);
        if (arrA->elements.size() != arrB->elements.size()) return false;
        for (size_t i = 0; i < arrA->elements.size(); ++i)
            if (!valuesEqual(arrA->elements[i], arrB->elements[i])) return false;
        return true;
    }

    // Everything else (string/int/double/bool compare by value; class instances,
    // classes, and functions compare by identity via shared_ptr's operator==,
    // so two distinct instances are correctly NOT equal).
    return a == b;
}

std::string Interpreter::valueToString(const Value& value) const {
    if (std::holds_alternative<std::string>(value))
        return std::get<std::string>(value);
    if (std::holds_alternative<int>(value))
        return std::to_string(std::get<int>(value));
    if (std::holds_alternative<double>(value)) {
        std::string s = std::to_string(std::get<double>(value));
        s.erase(s.find_last_not_of('0') + 1, std::string::npos);
        if (s.back() == '.') s.pop_back();
        return s;
    }
    if (std::holds_alternative<bool>(value))
        return std::get<bool>(value) ? "true" : "false";
    if (std::holds_alternative<std::shared_ptr<ValueArray>>(value)) {
        auto arr = std::get<std::shared_ptr<ValueArray>>(value);
        std::ostringstream oss; oss << "[";
        for (size_t i = 0; i < arr->elements.size(); ++i) {
            if (i > 0) oss << ", ";
            oss << valueToString(arr->elements[i]);
        }
        oss << "]";
        return oss.str();
    }
    if (std::holds_alternative<std::shared_ptr<ClassInstance>>(value))
        return "<instance>";
    if (std::holds_alternative<std::shared_ptr<Class>>(value))
        return "<class " + std::get<std::shared_ptr<Class>>(value)->name + ">";
    if (std::holds_alternative<std::shared_ptr<Function>>(value))
        return "<function>";
    return "???";
}

bool Interpreter::isTruthy(const Value& value) const {
    if (std::holds_alternative<bool>(value))
        return std::get<bool>(value);
    if (std::holds_alternative<int>(value))
        return std::get<int>(value) != 0;
    if (std::holds_alternative<double>(value))
        return std::get<double>(value) != 0.0;
    if (std::holds_alternative<std::string>(value))
        return !std::get<std::string>(value).empty();
    if (std::holds_alternative<std::shared_ptr<ValueArray>>(value))
        return !std::get<std::shared_ptr<ValueArray>>(value)->elements.empty();
    if (std::holds_alternative<std::shared_ptr<ClassInstance>>(value))
        return true;
    if (std::holds_alternative<std::shared_ptr<Class>>(value))
        return true;
    if (std::holds_alternative<std::shared_ptr<Function>>(value))
        return true;
    return false;
}

} // namespace luin