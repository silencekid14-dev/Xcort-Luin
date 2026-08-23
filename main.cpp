#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include "Lexer.h"
#include "Parser.h"
#include "Interpreter.h"

// v2.2: runs one .sx file (unchanged single-file behavior from v2.1.2).
// Returns the process exit code that this file's run should produce.
static int runOneFile(const std::string& filename) {
    if (filename.size() < 3 || filename.substr(filename.size() - 3) != ".sx") {
        std::cerr << "Error: Luin only runs .sx files ('" << filename << "')." << std::endl;
        return 1;
    }

    std::ifstream file(filename);
    if (!file.is_open()) {
        std::cerr << "Error: could not open file '" << filename << "'" << std::endl;
        return 1;
    }
    std::stringstream buffer;
    buffer << file.rdbuf();
    std::string source = buffer.str();
    file.close();

    luin::Lexer lexer(source);
    auto tokens = lexer.scanTokens();

    luin::Parser parser(std::move(tokens));
    auto program = parser.parse();
    if (!program) {
        std::cerr << "Parsing failed." << std::endl;
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

int main(int argc, char* argv[]) {
    if (argc < 2) {
        std::cerr << "Usage: luin <filename.sx> [more.sx ...]" << std::endl;
        return 1;
    }

    // v2.1.2 behavior: exactly one file, no banner -- kept 100% unchanged
    // so existing single-file workflows (including X-Luin-VSCode's "Run
    // Current File") see identical output to before.
    if (argc == 2) {
        return runOneFile(argv[1]);
    }

    // v2.2: multiple .sx files in one invocation, e.g.
    //   Luin_v2.2.exe test.sx if.sx
    //   .\Luin_v2.2.exe test.sx if.sx
    // Each file is run as its own independent program (its own fresh
    // interpreter/scope -- files don't share variables with each other;
    // use `import "file.sx"` from *inside* a script if you want that).
    // Before each file's output, a marker line is printed:
    //   == <filename> run: ==
    // X-Luin-VSCode's terminal panel (see terminal_panel.py) recognizes
    // this exact marker and re-renders it as a friendly "<file> run:"
    // section header; a plain VS Code / OS terminal just sees the raw
    // marker line followed by that file's normal output, which is still
    // perfectly readable on its own.
    int worstExitCode = 0;
    for (int i = 1; i < argc; ++i) {
        std::string filename = argv[i];
        std::cout << "== " << filename << " run: ==" << std::endl;
        int code = runOneFile(filename);
        if (code != 0) worstExitCode = code;
        std::cout << std::endl;
    }
    return worstExitCode;
}
