#pragma once

#include <JuceHeader.h>
#include "MatlabParser.h"
#include <map>
#include <vector>
#include <memory>

class DelayLine
{
public:
    DelayLine(int maxDelay = 1024);
    ~DelayLine() = default;

    void setMaxDelay(int samples);
    float process(float input, int delaySamples);
    void clear();

private:
    std::vector<float> buffer;
    int writeIndex = 0;
    int maxDelaySize;
};

class DSPEngine
{
public:
    DSPEngine();
    ~DSPEngine();

    void setSampleRate(double sampleRate);
    void setEquation(const std::string& equation);
    bool isEquationValid() const { return equationValid; }
    std::string getErrorMessage() const { return errorMessage; }

    float processSample(float input);
    void reset();

    // Variable management
    void setVariable(const std::string& name, float value);
    float getVariable(const std::string& name) const;

private:
    std::unique_ptr<MatlabParser> parser;
    std::unique_ptr<MatlabParser::ASTNode> ast;
    
    std::map<std::string, float> variables;
    std::map<int, std::unique_ptr<DelayLine>> delayLines; // delay amount -> delay line
    
    double sampleRate = 44100.0;
    bool equationValid = false;
    std::string errorMessage;
    
    float evaluateNode(const MatlabParser::ASTNode* node, float currentInput);
    float evaluateFunction(const std::string& name, const std::vector<float>& args);
    float evaluateBinaryOp(const std::string& op, float left, float right);
    
    DelayLine* getDelayLine(int delayAmount);
};