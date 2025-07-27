/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin processor.

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include <map>
#include <vector>
#include <memory>
#include <string>

// Forward declarations
class DelayLine;
class MatlabParser;

//==============================================================================
/**
*/
class OriginAudioProcessor  : public juce::AudioProcessor
{
public:
    //==============================================================================
    OriginAudioProcessor();
    ~OriginAudioProcessor() override;

    //==============================================================================
    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;

   #ifndef JucePlugin_PreferredChannelConfigurations
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
   #endif

    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    //==============================================================================
    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override;

    //==============================================================================
    const juce::String getName() const override;

    bool acceptsMidi() const override;
    bool producesMidi() const override;
    bool isMidiEffect() const override;
    double getTailLengthSeconds() const override;

    //==============================================================================
    int getNumPrograms() override;
    int getCurrentProgram() override;
    void setCurrentProgram (int index) override;
    const juce::String getProgramName (int index) override;
    void changeProgramName (int index, const juce::String& newName) override;

    //==============================================================================
    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    juce::String getCurrentEquation() const { return currentEquation; }
    void setEquation(const juce::String& equation);
    bool isEquationValid() const;
    juce::String getEquationError() const;

private:
    //==============================================================================
    // Real-time equation evaluator
    std::map<std::string, float> variables;
    std::map<int, std::unique_ptr<DelayLine>> delayLines;
    std::map<std::string, float> outputHistory; // For y_prev, y_prev2, etc.
    juce::String currentEquation;
    bool equationValid = false;
    std::string errorMessage;
    double sampleRate = 44100.0;
    
    // Equation processing methods
    float processSample(float input);
    void resetDSP();
    DelayLine* getDelayLine(int delayAmount);
    
    // Mathematical expression evaluator
    float evaluateExpression(const juce::String& expr, float x);
    float evaluateTerm(juce::String& expr, int& pos);
    float evaluateFactor(juce::String& expr, int& pos);
    float evaluateNumber(juce::String& expr, int& pos);
    void skipWhitespace(const juce::String& expr, int& pos);
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (OriginAudioProcessor)
};
