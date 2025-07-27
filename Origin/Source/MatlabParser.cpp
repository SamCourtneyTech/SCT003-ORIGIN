#include "MatlabParser.h"
#include <regex>
#include <cctype>
#include <cmath>
#include <memory>
#include <algorithm>

MatlabParser::MatlabParser() = default;
MatlabParser::~MatlabParser() = default;

bool MatlabParser::parseEquation(const std::string& equation)
{
    errorMessage.clear();
    tokens = tokenize(equation);
    currentToken = 0;

    if (tokens.empty())
    {
        errorMessage = "Empty equation";
        return false;
    }

    try
    {
        auto ast = parseExpression();
        if (!isAtEnd())
        {
            errorMessage = "Unexpected tokens at end of expression";
            return false;
        }
        return ast != nullptr;
    }
    catch (const std::exception& e)
    {
        errorMessage = e.what();
        return false;
    }
}

std::vector<MatlabParser::Token> MatlabParser::tokenize(const std::string& equation)
{
    std::vector<Token> result;
    std::string input = equation;
    
    // Remove whitespace
    input.erase(std::remove_if(input.begin(), input.end(), ::isspace), input.end());
    
    for (size_t i = 0; i < input.length(); ++i)
    {
        char c = input[i];
        
        if (std::isdigit(c) || c == '.')
        {
            // Parse number
            std::string number;
            while (i < input.length() && (std::isdigit(input[i]) || input[i] == '.'))
            {
                number += input[i++];
            }
            --i; // Back up one
            
            Token token;
            token.type = TokenType::Number;
            token.value = number;
            token.numericValue = std::stod(number);
            result.push_back(token);
        }
        else if (std::isalpha(c))
        {
            // Parse variable or function
            std::string name;
            while (i < input.length() && (std::isalnum(input[i]) || input[i] == '_'))
            {
                name += input[i++];
            }
            --i; // Back up one
            
            Token token;
            token.type = isSupportedFunction(name) ? TokenType::Function : TokenType::Variable;
            token.value = name;
            result.push_back(token);
        }
        else if (c == 'z' && i + 2 < input.length() && input[i + 1] == '^' && input[i + 2] == '-')
        {
            // Handle z^-n delay notation
            i += 3; // Skip "z^-"
            std::string delayNum;
            while (i < input.length() && std::isdigit(input[i]))
            {
                delayNum += input[i++];
            }
            --i; // Back up one
            
            Token token;
            token.type = TokenType::Variable;
            token.value = "z^-" + delayNum;
            token.numericValue = std::stod(delayNum);
            result.push_back(token);
        }
        else if (isSupportedOperator(c))
        {
            Token token;
            token.type = TokenType::Operator;
            token.value = c;
            result.push_back(token);
        }
        else if (c == '(')
        {
            Token token;
            token.type = TokenType::LeftParen;
            token.value = c;
            result.push_back(token);
        }
        else if (c == ')')
        {
            Token token;
            token.type = TokenType::RightParen;
            token.value = c;
            result.push_back(token);
        }
        else if (c == ',')
        {
            Token token;
            token.type = TokenType::Comma;
            token.value = c;
            result.push_back(token);
        }
    }
    
    Token endToken;
    endToken.type = TokenType::End;
    result.push_back(endToken);
    
    return result;
}

std::unique_ptr<MatlabParser::ASTNode> MatlabParser::parseExpression()
{
    auto left = parseTerm();
    
    while (match(TokenType::Operator) && (peek().value == "+" || peek().value == "-"))
    {
        std::string op = advance().value;
        auto right = parseTerm();
        
        auto node = std::make_unique<ASTNode>();
        node->type = ASTNode::Type::BinaryOp;
        node->value = op;
        node->children.push_back(std::move(left));
        node->children.push_back(std::move(right));
        left = std::move(node);
    }
    
    return left;
}

std::unique_ptr<MatlabParser::ASTNode> MatlabParser::parseTerm()
{
    auto left = parseFactor();
    
    while (match(TokenType::Operator) && (peek().value == "*" || peek().value == "/"))
    {
        std::string op = advance().value;
        auto right = parseFactor();
        
        auto node = std::make_unique<ASTNode>();
        node->type = ASTNode::Type::BinaryOp;
        node->value = op;
        node->children.push_back(std::move(left));
        node->children.push_back(std::move(right));
        left = std::move(node);
    }
    
    return left;
}

std::unique_ptr<MatlabParser::ASTNode> MatlabParser::parseFactor()
{
    if (match(TokenType::Number))
    {
        auto token = advance();
        auto node = std::make_unique<ASTNode>();
        node->type = ASTNode::Type::Number;
        node->numericValue = token.numericValue;
        node->value = token.value;
        return node;
    }
    
    if (match(TokenType::Variable))
    {
        auto token = advance();
        auto node = std::make_unique<ASTNode>();
        
        if (token.value.find("z^-") == 0)
        {
            // This is a delay operation
            node->type = ASTNode::Type::Delay;
            node->delayAmount = static_cast<int>(token.numericValue);
            node->value = token.value;
        }
        else
        {
            node->type = ASTNode::Type::Variable;
            node->value = token.value;
        }
        return node;
    }
    
    if (match(TokenType::Function))
    {
        auto token = advance();
        return parseFunction(token.value);
    }
    
    if (match(TokenType::LeftParen))
    {
        advance(); // consume '('
        auto expr = parseExpression();
        if (!match(TokenType::RightParen))
        {
            throw std::runtime_error("Expected ')' after expression");
        }
        advance(); // consume ')'
        return expr;
    }
    
    throw std::runtime_error("Unexpected token in expression");
}

std::unique_ptr<MatlabParser::ASTNode> MatlabParser::parseFunction(const std::string& name)
{
    auto node = std::make_unique<ASTNode>();
    node->type = ASTNode::Type::Function;
    node->value = name;
    
    if (!match(TokenType::LeftParen))
    {
        throw std::runtime_error("Expected '(' after function name");
    }
    advance(); // consume '('
    
    // Parse function arguments
    if (!match(TokenType::RightParen))
    {
        do
        {
            node->children.push_back(parseExpression());
        } while (match(TokenType::Comma) && (advance(), true));
    }
    
    if (!match(TokenType::RightParen))
    {
        throw std::runtime_error("Expected ')' after function arguments");
    }
    advance(); // consume ')'
    
    return node;
}

bool MatlabParser::isSupportedFunction(const std::string& name)
{
    static const std::vector<std::string> functions = {
        "sin", "cos", "tan", "exp", "log", "log10", "sqrt", "abs",
        "filter", "conv", "fft", "ifft", "freqz", "butter", "cheby1", "cheby2"
    };
    
    return std::find(functions.begin(), functions.end(), name) != functions.end();
}

bool MatlabParser::isSupportedOperator(char op)
{
    return op == '+' || op == '-' || op == '*' || op == '/' || op == '^';
}

const MatlabParser::Token& MatlabParser::peek() const
{
    if (isAtEnd()) return tokens.back(); // Return End token
    return tokens[currentToken];
}

const MatlabParser::Token& MatlabParser::advance()
{
    if (!isAtEnd()) ++currentToken;
    return tokens[currentToken - 1];
}

bool MatlabParser::match(TokenType type)
{
    if (check(type)) return true;
    return false;
}

bool MatlabParser::check(TokenType type) const
{
    if (isAtEnd()) return false;
    return peek().type == type;
}

std::string MatlabParser::getErrorMessage() const
{
    return errorMessage;
}