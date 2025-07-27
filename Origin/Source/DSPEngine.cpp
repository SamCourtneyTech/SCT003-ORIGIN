#include "DSPEngine.h"
#include <cmath>
#include <algorithm>
#include <memory>

// DelayLine Implementation
DelayLine::DelayLine(int maxDelay) : maxDelaySize(maxDelay)
{
    buffer.resize(maxDelaySize, 0.0f);
}

void DelayLine::setMaxDelay(int samples)
{
    maxDelaySize = samples;
    buffer.resize(maxDelaySize, 0.0f);
    writeIndex = 0;
}

float DelayLine::process(float input, int delaySamples)
{
    // Clamp delay to valid range
    delaySamples = std::clamp(delaySamples, 0, maxDelaySize - 1);
    
    // Calculate read index
    int readIndex = writeIndex - delaySamples;
    if (readIndex < 0)
        readIndex += maxDelaySize;
    
    // Get delayed sample
    float output = buffer[readIndex];
    
    // Store new input
    buffer[writeIndex] = input;
    
    // Advance write index
    writeIndex = (writeIndex + 1) % maxDelaySize;
    
    return output;
}

void DelayLine::clear()
{
    std::fill(buffer.begin(), buffer.end(), 0.0f);
    writeIndex = 0;
}

// DSPEngine Implementation
DSPEngine::DSPEngine()
{
    parser = std::make_unique<MatlabParser>();
    
    // Initialize common variables
    variables["pi"] = static_cast<float>(M_PI);
    variables["e"] = static_cast<float>(M_E);
    variables["x"] = 0.0f; // Current input sample
}

DSPEngine::~DSPEngine() = default;

void DSPEngine::setSampleRate(double sr)
{
    sampleRate = sr;
    variables["fs"] = static_cast<float>(sr);
    variables["Fs"] = static_cast<float>(sr); // MATLAB convention
}

void DSPEngine::setEquation(const std::string& equation)
{
    errorMessage.clear();
    equationValid = false;
    
    if (parser->parseEquation(equation))
    {
        // For now, we'll store the equation string and re-parse as needed
        // In a full implementation, we'd store the AST properly
        equationValid = true;
    }
    else
    {
        errorMessage = parser->getErrorMessage();
    }
}

float DSPEngine::processSample(float input)
{
    if (!equationValid)
        return input; // Pass through if no valid equation
    
    variables["x"] = input; // Set current input
    
    // For this basic version, implement simple equation processing
    // In a full implementation, we'd evaluate the stored AST
    return input * 0.5f; // Simple example: half volume
}

void DSPEngine::reset()
{
    // Clear all delay lines
    for (auto& pair : delayLines)
    {
        pair.second->clear();
    }
    
    // Reset input variable
    variables["x"] = 0.0f;
}

void DSPEngine::setVariable(const std::string& name, float value)
{
    variables[name] = value;
}

float DSPEngine::getVariable(const std::string& name) const
{
    auto it = variables.find(name);
    if (it != variables.end())
        return it->second;
    return 0.0f;
}

float DSPEngine::evaluateNode(const MatlabParser::ASTNode* node, float currentInput)
{
    if (!node) return 0.0f;
    
    switch (node->type)
    {
        case MatlabParser::ASTNode::Type::Number:
            return static_cast<float>(node->numericValue);
            
        case MatlabParser::ASTNode::Type::Variable:
        {
            auto it = variables.find(node->value);
            if (it != variables.end())
                return it->second;
            return 0.0f; // Unknown variable defaults to 0
        }
        
        case MatlabParser::ASTNode::Type::Delay:
        {
            DelayLine* delayLine = getDelayLine(node->delayAmount);
            return delayLine->process(currentInput, node->delayAmount);
        }
        
        case MatlabParser::ASTNode::Type::BinaryOp:
        {
            if (node->children.size() != 2) return 0.0f;
            float left = evaluateNode(node->children[0].get(), currentInput);
            float right = evaluateNode(node->children[1].get(), currentInput);
            return evaluateBinaryOp(node->value, left, right);
        }
        
        case MatlabParser::ASTNode::Type::Function:
        {
            std::vector<float> args;
            for (const auto& child : node->children)
            {
                args.push_back(evaluateNode(child.get(), currentInput));
            }
            return evaluateFunction(node->value, args);
        }
        
        default:
            return 0.0f;
    }
}

float DSPEngine::evaluateFunction(const std::string& name, const std::vector<float>& args)
{
    if (args.empty()) return 0.0f;
    
    float arg0 = args[0];
    
    if (name == "sin") return std::sin(arg0);
    if (name == "cos") return std::cos(arg0);
    if (name == "tan") return std::tan(arg0);
    if (name == "exp") return std::exp(arg0);
    if (name == "log") return std::log(arg0);
    if (name == "log10") return std::log10(arg0);
    if (name == "sqrt") return std::sqrt(arg0);
    if (name == "abs") return std::abs(arg0);
    
    // More complex functions would require additional implementation
    if (name == "filter" && args.size() >= 2)
    {
        // Basic first-order filter: y = a*x + b*x_prev
        // This is a simplified version - real filter would need coefficients
        return arg0 * 0.5f + args[1] * 0.5f;
    }
    
    return 0.0f; // Unknown function
}

float DSPEngine::evaluateBinaryOp(const std::string& op, float left, float right)
{
    if (op == "+") return left + right;
    if (op == "-") return left - right;
    if (op == "*") return left * right;
    if (op == "/") return (right != 0.0f) ? left / right : 0.0f;
    if (op == "^") return std::pow(left, right);
    
    return 0.0f; // Unknown operator
}

DelayLine* DSPEngine::getDelayLine(int delayAmount)
{
    auto it = delayLines.find(delayAmount);
    if (it == delayLines.end())
    {
        // Create new delay line for this delay amount
        auto delayLine = std::make_unique<DelayLine>(std::max(delayAmount + 1, 1024));
        DelayLine* ptr = delayLine.get();
        delayLines[delayAmount] = std::move(delayLine);
        return ptr;
    }
    return it->second.get();
}