#include "Token.h"
#include <stdexcept>

namespace luin {

static std::string tokenTypeName(TokenType type) {
    switch (type) {
        case TokenType::KEYWORD_SHOW:      return "KEYWORD_SHOW";
        case TokenType::KEYWORD_ASK:       return "KEYWORD_ASK";
        case TokenType::KEYWORD_IF:        return "KEYWORD_IF";
        case TokenType::KEYWORD_ELF:       return "KEYWORD_ELF";
        case TokenType::KEYWORD_ELS:       return "KEYWORD_ELS";
        case TokenType::KEYWORD_WHILE:     return "KEYWORD_WHILE";
        case TokenType::KEYWORD_FOR:       return "KEYWORD_FOR";
        case TokenType::KEYWORD_LOOP:      return "KEYWORD_LOOP";
        case TokenType::KEYWORD_SLOOP:     return "KEYWORD_SLOOP";
        case TokenType::KEYWORD_FN:        return "KEYWORD_FN";
        case TokenType::KEYWORD_CLS:       return "KEYWORD_CLS";
        case TokenType::KEYWORD_RTN:       return "KEYWORD_RTN";
        case TokenType::KEYWORD_SELF:      return "KEYWORD_SELF";
        case TokenType::KEYWORD_TRY:       return "KEYWORD_TRY";
        case TokenType::KW_TRUE:           return "KW_TRUE";
        case TokenType::KW_FALSE:          return "KW_FALSE";
        case TokenType::IDENTIFIER:        return "IDENTIFIER";
        case TokenType::STRING_LITERAL:    return "STRING_LITERAL";
        case TokenType::F_STRING:          return "F_STRING";
        case TokenType::NUMBER:            return "NUMBER";
        case TokenType::FLOAT:             return "FLOAT";
        case TokenType::PLUS:              return "PLUS";
        case TokenType::MINUS:             return "MINUS";
        case TokenType::STAR:              return "STAR";
        case TokenType::SLASH:             return "SLASH";
        case TokenType::PERCENT:           return "PERCENT";
        case TokenType::EQUAL_EQUAL:       return "EQUAL_EQUAL";
        case TokenType::BANG_EQUAL:        return "BANG_EQUAL";
        case TokenType::GREATER:           return "GREATER";
        case TokenType::GREATER_EQUAL:     return "GREATER_EQUAL";
        case TokenType::LESS:              return "LESS";
        case TokenType::LESS_EQUAL:        return "LESS_EQUAL";
        case TokenType::EQUALS:            return "EQUALS";
        case TokenType::LEFT_PAREN:        return "LEFT_PAREN";
        case TokenType::RIGHT_PAREN:       return "RIGHT_PAREN";
        case TokenType::COMMA:             return "COMMA";
        case TokenType::COLON:             return "COLON";
        case TokenType::LEFT_BRACE:        return "LEFT_BRACE";
        case TokenType::RIGHT_BRACE:       return "RIGHT_BRACE";
        case TokenType::LEFT_BRACKET:      return "LEFT_BRACKET";
        case TokenType::RIGHT_BRACKET:     return "RIGHT_BRACKET";
        case TokenType::DOT:               return "DOT";
        case TokenType::STREAM_IN:         return "STREAM_IN";
        case TokenType::END_OF_FILE:       return "END_OF_FILE";
        case TokenType::UNKNOWN:           return "UNKNOWN";
        default: throw std::logic_error("Unhandled TokenType");
    }
}

std::string Token::toString() const {
    return "[line " + std::to_string(line) + "] " + tokenTypeName(type) +
           " '" + lexeme + "'";
}

std::ostream& operator<<(std::ostream& os, const Token& token) {
    os << token.toString();
    return os;
}

} // namespace luin