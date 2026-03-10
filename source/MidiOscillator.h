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
    enum class OperationMode { Default, Etet, Tuned };
    void processBlock(
        const int bufferSize,
        juce::MidiBuffer& inputBuffer,
        juce::MidiBuffer& outputBuffer,
        const float speedScale,
        const juce::AudioPlayHead::PositionInfo* positionInfo,
        const OperationMode operationMode,
        double& nextQuarterNotePpq,
        double& nextNoteSample,
        float noteLength,
        int etetRootNote,
        float etetNumerator,
        float etetDenominator,
        float velocity,
        bool& killswitch,
        bool isKeytrack,
        int fixedNoteNumber
    );

    void setSampleRate(const double sr) { sampleRate = sr; }

private:
    void processUntuned(
        const int bufferSize,
        juce::MidiBuffer& inputBuffer,
        juce::MidiBuffer& outputBuffer,
        const float speedScale,
        const juce::AudioPlayHead::PositionInfo* positionInfo,
        const OperationMode operationMode,
        double& nextQuarterNotePpq,
        double& nextNoteSample,
        float noteLength,
        int etetRootNote,
        float etetNumerator,
        float etetDenominator,
        float velocity,
        bool& killswitch,
        bool isKeytrack,
        int fixedNoteNumber
    );

    void processTuned(
        const int bufferSize,
        juce::MidiBuffer& inputBuffer,
        juce::MidiBuffer& outputBuffer,
        const float speedScale,
        const juce::AudioPlayHead::PositionInfo* positionInfo,
        double& nextQuarterNotePpq,
        double& nextNoteSample,
        float noteLength,
        float velocity,
        bool& killswitch,
        bool isKeytrack,
        int fixedNoteNumber
    );

    double sampleRate = 0;
    bool wasPlaying = false;
    int lastNoteInput = 0; /**< The number of the last musical note that was recieved through the MIDI buffer. */
    int lastNoteOutput = 0; /**< The number of the last musical note that was output by the MIDI oscillator. */
    bool noteBeingHeld = false; /**< indicates whether a note is currently being held. */
};
