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

    for (auto *filter : allFilters)
    {
        filter->prepare(overSampleRate, getTotalNumInputChannels());
    }
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

    float driveDb = apvts.getRawParameterValue("GAIN")->load();
    float driveGain = juce::Decibels::decibelsToGain(driveDb);

    float lpFreq = apvts.getRawParameterValue("LPFREQ")->load();
    eqLowPass.setFreq(lpFreq);
    float lpQ = apvts.getRawParameterValue("LPQ")->load();
    eqLowPass.setQ(lpQ);

    eqPeak.setFreq(apvts.getRawParameterValue("PFREQ")->load());
    eqPeak.setQ(apvts.getRawParameterValue("PQ")->load());
    eqPeak.setGain(apvts.getRawParameterValue("PGAIN")->load());

    float clip1_gain = apvts.getRawParameterValue("CL1GAIN")->load();
    float bias2 = apvts.getRawParameterValue("BIAS2")->load();
    float clip2_gain = apvts.getRawParameterValue("CL2GAIN")->load();
    float speed2 = apvts.getRawParameterValue("SPEED2")->load();
    float clip3_gain = apvts.getRawParameterValue("CL3GAIN")->load();
    float output = apvts.getRawParameterValue("OUTPUT")->load();

    float bias = apvts.getRawParameterValue("BIAS")->load();

    float speed = apvts.getRawParameterValue("SPEED")->load();
    hpf1.setFreq(speed);
    hpf2.setFreq(speed2);

    size_t cl1_mode = static_cast<size_t> (apvts.getRawParameterValue ("CL1MODE")->load()); 
    size_t cl2_mode = static_cast<size_t> (apvts.getRawParameterValue ("CL2MODE")->load()); 
    size_t cl3_mode = static_cast<size_t> (apvts.getRawParameterValue ("CL3MODE")->load()); 

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
        

        for (auto *filter : allFilters)
        {
            filter->setSampleRate(overSampleRate);
        }
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
            
            x = eqPeak[ch].processSample(x);
            x = eqLowPass[ch].processSample(x);           

            x *= juce::Decibels::decibelsToGain(clip1_gain);
            x += bias;
            if (cl1_mode) if (fabs(x) > 1.0f) x = (x > 0 ? 1.0f : -1.0f);

            x = hpf1[ch].processSample(x);

            x *= juce::Decibels::decibelsToGain(clip2_gain);
            x += bias2;           
            if (cl2_mode) if (fabs(x) > 1.0f) x = (x > 0 ? 1.0f : -1.0f); 
            
            x = hpf2[ch].processSample(x);

            x *= juce::Decibels::decibelsToGain(clip3_gain);
            if (cl3_mode) if (fabs(x) > 1.0f) x = (x > 0 ? 1.0 : -1.0);

            x *= juce::Decibels::decibelsToGain(output);  

            x = hpf3[ch].processSample(x);

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

    params.push_back(std::make_unique<juce::AudioParameterFloat>("GAIN", "Gain (dB)", -10.0f, 20.0f, 0.0f));

    params.push_back(std::make_unique<juce::AudioParameterFloat>("LPFREQ", "Lowpass Hz", 200.0f, 15000.0f, 1956.8f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>("LPQ", "Lowpass Q", 0.1f, 10.0f, EqFilter::OctToQ(0.66f)));

    params.push_back(std::make_unique<juce::AudioParameterFloat>("PFREQ", "Peak Hz", 20.0f, 15000.0f, 383.1f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>("PQ", "Peak Q", 0.1f, 10.0f, EqFilter::OctToQ(2.60f)));  
    params.push_back(std::make_unique<juce::AudioParameterFloat>("PGAIN", "Peak Gain", 0.0f, 20.0f, 7.2f));  
    
    params.push_back(std::make_unique<juce::AudioParameterFloat>("CL1GAIN", "Clipper 1 Gain", -10.0f, 50.0f, 4.9f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>("BIAS", "Clipper 1 Bias", -2.0f, 2.0f, 0.7f));
    juce::StringArray cl1_choices { "Off", "Hard" };
    params.push_back(std::make_unique<juce::AudioParameterChoice>("CL1MODE", "Clipper 1 Mode", cl1_choices, 1));    
    params.push_back(std::make_unique<juce::AudioParameterFloat>("SPEED", "Release Speed", 0.1f, 30.0f, 1.0f));
    
    
    params.push_back(std::make_unique<juce::AudioParameterFloat>("CL2GAIN", "Clipper 2 Gain", -10.0f, 50.0f, 0.8f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>("BIAS2", "Clipper 2 Bias", -2.0f, 2.0f, -0.7f));
    juce::StringArray cl2_choices { "Off", "Hard" };
    params.push_back(std::make_unique<juce::AudioParameterChoice>("CL2MODE", "Clipper 2 Mode", cl2_choices, 1));     
    params.push_back(std::make_unique<juce::AudioParameterFloat>("SPEED2", "Release Speed 2", 0.1f, 30.0f, 30.0f));

    params.push_back(std::make_unique<juce::AudioParameterFloat>("CL3GAIN", "Clipper 3 Gain", -10.0f, 50.0f, 50.0f));
    juce::StringArray cl3_choices { "Off", "Hard" };
    params.push_back(std::make_unique<juce::AudioParameterChoice>("CL3MODE", "Clipper 3 Mode", cl3_choices, 1));    

    params.push_back(std::make_unique<juce::AudioParameterFloat>("OUTPUT", "Output", -10.0f, 20.0f, -10.6f));

    return { params.begin(), params.end() };
}
