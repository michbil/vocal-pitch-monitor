#include "PluginProcessor.h"
#include "PluginEditor.h"
#include "NoteRange.h"

#include <cmath>

namespace vocalpitch
{
    VocalPitchProcessor::VocalPitchProcessor()
        : juce::AudioProcessor (BusesProperties()
                                    .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
                                    .withOutput ("Output", juce::AudioChannelSet::stereo(), true))
    {
    }

    void VocalPitchProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
    {
        detector.prepare (sampleRate);
        monoBuffer.assign ((size_t) samplesPerBlock, 0.0f);
        currentTimeSec = 0.0;
        fifo.reset();
    }

    bool VocalPitchProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
    {
        const auto& mainOut = layouts.getMainOutputChannelSet();
        if (mainOut != juce::AudioChannelSet::mono() && mainOut != juce::AudioChannelSet::stereo())
            return false;
        return layouts.getMainInputChannelSet() == mainOut;
    }

    void VocalPitchProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
    {
        juce::ScopedNoDenormals noDenormals;

        const int numSamples  = buffer.getNumSamples();
        const int numChannels = buffer.getNumChannels();

        if ((int) monoBuffer.size() < numSamples)
            monoBuffer.assign ((size_t) numSamples, 0.0f);

        // Downmix to mono in-place (no allocation).
        if (numChannels == 1)
        {
            const float* src = buffer.getReadPointer (0);
            for (int i = 0; i < numSamples; ++i)
                monoBuffer[(size_t) i] = src[i];
        }
        else
        {
            const float* l = buffer.getReadPointer (0);
            const float* r = buffer.getReadPointer (1);
            for (int i = 0; i < numSamples; ++i)
                monoBuffer[(size_t) i] = 0.5f * (l[i] + r[i]);
        }

        const double sr      = detector.getSampleRate();
        const int    hop     = detector.getHopSize();
        const double hopSecs = (double) hop / sr;

        // We get one pitch per hop; pitch sample timestamp is at the end of that hop.
        double pitchTime = currentTimeSec + hopSecs;

        detector.process (monoBuffer.data(), numSamples, [&] (float hz)
        {
            float midi = kUnvoiced;
            if (hz > 0.0f)
                midi = 69.0f + 12.0f * std::log2 (hz / 440.0f);

            int start1, size1, start2, size2;
            fifo.prepareToWrite (1, start1, size1, start2, size2);
            if (size1 > 0)
                ring[(size_t) start1] = PitchSample { pitchTime, midi };
            else if (size2 > 0)
                ring[(size_t) start2] = PitchSample { pitchTime, midi };
            fifo.finishedWrite (size1 + size2);

            pitchTime += hopSecs;
        });

        currentTimeSec += (double) numSamples / sr;

        // Plugin is an analyzer — leave audio untouched (pass-through).
    }

    int VocalPitchProcessor::drainPitches (PitchSample* out, int maxSamples)
    {
        const int available = fifo.getNumReady();
        const int toRead = juce::jmin (available, maxSamples);
        if (toRead <= 0)
            return 0;

        int start1, size1, start2, size2;
        fifo.prepareToRead (toRead, start1, size1, start2, size2);

        int written = 0;
        for (int i = 0; i < size1; ++i)
            out[written++] = ring[(size_t) (start1 + i)];
        for (int i = 0; i < size2; ++i)
            out[written++] = ring[(size_t) (start2 + i)];

        fifo.finishedRead (toRead);
        return written;
    }

    juce::AudioProcessorEditor* VocalPitchProcessor::createEditor()
    {
        return new VocalPitchEditor (*this);
    }
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new vocalpitch::VocalPitchProcessor();
}
