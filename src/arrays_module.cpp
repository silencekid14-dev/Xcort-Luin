#include "arrays_module.h"
#include <algorithm>
#include <stdexcept>

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
    throw std::runtime_error("arrays.sort(): array elements must all be numbers to sort numerically");
}

bool valuesEqualSimple(const Value& a, const Value& b) {
    if (a.index() != b.index()) return false;
    if (std::holds_alternative<std::string>(a)) return std::get<std::string>(a) == std::get<std::string>(b);
    if (std::holds_alternative<int>(a)) return std::get<int>(a) == std::get<int>(b);
    if (std::holds_alternative<double>(a)) return std::get<double>(a) == std::get<double>(b);
    if (std::holds_alternative<bool>(a)) return std::get<bool>(a) == std::get<bool>(b);
    return false;
}

} // namespace

std::shared_ptr<Module> createArraysModule() {
    auto mod = std::make_shared<Module>();
    mod->name = "arrays";

    mod->members["push"] = makeNative("push", [](std::vector<Value>& args) -> Value {
        if (args.size() != 2)
            throw std::runtime_error("arrays.push() expects 2 arguments (array, value)");
        auto src = arrArg(args, 0, "push");
        auto out = std::make_shared<ValueArray>(src->elements);
        out->elements.push_back(args[1]);
        return out;
    });

    mod->members["pop"] = makeNative("pop", [](std::vector<Value>& args) -> Value {
        if (args.size() != 1)
            throw std::runtime_error("arrays.pop() expects 1 argument (array)");
        auto src = arrArg(args, 0, "pop");
        auto out = std::make_shared<ValueArray>(src->elements);
        if (!out->elements.empty()) out->elements.pop_back();
        return out;
    });

    mod->members["reverse"] = makeNative("reverse", [](std::vector<Value>& args) -> Value {
        if (args.size() != 1)
            throw std::runtime_error("arrays.reverse() expects 1 argument (array)");
        auto src = arrArg(args, 0, "reverse");
        auto out = std::make_shared<ValueArray>(src->elements);
        std::reverse(out->elements.begin(), out->elements.end());
        return out;
    });

    mod->members["sort"] = makeNative("sort", [](std::vector<Value>& args) -> Value {
        if (args.size() != 1)
            throw std::runtime_error("arrays.sort() expects 1 argument (array)");
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
        if (args.size() != 2)
            throw std::runtime_error("arrays.contains() expects 2 arguments (array, value)");
        auto src = arrArg(args, 0, "contains");
        for (const auto& el : src->elements)
            if (valuesEqualSimple(el, args[1])) return true;
        return false;
    });

    return mod;
}

} // namespace luin
