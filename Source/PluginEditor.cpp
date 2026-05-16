#include "PluginEditor.h"

namespace vocalpitch
{
    VocalPitchEditor::VocalPitchEditor (VocalPitchProcessor& p)
        : juce::AudioProcessorEditor (&p), vpProcessor (p)
    {
        addAndMakeVisible (keyboard);
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
        const int keyboardWidth = juce::jlimit (60, 110, r.getWidth() / 8);
        keyboard.setBounds (r.removeFromLeft (keyboardWidth));
        graph.setBounds (r);
    }

    void VocalPitchEditor::timerCallback()
    {
        // Advance the UI clock by wall-clock time. This keeps the graph scrolling
        // smoothly even between processBlock callbacks.
        const double now = juce::Time::getMillisecondCounterHiRes();
        const double dt  = (now - lastTickMs) / 1000.0;
        lastTickMs = now;
        uiTimeSec += dt;

        PitchSample buf[256];
        int got;
        while ((got = vpProcessor.drainPitches (buf, 256)) > 0)
        {
            // Re-align the UI clock so the newest pitch sample stays at the right edge.
            uiTimeSec = std::max (uiTimeSec, buf[got - 1].timeSec);
            graph.addSamples (buf, got);
            if (got < 256) break;
        }

        graph.setNowTime (uiTimeSec);
        graph.repaint();
    }
}
