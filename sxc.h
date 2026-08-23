#ifndef LUIN_SXC_H
#define LUIN_SXC_H

#include "Token.h"
#include <string>
#include <vector>
#include <cstdint>

namespace luin {

// .sxc format (Luin Serialized Compiled)
// --------------------------------------
// Magic  : 8 bytes  "LUINSXC1"
// Version: uint32_t (currently 1)
// Count  : uint32_t number of tokens
// Then for each token:
//   type   : uint16_t
//   line   : uint32_t
//   len    : uint32_t
//   lexeme : len bytes (UTF-8, no trailing NUL)
//
// The file contains ONLY the token stream. Original source text is never stored.
// This is the "instructions that Luin understands" the user requested.

constexpr const char* SXC_MAGIC = "LUINSXC1";
constexpr uint32_t SXC_VERSION = 1;

// Serialize a token vector produced by the Lexer into a .sxc file.
// Returns true on success.
bool writeSxc(const std::string& path, const std::vector<Token>& tokens);

// Load a .sxc file and reconstruct the token vector (including a final EOF).
// Throws std::runtime_error on any format or I/O error.
std::vector<Token> readSxc(const std::string& path);

// Convenience: compile a .sx source file to .sxc (lexer only).
// Returns 0 on success, non-zero on failure (messages go to stderr).
int compileSxToSxc(const std::string& sxPath, const std::string& sxcPath);

} // namespace luin

#endif // LUIN_SXC_H
