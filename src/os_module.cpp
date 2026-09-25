#include "os_module.h"
#include <filesystem>
#include <stdexcept>

namespace fs = std::filesystem;

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
        throw std::runtime_error("os." + fnName + "(): argument " + std::to_string(i + 1) +
                                 " must be a string path");
    return std::get<std::string>(args[i]);
}

} // namespace

std::shared_ptr<Module> createOsModule() {
    auto mod = std::make_shared<Module>();
    mod->name = "os";

    mod->members["exists"] = makeNative("exists", [](std::vector<Value>& args) -> Value {
        return fs::exists(strArg(args, 0, "exists"));
    });

    mod->members["cwd"] = makeNative("cwd", [](std::vector<Value>& args) -> Value {
        return fs::current_path().string();
    });

    mod->members["listdir"] = makeNative("listdir", [](std::vector<Value>& args) -> Value {
        std::string path = args.empty() ? "." : strArg(args, 0, "listdir");
        auto arr = std::make_shared<ValueArray>();
        if (!fs::exists(path) || !fs::is_directory(path))
            throw std::runtime_error("os.listdir(): '" + path + "' is not a directory");
        for (const auto& entry : fs::directory_iterator(path))
            arr->elements.push_back(entry.path().filename().string());
        return arr;
    });

    mod->members["mkdir"] = makeNative("mkdir", [](std::vector<Value>& args) -> Value {
        const std::string& path = strArg(args, 0, "mkdir");
        return fs::create_directories(path);
    });

    mod->members["isFile"] = makeNative("isFile", [](std::vector<Value>& args) -> Value {
        return fs::exists(strArg(args, 0, "isFile")) && fs::is_regular_file(strArg(args, 0, "isFile"));
    });

    mod->members["isFolder"] = makeNative("isFolder", [](std::vector<Value>& args) -> Value {
        return fs::exists(strArg(args, 0, "isFolder")) && fs::is_directory(strArg(args, 0, "isFolder"));
    });

    return mod;
}

} // namespace luin
