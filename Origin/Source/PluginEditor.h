/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin editor.

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include "PluginProcessor.h"

//==============================================================================
/**
*/
class OriginAudioProcessorEditor  : public juce::AudioProcessorEditor, public juce::TextEditor::Listener
{
public:
    OriginAudioProcessorEditor (OriginAudioProcessor&);
    ~OriginAudioProcessorEditor() override;

    //==============================================================================
    void paint (juce::Graphics&) override;
    void resized() override;
    
    void textEditorReturnKeyPressed(juce::TextEditor& editor) override;
    void textEditorFocusLost(juce::TextEditor& editor) override;

private:
    // This reference is provided as a quick way for your editor to
    // access the processor object that created it.
    OriginAudioProcessor& audioProcessor;
    
    juce::Label equationLabel;
    juce::TextEditor equationEditor;
    juce::Label statusLabel;
    juce::Label examplesLabel;
    
    void updateEquation();
    void updateStatus();

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (OriginAudioProcessorEditor)
};
