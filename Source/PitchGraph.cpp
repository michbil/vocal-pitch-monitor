#include "PitchGraph.h"
#include "NoteRange.h"

namespace vocalpitch
{
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

        g.fillAll (juce::Colour::fromRGB (18, 18, 22));

        // Horizontal note guidelines.
        for (int n = kLowestMidiNote; n <= kHighestMidiNote; ++n)
        {
            const float y = keyboard.yForMidi ((float) n, h);
            const int p = ((n % 12) + 12) % 12;
            const bool isC = (p == 0);
            const bool isBlack = (p == 1 || p == 3 || p == 6 || p == 8 || p == 10);

            if (isC)
                g.setColour (juce::Colour::fromRGB (60, 60, 70));
            else if (isBlack)
                g.setColour (juce::Colour::fromRGB (28, 28, 34));
            else
                g.setColour (juce::Colour::fromRGB (38, 38, 44));

            g.drawHorizontalLine ((int) y, 0.0f, w);
        }

        // Vertical time gridlines, one per second.
        const double tStart = nowTime - windowSeconds;
        const double firstSec = std::ceil (tStart);
        g.setColour (juce::Colour::fromRGB (40, 40, 48));
        for (double t = firstSec; t < nowTime; t += 1.0)
        {
            const float x = (float) ((t - tStart) / windowSeconds) * w;
            g.drawVerticalLine ((int) x, 0.0f, h);
        }

        // Pitch trace.
        if (history.empty())
            return;

        const auto xFor = [&] (double t)
        {
            return (float) ((t - tStart) / windowSeconds) * w;
        };

        juce::Path path;
        bool started = false;
        float lastX = 0.0f, lastY = 0.0f;

        g.setColour (juce::Colour::fromRGB (120, 220, 140));

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
                // Break the line on big jumps (octave errors) to keep it readable.
                if (std::abs (y - lastY) > h * 0.25f && std::abs (x - lastX) < 4.0f)
                    path.startNewSubPath (x, y);
                else
                    path.lineTo (x, y);
            }
            lastX = x; lastY = y;
        }

        g.strokePath (path, juce::PathStrokeType (2.0f, juce::PathStrokeType::curved,
                                                   juce::PathStrokeType::rounded));

        // Latest reading: dot + cents readout at top-right.
        for (auto it = history.rbegin(); it != history.rend(); ++it)
        {
            if (it->midiNote >= 0.0f)
            {
                const float x = xFor (it->timeSec);
                const float y = keyboard.yForMidi (it->midiNote, h);
                g.setColour (juce::Colours::white);
                g.fillEllipse (x - 4.0f, y - 4.0f, 8.0f, 8.0f);

                const int   nearest = (int) std::round (it->midiNote);
                const float cents   = (it->midiNote - (float) nearest) * 100.0f;
                static const char* names[] = { "C","C#","D","D#","E","F","F#","G","G#","A","A#","B" };
                const int p = ((nearest % 12) + 12) % 12;
                const int octave = nearest / 12 - 1;
                auto label = juce::String (names[p]) + juce::String (octave)
                           + "  " + (cents >= 0 ? "+" : "") + juce::String (cents, 1) + " ct";
                g.setFont (juce::FontOptions (14.0f).withStyle ("Bold"));
                g.drawText (label, juce::Rectangle<int> (8, 6, (int) w - 16, 20),
                            juce::Justification::topRight);
                break;
            }
        }
    }
}
