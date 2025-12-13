#pragma once

#include <juce_audio_processors/juce_audio_processors.h>

class MidiOscillator {
public:
    MidiOscillator();
    ~MidiOscillator();

    /**
     * @brief Secondary processBlock method called by the main processBlock in PluginProcessor.
     * 
     * Creates a new MIDI buffer based on the input buffer with notes played at a
     * speed dictated by the bpm * speedScale.
     * @param bufferSize 
     * @param inputBuffer Input MIDI notes.
     * @param outputBuffer Modified MIDI buffer.
     * @param speedScale Current value of the speed parameter.
     * @param positionInfo Position info given by the host DAW.
     */
    void processBlock(
            const int bufferSize,
            juce::MidiBuffer& inputBuffer,
            juce::MidiBuffer& outputBuffer,
            const float speedScale,
            const juce::AudioPlayHead::PositionInfo* positionInfo,
            bool isTuned,
            double& nextQuarterNotePpq,
            double& nextNoteSample);

    void setSampleRate(const double sr) { sampleRate = sr; }

private:
    double sampleRate = 0;
    bool wasPlaying = false;
    int lastNoteNum = 0; /**< The number of the last musical note that was played. */
};
