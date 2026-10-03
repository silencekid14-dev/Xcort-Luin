#include "app_module.h"
#include <cctype>
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


    // --- 20 expanded features ---
    // 1. input(prompt) -> string
    mod->members["input"] = makeNative("input", [](std::vector<Value>& args) -> Value {
        std::string prompt = args.empty() ? "" : (std::holds_alternative<std::string>(args[0])
            ? std::get<std::string>(args[0]) : "");
        std::cout << prompt << std::flush;
        std::string line;
        std::getline(std::cin, line);
        return line;
    });

    // 2. ask_int(prompt) -> int
    mod->members["ask_int"] = makeNative("ask_int", [](std::vector<Value>& args) -> Value {
        std::string prompt = args.empty() ? "Enter integer: " : strArg(args, 0, "ask_int");
        while (true) {
            std::cout << prompt << std::flush;
            std::string line;
            std::getline(std::cin, line);
            try {
                size_t pos = 0;
                int v = std::stoi(line, &pos);
                if (pos == line.size()) return v;
            } catch (...) {}
            std::cout << "Please enter a valid integer.\n";
        }
    });

    // 3. ask_float(prompt) -> double
    mod->members["ask_float"] = makeNative("ask_float", [](std::vector<Value>& args) -> Value {
        std::string prompt = args.empty() ? "Enter number: " : strArg(args, 0, "ask_float");
        while (true) {
            std::cout << prompt << std::flush;
            std::string line;
            std::getline(std::cin, line);
            try {
                size_t pos = 0;
                double v = std::stod(line, &pos);
                if (pos == line.size()) return v;
            } catch (...) {}
            std::cout << "Please enter a valid number.\n";
        }
    });

    // 4. progress(current, total [, width])
    mod->members["progress"] = makeNative("progress", [](std::vector<Value>& args) -> Value {
        if (args.size() < 2)
            throw std::runtime_error("app.progress() expects (current, total [, width])");
        int cur = std::holds_alternative<int>(args[0]) ? std::get<int>(args[0]) : static_cast<int>(std::get<double>(args[0]));
        int tot = std::holds_alternative<int>(args[1]) ? std::get<int>(args[1]) : static_cast<int>(std::get<double>(args[1]));
        int width = args.size() >= 3 ? (std::holds_alternative<int>(args[2]) ? std::get<int>(args[2]) : 30) : 30;
        if (tot <= 0) tot = 1;
        if (cur < 0) cur = 0;
        if (cur > tot) cur = tot;
        int filled = (cur * width) / tot;
        std::cout << "[";
        for (int i = 0; i < width; ++i) std::cout << (i < filled ? '#' : '-');
        std::cout << "] " << cur << "/" << tot << "\r" << std::flush;
        if (cur >= tot) std::cout << "\n";
        return std::string("");
    });

    // 5. color(code, text) — ANSI color wrap
    mod->members["color"] = makeNative("color", [](std::vector<Value>& args) -> Value {
        if (args.size() < 2)
            throw std::runtime_error("app.color() expects (code, text)");
        int code = std::holds_alternative<int>(args[0]) ? std::get<int>(args[0]) : 0;
        std::string text = std::holds_alternative<std::string>(args[1]) ? std::get<std::string>(args[1]) : "";
        return "\033[" + std::to_string(code) + "m" + text + "\033[0m";
    });

    // 6. print(text) — no newline
    mod->members["print"] = makeNative("print", [](std::vector<Value>& args) -> Value {
        if (!args.empty()) {
            if (std::holds_alternative<std::string>(args[0])) std::cout << std::get<std::string>(args[0]);
            else if (std::holds_alternative<int>(args[0])) std::cout << std::get<int>(args[0]);
            else if (std::holds_alternative<double>(args[0])) std::cout << std::get<double>(args[0]);
            else if (std::holds_alternative<bool>(args[0])) std::cout << (std::get<bool>(args[0]) ? "true" : "false");
        }
        std::cout << std::flush;
        return std::string("");
    });

    // 7. println(text)
    mod->members["println"] = makeNative("println", [](std::vector<Value>& args) -> Value {
        if (!args.empty()) {
            if (std::holds_alternative<std::string>(args[0])) std::cout << std::get<std::string>(args[0]);
            else if (std::holds_alternative<int>(args[0])) std::cout << std::get<int>(args[0]);
            else if (std::holds_alternative<double>(args[0])) std::cout << std::get<double>(args[0]);
            else if (std::holds_alternative<bool>(args[0])) std::cout << (std::get<bool>(args[0]) ? "true" : "false");
        }
        std::cout << std::endl;
        return std::string("");
    });

    // 8. hr([char, width])
    mod->members["hr"] = makeNative("hr", [](std::vector<Value>& args) -> Value {
        char ch = '=';
        int width = 48;
        if (!args.empty() && std::holds_alternative<std::string>(args[0]) && !std::get<std::string>(args[0]).empty())
            ch = std::get<std::string>(args[0])[0];
        if (args.size() >= 2 && std::holds_alternative<int>(args[1]))
            width = std::get<int>(args[1]);
        for (int i = 0; i < width; ++i) std::cout << ch;
        std::cout << '\n';
        return std::string("");
    });

    // 9. box(text)
    mod->members["box"] = makeNative("box", [](std::vector<Value>& args) -> Value {
        std::string text = args.empty() ? "" : (std::holds_alternative<std::string>(args[0]) ? std::get<std::string>(args[0]) : "");
        size_t w = text.size() + 4;
        std::cout << '+'; for (size_t i = 0; i < w - 2; ++i) std::cout << '-'; std::cout << "+\n";
        std::cout << "| " << text << " |\n";
        std::cout << '+'; for (size_t i = 0; i < w - 2; ++i) std::cout << '-'; std::cout << "+\n";
        return std::string("");
    });

    // 10. choice(prompt, options) — same as menu but returns label string
    mod->members["choice"] = makeNative("choice", [](std::vector<Value>& args) -> Value {
        if (args.size() != 2)
            throw std::runtime_error("app.choice() expects (title, options_array)");
        const std::string& title = strArg(args, 0, "choice");
        if (!std::holds_alternative<std::shared_ptr<ValueArray>>(args[1]))
            throw std::runtime_error("app.choice(): options must be an array");
        auto opts = std::get<std::shared_ptr<ValueArray>>(args[1]);
        if (opts->elements.empty()) return std::string("");
        std::cout << '\n' << title << '\n';
        for (size_t i = 0; i < opts->elements.size(); ++i) {
            std::string label = std::holds_alternative<std::string>(opts->elements[i])
                ? std::get<std::string>(opts->elements[i]) : "?";
            std::cout << "  " << (i + 1) << ") " << label << '\n';
        }
        std::cout << "Choice: " << std::flush;
        std::string line; std::getline(std::cin, line);
        try {
            int choice = std::stoi(line);
            if (choice >= 1 && choice <= static_cast<int>(opts->elements.size()))
                return opts->elements[static_cast<size_t>(choice - 1)];
        } catch (...) {}
        return std::string("");
    });

    // 11. spinner(msg) — print msg + spinning dots once
    mod->members["spinner"] = makeNative("spinner", [](std::vector<Value>& args) -> Value {
        std::string msg = args.empty() ? "Working" : (std::holds_alternative<std::string>(args[0]) ? std::get<std::string>(args[0]) : "Working");
        const char* frames = "|/-\\";
        for (int i = 0; i < 8; ++i) {
            std::cout << "\r" << msg << " " << frames[i % 4] << " " << std::flush;
            // tiny busy wait without sleep header
            for (volatile int j = 0; j < 2000000; ++j) {}
        }
        std::cout << "\r" << msg << " done\n";
        return std::string("");
    });

    // 12. alert(msg)
    mod->members["alert"] = makeNative("alert", [](std::vector<Value>& args) -> Value {
        std::string text = args.empty() ? "Alert" : (std::holds_alternative<std::string>(args[0]) ? std::get<std::string>(args[0]) : "Alert");
        std::cout << "\n*** " << text << " ***\n";
        return std::string("");
    });

    // 13. error(msg) — print to stderr
    mod->members["error"] = makeNative("error", [](std::vector<Value>& args) -> Value {
        std::string text = args.empty() ? "Error" : (std::holds_alternative<std::string>(args[0]) ? std::get<std::string>(args[0]) : "Error");
        std::cerr << "[ERROR] " << text << std::endl;
        return std::string("");
    });

    // 14. success(msg)
    mod->members["success"] = makeNative("success", [](std::vector<Value>& args) -> Value {
        std::string text = args.empty() ? "OK" : (std::holds_alternative<std::string>(args[0]) ? std::get<std::string>(args[0]) : "OK");
        std::cout << "\033[32m[OK]\033[0m " << text << std::endl;
        return std::string("");
    });

    // 15. warn(msg)
    mod->members["warn"] = makeNative("warn", [](std::vector<Value>& args) -> Value {
        std::string text = args.empty() ? "Warning" : (std::holds_alternative<std::string>(args[0]) ? std::get<std::string>(args[0]) : "Warning");
        std::cout << "\033[33m[WARN]\033[0m " << text << std::endl;
        return std::string("");
    });

    // 16. indent(text, n)
    mod->members["indent"] = makeNative("indent", [](std::vector<Value>& args) -> Value {
        if (args.size() < 1) return std::string("");
        std::string text = std::holds_alternative<std::string>(args[0]) ? std::get<std::string>(args[0]) : "";
        int n = args.size() >= 2 && std::holds_alternative<int>(args[1]) ? std::get<int>(args[1]) : 2;
        return std::string(static_cast<size_t>(n > 0 ? n : 0), ' ') + text;
    });

    // 17. center(text, width)
    mod->members["center"] = makeNative("center", [](std::vector<Value>& args) -> Value {
        if (args.empty()) return std::string("");
        std::string text = std::holds_alternative<std::string>(args[0]) ? std::get<std::string>(args[0]) : "";
        int width = args.size() >= 2 && std::holds_alternative<int>(args[1]) ? std::get<int>(args[1]) : 48;
        int pad = width - static_cast<int>(text.size());
        if (pad <= 0) return text;
        int left = pad / 2;
        return std::string(static_cast<size_t>(left), ' ') + text;
    });

    // 18. columns(arr, col_count)
    mod->members["columns"] = makeNative("columns", [](std::vector<Value>& args) -> Value {
        if (args.size() < 1 || !std::holds_alternative<std::shared_ptr<ValueArray>>(args[0]))
            throw std::runtime_error("app.columns() expects (array [, cols])");
        auto arr = std::get<std::shared_ptr<ValueArray>>(args[0]);
        int cols = args.size() >= 2 && std::holds_alternative<int>(args[1]) ? std::get<int>(args[1]) : 2;
        if (cols < 1) cols = 1;
        for (size_t i = 0; i < arr->elements.size(); ++i) {
            std::string cell;
            if (std::holds_alternative<std::string>(arr->elements[i]))
                cell = std::get<std::string>(arr->elements[i]);
            else if (std::holds_alternative<int>(arr->elements[i]))
                cell = std::to_string(std::get<int>(arr->elements[i]));
            std::cout << cell << "\t";
            if ((static_cast<int>(i) + 1) % cols == 0) std::cout << '\n';
        }
        if (arr->elements.size() % static_cast<size_t>(cols) != 0) std::cout << '\n';
        return std::string("");
    });

    // 19. yes_no(prompt) alias of confirm
    mod->members["yes_no"] = makeNative("yes_no", [](std::vector<Value>& args) -> Value {
        std::string prompt = args.empty() ? "Yes/No? [y/N] " : strArg(args, 0, "yes_no");
        std::cout << prompt << std::flush;
        std::string line; std::getline(std::cin, line);
        if (line.empty()) return false;
        char c = static_cast<char>(std::tolower(static_cast<unsigned char>(line[0])));
        return c == 'y';
    });

    // 20. countdown(seconds)
    mod->members["countdown"] = makeNative("countdown", [](std::vector<Value>& args) -> Value {
        int n = args.empty() ? 3 : (std::holds_alternative<int>(args[0]) ? std::get<int>(args[0]) : 3);
        for (int i = n; i >= 1; --i) {
            std::cout << i << "... " << std::flush;
            for (volatile int j = 0; j < 15000000; ++j) {}
        }
        std::cout << "Go!\n";
        return std::string("");
    });


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
