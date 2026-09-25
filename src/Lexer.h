#ifndef LUIN_LEXER_H
#define LUIN_LEXER_H

#include <string>
#include <vector>
#include "Token.h"

namespace luin {

class Lexer {
public:
    explicit Lexer(std::string source);
    std::vector<Token> scanTokens();

private:
    void scanToken();
    void addToken(TokenType type);
    void addToken(TokenType type, const std::string& lexeme);

    char advance();
    char peek() const;
    char peekNext() const;
    bool match(char expected);
    bool isAtEnd() const;

    void stringLiteral();
    void fStringLiteral();
    void numberLiteral();
    void identifierOrKeyword();
    static std::string unescape(const std::string& raw);

    const std::string m_source;
    std::vector<Token> m_tokens;
    size_t m_start = 0;
    size_t m_current = 0;
    int m_line = 1;
};

} // namespace luin

#endif // LUIN_LEXER_H