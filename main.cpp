#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include "Lexer.h"
#include "Parser.h"
#include "Interpreter.h"

int main(int argc, char* argv[]) {
    if (argc != 2) {
        std::cerr << "Usage: luin <filename.sx>" << std::endl;
        return 1;
    }

    std::string filename = argv[1];
    if (filename.size() < 3 || filename.substr(filename.size() - 3) != ".sx") {
        std::cerr << "Error: Luin only runs .sx files." << std::endl;
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