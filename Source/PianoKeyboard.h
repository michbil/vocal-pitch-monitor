#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

namespace vocalpitch
{
    // Vertical piano keyboard. Uses a moving viewport (centerMidi ± visibleSpan/2),
    // so the layout can scroll with the singer.
    class PianoKeyboard : public juce::Component
    {
    public:
        PianoKeyboard();
        void paint (juce::Graphics&) override;

        // Y coordinate (within this component's height) for a given midi note.
        float yForMidi (float midiNote, float height) const;

        void  setViewportCenter (float midi) { centerMidi = midi; }
        float getViewportCenter() const noexcept { return centerMidi; }

        void  setVisibleSpan (float notes) { visibleSpan = notes; }
        float getVisibleSpan() const noexcept { return visibleSpan; }

    private:
        float centerMidi  = 60.0f;  // C4
        float visibleSpan = 24.0f;  // 2 octaves visible at once
    };
}
