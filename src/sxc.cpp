#include "sxc.h"
#include "Lexer.h"
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <cstring>

namespace luin {

namespace {

void writeU16(std::ostream& out, uint16_t v) {
    out.put(static_cast<char>(v & 0xFF));
    out.put(static_cast<char>((v >> 8) & 0xFF));
}

void writeU32(std::ostream& out, uint32_t v) {
    out.put(static_cast<char>(v & 0xFF));
    out.put(static_cast<char>((v >> 8) & 0xFF));
    out.put(static_cast<char>((v >> 16) & 0xFF));
    out.put(static_cast<char>((v >> 24) & 0xFF));
}

uint16_t readU16(std::istream& in) {
    char b[2];
    in.read(b, 2);
    if (!in) throw std::runtime_error("sxc: truncated file while reading u16");
    return static_cast<uint16_t>(static_cast<unsigned char>(b[0])) |
           (static_cast<uint16_t>(static_cast<unsigned char>(b[1])) << 8);
}

uint32_t readU32(std::istream& in) {
    char b[4];
    in.read(b, 4);
    if (!in) throw std::runtime_error("sxc: truncated file while reading u32");
    return static_cast<uint32_t>(static_cast<unsigned char>(b[0])) |
           (static_cast<uint32_t>(static_cast<unsigned char>(b[1])) << 8) |
           (static_cast<uint32_t>(static_cast<unsigned char>(b[2])) << 16) |
           (static_cast<uint32_t>(static_cast<unsigned char>(b[3])) << 24);
}

} // namespace

bool writeSxc(const std::string& path, const std::vector<Token>& tokens) {
    std::ofstream out(path, std::ios::binary | std::ios::trunc);
    if (!out) return false;

    out.write(SXC_MAGIC, 8);
    writeU32(out, SXC_VERSION);

    // Do not write the final END_OF_FILE token; the loader will re-append it.
    uint32_t count = 0;
    for (const auto& t : tokens) {
        if (t.type != TokenType::END_OF_FILE) ++count;
    }
    writeU32(out, count);

    for (const auto& t : tokens) {
        if (t.type == TokenType::END_OF_FILE) continue;
        writeU16(out, static_cast<uint16_t>(t.type));
        writeU32(out, static_cast<uint32_t>(t.line));
        writeU32(out, static_cast<uint32_t>(t.lexeme.size()));
        out.write(t.lexeme.data(), static_cast<std::streamsize>(t.lexeme.size()));
    }
    return static_cast<bool>(out);
}

std::vector<Token> readSxc(const std::string& path) {
    std::ifstream in(path, std::ios::binary);
    if (!in) throw std::runtime_error("sxc: could not open '" + path + "'");

    char magic[8];
    in.read(magic, 8);
    if (!in || std::memcmp(magic, SXC_MAGIC, 8) != 0)
        throw std::runtime_error("sxc: bad magic in '" + path + "'");

    uint32_t version = readU32(in);
    if (version != SXC_VERSION)
        throw std::runtime_error("sxc: unsupported version " + std::to_string(version));

    uint32_t count = readU32(in);
    std::vector<Token> tokens;
    tokens.reserve(count + 1);

    for (uint32_t i = 0; i < count; ++i) {
        uint16_t typeRaw = readU16(in);
        uint32_t line = readU32(in);
        uint32_t len = readU32(in);
        std::string lexeme(len, '\0');
        if (len > 0) {
            in.read(&lexeme[0], static_cast<std::streamsize>(len));
            if (!in) throw std::runtime_error("sxc: truncated lexeme data");
        }
        tokens.emplace_back(static_cast<TokenType>(typeRaw), std::move(lexeme), static_cast<int>(line));
    }

    // Always terminate with EOF so the Parser sees a complete stream.
    int lastLine = tokens.empty() ? 1 : tokens.back().line;
    tokens.emplace_back(TokenType::END_OF_FILE, "", lastLine);
    return tokens;
}

int compileSxToSxc(const std::string& sxPath, const std::string& sxcPath) {
    if (sxPath.size() < 3 || sxPath.substr(sxPath.size() - 3) != ".sx") {
        std::cerr << "Error: can only compile .sx files ('" << sxPath << "').\n";
        return 1;
    }

    std::ifstream file(sxPath);
    if (!file.is_open()) {
        std::cerr << "Error: could not open '" << sxPath << "'\n";
        return 1;
    }
    std::string source((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
    file.close();

    Lexer lexer(std::move(source));
    auto tokens = lexer.scanTokens();

    if (!writeSxc(sxcPath, tokens)) {
        std::cerr << "Error: failed to write '" << sxcPath << "'\n";
        return 1;
    }
    return 0;
}

} // namespace luin
