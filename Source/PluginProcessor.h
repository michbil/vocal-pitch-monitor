#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <atomic>
#include <array>
#include <vector>

#include "YinPitchDetector.h"

namespace vocalpitch
{
    struct PitchSample
    {
        double timeSec  = 0.0;
        float  midiNote = -1.0f; // -1 == unvoiced
    };

    class VocalPitchProcessor : public juce::AudioProcessor
    {
    public:
        VocalPitchProcessor();
        ~VocalPitchProcessor() override = default;

        void prepareToPlay (double sampleRate, int samplesPerBlock) override;
        void releaseResources() override {}
        bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
        void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

        juce::AudioProcessorEditor* createEditor() override;
        bool hasEditor() const override { return true; }

        const juce::String getName() const override { return "VocalPitch"; }
        bool acceptsMidi()  const override { return false; }
        bool producesMidi() const override { return false; }
        double getTailLengthSeconds() const override { return 0.0; }

        int  getNumPrograms() override { return 1; }
        int  getCurrentProgram() override { return 0; }
        void setCurrentProgram (int) override {}
        const juce::String getProgramName (int) override { return {}; }
        void changeProgramName (int, const juce::String&) override {}

        void getStateInformation (juce::MemoryBlock&) override {}
        void setStateInformation (const void*, int) override {}

        // Called from the UI timer. Drains pending pitch samples into `out`.
        // Returns number of samples written.
        int drainPitches (PitchSample* out, int maxSamples);

        float  getCurrentRmsDb() const noexcept { return currentRmsDb.load(); }
        int    getInputChannelCount() const noexcept { return lastInputChannels.load(); }
        double getSampleRate() const noexcept { return juce::AudioProcessor::getSampleRate(); }

    private:
        static constexpr int kFifoSize = 4096;

        YinPitchDetector detector;
        juce::AbstractFifo fifo { kFifoSize };
        std::array<PitchSample, kFifoSize> ring {};

        std::vector<float> monoBuffer;
        double currentTimeSec = 0.0;
        std::atomic<float> currentRmsDb { -120.0f };
        std::atomic<int>   lastInputChannels { 0 };

        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (VocalPitchProcessor)
    };
}
