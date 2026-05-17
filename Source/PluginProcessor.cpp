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
        const auto& in  = layouts.getMainInputChannelSet();
        const auto& out = layouts.getMainOutputChannelSet();

        // Accept any non-disabled in/out layout with at least one channel —
        // built-in MacBook mic exposes as 3-channel array, USB interfaces vary.
        if (in.isDisabled() || out.isDisabled())
            return false;
        if (in.size() < 1 || out.size() < 1)
            return false;
        return true;
    }

    void VocalPitchProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
    {
        juce::ScopedNoDenormals noDenormals;

        const int numSamples = buffer.getNumSamples();

        // Read from the input bus explicitly — input channel count may differ
        // from the output bus (e.g. mono mic into a stereo track).
        auto inputBuffer = getBusBuffer (buffer, true, 0);
        const int inputChannels = inputBuffer.getNumChannels();

        if ((int) monoBuffer.size() < numSamples)
            monoBuffer.assign ((size_t) numSamples, 0.0f);

        if (inputChannels <= 0)
        {
            std::fill (monoBuffer.begin(), monoBuffer.begin() + numSamples, 0.0f);
        }
        else if (inputChannels == 1)
        {
            const float* src = inputBuffer.getReadPointer (0);
            for (int i = 0; i < numSamples; ++i)
                monoBuffer[(size_t) i] = src[i];
        }
        else
        {
            // Average all available input channels.
            const float scale = 1.0f / (float) inputChannels;
            for (int i = 0; i < numSamples; ++i)
                monoBuffer[(size_t) i] = 0.0f;
            for (int ch = 0; ch < inputChannels; ++ch)
            {
                const float* src = inputBuffer.getReadPointer (ch);
                for (int i = 0; i < numSamples; ++i)
                    monoBuffer[(size_t) i] += src[i] * scale;
            }
        }

        // Compute RMS of the mono signal we'll feed to the detector — for the UI level meter.
        {
            double sumSq = 0.0;
            for (int i = 0; i < numSamples; ++i)
                sumSq += (double) monoBuffer[(size_t) i] * (double) monoBuffer[(size_t) i];
            const float rms = (float) std::sqrt (sumSq / juce::jmax (1, numSamples));
            const float db  = rms > 1.0e-9f ? 20.0f * std::log10 (rms) : -120.0f;
            currentRmsDb.store (db);
            lastInputChannels.store (inputChannels);
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
