#pragma once

#include <vector>
#include <cstddef>

namespace vocalpitch
{
    // Single-channel YIN pitch estimator with internal accumulation buffer.
    // Feed it audio in arbitrary block sizes; it emits one pitch estimate per hop.
    class YinPitchDetector
    {
    public:
        YinPitchDetector() = default;

        void prepare (double sampleRate, int frameSize = 2048, int hopSize = 512);
        void reset();

        // Push samples; calls onPitch(freqHz, midiNote) for each completed hop.
        // freqHz == 0 means unvoiced.
        template <typename Callback>
        void process (const float* samples, int numSamples, Callback&& onPitch);

        double getSampleRate() const noexcept { return sampleRate; }
        int    getHopSize()    const noexcept { return hopSize; }

    private:
        float estimateFrequency();

        double sampleRate = 44100.0;
        int    frameSize  = 2048;
        int    hopSize    = 512;
        float  threshold  = 0.15f;

        std::vector<float> ringBuffer;   // size == frameSize
        std::vector<float> frame;        // working frame, size == frameSize
        std::vector<float> diff;         // YIN difference function buffer
        int    writeIndex     = 0;
        int    samplesSinceHop = 0;
        bool   primed         = false;
    };

    template <typename Callback>
    void YinPitchDetector::process (const float* samples, int numSamples, Callback&& onPitch)
    {
        for (int i = 0; i < numSamples; ++i)
        {
            ringBuffer[(size_t) writeIndex] = samples[i];
            writeIndex = (writeIndex + 1) % frameSize;

            if (! primed)
            {
                if (writeIndex == 0)
                    primed = true;
                else
                    continue;
            }

            if (++samplesSinceHop >= hopSize)
            {
                samplesSinceHop = 0;

                // Copy ring -> frame in chronological order.
                int idx = writeIndex;
                for (int j = 0; j < frameSize; ++j)
                {
                    frame[(size_t) j] = ringBuffer[(size_t) idx];
                    idx = (idx + 1) % frameSize;
                }

                const float hz = estimateFrequency();
                onPitch (hz);
            }
        }
    }
}
