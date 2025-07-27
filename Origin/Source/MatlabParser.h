#pragma once

#include <JuceHeader.h>
#include <string>
#include <vector>
#include <map>
#include <memory>

class MatlabParser
{
public:
    enum class TokenType
    {
        Number,
        Variable,
        Operator,
        Function,
        LeftParen,
        RightParen,
        Comma,
        End
    };

    struct Token
    {
        TokenType type;
        std::string value;
        double numericValue = 0.0;
    };

    struct ASTNode
    {
        enum class Type { Number, Variable, BinaryOp, UnaryOp, Function, Delay };
        Type type;
        std::string value;
        double numericValue = 0.0;
        int delayAmount = 0; // For z^-n operations
        std::vector<std::unique_ptr<ASTNode>> children;
    };

    MatlabParser();
    ~MatlabParser();

    bool parseEquation(const std::string& equation);
    std::string getErrorMessage() const;

    // Supported MATLAB-style functions and operators
    static bool isSupportedFunction(const std::string& name);
    static bool isSupportedOperator(char op);

private:
    std::vector<Token> tokenize(const std::string& equation);
    std::unique_ptr<ASTNode> parseExpression();
    std::unique_ptr<ASTNode> parseTerm();
    std::unique_ptr<ASTNode> parseFactor();
    std::unique_ptr<ASTNode> parseFunction(const std::string& name);
    std::unique_ptr<ASTNode> parseDelay(); // For z^-n notation

    std::vector<Token> tokens;
    size_t currentToken = 0;
    std::string errorMessage;

    bool isAtEnd() const { return currentToken >= tokens.size(); }
    const Token& peek() const;
    const Token& advance();
    bool match(TokenType type);
    bool check(TokenType type) const;
};