#include "YinPitchDetector.h"

#include <cmath>
#include <algorithm>

namespace vocalpitch
{
    void YinPitchDetector::prepare (double newSampleRate, int newFrameSize, int newHopSize)
    {
        sampleRate = newSampleRate;
        frameSize  = newFrameSize;
        hopSize    = newHopSize;

        ringBuffer.assign ((size_t) frameSize, 0.0f);
        frame     .assign ((size_t) frameSize, 0.0f);
        diff      .assign ((size_t) (frameSize / 2), 0.0f);
        reset();
    }

    void YinPitchDetector::reset()
    {
        std::fill (ringBuffer.begin(), ringBuffer.end(), 0.0f);
        writeIndex = 0;
        samplesSinceHop = 0;
        primed = false;
    }

    float YinPitchDetector::estimateFrequency()
    {
        const int W = frameSize / 2;

        // Energy gate — skip silence.
        float energy = 0.0f;
        for (int i = 0; i < frameSize; ++i)
            energy += frame[(size_t) i] * frame[(size_t) i];

        if (energy / (float) frameSize < 1.0e-6f)
            return 0.0f;

        // Step 1: difference function d(tau) = sum_{j=0}^{W-1} (x[j] - x[j+tau])^2
        diff[0] = 0.0f;
        for (int tau = 1; tau < W; ++tau)
        {
            float sum = 0.0f;
            for (int j = 0; j < W; ++j)
            {
                const float delta = frame[(size_t) j] - frame[(size_t) (j + tau)];
                sum += delta * delta;
            }
            diff[(size_t) tau] = sum;
        }

        // Step 2: cumulative mean normalized difference d'(tau).
        // In-place: d'(0) = 1, d'(tau) = d(tau) / ((1/tau) * sum_{k=1..tau} d(k))
        float runningSum = 0.0f;
        diff[0] = 1.0f;
        for (int tau = 1; tau < W; ++tau)
        {
            runningSum += diff[(size_t) tau];
            if (runningSum > 0.0f)
                diff[(size_t) tau] *= (float) tau / runningSum;
            else
                diff[(size_t) tau] = 1.0f;
        }

        // Step 3: absolute threshold — first tau where d'(tau) < threshold and is a local min.
        int tauEstimate = -1;
        const int tauMin = std::max (2, (int) (sampleRate / 1000.0));   // up to 1000 Hz
        const int tauMax = std::min (W - 1, (int) (sampleRate / 60.0)); // down to 60 Hz

        for (int tau = tauMin; tau < tauMax; ++tau)
        {
            if (diff[(size_t) tau] < threshold)
            {
                while (tau + 1 < tauMax && diff[(size_t) (tau + 1)] < diff[(size_t) tau])
                    ++tau;
                tauEstimate = tau;
                break;
            }
        }

        if (tauEstimate < 0)
            return 0.0f;

        // Step 4: parabolic interpolation around the minimum for sub-sample accuracy.
        float betterTau = (float) tauEstimate;
        if (tauEstimate > 0 && tauEstimate < W - 1)
        {
            const float s0 = diff[(size_t) (tauEstimate - 1)];
            const float s1 = diff[(size_t) tauEstimate];
            const float s2 = diff[(size_t) (tauEstimate + 1)];
            const float denom = (s0 + s2 - 2.0f * s1);
            if (std::abs (denom) > 1.0e-9f)
                betterTau = (float) tauEstimate + 0.5f * (s0 - s2) / denom;
        }

        return (float) (sampleRate / (double) betterTau);
    }
}
