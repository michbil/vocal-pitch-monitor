#pragma once

#include <juce_audio_processors/juce_audio_processors.h>

#include "PluginProcessor.h"
#include "PianoKeyboard.h"
#include "PitchGraph.h"

namespace vocalpitch
{
    class VocalPitchEditor : public juce::AudioProcessorEditor,
                              private juce::Timer
    {
    public:
        explicit VocalPitchEditor (VocalPitchProcessor&);
        ~VocalPitchEditor() override = default;

        void paint (juce::Graphics&) override;
        void resized() override;

    private:
        void timerCallback() override;

        VocalPitchProcessor& vpProcessor;
        PianoKeyboard keyboard;
        PitchGraph    graph { keyboard };

        double uiTimeSec = 0.0;
        double lastTickMs = 0.0;
        float  targetCenterMidi = 60.0f;
    };
}
