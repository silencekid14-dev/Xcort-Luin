#include "math.h"
#include <cmath>
#include <algorithm>
#include <stdexcept>
#include <vector>
#include <numeric>

namespace luin {

namespace {

double asNumeric(const Value& v, const std::string& fnName) {
    if (std::holds_alternative<int>(v))    return static_cast<double>(std::get<int>(v));
    if (std::holds_alternative<double>(v)) return std::get<double>(v);
    throw std::runtime_error("math." + fnName + "(): value must be a number");
}

double numArg(const std::vector<Value>& args, size_t i, const std::string& fnName) {
    return asNumeric(args[i], fnName);
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
        if (args.size() == 1 && std::holds_alternative<std::shared_ptr<ValueArray>>(args[0])) {
            auto arr = std::get<std::shared_ptr<ValueArray>>(args[0]);
            if (arr->elements.empty()) throw std::runtime_error("math.min(): array must not be empty");
            Value best = arr->elements[0];
            for (size_t i = 1; i < arr->elements.size(); ++i)
                if (asNumeric(arr->elements[i], "min") < asNumeric(best, "min")) best = arr->elements[i];
            return best;
        }
        requireArgCount(args, 2, "min");
        bool bothInt = std::holds_alternative<int>(args[0]) && std::holds_alternative<int>(args[1]);
        if (bothInt) return std::min(std::get<int>(args[0]), std::get<int>(args[1]));
        return std::min(numArg(args, 0, "min"), numArg(args, 1, "min"));
    });
    mod->members["max"] = makeNative("max", [](std::vector<Value>& args) -> Value {
        if (args.size() == 1 && std::holds_alternative<std::shared_ptr<ValueArray>>(args[0])) {
            auto arr = std::get<std::shared_ptr<ValueArray>>(args[0]);
            if (arr->elements.empty()) throw std::runtime_error("math.max(): array must not be empty");
            Value best = arr->elements[0];
            for (size_t i = 1; i < arr->elements.size(); ++i)
                if (asNumeric(arr->elements[i], "max") > asNumeric(best, "max")) best = arr->elements[i];
            return best;
        }
        requireArgCount(args, 2, "max");
        bool bothInt = std::holds_alternative<int>(args[0]) && std::holds_alternative<int>(args[1]);
        if (bothInt) return std::max(std::get<int>(args[0]), std::get<int>(args[1]));
        return std::max(numArg(args, 0, "max"), numArg(args, 1, "max"));
    });
    mod->members["sin"] = makeNative("sin", [](std::vector<Value>& args) -> Value {
        requireArgCount(args, 1, "sin"); return std::sin(numArg(args, 0, "sin"));
    });
    mod->members["cos"] = makeNative("cos", [](std::vector<Value>& args) -> Value {
        requireArgCount(args, 1, "cos"); return std::cos(numArg(args, 0, "cos"));
    });
    mod->members["tan"] = makeNative("tan", [](std::vector<Value>& args) -> Value {
        requireArgCount(args, 1, "tan"); return std::tan(numArg(args, 0, "tan"));
    });
    mod->members["log"] = makeNative("log", [](std::vector<Value>& args) -> Value {
        requireArgCount(args, 1, "log");
        double x = numArg(args, 0, "log");
        if (x <= 0) throw std::runtime_error("math.log(): argument must be > 0");
        return std::log(x);
    });
    mod->members["exp"] = makeNative("exp", [](std::vector<Value>& args) -> Value {
        requireArgCount(args, 1, "exp"); return std::exp(numArg(args, 0, "exp"));
    });
    mod->members["round"] = makeNative("round", [](std::vector<Value>& args) -> Value {
        requireArgCount(args, 1, "round");
        double x = numArg(args, 0, "round");
        return static_cast<int>(x >= 0 ? std::floor(x + 0.5) : std::ceil(x - 0.5));
    });
    mod->members["trunc"] = makeNative("trunc", [](std::vector<Value>& args) -> Value {
        requireArgCount(args, 1, "trunc");
        return static_cast<int>(std::trunc(numArg(args, 0, "trunc")));
    });
    mod->members["hypot"] = makeNative("hypot", [](std::vector<Value>& args) -> Value {
        requireArgCount(args, 2, "hypot");
        return std::hypot(numArg(args, 0, "hypot"), numArg(args, 1, "hypot"));
    });
    mod->members["log2"] = makeNative("log2", [](std::vector<Value>& args) -> Value {
        requireArgCount(args, 1, "log2");
        double x = numArg(args, 0, "log2");
        if (x <= 0) throw std::runtime_error("math.log2(): argument must be > 0");
        return std::log2(x);
    });
    mod->members["log10"] = makeNative("log10", [](std::vector<Value>& args) -> Value {
        requireArgCount(args, 1, "log10");
        double x = numArg(args, 0, "log10");
        if (x <= 0) throw std::runtime_error("math.log10(): argument must be > 0");
        return std::log10(x);
    });
    mod->members["gcd"] = makeNative("gcd", [](std::vector<Value>& args) -> Value {
        requireArgCount(args, 2, "gcd");
        long a = static_cast<long>(numArg(args, 0, "gcd"));
        long b = static_cast<long>(numArg(args, 1, "gcd"));
        a = std::abs(a); b = std::abs(b);
        while (b != 0) { long t = b; b = a % b; a = t; }
        return static_cast<int>(a);
    });
    mod->members["lcm"] = makeNative("lcm", [](std::vector<Value>& args) -> Value {
        requireArgCount(args, 2, "lcm");
        long a = static_cast<long>(numArg(args, 0, "lcm"));
        long b = static_cast<long>(numArg(args, 1, "lcm"));
        a = std::abs(a); b = std::abs(b);
        if (a == 0 || b == 0) return 0;
        long g = a, h = b;
        while (h != 0) { long t = h; h = g % h; g = t; }
        return static_cast<int>((a / g) * b);
    });
    mod->members["clamp"] = makeNative("clamp", [](std::vector<Value>& args) -> Value {
        requireArgCount(args, 3, "clamp");
        double x = numArg(args, 0, "clamp");
        double lo = numArg(args, 1, "clamp");
        double hi = numArg(args, 2, "clamp");
        double result = std::min(std::max(x, lo), hi);
        bool allInt = std::holds_alternative<int>(args[0]) &&
                      std::holds_alternative<int>(args[1]) &&
                      std::holds_alternative<int>(args[2]);
        if (allInt) return static_cast<int>(result);
        return result;
    });
    mod->members["sign"] = makeNative("sign", [](std::vector<Value>& args) -> Value {
        requireArgCount(args, 1, "sign");
        double x = numArg(args, 0, "sign");
        return (x > 0) - (x < 0);
    });

    // --- 20 new features ---
    mod->members["asin"] = makeNative("asin", [](std::vector<Value>& args) -> Value {
        requireArgCount(args, 1, "asin"); return std::asin(numArg(args, 0, "asin"));
    });
    mod->members["acos"] = makeNative("acos", [](std::vector<Value>& args) -> Value {
        requireArgCount(args, 1, "acos"); return std::acos(numArg(args, 0, "acos"));
    });
    mod->members["atan"] = makeNative("atan", [](std::vector<Value>& args) -> Value {
        requireArgCount(args, 1, "atan"); return std::atan(numArg(args, 0, "atan"));
    });
    mod->members["atan2"] = makeNative("atan2", [](std::vector<Value>& args) -> Value {
        requireArgCount(args, 2, "atan2");
        return std::atan2(numArg(args, 0, "atan2"), numArg(args, 1, "atan2"));
    });
    mod->members["sinh"] = makeNative("sinh", [](std::vector<Value>& args) -> Value {
        requireArgCount(args, 1, "sinh"); return std::sinh(numArg(args, 0, "sinh"));
    });
    mod->members["cosh"] = makeNative("cosh", [](std::vector<Value>& args) -> Value {
        requireArgCount(args, 1, "cosh"); return std::cosh(numArg(args, 0, "cosh"));
    });
    mod->members["tanh"] = makeNative("tanh", [](std::vector<Value>& args) -> Value {
        requireArgCount(args, 1, "tanh"); return std::tanh(numArg(args, 0, "tanh"));
    });
    mod->members["deg"] = makeNative("deg", [](std::vector<Value>& args) -> Value {
        requireArgCount(args, 1, "deg");
        return numArg(args, 0, "deg") * 180.0 / 3.141592653589793;
    });
    mod->members["rad"] = makeNative("rad", [](std::vector<Value>& args) -> Value {
        requireArgCount(args, 1, "rad");
        return numArg(args, 0, "rad") * 3.141592653589793 / 180.0;
    });
    mod->members["factorial"] = makeNative("factorial", [](std::vector<Value>& args) -> Value {
        requireArgCount(args, 1, "factorial");
        int n = static_cast<int>(numArg(args, 0, "factorial"));
        if (n < 0) throw std::runtime_error("math.factorial(): n >= 0");
        if (n > 20) throw std::runtime_error("math.factorial(): n too large");
        long long r = 1;
        for (int i = 2; i <= n; ++i) r *= i;
        return static_cast<int>(r);
    });
    mod->members["is_prime"] = makeNative("is_prime", [](std::vector<Value>& args) -> Value {
        requireArgCount(args, 1, "is_prime");
        int n = static_cast<int>(numArg(args, 0, "is_prime"));
        if (n < 2) return false;
        if (n == 2) return true;
        if (n % 2 == 0) return false;
        for (int i = 3; i * i <= n; i += 2)
            if (n % i == 0) return false;
        return true;
    });
    mod->members["lerp"] = makeNative("lerp", [](std::vector<Value>& args) -> Value {
        requireArgCount(args, 3, "lerp");
        double a = numArg(args, 0, "lerp"), b = numArg(args, 1, "lerp"), t = numArg(args, 2, "lerp");
        return a + (b - a) * t;
    });
    mod->members["map"] = makeNative("map", [](std::vector<Value>& args) -> Value {
        requireArgCount(args, 5, "map");
        double x = numArg(args, 0, "map");
        double inMin = numArg(args, 1, "map"), inMax = numArg(args, 2, "map");
        double outMin = numArg(args, 3, "map"), outMax = numArg(args, 4, "map");
        if (inMax == inMin) return outMin;
        return outMin + (x - inMin) * (outMax - outMin) / (inMax - inMin);
    });
    mod->members["mod"] = makeNative("mod", [](std::vector<Value>& args) -> Value {
        requireArgCount(args, 2, "mod");
        double a = numArg(args, 0, "mod"), b = numArg(args, 1, "mod");
        if (b == 0) throw std::runtime_error("math.mod(): division by zero");
        return std::fmod(a, b);
    });
    mod->members["fract"] = makeNative("fract", [](std::vector<Value>& args) -> Value {
        requireArgCount(args, 1, "fract");
        double x = numArg(args, 0, "fract");
        return x - std::floor(x);
    });
    mod->members["cbrt"] = makeNative("cbrt", [](std::vector<Value>& args) -> Value {
        requireArgCount(args, 1, "cbrt"); return std::cbrt(numArg(args, 0, "cbrt"));
    });
    mod->members["dist"] = makeNative("dist", [](std::vector<Value>& args) -> Value {
        requireArgCount(args, 4, "dist");
        double dx = numArg(args, 2, "dist") - numArg(args, 0, "dist");
        double dy = numArg(args, 3, "dist") - numArg(args, 1, "dist");
        return std::sqrt(dx * dx + dy * dy);
    });
    mod->members["sum"] = makeNative("sum", [](std::vector<Value>& args) -> Value {
        requireArgCount(args, 1, "sum");
        if (!std::holds_alternative<std::shared_ptr<ValueArray>>(args[0]))
            throw std::runtime_error("math.sum() expects an array");
        auto arr = std::get<std::shared_ptr<ValueArray>>(args[0]);
        double s = 0;
        for (const auto& e : arr->elements) s += asNumeric(e, "sum");
        return s;
    });
    mod->members["mean"] = makeNative("mean", [](std::vector<Value>& args) -> Value {
        requireArgCount(args, 1, "mean");
        if (!std::holds_alternative<std::shared_ptr<ValueArray>>(args[0]))
            throw std::runtime_error("math.mean() expects an array");
        auto arr = std::get<std::shared_ptr<ValueArray>>(args[0]);
        if (arr->elements.empty()) throw std::runtime_error("math.mean(): empty");
        double s = 0;
        for (const auto& e : arr->elements) s += asNumeric(e, "mean");
        return s / static_cast<double>(arr->elements.size());
    });
    mod->members["smoothstep"] = makeNative("smoothstep", [](std::vector<Value>& args) -> Value {
        requireArgCount(args, 3, "smoothstep");
        double edge0 = numArg(args, 0, "smoothstep");
        double edge1 = numArg(args, 1, "smoothstep");
        double x = numArg(args, 2, "smoothstep");
        if (edge0 == edge1) return x < edge0 ? 0.0 : 1.0;
        double t = std::min(std::max((x - edge0) / (edge1 - edge0), 0.0), 1.0);
        return t * t * (3.0 - 2.0 * t);
    });

    mod->members["pi"] = 3.141592653589793;
    mod->members["e"]  = 2.718281828459045;
    mod->members["tau"] = 6.283185307179586;
    mod->members["phi"] = 1.618033988749895;

    return mod;
}

} // namespace luin
