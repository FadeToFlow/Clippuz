/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin processor.

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include "EqFilter.h"

//==============================================================================
/**
*/
class ClippuzAudioProcessor  : public juce::AudioProcessor
{
public:
    //==============================================================================
    ClippuzAudioProcessor();
    ~ClippuzAudioProcessor() override;

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

private:
    juce::AudioProcessorValueTreeState::ParameterLayout createParameters();
    juce::AudioProcessorValueTreeState apvts;

    std::vector<std::unique_ptr<juce::dsp::Oversampling<float>>> oversamplers;

    size_t prevOsIndex;

  
    EqFilter eq1_f1  { EqFilter::Type::Peak, 383.1f, 7.2f, EqFilter::OctToQ(2.60f) };
    
    
    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ClippuzAudioProcessor)
};
