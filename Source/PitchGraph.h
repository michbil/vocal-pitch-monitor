#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include <deque>

#include "PluginProcessor.h"
#include "PianoKeyboard.h"

namespace vocalpitch
{
    // Scrolling pitch trace. Right edge is "now"; data slides left over time.
    class PitchGraph : public juce::Component
    {
    public:
        explicit PitchGraph (PianoKeyboard& keyboardRef) : keyboard (keyboardRef) {}

        void paint (juce::Graphics&) override;

        void addSamples (const PitchSample* samples, int count);
        void setWindowSeconds (double secs) { windowSeconds = secs; }
        void setNowTime (double t) { nowTime = t; }

    private:
        PianoKeyboard& keyboard;
        std::deque<PitchSample> history;
        double windowSeconds = 6.0;
        double nowTime = 0.0;
    };
}
