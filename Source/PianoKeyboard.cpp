#include "PianoKeyboard.h"
#include "NoteRange.h"

namespace vocalpitch
{
    static bool isBlackKey (int midi)
    {
        const int p = ((midi % 12) + 12) % 12;
        return p == 1 || p == 3 || p == 6 || p == 8 || p == 10;
    }

    static juce::String noteName (int midi)
    {
        static const char* names[] = { "C","C#","D","D#","E","F","F#","G","G#","A","A#","B" };
        const int p = ((midi % 12) + 12) % 12;
        const int octave = midi / 12 - 1;
        return juce::String (names[p]) + juce::String (octave);
    }

    PianoKeyboard::PianoKeyboard() = default;

    float PianoKeyboard::yForMidi (float midiNote, float height) const
    {
        const float lo = (float) kLowestMidiNote - 0.5f;
        const float hi = (float) kHighestMidiNote + 0.5f;
        const float norm = (midiNote - lo) / (hi - lo);
        return height - norm * height;
    }

    void PianoKeyboard::paint (juce::Graphics& g)
    {
        const auto bounds = getLocalBounds().toFloat();
        const float w = bounds.getWidth();
        const float h = bounds.getHeight();

        g.fillAll (juce::Colour::fromRGB (30, 30, 34));

        const float whiteW = w;
        const float blackW = w * 0.62f;

        // White keys first.
        for (int n = kLowestMidiNote; n <= kHighestMidiNote; ++n)
        {
            if (isBlackKey (n)) continue;
            const float yTop = yForMidi ((float) n + 0.5f, h);
            const float yBot = yForMidi ((float) n - 0.5f, h);
            juce::Rectangle<float> r (0.0f, yTop, whiteW, yBot - yTop);

            g.setColour (juce::Colours::white);
            g.fillRect (r.reduced (0.0f, 0.5f));
            g.setColour (juce::Colour::fromRGB (60, 60, 60));
            g.drawRect (r, 0.5f);

            // C label.
            if ((n % 12) == 0)
            {
                g.setColour (juce::Colour::fromRGB (80, 80, 80));
                g.setFont (juce::FontOptions (10.0f));
                g.drawText (noteName (n), r.reduced (3.0f), juce::Justification::centredLeft);
            }
        }

        // Black keys on top.
        for (int n = kLowestMidiNote; n <= kHighestMidiNote; ++n)
        {
            if (! isBlackKey (n)) continue;
            const float yTop = yForMidi ((float) n + 0.5f, h);
            const float yBot = yForMidi ((float) n - 0.5f, h);
            juce::Rectangle<float> r (0.0f, yTop, blackW, yBot - yTop);
            g.setColour (juce::Colours::black);
            g.fillRect (r);
        }
    }
}
