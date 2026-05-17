#include "PluginEditor.h"
#include "NoteRange.h"

namespace vocalpitch
{
    static constexpr int kMeterHeight = 6;
    static constexpr int kFooterHeight = 18;

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
        paintDiagnostics (g, getLocalBounds().removeFromTop (kMeterHeight));

        auto footer = getLocalBounds().removeFromBottom (kFooterHeight);
        g.setColour (juce::Colour::fromRGB (140, 140, 150));
        g.setFont (juce::FontOptions (11.0f));
        g.drawText ("Use headphones for live monitoring (Logic disables it when output = built-in speakers)",
                    footer, juce::Justification::centred);
    }

    void VocalPitchEditor::paintDiagnostics (juce::Graphics& g, juce::Rectangle<int> area)
    {
        const float rms = vpProcessor.getCurrentRmsDb();
        const float meterDb = juce::jlimit (-60.0f, 0.0f, rms);
        const float meterFrac = (meterDb + 60.0f) / 60.0f;

        g.setColour (juce::Colour::fromRGB (24, 24, 28));
        g.fillRect (area);

        g.setColour (rms > -6.0f  ? juce::Colours::red
                   : rms > -24.0f ? juce::Colours::yellow
                                  : juce::Colours::limegreen);
        g.fillRect (area.withWidth ((int) (area.getWidth() * meterFrac)));
    }

    void VocalPitchEditor::resized()
    {
        auto r = getLocalBounds();
        r.removeFromTop    (kMeterHeight);
        r.removeFromBottom (kFooterHeight);
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

        if (latestVoiced > 0.0f)
        {
            targetCenterMidi += 0.35f * (latestVoiced - targetCenterMidi);
            const float half = keyboard.getVisibleSpan() * 0.5f;
            targetCenterMidi = juce::jlimit ((float) kLowestMidiNote  + half,
                                              (float) kHighestMidiNote - half,
                                              targetCenterMidi);
        }

        const float k = 1.0f - std::exp (-(float) dt * 4.0f);
        const float current = keyboard.getViewportCenter();
        const float updated = current + k * (targetCenterMidi - current);
        keyboard     .setViewportCenter (updated);
        keyboardRight.setViewportCenter (updated);

        graph.setNowTime (uiTimeSec);

        keyboard.repaint();
        keyboardRight.repaint();
        graph.repaint();
        repaint (getLocalBounds().removeFromTop (kMeterHeight));
    }
}
