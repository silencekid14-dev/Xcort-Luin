#include "Lexer.h"
#include <cctype>
#include <cstdlib>

namespace luin {

Lexer::Lexer(std::string source)
    : m_source(std::move(source)) {}

std::vector<Token> Lexer::scanTokens() {
    while (!isAtEnd()) {
        m_start = m_current;
        scanToken();
    }

    m_tokens.emplace_back(TokenType::END_OF_FILE, "", m_line);
    return m_tokens;
}

void Lexer::scanToken() {
    char c = advance();

    switch (c) {
        case ' ': case '\r': case '\t': break;
        case '\n': m_line++; break;

        case '#':
            // A comment goes until the end of the line.
            while (peek() != '\n' && !isAtEnd()) {
                advance();
            }
            break;

        case '(': addToken(TokenType::LEFT_PAREN);    break;
        case ')': addToken(TokenType::RIGHT_PAREN);   break;
        case ':': addToken(TokenType::COLON);         break;
        case ',': addToken(TokenType::COMMA);         break;
        case '%': addToken(TokenType::PERCENT);       break;
        case '+': addToken(TokenType::PLUS);          break;
        case '-': addToken(TokenType::MINUS);         break;
        case '*': addToken(TokenType::STAR);          break;
        case '/': addToken(TokenType::SLASH);         break;
        case '{': addToken(TokenType::LEFT_BRACE);    break;
        case '}': addToken(TokenType::RIGHT_BRACE);   break;
        case '[': addToken(TokenType::LEFT_BRACKET);  break;
        case ']': addToken(TokenType::RIGHT_BRACKET); break;
        case '.': addToken(TokenType::DOT);           break;

        case '=':
            if (match('=')) addToken(TokenType::EQUAL_EQUAL, "==");
            else            addToken(TokenType::EQUALS, "=");
            break;

        case '>':
            if (match('='))      addToken(TokenType::GREATER_EQUAL, ">=");
            else if (match('>')) addToken(TokenType::STREAM_IN, ">>");
            else                 addToken(TokenType::GREATER, ">");
            break;

        case '<':
            if (match('=')) addToken(TokenType::LESS_EQUAL, "<=");
            else            addToken(TokenType::LESS, "<");
            break;

        case '!':
            if (match('=')) addToken(TokenType::BANG_EQUAL, "!=");
            else            addToken(TokenType::UNKNOWN, "!");
            break;

        case '"': stringLiteral(); break;

        default:
            if (c == 'f' && peek() == '"') {
                fStringLiteral();
            } else if (std::isdigit(static_cast<unsigned char>(c))) {
                numberLiteral();
            } else if (std::isalpha(static_cast<unsigned char>(c)) || c == '_') {
                identifierOrKeyword();
            } else {
                addToken(TokenType::UNKNOWN, std::string(1, c));
            }
            break;
    }
}

void Lexer::addToken(TokenType type) {
    addToken(type, m_source.substr(m_start, m_current - m_start));
}

void Lexer::addToken(TokenType type, const std::string& lexeme) {
    m_tokens.emplace_back(type, lexeme, m_line);
}

char Lexer::advance() { return m_source[m_current++]; }
char Lexer::peek() const { return isAtEnd() ? '\0' : m_source[m_current]; }
char Lexer::peekNext() const {
    if (m_current + 1 >= m_source.size()) return '\0';
    return m_source[m_current + 1];
}
bool Lexer::match(char expected) {
    if (isAtEnd()) return false;
    if (m_source[m_current] != expected) return false;
    m_current++;
    return true;
}
bool Lexer::isAtEnd() const { return m_current >= m_source.size(); }

void Lexer::stringLiteral() {
    while (peek() != '"' && !isAtEnd()) {
        if (peek() == '\\' && !isAtEnd()) {
            advance(); // consume the backslash so the escaped char (even a '"') isn't
                       // mistaken for the closing quote or a newline miscount
            if (isAtEnd()) break;
            if (peek() == '\n') m_line++;
            advance();
            continue;
        }
        if (peek() == '\n') m_line++;
        advance();
    }

    if (isAtEnd()) {
        std::string raw = m_source.substr(m_start + 1, m_current - m_start - 1);
        addToken(TokenType::STRING_LITERAL, unescape(raw));
        return;
    }

    advance(); // closing quote
    std::string raw = m_source.substr(m_start + 1, m_current - m_start - 2);
    addToken(TokenType::STRING_LITERAL, unescape(raw));
}

void Lexer::fStringLiteral() {
    advance(); // the opening '"'

    while (peek() != '"' && !isAtEnd()) {
        if (peek() == '\\' && !isAtEnd()) {
            advance(); // consume the backslash
            if (isAtEnd()) break;
            if (peek() == '\n') m_line++;
            advance();
            continue;
        }
        if (peek() == '\n') m_line++;
        advance();
    }

    if (isAtEnd()) {
        std::string raw = m_source.substr(m_start + 2, m_current - m_start - 2);
        addToken(TokenType::F_STRING, unescape(raw));
        return;
    }

    advance(); // closing quote
    std::string raw = m_source.substr(m_start + 2, m_current - m_start - 3);
    addToken(TokenType::F_STRING, unescape(raw));
}

// Turns backslash escape sequences (\n, \t, \r, \", \\) into their real
// characters. Interpolation braces { } in f-strings are untouched since
// they don't involve a backslash.
std::string Lexer::unescape(const std::string& raw) {
    std::string out;
    out.reserve(raw.size());
    for (size_t i = 0; i < raw.size(); ++i) {
        if (raw[i] == '\\' && i + 1 < raw.size()) {
            char next = raw[i + 1];
            switch (next) {
                case 'n':  out += '\n'; break;
                case 't':  out += '\t'; break;
                case 'r':  out += '\r'; break;
                case '"':  out += '"';  break;
                case '\\': out += '\\'; break;
                default:   out += next; break; // unknown escape: keep the literal char
            }
            ++i; // skip the escaped character
        } else {
            out += raw[i];
        }
    }
    return out;
}

void Lexer::numberLiteral() {
    while (std::isdigit(static_cast<unsigned char>(peek()))) {
        advance();
    }

    bool isFloat = false;
    if (peek() == '.' && std::isdigit(static_cast<unsigned char>(peekNext()))) {
        isFloat = true;
        advance(); // consume '.'
        while (std::isdigit(static_cast<unsigned char>(peek()))) {
            advance();
        }
    }

    addToken(isFloat ? TokenType::FLOAT : TokenType::NUMBER);
}

void Lexer::identifierOrKeyword() {
    while (std::isalnum(static_cast<unsigned char>(peek())) || peek() == '_') {
        advance();
    }

    std::string lexeme = m_source.substr(m_start, m_current - m_start);

    if (lexeme == "show")       addToken(TokenType::KEYWORD_SHOW, lexeme);
    else if (lexeme == "ask")   addToken(TokenType::KEYWORD_ASK, lexeme);
    else if (lexeme == "if")    addToken(TokenType::KEYWORD_IF, lexeme);
    else if (lexeme == "elf")   addToken(TokenType::KEYWORD_ELF, lexeme);
    else if (lexeme == "els" || lexeme == "else") addToken(TokenType::KEYWORD_ELS, lexeme);
    else if (lexeme == "while") addToken(TokenType::KEYWORD_WHILE, lexeme);
    else if (lexeme == "for")   addToken(TokenType::KEYWORD_FOR, lexeme);
    else if (lexeme == "loop")  addToken(TokenType::KEYWORD_LOOP, lexeme);
    else if (lexeme == "sloop") addToken(TokenType::KEYWORD_SLOOP, lexeme);
    else if (lexeme == "fn")    addToken(TokenType::KEYWORD_FN, lexeme);
    else if (lexeme == "cls")   addToken(TokenType::KEYWORD_CLS, lexeme);
    else if (lexeme == "rtn")   addToken(TokenType::KEYWORD_RTN, lexeme);
    else if (lexeme == "self")  addToken(TokenType::KEYWORD_SELF, lexeme);
    else if (lexeme == "try")   addToken(TokenType::KEYWORD_TRY, lexeme);
    else if (lexeme == "import") addToken(TokenType::KEYWORD_IMPORT, lexeme);
    else if (lexeme == "in")    addToken(TokenType::KEYWORD_IN, lexeme);
    else if (lexeme == "true")  addToken(TokenType::KW_TRUE, lexeme);
    else if (lexeme == "false") addToken(TokenType::KW_FALSE, lexeme);
    else                        addToken(TokenType::IDENTIFIER, lexeme);
}

} // namespace luin