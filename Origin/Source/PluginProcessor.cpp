/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin processor.

  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"
#include <cmath>
#include <algorithm>

// Simple DelayLine class implementation
class DelayLine
{
public:
    DelayLine(int maxDelay = 1024) : maxDelaySize(maxDelay)
    {
        buffer.resize(maxDelaySize, 0.0f);
    }
    
    float process(float input, int delaySamples)
    {
        delaySamples = std::clamp(delaySamples, 1, maxDelaySize - 1); // Minimum 1 sample delay
        
        // Store the current input
        buffer[writeIndex] = input;
        
        // Calculate read index for the delayed sample
        int readIndex = writeIndex - delaySamples;
        if (readIndex < 0)
            readIndex += maxDelaySize;
        
        // Get the delayed sample
        float output = buffer[readIndex];
        
        // Advance write index
        writeIndex = (writeIndex + 1) % maxDelaySize;
        
        return output;
    }
    
    void clear()
    {
        std::fill(buffer.begin(), buffer.end(), 0.0f);
        writeIndex = 0;
    }

private:
    std::vector<float> buffer;
    int writeIndex = 0;
    int maxDelaySize;
};

//==============================================================================
OriginAudioProcessor::OriginAudioProcessor()
#ifndef JucePlugin_PreferredChannelConfigurations
     : AudioProcessor (BusesProperties()
                     #if ! JucePlugin_IsMidiEffect
                      #if ! JucePlugin_IsSynth
                       .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
                      #endif
                       .withOutput ("Output", juce::AudioChannelSet::stereo(), true)
                     #endif
                       )
#endif
{
    // Initialize variables
    variables["pi"] = static_cast<float>(M_PI);
    variables["e"] = static_cast<float>(M_E);
    variables["x"] = 0.0f;
    
    currentEquation = "x"; // Default pass-through
    equationValid = true;
}

OriginAudioProcessor::~OriginAudioProcessor()
{
}

//==============================================================================
const juce::String OriginAudioProcessor::getName() const
{
    return JucePlugin_Name;
}

bool OriginAudioProcessor::acceptsMidi() const
{
   #if JucePlugin_WantsMidiInput
    return true;
   #else
    return false;
   #endif
}

bool OriginAudioProcessor::producesMidi() const
{
   #if JucePlugin_ProducesMidiOutput
    return true;
   #else
    return false;
   #endif
}

bool OriginAudioProcessor::isMidiEffect() const
{
   #if JucePlugin_IsMidiEffect
    return true;
   #else
    return false;
   #endif
}

double OriginAudioProcessor::getTailLengthSeconds() const
{
    return 0.0;
}

int OriginAudioProcessor::getNumPrograms()
{
    return 1;   // NB: some hosts don't cope very well if you tell them there are 0 programs,
                // so this should be at least 1, even if you're not really implementing programs.
}

int OriginAudioProcessor::getCurrentProgram()
{
    return 0;
}

void OriginAudioProcessor::setCurrentProgram (int index)
{
}

const juce::String OriginAudioProcessor::getProgramName (int index)
{
    return {};
}

void OriginAudioProcessor::changeProgramName (int index, const juce::String& newName)
{
}

//==============================================================================
void OriginAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    // Use this method as the place to do any pre-playback
    // initialisation that you need..
    this->sampleRate = sampleRate;
    variables["fs"] = static_cast<float>(sampleRate);
    variables["Fs"] = static_cast<float>(sampleRate);
    resetDSP();
}

void OriginAudioProcessor::releaseResources()
{
    // When playback stops, you can use this as an opportunity to free up any
    // spare memory, etc.
    resetDSP();
}

#ifndef JucePlugin_PreferredChannelConfigurations
bool OriginAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
  #if JucePlugin_IsMidiEffect
    juce::ignoreUnused (layouts);
    return true;
  #else
    // This is the place where you check if the layout is supported.
    // In this template code we only support mono or stereo.
    // Some plugin hosts, such as certain GarageBand versions, will only
    // load plugins that support stereo bus layouts.
    if (layouts.getMainOutputChannelSet() != juce::AudioChannelSet::mono()
     && layouts.getMainOutputChannelSet() != juce::AudioChannelSet::stereo())
        return false;

    // This checks if the input layout matches the output layout
   #if ! JucePlugin_IsSynth
    if (layouts.getMainOutputChannelSet() != layouts.getMainInputChannelSet())
        return false;
   #endif

    return true;
  #endif
}
#endif

void OriginAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    juce::ScopedNoDenormals noDenormals;
    auto totalNumInputChannels  = getTotalNumInputChannels();
    auto totalNumOutputChannels = getTotalNumOutputChannels();

    // In case we have more outputs than inputs, this code clears any output
    // channels that didn't contain input data, (because these aren't
    // guaranteed to be empty - they may contain garbage).
    // This is here to avoid people getting screaming feedback
    // when they first compile a plugin, but obviously you don't need to keep
    // this code if your algorithm always overwrites all the output channels.
    for (auto i = totalNumInputChannels; i < totalNumOutputChannels; ++i)
        buffer.clear (i, 0, buffer.getNumSamples());

    // This is the place where you'd normally do the guts of your plugin's
    // audio processing...
    // Make sure to reset the state if your inner loop is processing
    // the samples and the outer loop is handling the channels.
    // Alternatively, you can process the samples with the channels
    // interleaved by keeping the same state.
    for (int channel = 0; channel < totalNumInputChannels; ++channel)
    {
        auto* channelData = buffer.getWritePointer (channel);

        // Process each sample through our simple DSP
        for (int sample = 0; sample < buffer.getNumSamples(); ++sample)
        {
            float inputSample = channelData[sample];
            float outputSample = processSample(inputSample);
            channelData[sample] = outputSample;
        }
    }
}

//==============================================================================
bool OriginAudioProcessor::hasEditor() const
{
    return true; // (change this to false if you choose to not supply an editor)
}

juce::AudioProcessorEditor* OriginAudioProcessor::createEditor()
{
    return new OriginAudioProcessorEditor (*this);
}

//==============================================================================
void OriginAudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    // You should use this method to store your parameters in the memory block.
    // You could do that either as raw data, or use the XML or ValueTree classes
    // as intermediaries to make it easy to save and load complex data.
}

void OriginAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    // You should use this method to restore your parameters from this memory block,
    // whose contents will have been created by the getStateInformation() call.
}

//==============================================================================
// This creates new instances of the plugin..
void OriginAudioProcessor::setEquation(const juce::String& equation)
{
    currentEquation = equation;
    // Simple validation - just check if it's not empty
    equationValid = !equation.isEmpty();
    if (!equationValid)
    {
        errorMessage = "Empty equation";
    }
    else
    {
        errorMessage.clear();
    }
}

bool OriginAudioProcessor::isEquationValid() const
{
    return equationValid;
}

juce::String OriginAudioProcessor::getEquationError() const
{
    return juce::String(errorMessage);
}

// Real-time mathematical expression evaluator
float OriginAudioProcessor::processSample(float input)
{
    if (!equationValid)
        return input;
    
    // Set current input
    variables["x"] = input;
    
    try
    {
        // Evaluate the mathematical expression
        float output = evaluateExpression(currentEquation, input);
        
        // Store output history for future references (y_prev, etc.)
        outputHistory["y_prev2"] = outputHistory["y_prev"];
        outputHistory["y_prev"] = output;
        
        return output;
    }
    catch (...)
    {
        return input; // Fall back to pass-through on error
    }
}

float OriginAudioProcessor::evaluateExpression(const juce::String& expr, float x)
{
    juce::String cleanExpr = expr.removeCharacters(" \t\n\r");
    int pos = 0;
    
    // Handle simple cases first
    if (cleanExpr == "x") return x;
    if (cleanExpr == "0") return 0.0f;
    if (cleanExpr == "1") return 1.0f;
    
    return evaluateTerm(cleanExpr, pos);
}

float OriginAudioProcessor::evaluateTerm(juce::String& expr, int& pos)
{
    float result = evaluateFactor(expr, pos);
    
    while (pos < expr.length())
    {
        skipWhitespace(expr, pos);
        if (pos >= expr.length()) break;
        
        char op = expr[pos];
        if (op == '+' || op == '-')
        {
            pos++;
            float nextFactor = evaluateFactor(expr, pos);
            if (op == '+')
                result += nextFactor;
            else
                result -= nextFactor;
        }
        else
        {
            break;
        }
    }
    
    return result;
}

float OriginAudioProcessor::evaluateFactor(juce::String& expr, int& pos)
{
    float result = 1.0f;
    bool firstTerm = true;
    
    while (pos < expr.length())
    {
        skipWhitespace(expr, pos);
        if (pos >= expr.length()) break;
        
        char ch = expr[pos];
        
        // Handle multiplication and division
        if (!firstTerm && ch != '*' && ch != '/' && ch != '+' && ch != '-')
        {
            // Implicit multiplication
        }
        else if (ch == '*' || ch == '/')
        {
            pos++;
            skipWhitespace(expr, pos);
        }
        else if (ch == '+' || ch == '-')
        {
            break; // Let evaluateTerm handle this
        }
        
        float factor = 1.0f;
        
        // Parse numbers
        if (std::isdigit(ch) || ch == '.')
        {
            factor = evaluateNumber(expr, pos);
        }
        // Parse variables
        else if (ch == 'x')
        {
            pos++;
            factor = variables["x"];
        }
        else if (expr.substring(pos).startsWith("y_prev"))
        {
            if (expr.substring(pos).startsWith("y_prev2"))
            {
                pos += 7; // "y_prev2"
                factor = outputHistory["y_prev2"];
            }
            else
            {
                pos += 6; // "y_prev"
                factor = outputHistory["y_prev"];
            }
        }
        // Parse z^-n (delay terms)
        else if (expr.substring(pos).startsWith("z^-"))
        {
            pos += 3; // Skip "z^-"
            int delayAmount = static_cast<int>(evaluateNumber(expr, pos));
            DelayLine* delay = getDelayLine(delayAmount);
            factor = delay->process(variables["x"], delayAmount);
        }
        // Parse parentheses
        else if (ch == '(')
        {
            pos++; // Skip '('
            factor = evaluateTerm(expr, pos);
            if (pos < expr.length() && expr[pos] == ')')
                pos++; // Skip ')'
        }
        else
        {
            pos++; // Skip unknown character
            continue;
        }
        
        if (firstTerm)
        {
            result = factor;
            firstTerm = false;
        }
        else
        {
            if (pos > 1 && expr[pos-2] == '/')
                result /= factor;
            else
                result *= factor;
        }
        
        skipWhitespace(expr, pos);
        if (pos >= expr.length() || (expr[pos] != '*' && expr[pos] != '/' && !std::isdigit(expr[pos]) && expr[pos] != 'x' && expr[pos] != 'y' && expr[pos] != 'z' && expr[pos] != '('))
            break;
    }
    
    return result;
}

float OriginAudioProcessor::evaluateNumber(juce::String& expr, int& pos)
{
    juce::String numberStr;
    
    while (pos < expr.length() && (std::isdigit(expr[pos]) || expr[pos] == '.'))
    {
        numberStr += expr[pos];
        pos++;
    }
    
    return numberStr.getFloatValue();
}

void OriginAudioProcessor::skipWhitespace(const juce::String& expr, int& pos)
{
    while (pos < expr.length() && std::isspace(expr[pos]))
        pos++;
}

void OriginAudioProcessor::resetDSP()
{
    for (auto& pair : delayLines)
    {
        pair.second->clear();
    }
    variables["x"] = 0.0f;
}

DelayLine* OriginAudioProcessor::getDelayLine(int delayAmount)
{
    auto it = delayLines.find(delayAmount);
    if (it == delayLines.end())
    {
        auto delayLine = std::make_unique<DelayLine>(std::max(delayAmount + 1, 1024));
        DelayLine* ptr = delayLine.get();
        delayLines[delayAmount] = std::move(delayLine);
        return ptr;
    }
    return it->second.get();
}

//==============================================================================
// This creates new instances of the plugin..
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new OriginAudioProcessor();
}
