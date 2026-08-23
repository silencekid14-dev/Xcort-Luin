#include "app_module.h"
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <cstdlib>
#include <algorithm>

namespace luin {

namespace {

Value makeNative(std::string name, std::function<Value(std::vector<Value>&)> fn) {
    auto nf = std::make_shared<NativeFunction>();
    nf->name = std::move(name);
    nf->fn = std::move(fn);
    return nf;
}

const std::string& strArg(const std::vector<Value>& args, size_t i, const std::string& fn) {
    if (i >= args.size() || !std::holds_alternative<std::string>(args[i]))
        throw std::runtime_error("app." + fn + "(): argument " + std::to_string(i + 1) + " must be a string");
    return std::get<std::string>(args[i]);
}

// Very small helper to print a horizontal rule.
void rule(int width = 48) {
    for (int i = 0; i < width; ++i) std::cout << '=';
    std::cout << '\n';
}

} // namespace

std::shared_ptr<Module> createAppModule() {
    auto mod = std::make_shared<Module>();
    mod->name = "app";

    // app.create(title) -> a plain ClassInstance that the other functions treat as the App.
    // We store title and a screens array inside its fields. No real Class is required;
    // the Interpreter already supports free-form instances via fields.
    mod->members["create"] = makeNative("create", [](std::vector<Value>& args) -> Value {
        std::string title = args.empty() ? "Luin App" : (std::holds_alternative<std::string>(args[0])
            ? std::get<std::string>(args[0]) : "Luin App");
        auto inst = std::make_shared<ClassInstance>();
        // No klass needed for a pure data bag.
        inst->fields["title"] = title;
        inst->fields["start"] = std::string("home");
        inst->fields["screens"] = std::make_shared<ValueArray>(); // list of [name, fn]
        inst->fields["running"] = true;
        return inst;
    });

    mod->members["add_screen"] = makeNative("add_screen", [](std::vector<Value>& args) -> Value {
        if (args.size() != 3)
            throw std::runtime_error("app.add_screen() expects 3 arguments (app, name, fn)");
        if (!std::holds_alternative<std::shared_ptr<ClassInstance>>(args[0]))
            throw std::runtime_error("app.add_screen(): first argument must be an app created by app.create()");
        auto app = std::get<std::shared_ptr<ClassInstance>>(args[0]);
        const std::string& name = strArg(args, 1, "add_screen");
        // Accept either a Function or a NativeFunction; we just store the Value.
        auto screens = std::get<std::shared_ptr<ValueArray>>(app->fields["screens"]);
        auto entry = std::make_shared<ValueArray>();
        entry->elements.push_back(name);
        entry->elements.push_back(args[2]);
        screens->elements.push_back(entry);
        return std::string("");
    });

    mod->members["set_start"] = makeNative("set_start", [](std::vector<Value>& args) -> Value {
        if (args.size() != 2)
            throw std::runtime_error("app.set_start() expects 2 arguments (app, name)");
        auto app = std::get<std::shared_ptr<ClassInstance>>(args[0]);
        app->fields["start"] = strArg(args, 1, "set_start");
        return std::string("");
    });

    // app.banner(text)
    mod->members["banner"] = makeNative("banner", [](std::vector<Value>& args) -> Value {
        std::string text = args.empty() ? "" : (std::holds_alternative<std::string>(args[0])
            ? std::get<std::string>(args[0]) : "");
        std::cout << '\n';
        rule();
        std::cout << "  " << text << '\n';
        rule();
        std::cout << '\n';
        return std::string("");
    });

    // app.clear() — best-effort ANSI / Windows clear
    mod->members["clear"] = makeNative("clear", [](std::vector<Value>&) -> Value {
#if defined(_WIN32)
        std::system("cls");
#else
        std::cout << "\033[2J\033[H" << std::flush;
#endif
        return std::string("");
    });

    // app.pause([msg])
    mod->members["pause"] = makeNative("pause", [](std::vector<Value>& args) -> Value {
        if (!args.empty() && std::holds_alternative<std::string>(args[0]))
            std::cout << std::get<std::string>(args[0]);
        else
            std::cout << "Press Enter to continue...";
        std::cout << std::flush;
        std::string dummy;
        std::getline(std::cin, dummy);
        return std::string("");
    });

    // app.confirm(prompt) -> bool
    mod->members["confirm"] = makeNative("confirm", [](std::vector<Value>& args) -> Value {
        std::string prompt = args.empty() ? "Continue? [y/N] " : strArg(args, 0, "confirm");
        std::cout << prompt << std::flush;
        std::string line;
        std::getline(std::cin, line);
        if (line.empty()) return false;
        char c = static_cast<char>(std::tolower(static_cast<unsigned char>(line[0])));
        return c == 'y';
    });

    // app.menu(title, options_array) -> chosen 0-based index, or -1 on empty/quit
    mod->members["menu"] = makeNative("menu", [](std::vector<Value>& args) -> Value {
        if (args.size() != 2)
            throw std::runtime_error("app.menu() expects 2 arguments (title, options_array)");
        const std::string& title = strArg(args, 0, "menu");
        if (!std::holds_alternative<std::shared_ptr<ValueArray>>(args[1]))
            throw std::runtime_error("app.menu(): options must be an array of strings");
        auto opts = std::get<std::shared_ptr<ValueArray>>(args[1]);
        if (opts->elements.empty()) return -1;

        std::cout << '\n' << title << '\n';
        for (size_t i = 0; i < opts->elements.size(); ++i) {
            std::string label = std::holds_alternative<std::string>(opts->elements[i])
                ? std::get<std::string>(opts->elements[i]) : "?";
            std::cout << "  " << (i + 1) << ") " << label << '\n';
        }
        std::cout << "  0) Quit\n";
        std::cout << "Choice: " << std::flush;

        std::string line;
        std::getline(std::cin, line);
        try {
            int choice = std::stoi(line);
            if (choice == 0) return -1;
            if (choice >= 1 && choice <= static_cast<int>(opts->elements.size()))
                return choice - 1;
        } catch (...) {}
        return -1;
    });

    // app.table(headers, rows) — headers is array of string, rows is array of arrays
    mod->members["table"] = makeNative("table", [](std::vector<Value>& args) -> Value {
        if (args.size() != 2)
            throw std::runtime_error("app.table() expects 2 arguments (headers, rows)");
        if (!std::holds_alternative<std::shared_ptr<ValueArray>>(args[0]) ||
            !std::holds_alternative<std::shared_ptr<ValueArray>>(args[1]))
            throw std::runtime_error("app.table(): headers and rows must be arrays");
        auto headers = std::get<std::shared_ptr<ValueArray>>(args[0]);
        auto rows = std::get<std::shared_ptr<ValueArray>>(args[1]);

        // Compute column widths
        std::vector<size_t> widths(headers->elements.size(), 0);
        for (size_t c = 0; c < headers->elements.size(); ++c) {
            if (std::holds_alternative<std::string>(headers->elements[c]))
                widths[c] = std::get<std::string>(headers->elements[c]).size();
        }
        for (const auto& rowVal : rows->elements) {
            if (!std::holds_alternative<std::shared_ptr<ValueArray>>(rowVal)) continue;
            auto row = std::get<std::shared_ptr<ValueArray>>(rowVal);
            for (size_t c = 0; c < row->elements.size() && c < widths.size(); ++c) {
                std::string cell;
                if (std::holds_alternative<std::string>(row->elements[c]))
                    cell = std::get<std::string>(row->elements[c]);
                else if (std::holds_alternative<int>(row->elements[c]))
                    cell = std::to_string(std::get<int>(row->elements[c]));
                else if (std::holds_alternative<double>(row->elements[c]))
                    cell = std::to_string(std::get<double>(row->elements[c]));
                if (cell.size() > widths[c]) widths[c] = cell.size();
            }
        }

        auto printRow = [&](const std::shared_ptr<ValueArray>& row) {
            for (size_t c = 0; c < widths.size(); ++c) {
                std::string cell;
                if (c < row->elements.size()) {
                    if (std::holds_alternative<std::string>(row->elements[c]))
                        cell = std::get<std::string>(row->elements[c]);
                    else if (std::holds_alternative<int>(row->elements[c]))
                        cell = std::to_string(std::get<int>(row->elements[c]));
                    else if (std::holds_alternative<double>(row->elements[c]))
                        cell = std::to_string(std::get<double>(row->elements[c]));
                }
                std::cout << cell;
                for (size_t p = cell.size(); p < widths[c] + 2; ++p) std::cout << ' ';
            }
            std::cout << '\n';
        };

        printRow(headers);
        for (size_t c = 0; c < widths.size(); ++c) {
            for (size_t p = 0; p < widths[c]; ++p) std::cout << '-';
            std::cout << "  ";
        }
        std::cout << '\n';
        for (const auto& rowVal : rows->elements) {
            if (std::holds_alternative<std::shared_ptr<ValueArray>>(rowVal))
                printRow(std::get<std::shared_ptr<ValueArray>>(rowVal));
        }
        return std::string("");
    });

    // app.run(app) — simple dispatcher. Screens are stored as [name, callable].
    // The callable is expected to be a Luin function that receives the app instance
    // and returns the name of the next screen (string) or "" / false to quit.
    // Because we are inside a native function we cannot easily call back into the
    // Luin Function without the Interpreter; therefore the real run loop is
    // provided as a pure-Luin helper that the user is expected to call, or we
    // expose a lower-level "app.next_screen" style API.
    //
    // For maximal simplicity and honesty: the heavy lifting of the loop is done
    // by a small pure-.sx template that users copy, while the native module
    // supplies only the UI primitives (menu, banner, table, confirm, ...).
    // This keeps the C++ surface small and the behaviour transparent.

    mod->members["title"] = makeNative("title", [](std::vector<Value>& args) -> Value {
        if (args.size() != 1 || !std::holds_alternative<std::shared_ptr<ClassInstance>>(args[0]))
            throw std::runtime_error("app.title() expects the app instance");
        auto app = std::get<std::shared_ptr<ClassInstance>>(args[0]);
        auto it = app->fields.find("title");
        if (it == app->fields.end()) return std::string("");
        return it->second;
    });

    return mod;
}

} // namespace luin
