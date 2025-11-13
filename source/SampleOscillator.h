#pragma once

#include <juce_audio_formats/juce_audio_formats.h>
#include <juce_audio_processors/juce_audio_processors.h>

class SampleOscillator {
public:
    SampleOscillator(juce::AudioProcessorValueTreeState& paramsRef);
    ~SampleOscillator();

    void processBlock(
            juce::MidiBuffer& inputBuffer,
            juce::AudioBuffer<float>& outputBuffer,
            const float speedScale,
            const juce::AudioPlayHead::PositionInfo* positionInfo);

    void setSampleRate(const double sr) { sampleRate = sr; }

    void loadSampleFromFile(const juce::File& file);

private:
    double sampleRate = 0.0;
    bool wasPlaying = false;
    double nextQuarterNotePpq = 0;
    int lastNoteNum = 0;

    juce::AudioFormatManager formatManager;
    std::unique_ptr<juce::MemoryAudioSource> audioSampleSource;
    bool noteBeingHeld = false;

    juce::AudioProcessorValueTreeState& parametersRef;

};