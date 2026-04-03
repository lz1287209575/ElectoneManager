#pragma once
#include "DrumPattern.h"
#include <string>
#include <cstring>

namespace em {

/// Factory functions for 7 preset drum patterns matching the ControlPanel rhythm buttons.
/// Each returns a fully-initialized DrumPattern ready for the DrumSequencer.
namespace DrumPatternBank {

inline void setName(DrumPattern& p, const char* n) {
    std::strncpy(p.name, n, sizeof(p.name) - 1);
    p.name[sizeof(p.name) - 1] = '\0';
}

/// Indices into kDrumInstruments:
///  0=BD  1=SD  2=SS  3=CHH  4=OHH  5=PHH  6=CR  7=RD  8=LT  9=HT  10=CLP 11=CB

/// Pattern 0: March (进行曲) — 4/4, 120 BPM
inline DrumPattern createMarch() {
    DrumPattern p;
    setName(p, "March");
    p.steps = 16; p.stepsPerBeat = 4; p.beatsPerBar = 4;
    p.clear();
    //                 0  1  2  3  4  5  6  7  8  9 10 11 12 13 14 15
    // BD on 1, 3
    p.set(0, 0, 110); p.set(0, 8, 100);
    // SD on 2, 4
    p.set(1, 4, 100); p.set(1, 12, 100);
    // CHH on every 8th note
    for (int i = 0; i < 16; i += 2) p.set(3, i, 80);
    // Crash on beat 1
    p.set(6, 0, 70);
    return p;
}

/// Pattern 1: Waltz (华尔兹) — 3/4, 160 BPM
inline DrumPattern createWaltz() {
    DrumPattern p;
    setName(p, "Waltz");
    p.steps = 12; p.stepsPerBeat = 4; p.beatsPerBar = 3;
    p.clear();
    // BD on beat 1
    p.set(0, 0, 100);
    // CHH on beats 2, 3
    p.set(3, 4, 80); p.set(3, 8, 80);
    // Light SD ghost on beat 3 "and"
    p.set(1, 10, 50);
    return p;
}

/// Pattern 2: Swing (摇摆) — 4/4, 130 BPM, shuffle feel
inline DrumPattern createSwing() {
    DrumPattern p;
    setName(p, "Swing");
    p.steps = 16; p.stepsPerBeat = 4; p.beatsPerBar = 4;
    p.clear();
    // BD on 1 and 3
    p.set(0, 0, 100); p.set(0, 8, 90);
    // SD on 2 and 4
    p.set(1, 4, 100); p.set(1, 12, 100);
    // Ride on 1, 1+, 2, 2+, etc. (swing feel = emphasize downbeats)
    p.set(7, 0, 90);  p.set(7, 3, 60);  // 1 and swing-&
    p.set(7, 4, 80);  p.set(7, 7, 60);  // 2 and swing-&
    p.set(7, 8, 90);  p.set(7, 11, 60); // 3 and swing-&
    p.set(7, 12, 80); p.set(7, 15, 60); // 4 and swing-&
    // PHH on 2 and 4
    p.set(5, 4, 60); p.set(5, 12, 60);
    return p;
}

/// Pattern 3: Pop (流行) — 4/4, 120 BPM, standard 8-beat
inline DrumPattern createPop() {
    DrumPattern p;
    setName(p, "Pop");
    p.steps = 16; p.stepsPerBeat = 4; p.beatsPerBar = 4;
    p.clear();
    // BD on 1, 1e, 3
    p.set(0, 0, 110); p.set(0, 2, 70); p.set(0, 8, 100);
    // SD on 2, 4
    p.set(1, 4, 100); p.set(1, 12, 100);
    // CHH on every 8th note
    for (int i = 0; i < 16; i += 2) p.set(3, i, 85);
    // OHH on the & of 4
    p.set(4, 14, 75);
    return p;
}

/// Pattern 4: R&B — 4/4, 100 BPM, syncopated
inline DrumPattern createRnB() {
    DrumPattern p;
    setName(p, "R&B");
    p.steps = 16; p.stepsPerBeat = 4; p.beatsPerBar = 4;
    p.clear();
    // BD on 1, 2-and, 3
    p.set(0, 0, 110); p.set(0, 6, 80); p.set(0, 8, 100);
    // SD on 2, 4
    p.set(1, 4, 100); p.set(1, 12, 100);
    // CHH 16th notes
    for (int i = 0; i < 16; ++i) p.set(3, i, (i % 2 == 0) ? 80 : 55);
    // Clap layered with snare
    p.set(10, 4, 70); p.set(10, 12, 70);
    return p;
}

/// Pattern 5: Latin (拉丁) — 4/4, 128 BPM, clave-based
inline DrumPattern createLatin() {
    DrumPattern p;
    setName(p, "Latin");
    p.steps = 16; p.stepsPerBeat = 4; p.beatsPerBar = 4;
    p.clear();
    // Son clave 3-2 pattern on side stick
    p.set(2, 0, 90); p.set(2, 3, 90); p.set(2, 6, 90);
    p.set(2, 10, 90); p.set(2, 12, 90);
    // BD on 1, 2-and, 4
    p.set(0, 0, 100); p.set(0, 6, 80); p.set(0, 12, 90);
    // CHH driving 8th notes
    for (int i = 0; i < 16; i += 2) p.set(3, i, 80);
    // Cowbell on offbeats
    p.set(11, 2, 70); p.set(11, 6, 70); p.set(11, 10, 70); p.set(11, 14, 70);
    return p;
}

/// Pattern 6: Ethnic (民乐) — 4/4, 108 BPM, world-percussion feel
inline DrumPattern createEthnic() {
    DrumPattern p;
    setName(p, "Ethnic");
    p.steps = 16; p.stepsPerBeat = 4; p.beatsPerBar = 4;
    p.clear();
    // Low tom as doumbek-like pattern
    p.set(8, 0, 110); p.set(8, 3, 70); p.set(8, 6, 80);
    p.set(8, 8, 100); p.set(8, 11, 60); p.set(8, 14, 70);
    // High tom fills
    p.set(9, 2, 60); p.set(9, 10, 50);
    // Clap on 2, 4
    p.set(10, 4, 90); p.set(10, 12, 90);
    // Cowbell accents
    p.set(11, 0, 80); p.set(11, 4, 60); p.set(11, 8, 80); p.set(11, 12, 60);
    return p;
}

/// Get a preset pattern by index (0-6, matching ControlPanel rhythm button order).
inline DrumPattern getPreset(int index) {
    switch (index) {
    case 0: return createMarch();
    case 1: return createWaltz();
    case 2: return createSwing();
    case 3: return createPop();
    case 4: return createRnB();
    case 5: return createLatin();
    case 6: return createEthnic();
    default: return createPop();
    }
}

/// Get a preset pattern by name (case-sensitive, English name).
inline DrumPattern patternByName(const std::string& name) {
    if (name == "March")  return createMarch();
    if (name == "Waltz")  return createWaltz();
    if (name == "Swing")  return createSwing();
    if (name == "Pop")    return createPop();
    if (name == "R&B")    return createRnB();
    if (name == "Latin")  return createLatin();
    if (name == "Ethnic") return createEthnic();
    return createPop(); // default
}

} // namespace DrumPatternBank
} // namespace em
