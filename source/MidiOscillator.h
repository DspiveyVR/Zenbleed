#pragma once

#include <juce_audio_processors/juce_audio_processors.h>

class MidiOscillator {
public:
    MidiOscillator();
    ~MidiOscillator();

    void processBlock(
            const juce::AudioBuffer<float>& audioBuffer,
            juce::MidiBuffer& newBuffer,
            float speedScale,
            const juce::AudioPlayHead::PositionInfo* positionInfo);

    void setSampleRate(const double sr) { sampleRate = sr; }

private:
    double sampleRate = 0;
    bool wasPlaying = false;
    double nextQuarterNotePpq = 0;
    int lastNoteNum = 0;
};
