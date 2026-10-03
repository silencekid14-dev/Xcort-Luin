#include "pkg_module.h"
#include <filesystem>
#include <fstream>
#include <sstream>
#include <stdexcept>
#include <algorithm>
#include <map>
#include <set>

namespace fs = std::filesystem;

namespace luin {

namespace {

// In-memory package registry for this process.
struct PkgInfo {
    std::string name;
    std::string version;
    std::string path;
    std::string description;
    std::vector<std::string> deps;
};
std::map<std::string, PkgInfo> g_registry;
std::string g_pkgRoot = "packages";

Value makeNative(std::string name, std::function<Value(std::vector<Value>&)> fn) {
    auto nf = std::make_shared<NativeFunction>();
    nf->name = std::move(name);
    nf->fn = std::move(fn);
    return nf;
}

const std::string& strArg(const std::vector<Value>& args, size_t i, const std::string& fn) {
    if (i >= args.size() || !std::holds_alternative<std::string>(args[i]))
        throw std::runtime_error("pkg." + fn + "(): argument " + std::to_string(i + 1) + " must be a string");
    return std::get<std::string>(args[i]);
}

std::string readFile(const fs::path& p) {
    std::ifstream in(p);
    if (!in) return "";
    std::ostringstream ss;
    ss << in.rdbuf();
    return ss.str();
}

bool writeFile(const fs::path& p, const std::string& content) {
    if (p.has_parent_path())
        fs::create_directories(p.parent_path());
    std::ofstream out(p, std::ios::trunc);
    if (!out) return false;
    out << content;
    return true;
}

// Simple KEY=VALUE parser for package.meta files
std::map<std::string, std::string> parseMeta(const std::string& text) {
    std::map<std::string, std::string> m;
    std::istringstream iss(text);
    std::string line;
    while (std::getline(iss, line)) {
        if (!line.empty() && line.back() == '\r') line.pop_back();
        size_t eq = line.find('=');
        if (eq == std::string::npos) continue;
        std::string k = line.substr(0, eq);
        std::string v = line.substr(eq + 1);
        while (!k.empty() && (k.back() == ' ' || k.back() == '\t')) k.pop_back();
        while (!v.empty() && (v[0] == ' ' || v[0] == '\t')) v.erase(0, 1);
        if (!k.empty()) m[k] = v;
    }
    return m;
}

} // namespace

std::shared_ptr<Module> createPkgModule() {
    auto mod = std::make_shared<Module>();
    mod->name = "pkg";

    // 1. set_root(path)
    mod->members["set_root"] = makeNative("set_root", [](std::vector<Value>& args) -> Value {
        g_pkgRoot = strArg(args, 0, "set_root");
        return std::string("");
    });

    // 2. root()
    mod->members["root"] = makeNative("root", [](std::vector<Value>&) -> Value {
        return g_pkgRoot;
    });

    // 3. register(name, version, path [, description])
    mod->members["register"] = makeNative("register", [](std::vector<Value>& args) -> Value {
        if (args.size() < 3)
            throw std::runtime_error("pkg.register() expects (name, version, path [, desc])");
        PkgInfo info;
        info.name = strArg(args, 0, "register");
        info.version = strArg(args, 1, "register");
        info.path = strArg(args, 2, "register");
        if (args.size() >= 4) info.description = strArg(args, 3, "register");
        g_registry[info.name] = info;
        return true;
    });

    // 4. unregister(name)
    mod->members["unregister"] = makeNative("unregister", [](std::vector<Value>& args) -> Value {
        return g_registry.erase(strArg(args, 0, "unregister")) > 0;
    });

    // 5. is_registered(name)
    mod->members["is_registered"] = makeNative("is_registered", [](std::vector<Value>& args) -> Value {
        return g_registry.count(strArg(args, 0, "is_registered")) > 0;
    });

    // 6. list() -> array of names
    mod->members["list"] = makeNative("list", [](std::vector<Value>&) -> Value {
        auto arr = std::make_shared<ValueArray>();
        for (const auto& p : g_registry)
            arr->elements.push_back(p.first);
        return arr;
    });

    // 7. version(name)
    mod->members["version"] = makeNative("version", [](std::vector<Value>& args) -> Value {
        auto it = g_registry.find(strArg(args, 0, "version"));
        if (it == g_registry.end()) return std::string("");
        return it->second.version;
    });

    // 8. path(name)
    mod->members["path"] = makeNative("path", [](std::vector<Value>& args) -> Value {
        auto it = g_registry.find(strArg(args, 0, "path"));
        if (it == g_registry.end()) return std::string("");
        return it->second.path;
    });

    // 9. description(name)
    mod->members["description"] = makeNative("description", [](std::vector<Value>& args) -> Value {
        auto it = g_registry.find(strArg(args, 0, "description"));
        if (it == g_registry.end()) return std::string("");
        return it->second.description;
    });

    // 10. info(name) -> [name, version, path, description]
    mod->members["info"] = makeNative("info", [](std::vector<Value>& args) -> Value {
        auto it = g_registry.find(strArg(args, 0, "info"));
        auto arr = std::make_shared<ValueArray>();
        if (it == g_registry.end()) return arr;
        arr->elements.push_back(it->second.name);
        arr->elements.push_back(it->second.version);
        arr->elements.push_back(it->second.path);
        arr->elements.push_back(it->second.description);
        return arr;
    });

    // 11. add_dep(name, dep)
    mod->members["add_dep"] = makeNative("add_dep", [](std::vector<Value>& args) -> Value {
        if (args.size() < 2)
            throw std::runtime_error("pkg.add_dep() expects (name, dep)");
        auto it = g_registry.find(strArg(args, 0, "add_dep"));
        if (it == g_registry.end()) return false;
        it->second.deps.push_back(strArg(args, 1, "add_dep"));
        return true;
    });

    // 12. deps(name) -> array
    mod->members["deps"] = makeNative("deps", [](std::vector<Value>& args) -> Value {
        auto arr = std::make_shared<ValueArray>();
        auto it = g_registry.find(strArg(args, 0, "deps"));
        if (it == g_registry.end()) return arr;
        for (const auto& d : it->second.deps) arr->elements.push_back(d);
        return arr;
    });

    // 13. mkdir(name) — create package folder under root
    mod->members["mkdir"] = makeNative("mkdir", [](std::vector<Value>& args) -> Value {
        fs::path p = fs::path(g_pkgRoot) / strArg(args, 0, "mkdir");
        return fs::create_directories(p);
    });

    // 14. exists(name) — package folder exists under root
    mod->members["exists"] = makeNative("exists", [](std::vector<Value>& args) -> Value {
        return fs::exists(fs::path(g_pkgRoot) / strArg(args, 0, "exists"));
    });

    // 15. write_meta(name, content) — write package.meta
    mod->members["write_meta"] = makeNative("write_meta", [](std::vector<Value>& args) -> Value {
        if (args.size() < 2)
            throw std::runtime_error("pkg.write_meta() expects (name, content)");
        fs::path p = fs::path(g_pkgRoot) / strArg(args, 0, "write_meta") / "package.meta";
        return writeFile(p, strArg(args, 1, "write_meta"));
    });

    // 16. read_meta(name) -> string
    mod->members["read_meta"] = makeNative("read_meta", [](std::vector<Value>& args) -> Value {
        fs::path p = fs::path(g_pkgRoot) / strArg(args, 0, "read_meta") / "package.meta";
        return readFile(p);
    });

    // 17. load_meta(name) — parse package.meta into registry
    mod->members["load_meta"] = makeNative("load_meta", [](std::vector<Value>& args) -> Value {
        std::string name = strArg(args, 0, "load_meta");
        fs::path p = fs::path(g_pkgRoot) / name / "package.meta";
        auto m = parseMeta(readFile(p));
        if (m.empty()) return false;
        PkgInfo info;
        info.name = m.count("name") ? m["name"] : name;
        info.version = m.count("version") ? m["version"] : "0.0.0";
        info.path = (fs::path(g_pkgRoot) / name).string();
        info.description = m.count("description") ? m["description"] : "";
        if (m.count("deps")) {
            std::istringstream ds(m["deps"]);
            std::string d;
            while (std::getline(ds, d, ',')) {
                while (!d.empty() && d[0] == ' ') d.erase(0, 1);
                if (!d.empty()) info.deps.push_back(d);
            }
        }
        g_registry[info.name] = info;
        return true;
    });

    // 18. scan() — scan root for packages with package.meta
    mod->members["scan"] = makeNative("scan", [](std::vector<Value>&) -> Value {
        auto arr = std::make_shared<ValueArray>();
        if (!fs::exists(g_pkgRoot) || !fs::is_directory(g_pkgRoot)) return arr;
        for (const auto& entry : fs::directory_iterator(g_pkgRoot)) {
            if (!entry.is_directory()) continue;
            fs::path meta = entry.path() / "package.meta";
            if (!fs::exists(meta)) continue;
            std::string name = entry.path().filename().string();
            auto m = parseMeta(readFile(meta));
            PkgInfo info;
            info.name = m.count("name") ? m["name"] : name;
            info.version = m.count("version") ? m["version"] : "0.0.0";
            info.path = entry.path().string();
            info.description = m.count("description") ? m["description"] : "";
            g_registry[info.name] = info;
            arr->elements.push_back(info.name);
        }
        return arr;
    });

    // 19. compare_version(a, b) -> -1/0/1
    mod->members["compare_version"] = makeNative("compare_version", [](std::vector<Value>& args) -> Value {
        if (args.size() < 2)
            throw std::runtime_error("pkg.compare_version() expects (a, b)");
        auto parse = [](std::string v) {
            std::vector<int> parts;
            std::string cur;
            for (char c : v) {
                if (c == '.') {
                    if (!cur.empty()) { try { parts.push_back(std::stoi(cur)); } catch (...) { parts.push_back(0); } cur.clear(); }
                } else if (std::isdigit(static_cast<unsigned char>(c))) cur += c;
            }
            if (!cur.empty()) { try { parts.push_back(std::stoi(cur)); } catch (...) { parts.push_back(0); } }
            return parts;
        };
        auto pa = parse(strArg(args, 0, "compare_version"));
        auto pb = parse(strArg(args, 1, "compare_version"));
        size_t n = std::max(pa.size(), pb.size());
        for (size_t i = 0; i < n; ++i) {
            int a = i < pa.size() ? pa[i] : 0;
            int b = i < pb.size() ? pb[i] : 0;
            if (a < b) return -1;
            if (a > b) return 1;
        }
        return 0;
    });

    // 20. has_dep(name, dep)
    mod->members["has_dep"] = makeNative("has_dep", [](std::vector<Value>& args) -> Value {
        if (args.size() < 2)
            throw std::runtime_error("pkg.has_dep() expects (name, dep)");
        auto it = g_registry.find(strArg(args, 0, "has_dep"));
        if (it == g_registry.end()) return false;
        const std::string& d = strArg(args, 1, "has_dep");
        return std::find(it->second.deps.begin(), it->second.deps.end(), d) != it->second.deps.end();
    });

    // 21. count()
    mod->members["count"] = makeNative("count", [](std::vector<Value>&) -> Value {
        return static_cast<int>(g_registry.size());
    });

    // 22. clear()
    mod->members["clear"] = makeNative("clear", [](std::vector<Value>&) -> Value {
        g_registry.clear();
        return std::string("");
    });

    // 23. search(query) — names containing query
    mod->members["search"] = makeNative("search", [](std::vector<Value>& args) -> Value {
        std::string q = strArg(args, 0, "search");
        auto arr = std::make_shared<ValueArray>();
        for (const auto& p : g_registry)
            if (p.first.find(q) != std::string::npos)
                arr->elements.push_back(p.first);
        return arr;
    });

    // 24. write_file(pkg, relative, content)
    mod->members["write_file"] = makeNative("write_file", [](std::vector<Value>& args) -> Value {
        if (args.size() < 3)
            throw std::runtime_error("pkg.write_file() expects (pkg, relative, content)");
        fs::path p = fs::path(g_pkgRoot) / strArg(args, 0, "write_file") / strArg(args, 1, "write_file");
        return writeFile(p, strArg(args, 2, "write_file"));
    });

    // 25. read_file(pkg, relative)
    mod->members["read_file"] = makeNative("read_file", [](std::vector<Value>& args) -> Value {
        if (args.size() < 2)
            throw std::runtime_error("pkg.read_file() expects (pkg, relative)");
        fs::path p = fs::path(g_pkgRoot) / strArg(args, 0, "read_file") / strArg(args, 1, "read_file");
        return readFile(p);
    });

    // 26. list_files(pkg) -> array
    mod->members["list_files"] = makeNative("list_files", [](std::vector<Value>& args) -> Value {
        auto arr = std::make_shared<ValueArray>();
        fs::path dir = fs::path(g_pkgRoot) / strArg(args, 0, "list_files");
        if (!fs::exists(dir) || !fs::is_directory(dir)) return arr;
        for (const auto& e : fs::directory_iterator(dir))
            arr->elements.push_back(e.path().filename().string());
        return arr;
    });

    // 27. remove(name) — delete package folder
    mod->members["remove"] = makeNative("remove", [](std::vector<Value>& args) -> Value {
        std::string name = strArg(args, 0, "remove");
        g_registry.erase(name);
        fs::path p = fs::path(g_pkgRoot) / name;
        std::error_code ec;
        if (fs::exists(p)) {
            fs::remove_all(p, ec);
            return !ec;
        }
        return false;
    });

    // 28. export_json(name) -> string of simple JSON-ish meta
    mod->members["export_json"] = makeNative("export_json", [](std::vector<Value>& args) -> Value {
        auto it = g_registry.find(strArg(args, 0, "export_json"));
        if (it == g_registry.end()) return std::string("{}");
        const auto& i = it->second;
        std::ostringstream ss;
        ss << "{\"name\":\"" << i.name << "\",\"version\":\"" << i.version
           << "\",\"path\":\"" << i.path << "\",\"description\":\"" << i.description << "\"}";
        return ss.str();
    });

    // 29. require(name) — true if registered and path exists
    mod->members["require"] = makeNative("require", [](std::vector<Value>& args) -> Value {
        auto it = g_registry.find(strArg(args, 0, "require"));
        if (it == g_registry.end()) return false;
        return fs::exists(it->second.path);
    });

    // 30. install_local(name, version, description) — create folder + meta + register
    mod->members["install_local"] = makeNative("install_local", [](std::vector<Value>& args) -> Value {
        if (args.size() < 2)
            throw std::runtime_error("pkg.install_local() expects (name, version [, desc])");
        std::string name = strArg(args, 0, "install_local");
        std::string ver = strArg(args, 1, "install_local");
        std::string desc = args.size() >= 3 ? strArg(args, 2, "install_local") : "";
        fs::path dir = fs::path(g_pkgRoot) / name;
        fs::create_directories(dir);
        std::ostringstream meta;
        meta << "name=" << name << "\nversion=" << ver << "\ndescription=" << desc << "\n";
        writeFile(dir / "package.meta", meta.str());
        PkgInfo info;
        info.name = name;
        info.version = ver;
        info.path = dir.string();
        info.description = desc;
        g_registry[name] = info;
        return true;
    });

    return mod;
}

} // namespace luin
