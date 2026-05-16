#include "PitchGraph.h"
#include "NoteRange.h"

namespace vocalpitch
{
    static bool isBlackKey (int midi)
    {
        const int p = ((midi % 12) + 12) % 12;
        return p == 1 || p == 3 || p == 6 || p == 8 || p == 10;
    }

    void PitchGraph::addSamples (const PitchSample* samples, int count)
    {
        for (int i = 0; i < count; ++i)
            history.push_back (samples[i]);

        const double cutoff = nowTime - windowSeconds * 1.2;
        while (! history.empty() && history.front().timeSec < cutoff)
            history.pop_front();
    }

    void PitchGraph::paint (juce::Graphics& g)
    {
        const auto bounds = getLocalBounds().toFloat();
        const float w = bounds.getWidth();
        const float h = bounds.getHeight();

        // Striped per-note background: black-key rows darker, white-key rows lighter.
        for (int n = kLowestMidiNote; n <= kHighestMidiNote; ++n)
        {
            const float yTop = keyboard.yForMidi ((float) n + 0.5f, h);
            const float yBot = keyboard.yForMidi ((float) n - 0.5f, h);
            const auto colour = isBlackKey (n)
                ? juce::Colour::fromRGB (28, 28, 32)
                : juce::Colour::fromRGB (44, 44, 50);
            g.setColour (colour);
            g.fillRect (juce::Rectangle<float> (0.0f, yTop, w, yBot - yTop));
        }

        // Octave separator lines on every C.
        g.setColour (juce::Colour::fromRGB (18, 18, 22));
        for (int n = kLowestMidiNote; n <= kHighestMidiNote; ++n)
        {
            if ((n % 12) != 0) continue;
            const float y = keyboard.yForMidi ((float) n - 0.5f, h);
            g.drawHorizontalLine ((int) y, 0.0f, w);
        }

        if (history.empty())
            return;

        const double tStart = nowTime - windowSeconds;
        const auto xFor = [&] (double t)
        {
            return (float) ((t - tStart) / windowSeconds) * w;
        };

        // Orange "current note" markers: one filled rect per pitch sample at the snapped row.
        // Drawn behind the trace so the white line reads on top.
        const auto orange = juce::Colour::fromRGB (240, 150, 50);
        g.setColour (orange);
        for (size_t i = 0; i < history.size(); ++i)
        {
            const auto& s = history[i];
            if (s.midiNote < 0.0f || s.timeSec < tStart) continue;

            const int snapped = (int) std::round (s.midiNote);
            const float yTop = keyboard.yForMidi ((float) snapped + 0.5f, h);
            const float yBot = keyboard.yForMidi ((float) snapped - 0.5f, h);

            // Width: span from this sample to the next sample (or a small fallback).
            float x0 = xFor (s.timeSec);
            float x1 = x0 + 6.0f;
            if (i + 1 < history.size() && history[i + 1].midiNote >= 0.0f)
                x1 = xFor (history[i + 1].timeSec);

            g.fillRect (juce::Rectangle<float> (x0, yTop, juce::jmax (1.0f, x1 - x0),
                                                yBot - yTop));
        }

        // Pitch trace on top.
        juce::Path path;
        bool started = false;
        float lastX = 0.0f, lastY = 0.0f;

        for (const auto& s : history)
        {
            if (s.timeSec < tStart) continue;
            if (s.midiNote < 0.0f) { started = false; continue; }

            const float x = xFor (s.timeSec);
            const float y = keyboard.yForMidi (s.midiNote, h);

            if (! started)
            {
                path.startNewSubPath (x, y);
                started = true;
            }
            else
            {
                if (std::abs (y - lastY) > h * 0.25f && std::abs (x - lastX) < 4.0f)
                    path.startNewSubPath (x, y);
                else
                    path.lineTo (x, y);
            }
            lastX = x; lastY = y;
        }

        g.setColour (juce::Colours::white);
        g.strokePath (path, juce::PathStrokeType (2.0f, juce::PathStrokeType::curved,
                                                   juce::PathStrokeType::rounded));

        // Latest reading readout at top-right.
        for (auto it = history.rbegin(); it != history.rend(); ++it)
        {
            if (it->midiNote >= 0.0f)
            {
                const int   nearest = (int) std::round (it->midiNote);
                const float cents   = (it->midiNote - (float) nearest) * 100.0f;
                static const char* names[] = { "C","C#","D","D#","E","F","F#","G","G#","A","A#","B" };
                const int p = ((nearest % 12) + 12) % 12;
                const int octave = nearest / 12 - 1;
                auto label = juce::String (names[p]) + juce::String (octave)
                           + "  " + (cents >= 0 ? "+" : "") + juce::String (cents, 1) + " ct";
                g.setColour (juce::Colours::white);
                g.setFont (juce::FontOptions (14.0f).withStyle ("Bold"));
                g.drawText (label, juce::Rectangle<int> (8, 6, (int) w - 16, 20),
                            juce::Justification::topRight);
                break;
            }
        }
    }
}
