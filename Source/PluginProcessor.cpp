/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin processor.

  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"

//==============================================================================
ClippuzAudioProcessor::ClippuzAudioProcessor()
#ifndef JucePlugin_PreferredChannelConfigurations
     : AudioProcessor (BusesProperties()
                     #if ! JucePlugin_IsMidiEffect
                      #if ! JucePlugin_IsSynth
                       .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
                      #endif
                       .withOutput ("Output", juce::AudioChannelSet::stereo(), true)
                     #endif
                       ), apvts (*this, nullptr, "Parameters", createParameters())
#endif
{
}

ClippuzAudioProcessor::~ClippuzAudioProcessor()
{
}

//==============================================================================
const juce::String ClippuzAudioProcessor::getName() const
{
    return JucePlugin_Name;
}

bool ClippuzAudioProcessor::acceptsMidi() const
{
   #if JucePlugin_WantsMidiInput
    return true;
   #else
    return false;
   #endif
}

bool ClippuzAudioProcessor::producesMidi() const
{
   #if JucePlugin_ProducesMidiOutput
    return true;
   #else
    return false;
   #endif
}

bool ClippuzAudioProcessor::isMidiEffect() const
{
   #if JucePlugin_IsMidiEffect
    return true;
   #else
    return false;
   #endif
}

double ClippuzAudioProcessor::getTailLengthSeconds() const
{
    return 0.0;
}

int ClippuzAudioProcessor::getNumPrograms()
{
    return 1;   // NB: some hosts don't cope very well if you tell them there are 0 programs,
                // so this should be at least 1, even if you're not really implementing programs.
}

int ClippuzAudioProcessor::getCurrentProgram()
{
    return 0;
}

void ClippuzAudioProcessor::setCurrentProgram (int index)
{
}

const juce::String ClippuzAudioProcessor::getProgramName (int index)
{
    return {};
}

void ClippuzAudioProcessor::changeProgramName (int index, const juce::String& newName)
{
}

//==============================================================================
void ClippuzAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    const auto numChannels = static_cast<size_t>(getTotalNumInputChannels());
    oversamplers.clear();
    for (size_t factor = 1; factor <= 4; ++factor)
    {
        auto os = std::make_unique<juce::dsp::Oversampling<float>>(
            numChannels,                                                          
            factor,                                                   
            juce::dsp::Oversampling<float>::filterHalfBandPolyphaseIIR, 
            true                                                        
        );
        
        os->initProcessing(static_cast<size_t>(samplesPerBlock));
        os->reset();
        
        oversamplers.push_back(std::move(os));
    }

    size_t osIndex = static_cast<size_t> (apvts.getRawParameterValue ("OS")->load());
    float osMultiplier = static_cast<float>(1 << osIndex); 
    float overSampleRate = static_cast<float>(getSampleRate()) * osMultiplier;
    prevOsIndex = osIndex;
    if (osIndex > 0)
    {
        const float latency = oversamplers[osIndex - 1]->getLatencyInSamples();
        setLatencySamples (juce::roundToInt (latency));
    }
    else
    {
        setLatencySamples (0);
    }       

    eq1_f1.prepare(overSampleRate, getTotalNumInputChannels());
}

void ClippuzAudioProcessor::releaseResources()
{
    // When playback stops, you can use this as an opportunity to free up any
    // spare memory, etc.
 
}

#ifndef JucePlugin_PreferredChannelConfigurations
bool ClippuzAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
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

void ClippuzAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
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


    size_t osIndex = static_cast<size_t> (apvts.getRawParameterValue ("OS")->load());   
    float osMultiplier = static_cast<float>(1 << osIndex); 
    float overSampleRate = static_cast<float>(getSampleRate()) * osMultiplier;

    float driveDb = apvts.getRawParameterValue("DRIVE")->load();
    float driveGain = juce::Decibels::decibelsToGain(driveDb);


    juce::dsp::AudioBlock<float> mainBlock(buffer);
    juce::dsp::AudioBlock<float> blockToProcess = mainBlock; 

    if (prevOsIndex != osIndex)
    {
        prevOsIndex = osIndex;
        if (osIndex > 0)
        {
            const float latency = oversamplers[osIndex - 1]->getLatencyInSamples();
            setLatencySamples (juce::roundToInt (latency));
        }
        else
        {
            setLatencySamples (0);
        }  
        
        eq1_f1.setSampleRate(overSampleRate);
    }
 
    if (osIndex > 0)
    {
        blockToProcess = oversamplers[osIndex - 1]->processSamplesUp(mainBlock);
    }

    for (size_t ch = 0; ch < blockToProcess.getNumChannels(); ++ch)
    {
        auto* data = blockToProcess.getChannelPointer(ch);
        
        for (size_t i = 0; i < blockToProcess.getNumSamples(); ++i)
        {
            float x = data[i] * driveGain;
            
            x = eq1_f1[ch].processSample(x);
            
            data[i] = x;
        }       
    }

    if (osIndex > 0)
    {
        oversamplers[osIndex - 1]->processSamplesDown(mainBlock);
    }    
}

//==============================================================================
bool ClippuzAudioProcessor::hasEditor() const
{
    return true; // (change this to false if you choose to not supply an editor)
}

juce::AudioProcessorEditor* ClippuzAudioProcessor::createEditor()
{
    //return new ClippuzAudioProcessorEditor (*this);
    return new juce::GenericAudioProcessorEditor (*this); 
}

//==============================================================================
void ClippuzAudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    auto state = apvts.copyState();
    std::unique_ptr<juce::XmlElement> xml (state.createXml());
    copyXmlToBinary (*xml, destData);    
}

void ClippuzAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    std::unique_ptr<juce::XmlElement> xmlState (getXmlFromBinary (data, sizeInBytes));
    
    if (xmlState.get() != nullptr)
        if (xmlState->hasTagName (apvts.state.getType()))
            apvts.replaceState (juce::ValueTree::fromXml (*xmlState));
}

//==============================================================================
// This creates new instances of the plugin..
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new ClippuzAudioProcessor();
}


juce::AudioProcessorValueTreeState::ParameterLayout ClippuzAudioProcessor::createParameters()
{
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> params;
        
    juce::StringArray osChoices { "1x (Off)", "2x", "4x", "8x", "16x" };
    params.push_back(std::make_unique<juce::AudioParameterChoice>("OS", "Oversampling", osChoices, 0));

    params.push_back(std::make_unique<juce::AudioParameterFloat>("DRIVE", "Drive (dB)", 0.0f, 40.0f, 0.0f));


    return { params.begin(), params.end() };
}
