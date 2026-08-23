#include "random_module.h"
#include <random>
#include <stdexcept>
#include <ctime>

namespace luin {

namespace {

Value makeNative(std::string name, std::function<Value(std::vector<Value>&)> fn) {
    auto nf = std::make_shared<NativeFunction>();
    nf->name = std::move(name);
    nf->fn = std::move(fn);
    return nf;
}

// One shared engine per process, seeded from the clock at first use so
// repeated `import random` in the same script doesn't reset the stream.
std::mt19937& engine() {
    static std::mt19937 gen(static_cast<unsigned>(std::time(nullptr)));
    return gen;
}

int asInt(const Value& v, const std::string& fnName) {
    if (std::holds_alternative<int>(v)) return std::get<int>(v);
    if (std::holds_alternative<double>(v)) return static_cast<int>(std::get<double>(v));
    throw std::runtime_error("random." + fnName + "(): argument must be a number");
}

} // namespace

std::shared_ptr<Module> createRandomModule() {
    auto mod = std::make_shared<Module>();
    mod->name = "random";

    mod->members["int"] = makeNative("int", [](std::vector<Value>& args) -> Value {
        if (args.size() != 2)
            throw std::runtime_error("random.int() expects 2 arguments (min, max)");
        int lo = asInt(args[0], "int");
        int hi = asInt(args[1], "int");
        if (lo > hi) std::swap(lo, hi);
        std::uniform_int_distribution<int> dist(lo, hi);
        return dist(engine());
    });

    mod->members["float"] = makeNative("float", [](std::vector<Value>& args) -> Value {
        std::uniform_real_distribution<double> dist(0.0, 1.0);
        return dist(engine());
    });

    mod->members["choice"] = makeNative("choice", [](std::vector<Value>& args) -> Value {
        if (args.size() != 1 || !std::holds_alternative<std::shared_ptr<ValueArray>>(args[0]))
            throw std::runtime_error("random.choice(): argument must be an array");
        auto arr = std::get<std::shared_ptr<ValueArray>>(args[0]);
        if (arr->elements.empty())
            throw std::runtime_error("random.choice(): array must not be empty");
        std::uniform_int_distribution<size_t> dist(0, arr->elements.size() - 1);
        return arr->elements[dist(engine())];
    });

    mod->members["seed"] = makeNative("seed", [](std::vector<Value>& args) -> Value {
        if (args.size() != 1)
            throw std::runtime_error("random.seed() expects 1 argument");
        engine().seed(static_cast<unsigned>(asInt(args[0], "seed")));
        return std::string("");
    });

    return mod;
}

} // namespace luin
