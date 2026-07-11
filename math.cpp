#include "math.h"
#include <cmath>
#include <algorithm>
#include <stdexcept>

namespace luin {

namespace {

// Pulls a numeric (int or double) argument out as a double, or throws a
// clear runtime error naming the offending math function.
double numArg(const std::vector<Value>& args, size_t i, const std::string& fnName) {
    const Value& v = args[i];
    if (std::holds_alternative<int>(v))    return static_cast<double>(std::get<int>(v));
    if (std::holds_alternative<double>(v)) return std::get<double>(v);
    throw std::runtime_error("math." + fnName + "(): argument " + std::to_string(i + 1) +
                             " must be a number");
}

void requireArgCount(const std::vector<Value>& args, size_t n, const std::string& fnName) {
    if (args.size() != n)
        throw std::runtime_error("math." + fnName + "() expects " + std::to_string(n) +
                                 " argument(s), got " + std::to_string(args.size()));
}

Value makeNative(std::string name, std::function<Value(std::vector<Value>&)> fn) {
    auto nf = std::make_shared<NativeFunction>();
    nf->name = std::move(name);
    nf->fn = std::move(fn);
    return nf;
}

} // namespace

std::shared_ptr<Module> createMathModule() {
    auto mod = std::make_shared<Module>();
    mod->name = "math";

    // ---------- basic ----------
    mod->members["abs"] = makeNative("abs", [](std::vector<Value>& args) -> Value {
        requireArgCount(args, 1, "abs");
        if (std::holds_alternative<int>(args[0])) return std::abs(std::get<int>(args[0]));
        return std::fabs(numArg(args, 0, "abs"));
    });

    mod->members["pow"] = makeNative("pow", [](std::vector<Value>& args) -> Value {
        requireArgCount(args, 2, "pow");
        return std::pow(numArg(args, 0, "pow"), numArg(args, 1, "pow"));
    });

    mod->members["sqrt"] = makeNative("sqrt", [](std::vector<Value>& args) -> Value {
        requireArgCount(args, 1, "sqrt");
        double x = numArg(args, 0, "sqrt");
        if (x < 0) throw std::runtime_error("math.sqrt(): argument must be >= 0");
        return std::sqrt(x);
    });

    mod->members["floor"] = makeNative("floor", [](std::vector<Value>& args) -> Value {
        requireArgCount(args, 1, "floor");
        return static_cast<int>(std::floor(numArg(args, 0, "floor")));
    });

    mod->members["ceil"] = makeNative("ceil", [](std::vector<Value>& args) -> Value {
        requireArgCount(args, 1, "ceil");
        return static_cast<int>(std::ceil(numArg(args, 0, "ceil")));
    });

    mod->members["min"] = makeNative("min", [](std::vector<Value>& args) -> Value {
        requireArgCount(args, 2, "min");
        bool bothInt = std::holds_alternative<int>(args[0]) && std::holds_alternative<int>(args[1]);
        if (bothInt) return std::min(std::get<int>(args[0]), std::get<int>(args[1]));
        return std::min(numArg(args, 0, "min"), numArg(args, 1, "min"));
    });

    mod->members["max"] = makeNative("max", [](std::vector<Value>& args) -> Value {
        requireArgCount(args, 2, "max");
        bool bothInt = std::holds_alternative<int>(args[0]) && std::holds_alternative<int>(args[1]);
        if (bothInt) return std::max(std::get<int>(args[0]), std::get<int>(args[1]));
        return std::max(numArg(args, 0, "max"), numArg(args, 1, "max"));
    });

    // ---------- advanced ----------
    mod->members["sin"] = makeNative("sin", [](std::vector<Value>& args) -> Value {
        requireArgCount(args, 1, "sin");
        return std::sin(numArg(args, 0, "sin"));
    });

    mod->members["cos"] = makeNative("cos", [](std::vector<Value>& args) -> Value {
        requireArgCount(args, 1, "cos");
        return std::cos(numArg(args, 0, "cos"));
    });

    mod->members["tan"] = makeNative("tan", [](std::vector<Value>& args) -> Value {
        requireArgCount(args, 1, "tan");
        return std::tan(numArg(args, 0, "tan"));
    });

    mod->members["log"] = makeNative("log", [](std::vector<Value>& args) -> Value {
        requireArgCount(args, 1, "log");
        double x = numArg(args, 0, "log");
        if (x <= 0) throw std::runtime_error("math.log(): argument must be > 0");
        return std::log(x);
    });

    mod->members["exp"] = makeNative("exp", [](std::vector<Value>& args) -> Value {
        requireArgCount(args, 1, "exp");
        return std::exp(numArg(args, 0, "exp"));
    });

    // ---------- constants ----------
    mod->members["pi"] = 3.141592653589793;
    mod->members["e"]  = 2.718281828459045;

    return mod;
}

} // namespace luin
