/*
 
*/

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_audio_formats/juce_audio_formats.h>
#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_events/juce_events.h>

#include "PluginProcessor.h"

#include <iostream>
#include <map>
#include <string>
#include <algorithm>

namespace
{
    std::map<juce::String, float> defaultParams {
        { "gain",      0.0f },
        { "lpfreq", 1956.8f },
        { "lpq",       0.66f }, 
        { "bias",      0.52f },
        { "speed",     1.0f  },
        { "os",        0.0f  },
        { "blocksize", 512.0f },
    };

    std::map<juce::String, float> parseParams (int argc, char* argv[])
    {
        auto params = defaultParams;
        for (int i = 3; i < argc; ++i)
        {
            juce::String arg (argv[i]);
            if (! arg.startsWith ("--"))
                continue;

            auto eq = arg.indexOfChar ('=');
            if (eq < 0)
                continue;

            auto key   = arg.substring (2, eq);
            auto value = arg.substring (eq + 1).getFloatValue();
            params[key] = value;
        }
        return params;
    }
}

int main (int argc, char* argv[])
{
    juce::ScopedJuceInitialiser_GUI juceInit;

    if (argc < 3)
    {
        std::cerr << "Usage: regression_render <input.wav> <output.wav> "
                     "[--gain=0] [--lpfreq=1956.8] [--lpq=..] [--bias=0.52] "
                     "[--speed=1.0] [--os=0] [--blocksize=512]\n";
        return 1;
    }

    const juce::File inputFile  (argv[1]);
    const juce::File outputFile (argv[2]);
    const auto params = parseParams (argc, argv);

    juce::WavAudioFormat wavFormat;

    std::unique_ptr<juce::AudioFormatReader> reader (
        wavFormat.createReaderFor (new juce::FileInputStream (inputFile), true));

    if (reader == nullptr)
    {
        std::cerr << "Can not open input WAV: " << inputFile.getFullPathName() << "\n";
        return 1;
    }

    const int numChannels   = (int) reader->numChannels;
    const int numSamples    = (int) reader->lengthInSamples;
    const double sampleRate = reader->sampleRate;

    juce::AudioBuffer<float> inputBuffer (numChannels, numSamples);
    reader->read (&inputBuffer, 0, numSamples, 0, true, true);

    ClippuzAudioProcessor processor;

    auto setParam = [&] (const juce::String& id, float value)
    {
        if (auto* p = processor.getAPVTS().getParameter (id))
            p->setValueNotifyingHost (p->convertTo0to1 (value));
        else
            std::cerr << "Warning: field " << id << " not found apvts\n";
    };

    setParam ("GAIN",   params.at ("gain"));
    setParam ("LPFREQ", params.at ("lpfreq"));
    setParam ("LPQ",    params.at ("lpq"));
    setParam ("BIAS",   params.at ("bias"));
    setParam ("SPEED",  params.at ("speed"));
    setParam ("OS",     params.at ("os"));

    const int blockSize = (int) params.at ("blocksize");


    processor.setRateAndBufferSizeDetails (sampleRate, blockSize);
    processor.prepareToPlay (sampleRate, blockSize);


    const int latencySamples = processor.getLatencySamples();
    const int paddedLength   = numSamples + latencySamples + blockSize;

    juce::AudioBuffer<float> outputBuffer (numChannels, paddedLength);
    outputBuffer.clear();

    juce::MidiBuffer midi;
    int pos = 0;
    while (pos < paddedLength)
    {
        const int thisBlock = std::min (blockSize, paddedLength - pos);

        juce::AudioBuffer<float> block (numChannels, thisBlock);
        block.clear();

        for (int ch = 0; ch < numChannels; ++ch)
        {
            const int available = std::max (0, std::min (thisBlock, numSamples - pos));
            if (available > 0)
                block.copyFrom (ch, 0, inputBuffer, ch, pos, available);
        }

        processor.processBlock (block, midi);

        for (int ch = 0; ch < numChannels; ++ch)
            outputBuffer.copyFrom (ch, pos, block, ch, 0, thisBlock);

        pos += thisBlock;
    }

    outputFile.deleteFile();

    std::unique_ptr<juce::AudioFormatWriter> writer (
        wavFormat.createWriterFor (new juce::FileOutputStream (outputFile),
                                    sampleRate,
                                    (unsigned int) numChannels,
                                    32,   
                                    {},
                                    0));

    if (writer == nullptr)
    {
        std::cerr << "Can not creato output WAV: " << outputFile.getFullPathName() << "\n";
        return 1;
    }

    writer->writeFromAudioSampleBuffer (outputBuffer, 0, paddedLength);

    std::cout << "OK: " << outputFile.getFullPathName()
               << " (" << paddedLength << " samples, latency=" << latencySamples << ")\n";
    return 0;
}