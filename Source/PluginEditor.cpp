#include "PluginEditor.h"
#include "NoteRange.h"

namespace vocalpitch
{
    VocalPitchEditor::VocalPitchEditor (VocalPitchProcessor& p)
        : juce::AudioProcessorEditor (&p), vpProcessor (p)
    {
        addAndMakeVisible (keyboard);
        addAndMakeVisible (keyboardRight);
        addAndMakeVisible (graph);
        setResizable (true, true);
        setResizeLimits (480, 240, 4000, 2000);
        setSize (900, 480);

        lastTickMs = juce::Time::getMillisecondCounterHiRes();
        startTimerHz (30);
    }

    void VocalPitchEditor::paint (juce::Graphics& g)
    {
        g.fillAll (juce::Colour::fromRGB (12, 12, 14));
    }

    void VocalPitchEditor::resized()
    {
        auto r = getLocalBounds();
        const int keyboardWidth = juce::jlimit (32, 56, r.getWidth() / 16);
        keyboard     .setBounds (r.removeFromLeft  (keyboardWidth));
        keyboardRight.setBounds (r.removeFromRight (keyboardWidth));
        graph.setBounds (r);
    }

    void VocalPitchEditor::timerCallback()
    {
        const double now = juce::Time::getMillisecondCounterHiRes();
        const double dt  = (now - lastTickMs) / 1000.0;
        lastTickMs = now;
        uiTimeSec += dt;

        PitchSample buf[256];
        int got;
        float latestVoiced = -1.0f;
        while ((got = vpProcessor.drainPitches (buf, 256)) > 0)
        {
            uiTimeSec = std::max (uiTimeSec, buf[got - 1].timeSec);
            graph.addSamples (buf, got);
            for (int i = got - 1; i >= 0; --i)
                if (buf[i].midiNote >= 0.0f) { latestVoiced = buf[i].midiNote; break; }
            if (got < 256) break;
        }

        // Update auto-scroll target from the latest voiced pitch.
        if (latestVoiced > 0.0f)
        {
            // Smooth the target a bit so brief outliers don't whip the viewport.
            targetCenterMidi += 0.35f * (latestVoiced - targetCenterMidi);

            // Keep the viewport inside the supported note range.
            const float half = keyboard.getVisibleSpan() * 0.5f;
            targetCenterMidi = juce::jlimit ((float) kLowestMidiNote  + half,
                                              (float) kHighestMidiNote - half,
                                              targetCenterMidi);
        }

        // Smooth the visible center toward the target (frame-rate independent).
        const float k = 1.0f - std::exp (-(float) dt * 4.0f); // ~250ms time constant
        const float current = keyboard.getViewportCenter();
        const float updated = current + k * (targetCenterMidi - current);
        keyboard     .setViewportCenter (updated);
        keyboardRight.setViewportCenter (updated);

        graph.setNowTime (uiTimeSec);
        keyboard.repaint();
        keyboardRight.repaint();
        graph.repaint();
    }
}
