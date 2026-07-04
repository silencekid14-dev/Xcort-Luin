#include "Parser.h"
#include "Lexer.h"
#include <stdexcept>
#include <iostream>
#include <cstdlib>

namespace luin {

std::unordered_map<TokenType, int> Parser::precedence = {
    {TokenType::PLUS,           2},
    {TokenType::MINUS,          2},
    {TokenType::STAR,           3},
    {TokenType::SLASH,          3},
    {TokenType::PERCENT,        3},
    {TokenType::EQUAL_EQUAL,    1},
    {TokenType::BANG_EQUAL,     1},
    {TokenType::GREATER,        1},
    {TokenType::GREATER_EQUAL,  1},
    {TokenType::LESS,           1},
    {TokenType::LESS_EQUAL,     1}
};

Parser::Parser(std::vector<Token> tokens) : m_tokens(std::move(tokens)) {}

std::unique_ptr<Expr> Parser::parseExpressionFromTokens(std::vector<Token> tokens) {
    Parser p(std::move(tokens));
    auto expr = p.parseExpression();
    if (!expr) throw std::runtime_error("Invalid expression inside f-string interpolation");
    if (!p.isAtEnd())
        throw std::runtime_error("Unexpected token '" + p.peek().lexeme +
                                 "' inside f-string interpolation");
    return expr;
}

std::unique_ptr<Program> Parser::parse() {
    try {
        return parseProgram();
    } catch (const std::runtime_error& e) {
        std::cerr << "Parse error: " << e.what() << std::endl;
        return nullptr;
    }
}

const Token& Parser::peek() const { return m_tokens[m_current]; }
const Token& Parser::previous() const { return m_tokens[m_current - 1]; }
bool Parser::isAtEnd() const { return peek().type == TokenType::END_OF_FILE; }
Token Parser::advance() {
    if (!isAtEnd()) m_current++;
    return previous();
}
bool Parser::check(TokenType type) const {
    if (isAtEnd()) return false;
    return peek().type == type;
}
bool Parser::match(TokenType type) {
    if (check(type)) { advance(); return true; }
    return false;
}
Token Parser::consume(TokenType type, const std::string& msg) {
    if (check(type)) return advance();
    errorAtCurrent(msg);
    throw std::runtime_error(msg);
}

void Parser::errorAt(const Token& token, const std::string& msg) {
    std::cerr << "[line " << token.line << "] Error at '" << token.lexeme << "': " << msg << std::endl;
}
void Parser::errorAtCurrent(const std::string& msg) {
    errorAt(peek(), msg);
}

std::unique_ptr<Program> Parser::parseProgram() {
    auto program = std::make_unique<Program>();
    while (!isAtEnd()) {
        auto stmt = parseStatement();
        if (stmt)
            program->statements.push_back(std::move(stmt));
        else
            advance(); // skip for error recovery
    }
    return program;
}

std::unique_ptr<Stmt> Parser::parseStatement() {
    if (match(TokenType::KEYWORD_SHOW))   return parseShowStmt();
    if (match(TokenType::KEYWORD_ASK))    return parseAskStmt();
    if (match(TokenType::KEYWORD_IF))     return parseIfStmt();
    if (match(TokenType::KEYWORD_WHILE))  return parseWhileStmt();
    if (match(TokenType::KEYWORD_FOR))    return parseForStmt();
    if (match(TokenType::KEYWORD_LOOP))   return parseLoopStmt();
    if (match(TokenType::KEYWORD_SLOOP))  return parseSloopStmt();
    if (match(TokenType::KEYWORD_RTN))    return parseRtnStmt();
    if (match(TokenType::KEYWORD_FN))     return parseFnStmt();
    if (match(TokenType::KEYWORD_CLS))    return parseClassStmt();

    // Support for expressions, variable assignments, and property mutations
    auto expr = parseExpression();
    
    // Check if it's an assignment operation
    if (match(TokenType::EQUALS)) {
        auto value = parseExpression();
        
        // 1. Is it a field assignment (e.g., self.hp = hp)?
        if (auto* mem = dynamic_cast<MemberAccessExpr*>(expr.get())) {
            return std::make_unique<MemberAssignmentStmt>(std::move(mem->object), mem->member, std::move(value));
        }
        
        // 2. Is it a normal variable assignment (e.g., counter = 0)?
        if (auto* var = dynamic_cast<VariableExpr*>(expr.get())) {
            return std::make_unique<AssignmentStmt>(var->name, std::move(value));
        }
        
        errorAtCurrent("Invalid assignment target.");
        return nullptr;
    }

    // 3. If no equals sign, it is a standalone expression statement (e.g., p.takeDamage(30))
    return std::make_unique<ExprStmt>(std::move(expr));
}

std::unique_ptr<BlockStmt> Parser::parseBlock() {
    consume(TokenType::LEFT_BRACE, "Expected '{' to start a block");
    auto block = std::make_unique<BlockStmt>();
    while (!check(TokenType::RIGHT_BRACE) && !isAtEnd()) {
        auto stmt = parseStatement();
        if (stmt) block->statements.push_back(std::move(stmt));
        else advance();
    }
    consume(TokenType::RIGHT_BRACE, "Expected '}' after block");
    return block;
}

std::unique_ptr<Stmt> Parser::parseShowStmt() {
    consume(TokenType::LEFT_PAREN, "Expected '(' after 'show'");
    auto expr = parseExpression();
    if (!expr) throw std::runtime_error("Missing expression in show");
    consume(TokenType::RIGHT_PAREN, "Expected ')' after show expression");
    return std::make_unique<ShowStmt>(std::move(expr));
}

std::unique_ptr<Stmt> Parser::parseAssignmentStmt() {
    Token name = consume(TokenType::IDENTIFIER, "Expected variable name");
    consume(TokenType::EQUALS, "Expected '=' in assignment");
    auto expr = parseExpression();
    if (!expr) throw std::runtime_error("Missing value expression");
    return std::make_unique<AssignmentStmt>(name.lexeme, std::move(expr));
}

std::unique_ptr<Stmt> Parser::parseAskStmt() {
    consume(TokenType::LEFT_PAREN, "Expected '(' after 'ask'");
    auto prompt = parseExpression();
    if (!prompt) throw std::runtime_error("Missing prompt in ask");
    consume(TokenType::STREAM_IN, "Expected '>>' after prompt");
    std::vector<std::string> vars;
    vars.push_back(consume(TokenType::IDENTIFIER, "Expected variable").lexeme);
    while (match(TokenType::COMMA))
        vars.push_back(consume(TokenType::IDENTIFIER, "Expected variable").lexeme);
    consume(TokenType::RIGHT_PAREN, "Expected ')' after ask variables");
    return std::make_unique<AskStmt>(std::move(prompt), std::move(vars));
}

std::unique_ptr<Stmt> Parser::parseIfStmt() {
    auto cond = parseExpression();
    if (!cond) throw std::runtime_error("Missing condition after 'if'");
    consume(TokenType::COLON, "Expected ':' after if condition");
    auto thenB = parseStatement();
    if (!thenB) throw std::runtime_error("Missing statement for if");
    std::unique_ptr<Stmt> elseB = nullptr;
    if (match(TokenType::KEYWORD_ELF)) {
        auto elfCond = parseExpression();
        if (!elfCond) throw std::runtime_error("Missing condition after 'elf'");
        consume(TokenType::COLON, "Expected ':' after elf condition");
        auto elfThen = parseStatement();
        if (!elfThen) throw std::runtime_error("Missing statement for elf");
        elseB = std::make_unique<IfStmt>(std::move(elfCond), std::move(elfThen));
        IfStmt* curIf = static_cast<IfStmt*>(elseB.get());
        while (true) {
            if (match(TokenType::KEYWORD_ELF)) {
                auto nextCond = parseExpression();
                if (!nextCond) throw std::runtime_error("Missing condition after 'elf'");
                consume(TokenType::COLON, "Expected ':'");
                auto nextThen = parseStatement();
                if (!nextThen) throw std::runtime_error("Missing statement for elf");
                auto nextIf = std::make_unique<IfStmt>(std::move(nextCond), std::move(nextThen));
                curIf->elseBranch = std::move(nextIf);
                curIf = static_cast<IfStmt*>(curIf->elseBranch.get());
            } else break;
        }
    }
    if (match(TokenType::KEYWORD_ELS)) {
        consume(TokenType::COLON, "Expected ':' after els");
        auto elsB = parseStatement();
        if (!elsB) throw std::runtime_error("Missing statement for els");
        if (elseB) {
            IfStmt* last = static_cast<IfStmt*>(elseB.get());
            while (last->elseBranch && dynamic_cast<IfStmt*>(last->elseBranch.get()))
                last = static_cast<IfStmt*>(last->elseBranch.get());
            last->elseBranch = std::move(elsB);
        } else {
            elseB = std::move(elsB);
        }
    }
    return std::make_unique<IfStmt>(std::move(cond), std::move(thenB), std::move(elseB));
}

std::unique_ptr<Stmt> Parser::parseWhileStmt() {
    auto cond = parseExpression();
    if (!cond) throw std::runtime_error("Missing condition for while");
    auto body = parseBlock();
    return std::make_unique<WhileStmt>(std::move(cond), std::move(body));
}

std::unique_ptr<Stmt> Parser::parseForStmt() {
    Token iter = consume(TokenType::IDENTIFIER, "Expected iterator variable after 'for'");
    auto iterable = parseExpression();
    if (!iterable) throw std::runtime_error("Missing iterable in for");
    auto body = parseBlock();
    return std::make_unique<ForStmt>(iter.lexeme, std::move(iterable), std::move(body));
}

std::unique_ptr<Stmt> Parser::parseLoopStmt() {
    auto body = parseBlock();
    return std::make_unique<LoopStmt>(std::move(body));
}

std::unique_ptr<Stmt> Parser::parseSloopStmt() {
    consume(TokenType::LEFT_PAREN, "Expected '(' after 'sloop'");
    consume(TokenType::RIGHT_PAREN, "Expected ')' after sloop");
    return std::make_unique<SloopStmt>();
}

std::unique_ptr<Stmt> Parser::parseRtnStmt() {
    if (check(TokenType::END_OF_FILE) || check(TokenType::RIGHT_BRACE) ||
        peek().line != previous().line)
        return std::make_unique<RtnStmt>();  // void return
    auto value = parseExpression();
    return std::make_unique<RtnStmt>(std::move(value));
}

std::unique_ptr<Stmt> Parser::parseFnStmt() {
    Token name = consume(TokenType::IDENTIFIER, "Expected function name after 'fn'");
    consume(TokenType::LEFT_PAREN, "Expected '(' after function name");
    std::vector<std::string> params;
    if (!check(TokenType::RIGHT_PAREN)) {
        do {
            params.push_back(consume(TokenType::IDENTIFIER, "Expected parameter name").lexeme);
        } while (match(TokenType::COMMA));
    }
    consume(TokenType::RIGHT_PAREN, "Expected ')' after parameters");
    auto body = parseBlock();
    return std::make_unique<FnStmt>(name.lexeme, std::move(params), std::move(body));
}

std::unique_ptr<Stmt> Parser::parseClassStmt() {
    Token name = consume(TokenType::IDENTIFIER, "Expected class name after 'cls'");
    consume(TokenType::LEFT_BRACE, "Expected '{' to start class body");
    std::vector<std::unique_ptr<FnStmt>> methods;
    while (!check(TokenType::RIGHT_BRACE) && !isAtEnd()) {
        if (match(TokenType::KEYWORD_FN)) {
            auto fn = parseFnStmt();
            // Downcast to FnStmt* (it is safe because parseFnStmt returns FnStmt unique_ptr)
            methods.push_back(std::unique_ptr<FnStmt>(static_cast<FnStmt*>(fn.release())));
        } else {
            errorAtCurrent("Only function definitions allowed in class body");
            advance();
        }
    }
    consume(TokenType::RIGHT_BRACE, "Expected '}' after class body");
    return std::make_unique<ClassStmt>(name.lexeme, std::move(methods));
}

// ---------- Expression parsing ----------

std::unique_ptr<Expr> Parser::parseExpression() {
    return parsePrecedence(0);
}

std::unique_ptr<Expr> Parser::parsePrecedence(int prec) {
    auto left = parsePrimary();
    if (!left) return nullptr;

    while (true) {
        TokenType opType = peek().type;
        auto it = precedence.find(opType);
        if (it == precedence.end()) break;
        int opPrec = it->second;
        if (opPrec < prec) break;

        advance(); // operator
        auto right = parsePrecedence(opPrec + 1);
        if (!right) throw std::runtime_error("Missing right operand");

        BinaryOp binOp;
        switch (opType) {
            case TokenType::PLUS:          binOp = BinaryOp::PLUS; break;
            case TokenType::MINUS:         binOp = BinaryOp::MINUS; break;
            case TokenType::STAR:          binOp = BinaryOp::STAR; break;
            case TokenType::SLASH:         binOp = BinaryOp::SLASH; break;
            case TokenType::PERCENT:       binOp = BinaryOp::PERCENT; break;
            case TokenType::EQUAL_EQUAL:   binOp = BinaryOp::EQUAL_EQUAL; break;
            case TokenType::BANG_EQUAL:    binOp = BinaryOp::BANG_EQUAL; break;
            case TokenType::GREATER:       binOp = BinaryOp::GREATER; break;
            case TokenType::GREATER_EQUAL: binOp = BinaryOp::GREATER_EQUAL; break;
            case TokenType::LESS:          binOp = BinaryOp::LESS; break;
            case TokenType::LESS_EQUAL:    binOp = BinaryOp::LESS_EQUAL; break;
            default: throw std::logic_error("Unreachable operator type");
        }

        left = std::make_unique<BinaryOpExpr>(binOp, std::move(left), std::move(right));
    }

    return left;
}

std::unique_ptr<Expr> Parser::parsePrimary() {
    if (match(TokenType::STRING_LITERAL))
        return std::make_unique<StringLiteral>(previous().lexeme);
    if (match(TokenType::F_STRING))
        return parseFString(previous().lexeme);
    if (match(TokenType::NUMBER))
        return std::make_unique<NumberLiteral>(std::stoi(previous().lexeme));
    if (match(TokenType::FLOAT))
        return std::make_unique<FloatLiteral>(std::stod(previous().lexeme));
    if (match(TokenType::KW_TRUE))
        return std::make_unique<BoolLiteral>(true);
    if (match(TokenType::KW_FALSE))
        return std::make_unique<BoolLiteral>(false);
    if (match(TokenType::KEYWORD_SELF)) {
        auto selfNode = std::make_unique<SelfExpr>();
        return parsePostfix(std::move(selfNode));
    }
    if (match(TokenType::IDENTIFIER)) {
        auto var = std::make_unique<VariableExpr>(previous().lexeme);
        return parsePostfix(std::move(var));
    }
    if (match(TokenType::LEFT_PAREN)) {
        auto expr = parseExpression();
        if (!expr) throw std::runtime_error("Missing expression inside parentheses");
        consume(TokenType::RIGHT_PAREN, "Expected ')' after grouped expression");
        return parsePostfix(std::move(expr));
    }
    if (match(TokenType::LEFT_BRACKET))
        return parseArrayLiteral();
    if (match(TokenType::MINUS)) {
        auto operand = parsePrimary();
        if (!operand) throw std::runtime_error("Missing operand after unary '-'");
        return std::make_unique<UnaryMinusExpr>(std::move(operand));
    }

    errorAtCurrent("Expected an expression");
    throw std::runtime_error("Invalid expression");
}

std::unique_ptr<Expr> Parser::parsePostfix(std::unique_ptr<Expr> left) {
    while (true) {
        if (match(TokenType::LEFT_PAREN)) {
            std::vector<std::unique_ptr<Expr>> args;
            if (!check(TokenType::RIGHT_PAREN)) {
                do {
                    args.push_back(parseExpression());
                } while (match(TokenType::COMMA));
            }
            consume(TokenType::RIGHT_PAREN, "Expected ')' after arguments");
            left = std::make_unique<CallExpr>(std::move(left), std::move(args));
        } else if (match(TokenType::DOT)) {
            Token member = consume(TokenType::IDENTIFIER, "Expected member name after '.'");
            left = std::make_unique<MemberAccessExpr>(std::move(left), member.lexeme);
        } else {
            break;
        }
    }
    return left;
}

std::unique_ptr<Expr> Parser::parseArrayLiteral() {
    std::vector<std::unique_ptr<Expr>> elements;
    if (!check(TokenType::RIGHT_BRACKET)) {
        do {
            elements.push_back(parseExpression());
        } while (match(TokenType::COMMA));
    }
    consume(TokenType::RIGHT_BRACKET, "Expected ']' after array elements");
    return std::make_unique<ArrayLiteral>(std::move(elements));
}

std::unique_ptr<Expr> Parser::parseFString(const std::string& raw) {
    std::vector<FStringLiteral::Segment> segments;
    std::string literal; // accumulates plain text until the next '{' or end

    auto flushLiteral = [&]() {
        if (!literal.empty()) {
            FStringLiteral::Segment seg;
            seg.literal = literal;
            seg.expr = nullptr;
            segments.push_back(std::move(seg));
            literal.clear();
        }
    };

    for (size_t i = 0; i < raw.size(); ++i) {
        char c = raw[i];

        if (c == '{' && i + 1 < raw.size() && raw[i + 1] == '{') {
            literal += '{'; // "{{" -> literal '{'
            ++i;
            continue;
        }
        if (c == '}' && i + 1 < raw.size() && raw[i + 1] == '}') {
            literal += '}'; // "}}" -> literal '}'
            ++i;
            continue;
        }

        if (c == '{') {
            size_t end = raw.find('}', i + 1);
            if (end == std::string::npos)
                throw std::runtime_error("Unterminated '{' in f-string interpolation");

            std::string innerSrc = raw.substr(i + 1, end - i - 1);
            flushLiteral();

            // Re-lex and parse the interpolated text as a standalone expression,
            // so any expression (calls, arithmetic, member access, ...) works.
            Lexer innerLexer(innerSrc);
            std::vector<Token> innerTokens = innerLexer.scanTokens();
            std::unique_ptr<Expr> innerExpr;
            try {
                innerExpr = parseExpressionFromTokens(std::move(innerTokens));
            } catch (const std::runtime_error& e) {
                throw std::runtime_error(std::string("In f-string interpolation '{") +
                                         innerSrc + "}': " + e.what());
            }

            FStringLiteral::Segment seg;
            seg.expr = std::move(innerExpr);
            segments.push_back(std::move(seg));

            i = end; // resume scanning after the closing '}'
            continue;
        }

        literal += c;
    }

    flushLiteral();
    return std::make_unique<FStringLiteral>(std::move(segments));
}

} // namespace luin