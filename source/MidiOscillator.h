#pragma once

#include <juce_audio_processors/juce_audio_processors.h>

class MidiOscillator {
public:
    MidiOscillator();
    ~MidiOscillator();

    void processBlock(
            const int bufferSize,
            juce::MidiBuffer& inputBuffer,
            juce::MidiBuffer& outputBuffer,
            const float speedScale,
            const juce::AudioPlayHead::PositionInfo* positionInfo);

    void setSampleRate(const double sr) { sampleRate = sr; }

private:
    double sampleRate = 0;
    bool wasPlaying = false;
    double nextQuarterNotePpq = 0;
    int lastNoteNum = 0;
};
