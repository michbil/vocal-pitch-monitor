#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

namespace vocalpitch
{
    // Vertical piano keyboard covering [kLowestMidiNote, kHighestMidiNote].
    // Low notes at the bottom, high notes at the top — matches the pitch graph.
    class PianoKeyboard : public juce::Component
    {
    public:
        PianoKeyboard();
        void paint (juce::Graphics&) override;

        // Y coordinate (within this component's height) for a given midi note (can be fractional).
        float yForMidi (float midiNote, float height) const;
    };
}
