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
        const float lo = centerMidi - visibleSpan * 0.5f;
        const float hi = centerMidi + visibleSpan * 0.5f;
        const float norm = (midiNote - lo) / (hi - lo);
        return height - norm * height;
    }

    void PianoKeyboard::paint (juce::Graphics& g)
    {
        const auto bounds = getLocalBounds().toFloat();
        const float w = bounds.getWidth();
        const float h = bounds.getHeight();

        g.fillAll (juce::Colour::fromRGB (24, 24, 28));

        const int loVisible = (int) std::floor (centerMidi - visibleSpan * 0.5f) - 1;
        const int hiVisible = (int) std::ceil  (centerMidi + visibleSpan * 0.5f) + 1;
        const int loN = juce::jmax (kLowestMidiNote,  loVisible);
        const int hiN = juce::jmin (kHighestMidiNote, hiVisible);

        for (int n = loN; n <= hiN; ++n)
        {
            const float yTop = yForMidi ((float) n + 0.5f, h);
            const float yBot = yForMidi ((float) n - 0.5f, h);
            juce::Rectangle<float> r (0.0f, yTop, w, yBot - yTop);

            const bool black = isBlackKey (n);
            const juce::Colour bg   = black ? juce::Colour::fromRGB (40,  40,  46)
                                            : juce::Colour::fromRGB (210, 210, 215);
            const juce::Colour text = black ? juce::Colour::fromRGB (200, 200, 205)
                                            : juce::Colour::fromRGB (40,  40,  46);

            g.setColour (bg);
            g.fillRect (r.reduced (1.0f, 0.5f));

            if (r.getHeight() >= 9.0f)
            {
                g.setColour (text);
                g.setFont (juce::FontOptions (juce::jmin (14.0f, r.getHeight() - 4.0f)));
                g.drawText (noteName (n), r.reduced (4.0f, 0.0f), juce::Justification::centredLeft);
            }
        }
    }
}
