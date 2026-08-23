#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <cstring>
#include "Lexer.h"
#include "Parser.h"
#include "Interpreter.h"
#include "sxc.h"

// ---------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------

static bool endsWith(const std::string& s, const std::string& suffix) {
    if (s.size() < suffix.size()) return false;
    return s.compare(s.size() - suffix.size(), suffix.size(), suffix) == 0;
}

// Run a program from an already-obtained token stream.
// Returns process exit code (0 = success).
static int runTokens(std::vector<luin::Token> tokens, const std::string& displayName) {
    luin::Parser parser(std::move(tokens));
    auto program = parser.parse();
    if (!program) {
        std::cerr << "Parsing failed for '" << displayName << "'.\n";
        return 1;
    }

    luin::Interpreter interpreter;
    try {
        interpreter.interpret(*program);
    } catch (const std::exception& e) {
        std::cerr << "Runtime error: " << e.what() << std::endl;
        return 1;
    }
    return 0;
}

// Run a .sx source file (lex + parse + interpret).
static int runSxFile(const std::string& filename) {
    std::ifstream file(filename);
    if (!file.is_open()) {
        std::cerr << "Error: could not open file '" << filename << "'\n";
        return 1;
    }
    std::stringstream buffer;
    buffer << file.rdbuf();
    std::string source = buffer.str();
    file.close();

    luin::Lexer lexer(std::move(source));
    auto tokens = lexer.scanTokens();
    return runTokens(std::move(tokens), filename);
}

// Run a .sxc compiled file (load tokens + parse + interpret).
static int runSxcFile(const std::string& filename) {
    try {
        auto tokens = luin::readSxc(filename);
        return runTokens(std::move(tokens), filename);
    } catch (const std::exception& e) {
        std::cerr << "Error loading .sxc: " << e.what() << std::endl;
        return 1;
    }
}

// ---------------------------------------------------------------------------
// main
// ---------------------------------------------------------------------------

int main(int argc, char* argv[]) {
    if (argc < 2) {
        std::cerr << "Luin v2.3\n"
                  << "Usage:\n"
                  << "  Luin_v2.3 <file.sx> [more.sx ...]\n"
                  << "  Luin_v2.3 <file.sx> -b          # compile to <file.sxc>\n"
                  << "  Luin_v2.3 <file.sxc> -r         # run compiled form\n"
                  << "  Luin_v2.3 <file.sxc>            # also runs .sxc directly\n";
        return 1;
    }

    // Collect non-flag arguments and flags.
    std::vector<std::string> files;
    bool doBuild = false;
    bool doRunCompiled = false;

    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "-b" || arg == "--build") {
            doBuild = true;
        } else if (arg == "-r" || arg == "--run") {
            doRunCompiled = true;
        } else if (arg[0] == '-') {
            std::cerr << "Unknown option: " << arg << "\n";
            return 1;
        } else {
            files.push_back(arg);
        }
    }

    if (files.empty()) {
        std::cerr << "Error: no input file given.\n";
        return 1;
    }

    // ----- Build mode: .sx -> .sxc -----
    if (doBuild) {
        int worst = 0;
        for (const auto& f : files) {
            if (!endsWith(f, ".sx")) {
                std::cerr << "Error: -b only accepts .sx files ('" << f << "').\n";
                worst = 1;
                continue;
            }
            std::string out = f.substr(0, f.size() - 3) + ".sxc";
            std::cout << "Building " << f << " -> " << out << " ...\n";
            int code = luin::compileSxToSxc(f, out);
            if (code == 0) {
                std::cout << "  OK (" << out << ")\n";
            } else {
                worst = code;
            }
        }
        return worst;
    }

    // ----- Run mode -----
    // Single file keeps the exact v2.1/v2.2 output (no banner).
    // Multiple files print the == filename run: == markers.
    int worstExitCode = 0;
    bool multi = files.size() > 1;

    for (const auto& filename : files) {
        if (multi) {
            std::cout << "== " << filename << " run: ==" << std::endl;
        }

        int code = 0;
        if (endsWith(filename, ".sxc") || doRunCompiled) {
            // Prefer .sxc if the user asked for -r or the file already is .sxc
            std::string path = filename;
            if (doRunCompiled && endsWith(filename, ".sx")) {
                path = filename.substr(0, filename.size() - 3) + ".sxc";
            }
            code = runSxcFile(path);
        } else if (endsWith(filename, ".sx")) {
            code = runSxFile(filename);
        } else {
            std::cerr << "Error: Luin runs .sx or .sxc files ('" << filename << "').\n";
            code = 1;
        }

        if (code != 0) worstExitCode = code;
        if (multi) std::cout << std::endl;
    }

    return worstExitCode;
}
