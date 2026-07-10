#ifndef LUIN_TOKEN_H
#define LUIN_TOKEN_H

#include <string>
#include <ostream>

namespace luin {

enum class TokenType {
    // Keywords
    KEYWORD_SHOW,
    KEYWORD_ASK,
    KEYWORD_IF,
    KEYWORD_ELF,
    KEYWORD_ELS,
    KEYWORD_WHILE,
    KEYWORD_FOR,
    KEYWORD_LOOP,
    KEYWORD_SLOOP,
    KEYWORD_FN,
    KEYWORD_CLS,
    KEYWORD_RTN,
    KEYWORD_SELF,
    KEYWORD_TRY,

    // Literal keywords
    KW_TRUE,
    KW_FALSE,

    // Identifiers and literals
    IDENTIFIER,
    STRING_LITERAL,
    F_STRING,
    NUMBER,
    FLOAT,

    // Operators
    PLUS,
    MINUS,
    STAR,
    SLASH,
    PERCENT,
    EQUAL_EQUAL,
    BANG_EQUAL,
    GREATER,
    GREATER_EQUAL,
    LESS,
    LESS_EQUAL,

    // Assignment and grouping
    EQUALS,
    LEFT_PAREN,
    RIGHT_PAREN,
    COMMA,
    COLON,
    LEFT_BRACE,
    RIGHT_BRACE,
    LEFT_BRACKET,
    RIGHT_BRACKET,
    DOT,

    // Input stream
    STREAM_IN,          // >>

    // Special
    END_OF_FILE,
    UNKNOWN
};

struct Token {
    TokenType type;
    std::string lexeme;
    int line;

    Token(TokenType type, std::string lexeme, int line)
        : type(type), lexeme(std::move(lexeme)), line(line) {}

    std::string toString() const;
};

std::ostream& operator<<(std::ostream& os, const Token& token);

} // namespace luin

#endif // LUIN_TOKEN_H