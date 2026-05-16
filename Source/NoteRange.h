#pragma once

namespace vocalpitch
{
    // Vocal-friendly range: C2 (36) to C6 (84) — 4 octaves.
    constexpr int kLowestMidiNote  = 36;
    constexpr int kHighestMidiNote = 84;
    constexpr int kNoteCount       = kHighestMidiNote - kLowestMidiNote + 1;

    constexpr float kUnvoiced = -1.0f;
}
