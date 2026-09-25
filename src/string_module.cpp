#include "string_module.h"
#include <algorithm>
#include <cctype>
#include <stdexcept>

namespace luin {

namespace {

Value makeNative(std::string name, std::function<Value(std::vector<Value>&)> fn) {
    auto nf = std::make_shared<NativeFunction>();
    nf->name = std::move(name);
    nf->fn = std::move(fn);
    return nf;
}

const std::string& strArg(const std::vector<Value>& args, size_t i, const std::string& fnName) {
    if (i >= args.size() || !std::holds_alternative<std::string>(args[i]))
        throw std::runtime_error("string." + fnName + "(): argument " + std::to_string(i + 1) +
                                 " must be a string");
    return std::get<std::string>(args[i]);
}

void requireArgCount(const std::vector<Value>& args, size_t n, const std::string& fnName) {
    if (args.size() != n)
        throw std::runtime_error("string." + fnName + "() expects " + std::to_string(n) +
                                 " argument(s), got " + std::to_string(args.size()));
}

} // namespace

std::shared_ptr<Module> createStringModule() {
    auto mod = std::make_shared<Module>();
    mod->name = "string";

    mod->members["upper"] = makeNative("upper", [](std::vector<Value>& args) -> Value {
        requireArgCount(args, 1, "upper");
        std::string s = strArg(args, 0, "upper");
        std::transform(s.begin(), s.end(), s.begin(),
                       [](unsigned char c) { return std::toupper(c); });
        return s;
    });

    mod->members["lower"] = makeNative("lower", [](std::vector<Value>& args) -> Value {
        requireArgCount(args, 1, "lower");
        std::string s = strArg(args, 0, "lower");
        std::transform(s.begin(), s.end(), s.begin(),
                       [](unsigned char c) { return std::tolower(c); });
        return s;
    });

    mod->members["trim"] = makeNative("trim", [](std::vector<Value>& args) -> Value {
        requireArgCount(args, 1, "trim");
        std::string s = strArg(args, 0, "trim");
        size_t start = s.find_first_not_of(" \t\r\n");
        if (start == std::string::npos) return std::string("");
        size_t end = s.find_last_not_of(" \t\r\n");
        return s.substr(start, end - start + 1);
    });

    mod->members["split"] = makeNative("split", [](std::vector<Value>& args) -> Value {
        requireArgCount(args, 2, "split");
        const std::string& s = strArg(args, 0, "split");
        const std::string& sep = strArg(args, 1, "split");
        auto arr = std::make_shared<ValueArray>();
        if (sep.empty()) {
            for (char c : s) arr->elements.push_back(std::string(1, c));
            return arr;
        }
        size_t pos = 0, prev = 0;
        while ((pos = s.find(sep, prev)) != std::string::npos) {
            arr->elements.push_back(s.substr(prev, pos - prev));
            prev = pos + sep.size();
        }
        arr->elements.push_back(s.substr(prev));
        return arr;
    });

    mod->members["join"] = makeNative("join", [](std::vector<Value>& args) -> Value {
        requireArgCount(args, 2, "join");
        if (!std::holds_alternative<std::shared_ptr<ValueArray>>(args[0]))
            throw std::runtime_error("string.join(): first argument must be an array");
        auto arr = std::get<std::shared_ptr<ValueArray>>(args[0]);
        const std::string& sep = strArg(args, 1, "join");
        std::string out;
        for (size_t i = 0; i < arr->elements.size(); ++i) {
            if (i > 0) out += sep;
            if (std::holds_alternative<std::string>(arr->elements[i]))
                out += std::get<std::string>(arr->elements[i]);
            else if (std::holds_alternative<int>(arr->elements[i]))
                out += std::to_string(std::get<int>(arr->elements[i]));
            else if (std::holds_alternative<double>(arr->elements[i]))
                out += std::to_string(std::get<double>(arr->elements[i]));
        }
        return out;
    });

    mod->members["replace"] = makeNative("replace", [](std::vector<Value>& args) -> Value {
        requireArgCount(args, 3, "replace");
        std::string s = strArg(args, 0, "replace");
        const std::string& from = strArg(args, 1, "replace");
        const std::string& to = strArg(args, 2, "replace");
        if (from.empty()) return s;
        size_t pos = 0;
        while ((pos = s.find(from, pos)) != std::string::npos) {
            s.replace(pos, from.size(), to);
            pos += to.size();
        }
        return s;
    });

    mod->members["contains"] = makeNative("contains", [](std::vector<Value>& args) -> Value {
        requireArgCount(args, 2, "contains");
        const std::string& s = strArg(args, 0, "contains");
        const std::string& needle = strArg(args, 1, "contains");
        return s.find(needle) != std::string::npos;
    });

    return mod;
}

} // namespace luin
