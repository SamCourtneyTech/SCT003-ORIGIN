/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin editor.

  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"

//==============================================================================
OriginAudioProcessorEditor::OriginAudioProcessorEditor (OriginAudioProcessor& p)
    : AudioProcessorEditor (&p), audioProcessor (p)
{
    // Setup equation label
    equationLabel.setText("MATLAB Equation:", juce::dontSendNotification);
    equationLabel.setFont(juce::FontOptions(16.0f, juce::Font::bold));
    equationLabel.setColour(juce::Label::textColourId, juce::Colours::white);
    addAndMakeVisible(equationLabel);
    
    // Setup equation editor
    equationEditor.setMultiLine(false);
    equationEditor.setFont(juce::Font(juce::Font::getDefaultMonospacedFontName(), 14.0f, juce::Font::plain));
    equationEditor.setText(audioProcessor.getCurrentEquation());
    equationEditor.addListener(this);
    addAndMakeVisible(equationEditor);
    
    // Setup status label
    statusLabel.setFont(juce::FontOptions(12.0f));
    statusLabel.setColour(juce::Label::textColourId, juce::Colours::lightgreen);
    addAndMakeVisible(statusLabel);
    
    // Setup examples label
    examplesLabel.setText("Examples:\n" 
                         "x (pass-through)\n"
                         "0.5 * x (half volume)\n"
                         "x + 0.3 * z^-1 (echo)\n"
                         "0.1 * x + 0.9 * y_prev (low-pass)\n"
                         "x - 0.95 * z^-1 (high-pass)\n"
                         "0.5 * (x + z^-1) (comb filter)", juce::dontSendNotification);
    examplesLabel.setFont(juce::FontOptions(11.0f));
    examplesLabel.setColour(juce::Label::textColourId, juce::Colours::lightgrey);
    examplesLabel.setJustificationType(juce::Justification::topLeft);
    addAndMakeVisible(examplesLabel);
    
    updateStatus();
    
    setSize (500, 400);
}

OriginAudioProcessorEditor::~OriginAudioProcessorEditor()
{
}

//==============================================================================
void OriginAudioProcessorEditor::paint (juce::Graphics& g)
{
    // Fill background with dark color
    g.fillAll (juce::Colour(0xff2a2a2a));
    
    // Draw title
    g.setColour (juce::Colours::white);
    g.setFont (juce::FontOptions (24.0f, juce::Font::bold));
    g.drawText ("ORIGIN (SCT#003)", 20, 10, getWidth() - 40, 30, juce::Justification::centred);
    
    g.setColour (juce::Colours::lightgrey);
    g.setFont (juce::FontOptions (12.0f));
    g.drawText ("MATLAB DSP Equation Processor", 20, 40, getWidth() - 40, 20, juce::Justification::centred);
}

void OriginAudioProcessorEditor::resized()
{
    auto bounds = getLocalBounds();
    bounds.removeFromTop(70); // Leave space for title
    bounds.reduce(20, 10);
    
    auto topSection = bounds.removeFromTop(80);
    equationLabel.setBounds(topSection.removeFromTop(25));
    equationEditor.setBounds(topSection.removeFromTop(30));
    statusLabel.setBounds(topSection.removeFromTop(20));
    
    bounds.removeFromTop(20); // Gap
    examplesLabel.setBounds(bounds);
}

void OriginAudioProcessorEditor::textEditorReturnKeyPressed(juce::TextEditor& editor)
{
    if (&editor == &equationEditor)
    {
        updateEquation();
    }
}

void OriginAudioProcessorEditor::textEditorFocusLost(juce::TextEditor& editor)
{
    if (&editor == &equationEditor)
    {
        updateEquation();
    }
}

void OriginAudioProcessorEditor::updateEquation()
{
    audioProcessor.setEquation(equationEditor.getText());
    updateStatus();
}

void OriginAudioProcessorEditor::updateStatus()
{
    if (audioProcessor.isEquationValid())
    {
        statusLabel.setText("✓ Equation valid", juce::dontSendNotification);
        statusLabel.setColour(juce::Label::textColourId, juce::Colours::lightgreen);
    }
    else
    {
        auto error = audioProcessor.getEquationError();
        statusLabel.setText("✗ " + error, juce::dontSendNotification);
        statusLabel.setColour(juce::Label::textColourId, juce::Colours::red);
    }
}
