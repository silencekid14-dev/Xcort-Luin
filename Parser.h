#ifndef LUIN_PARSER_H
#define LUIN_PARSER_H

#include <vector>
#include <memory>
#include <unordered_map>
#include "Token.h"
#include "AST.h"

namespace luin {

class Parser {
public:
    explicit Parser(std::vector<Token> tokens);
    std::unique_ptr<Program> parse();

    // Parses a single standalone expression from an already-tokenized
    // fragment (used to compile the {...} interpolations inside f-strings).
    static std::unique_ptr<Expr> parseExpressionFromTokens(std::vector<Token> tokens);

private:
    const Token& peek() const;
    const Token& previous() const;
    bool isAtEnd() const;
    Token advance();
    bool check(TokenType type) const;
    bool match(TokenType type);
    Token consume(TokenType type, const std::string& errorMessage);

    // Statement parsers
    std::unique_ptr<Program> parseProgram();
    std::unique_ptr<Stmt> parseStatement();
    std::unique_ptr<BlockStmt> parseBlock();
    std::unique_ptr<Stmt> parseShowStmt();
    std::unique_ptr<Stmt> parseAssignmentStmt();
    std::unique_ptr<Stmt> parseAskStmt();
    std::unique_ptr<Stmt> parseIfStmt();
    std::unique_ptr<Stmt> parseWhileStmt();
    std::unique_ptr<Stmt> parseForStmt();
    std::unique_ptr<Stmt> parseLoopStmt();
    std::unique_ptr<Stmt> parseSloopStmt();
    std::unique_ptr<Stmt> parseRtnStmt();
    std::unique_ptr<Stmt> parseFnStmt();
    std::unique_ptr<Stmt> parseClassStmt();

    // Expression handling
    std::unique_ptr<Expr> parseExpression();
    std::unique_ptr<Expr> parsePrecedence(int precedence);
    std::unique_ptr<Expr> parsePrimary();
    std::unique_ptr<Expr> parsePostfix(std::unique_ptr<Expr> left);
    std::unique_ptr<Expr> parseArrayLiteral();
    std::unique_ptr<Expr> parseFString(const std::string& raw);

    static std::unordered_map<TokenType, int> precedence;

    void errorAt(const Token& token, const std::string& message);
    void errorAtCurrent(const std::string& message);

    std::vector<Token> m_tokens;
    size_t m_current = 0;
};

} // namespace luin

#endif // LUIN_PARSER_H