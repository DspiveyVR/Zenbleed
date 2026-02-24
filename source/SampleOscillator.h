#pragma once

#include <juce_audio_formats/juce_audio_formats.h>
#include <juce_audio_processors/juce_audio_processors.h>

class SampleOscillator {
public:
    SampleOscillator(juce::AudioProcessorValueTreeState& paramsRef);
    ~SampleOscillator();

    /**
     * @brief Secondary processBlock method called by the main processBlock in PluginProcessor.
     * 
     * Internally triggers samples at a speed dictated by bpm * speedScale.
     * @param inputBuffer MIDI notes to be processed by the SampleOscillator.
     * @param outputBuffer Audio output.
     * @param speedScale Current value of the speed parameter.
     * @param positionInfo Position info given by the host DAW.
     */
    void processBlock(
            juce::MidiBuffer& inputBuffer,
            juce::AudioBuffer<float>& outputBuffer,
            const float speedScale,
            const juce::AudioPlayHead::PositionInfo* positionInfo,
            bool isTuned,
            double& nextQuarterNotePpq,
            double& nextNoteSample,
            float noteLength,
            float samplePitchBendRatio,
            bool isEtet,
            int etetRootNote,
            float etetNumerator,
            float etetDenominator,
            bool& killswitch);

    void setSampleRate(const double sr) { sampleRate = sr; }

    /**
     * @brief Loads an audio sample based on a given file path.
     * 
     * This method is called when the user selects a file and also when
     * the plugin is first loaded by the DAW (assuming a sample has been loaded previously).
     * 
     * @warning This method should only be called from the message thread since File I/O is not a realtime operation.
     * @param file 
     */
    void loadSampleFromFile(const juce::File& file);

private:
    void processUntuned(
            juce::MidiBuffer& inputBuffer,
            juce::AudioBuffer<float>& outputBuffer,
            const float speedScale,
            const juce::AudioPlayHead::PositionInfo* positionInfo,
            double& nextQuarterNotePpq,
            double& nextNoteSample,
            float noteLength,
            float samplePitchBendRatio,
            bool success,
            double firstEventTime,
            juce::MidiMessage firstMessage,
            double currentSamples,
            double samplePerPpq,
            const int bufferSize,
            juce::MidiBuffer::Iterator& iterator,
            juce::AudioBuffer<float>* sampleBuffer,
            const double bpm,
            double currentPpq,
            bool isEtet,
            int etetRootNote,
            float etetNumerator,
            float etetDenominator,
            bool& killswitch);

    void processTuned(
            juce::MidiBuffer& inputBuffer,
            juce::AudioBuffer<float>& outputBuffer,
            const float speedScale,
            const juce::AudioPlayHead::PositionInfo* positionInfo,
            double& nextQuarterNotePpq,
            double& nextNoteSample,
            float noteLength,
            float samplePitchBendRatio,
            bool success,
            double firstEventTime,
            juce::MidiMessage firstMessage,
            double currentSamples,
            double samplePerPpq,
            const int bufferSize,
            juce::MidiBuffer::Iterator& iterator,
            juce::AudioBuffer<float>* sampleBuffer,
            const double bpm,
            double currentPpq,
            bool& killswitch);

    double sampleRate = 0.0;
    bool wasPlaying = false;
    bool currentSampleEnded = false;
    int lastNoteInput = 0; /**< The number of the last musical note that was played. */
    double nextReadPosition = 0.0;

    juce::AudioFormatManager formatManager;
    std::unique_ptr<juce::AudioBuffer<float>> loadedSampleBuffer; /**< The buffer that holds the loaded sample data. */
    bool noteBeingHeld = false; /**< indicates whether a note is currently being held. */

    juce::AudioProcessorValueTreeState& parametersRef;
};
