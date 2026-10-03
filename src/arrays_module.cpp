#include "arrays_module.h"
#include <algorithm>
#include <stdexcept>
#include <cmath>
#include <numeric>
#include <set>

namespace luin {

namespace {

Value makeNative(std::string name, std::function<Value(std::vector<Value>&)> fn) {
    auto nf = std::make_shared<NativeFunction>();
    nf->name = std::move(name);
    nf->fn = std::move(fn);
    return nf;
}

std::shared_ptr<ValueArray> arrArg(const std::vector<Value>& args, size_t i, const std::string& fnName) {
    if (i >= args.size() || !std::holds_alternative<std::shared_ptr<ValueArray>>(args[i]))
        throw std::runtime_error("arrays." + fnName + "(): argument " + std::to_string(i + 1) +
                                 " must be an array");
    return std::get<std::shared_ptr<ValueArray>>(args[i]);
}

double asNumeric(const Value& v) {
    if (std::holds_alternative<int>(v)) return std::get<int>(v);
    if (std::holds_alternative<double>(v)) return std::get<double>(v);
    throw std::runtime_error("arrays: array elements must all be numbers for this operation");
}

bool valuesEqualSimple(const Value& a, const Value& b) {
    if (a.index() != b.index()) {
        if ((std::holds_alternative<int>(a) || std::holds_alternative<double>(a)) &&
            (std::holds_alternative<int>(b) || std::holds_alternative<double>(b)))
            return asNumeric(a) == asNumeric(b);
        return false;
    }
    if (std::holds_alternative<std::string>(a)) return std::get<std::string>(a) == std::get<std::string>(b);
    if (std::holds_alternative<int>(a)) return std::get<int>(a) == std::get<int>(b);
    if (std::holds_alternative<double>(a)) return std::get<double>(a) == std::get<double>(b);
    if (std::holds_alternative<bool>(a)) return std::get<bool>(a) == std::get<bool>(b);
    return false;
}

int asInt(const Value& v, const std::string& fn) {
    if (std::holds_alternative<int>(v)) return std::get<int>(v);
    if (std::holds_alternative<double>(v)) return static_cast<int>(std::get<double>(v));
    throw std::runtime_error("arrays." + fn + "(): expected number");
}

} // namespace

std::shared_ptr<Module> createArraysModule() {
    auto mod = std::make_shared<Module>();
    mod->name = "arrays";

    mod->members["push"] = makeNative("push", [](std::vector<Value>& args) -> Value {
        if (args.size() != 2) throw std::runtime_error("arrays.push() expects 2 arguments (array, value)");
        auto src = arrArg(args, 0, "push");
        auto out = std::make_shared<ValueArray>(src->elements);
        out->elements.push_back(args[1]);
        return out;
    });
    mod->members["pop"] = makeNative("pop", [](std::vector<Value>& args) -> Value {
        if (args.size() != 1) throw std::runtime_error("arrays.pop() expects 1 argument (array)");
        auto src = arrArg(args, 0, "pop");
        auto out = std::make_shared<ValueArray>(src->elements);
        if (!out->elements.empty()) out->elements.pop_back();
        return out;
    });
    mod->members["reverse"] = makeNative("reverse", [](std::vector<Value>& args) -> Value {
        if (args.size() != 1) throw std::runtime_error("arrays.reverse() expects 1 argument (array)");
        auto src = arrArg(args, 0, "reverse");
        auto out = std::make_shared<ValueArray>(src->elements);
        std::reverse(out->elements.begin(), out->elements.end());
        return out;
    });
    mod->members["sort"] = makeNative("sort", [](std::vector<Value>& args) -> Value {
        if (args.size() != 1) throw std::runtime_error("arrays.sort() expects 1 argument (array)");
        auto src = arrArg(args, 0, "sort");
        auto out = std::make_shared<ValueArray>(src->elements);
        if (!out->elements.empty() && std::holds_alternative<std::string>(out->elements[0])) {
            std::sort(out->elements.begin(), out->elements.end(),
                      [](const Value& a, const Value& b) {
                          return std::get<std::string>(a) < std::get<std::string>(b);
                      });
        } else {
            std::sort(out->elements.begin(), out->elements.end(),
                      [](const Value& a, const Value& b) { return asNumeric(a) < asNumeric(b); });
        }
        return out;
    });
    mod->members["contains"] = makeNative("contains", [](std::vector<Value>& args) -> Value {
        if (args.size() != 2) throw std::runtime_error("arrays.contains() expects 2 arguments (array, value)");
        auto src = arrArg(args, 0, "contains");
        for (const auto& el : src->elements)
            if (valuesEqualSimple(el, args[1])) return true;
        return false;
    });

    // Complex / extended features
    mod->members["length"] = makeNative("length", [](std::vector<Value>& args) -> Value {
        return static_cast<int>(arrArg(args, 0, "length")->elements.size());
    });
    mod->members["first"] = makeNative("first", [](std::vector<Value>& args) -> Value {
        auto a = arrArg(args, 0, "first");
        if (a->elements.empty()) throw std::runtime_error("arrays.first(): empty array");
        return a->elements.front();
    });
    mod->members["last"] = makeNative("last", [](std::vector<Value>& args) -> Value {
        auto a = arrArg(args, 0, "last");
        if (a->elements.empty()) throw std::runtime_error("arrays.last(): empty array");
        return a->elements.back();
    });
    mod->members["slice"] = makeNative("slice", [](std::vector<Value>& args) -> Value {
        if (args.size() < 3) throw std::runtime_error("arrays.slice() expects (arr, start, end)");
        auto src = arrArg(args, 0, "slice");
        int start = asInt(args[1], "slice");
        int end = asInt(args[2], "slice");
        int n = static_cast<int>(src->elements.size());
        if (start < 0) start = 0;
        if (end > n) end = n;
        auto out = std::make_shared<ValueArray>();
        for (int i = start; i < end; ++i) out->elements.push_back(src->elements[i]);
        return out;
    });
    mod->members["concat"] = makeNative("concat", [](std::vector<Value>& args) -> Value {
        if (args.size() != 2) throw std::runtime_error("arrays.concat() expects (arr1, arr2)");
        auto a = arrArg(args, 0, "concat");
        auto b = arrArg(args, 1, "concat");
        auto out = std::make_shared<ValueArray>(a->elements);
        out->elements.insert(out->elements.end(), b->elements.begin(), b->elements.end());
        return out;
    });
    mod->members["unique"] = makeNative("unique", [](std::vector<Value>& args) -> Value {
        auto src = arrArg(args, 0, "unique");
        auto out = std::make_shared<ValueArray>();
        for (const auto& el : src->elements) {
            bool found = false;
            for (const auto& o : out->elements)
                if (valuesEqualSimple(el, o)) { found = true; break; }
            if (!found) out->elements.push_back(el);
        }
        return out;
    });
    mod->members["flatten"] = makeNative("flatten", [](std::vector<Value>& args) -> Value {
        auto src = arrArg(args, 0, "flatten");
        auto out = std::make_shared<ValueArray>();
        for (const auto& el : src->elements) {
            if (std::holds_alternative<std::shared_ptr<ValueArray>>(el)) {
                auto inner = std::get<std::shared_ptr<ValueArray>>(el);
                out->elements.insert(out->elements.end(), inner->elements.begin(), inner->elements.end());
            } else {
                out->elements.push_back(el);
            }
        }
        return out;
    });
    mod->members["sum"] = makeNative("sum", [](std::vector<Value>& args) -> Value {
        auto src = arrArg(args, 0, "sum");
        double s = 0;
        bool allInt = true;
        for (const auto& el : src->elements) {
            if (!std::holds_alternative<int>(el)) allInt = false;
            s += asNumeric(el);
        }
        if (allInt) return static_cast<int>(s);
        return s;
    });
    mod->members["avg"] = makeNative("avg", [](std::vector<Value>& args) -> Value {
        auto src = arrArg(args, 0, "avg");
        if (src->elements.empty()) throw std::runtime_error("arrays.avg(): empty");
        double s = 0;
        for (const auto& el : src->elements) s += asNumeric(el);
        return s / static_cast<double>(src->elements.size());
    });
    mod->members["min"] = makeNative("min", [](std::vector<Value>& args) -> Value {
        auto src = arrArg(args, 0, "min");
        if (src->elements.empty()) throw std::runtime_error("arrays.min(): empty");
        Value best = src->elements[0];
        for (size_t i = 1; i < src->elements.size(); ++i)
            if (asNumeric(src->elements[i]) < asNumeric(best)) best = src->elements[i];
        return best;
    });
    mod->members["max"] = makeNative("max", [](std::vector<Value>& args) -> Value {
        auto src = arrArg(args, 0, "max");
        if (src->elements.empty()) throw std::runtime_error("arrays.max(): empty");
        Value best = src->elements[0];
        for (size_t i = 1; i < src->elements.size(); ++i)
            if (asNumeric(src->elements[i]) > asNumeric(best)) best = src->elements[i];
        return best;
    });
    mod->members["index_of"] = makeNative("index_of", [](std::vector<Value>& args) -> Value {
        if (args.size() != 2) throw std::runtime_error("arrays.index_of() expects (arr, value)");
        auto src = arrArg(args, 0, "index_of");
        for (size_t i = 0; i < src->elements.size(); ++i)
            if (valuesEqualSimple(src->elements[i], args[1])) return static_cast<int>(i);
        return -1;
    });
    mod->members["fill"] = makeNative("fill", [](std::vector<Value>& args) -> Value {
        if (args.size() != 2) throw std::runtime_error("arrays.fill() expects (size, value)");
        int n = asInt(args[0], "fill");
        if (n < 0) n = 0;
        auto out = std::make_shared<ValueArray>();
        out->elements.resize(static_cast<size_t>(n), args[1]);
        return out;
    });
    mod->members["range"] = makeNative("range", [](std::vector<Value>& args) -> Value {
        if (args.size() < 1) throw std::runtime_error("arrays.range() expects (end) or (start, end [, step])");
        int start = 0, end = 0, step = 1;
        if (args.size() == 1) { end = asInt(args[0], "range"); }
        else {
            start = asInt(args[0], "range");
            end = asInt(args[1], "range");
            if (args.size() >= 3) step = asInt(args[2], "range");
        }
        if (step == 0) throw std::runtime_error("arrays.range(): step cannot be 0");
        auto out = std::make_shared<ValueArray>();
        if (step > 0) for (int i = start; i < end; i += step) out->elements.push_back(i);
        else for (int i = start; i > end; i += step) out->elements.push_back(i);
        return out;
    });
    mod->members["zip"] = makeNative("zip", [](std::vector<Value>& args) -> Value {
        if (args.size() != 2) throw std::runtime_error("arrays.zip() expects (arr1, arr2)");
        auto a = arrArg(args, 0, "zip");
        auto b = arrArg(args, 1, "zip");
        size_t n = std::min(a->elements.size(), b->elements.size());
        auto out = std::make_shared<ValueArray>();
        for (size_t i = 0; i < n; ++i) {
            auto pair = std::make_shared<ValueArray>();
            pair->elements.push_back(a->elements[i]);
            pair->elements.push_back(b->elements[i]);
            out->elements.push_back(pair);
        }
        return out;
    });
    mod->members["chunk"] = makeNative("chunk", [](std::vector<Value>& args) -> Value {
        if (args.size() != 2) throw std::runtime_error("arrays.chunk() expects (arr, size)");
        auto src = arrArg(args, 0, "chunk");
        int sz = asInt(args[1], "chunk");
        if (sz <= 0) throw std::runtime_error("arrays.chunk(): size must be > 0");
        auto out = std::make_shared<ValueArray>();
        for (size_t i = 0; i < src->elements.size(); i += static_cast<size_t>(sz)) {
            auto chunk = std::make_shared<ValueArray>();
            for (size_t j = i; j < src->elements.size() && j < i + static_cast<size_t>(sz); ++j)
                chunk->elements.push_back(src->elements[j]);
            out->elements.push_back(chunk);
        }
        return out;
    });
    mod->members["insert"] = makeNative("insert", [](std::vector<Value>& args) -> Value {
        if (args.size() != 3) throw std::runtime_error("arrays.insert() expects (arr, index, value)");
        auto src = arrArg(args, 0, "insert");
        int idx = asInt(args[1], "insert");
        auto out = std::make_shared<ValueArray>(src->elements);
        if (idx < 0) idx = 0;
        if (idx > static_cast<int>(out->elements.size())) idx = static_cast<int>(out->elements.size());
        out->elements.insert(out->elements.begin() + idx, args[2]);
        return out;
    });
    mod->members["remove_at"] = makeNative("remove_at", [](std::vector<Value>& args) -> Value {
        if (args.size() != 2) throw std::runtime_error("arrays.remove_at() expects (arr, index)");
        auto src = arrArg(args, 0, "remove_at");
        int idx = asInt(args[1], "remove_at");
        auto out = std::make_shared<ValueArray>(src->elements);
        if (idx >= 0 && idx < static_cast<int>(out->elements.size()))
            out->elements.erase(out->elements.begin() + idx);
        return out;
    });
    mod->members["repeat"] = makeNative("repeat", [](std::vector<Value>& args) -> Value {
        if (args.size() != 2) throw std::runtime_error("arrays.repeat() expects (arr, times)");
        auto src = arrArg(args, 0, "repeat");
        int times = asInt(args[1], "repeat");
        if (times < 0) times = 0;
        auto out = std::make_shared<ValueArray>();
        for (int t = 0; t < times; ++t)
            out->elements.insert(out->elements.end(), src->elements.begin(), src->elements.end());
        return out;
    });

    return mod;
}

} // namespace luin
