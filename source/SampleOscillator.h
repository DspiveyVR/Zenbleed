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
            bool isUnjustIntonation,
            int unjustRootNote,
            float unjustNumerator,
            float unjustDenominator);

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
            bool success,
            double firstEventTime,
            juce::AudioSourceChannelInfo& sampleInfo,
            juce::MidiMessage firstMessage,
            double currentSamples,
            double samplePerPpq,
            const int bufferSize,
            juce::MidiBuffer::Iterator& iterator,
            juce::MemoryAudioSource* sampleSource,
            const double bpm,
            double currentPpq,
            bool isUnjustIntonation,
            int unjustRootNote,
            float unjustNumerator,
            float unjustDenominator);

    void processTuned(
            juce::MidiBuffer& inputBuffer,
            juce::AudioBuffer<float>& outputBuffer,
            const float speedScale,
            const juce::AudioPlayHead::PositionInfo* positionInfo,
            double& nextQuarterNotePpq,
            double& nextNoteSample,
            float noteLength,
            bool success,
            double firstEventTime,
            juce::AudioSourceChannelInfo& sampleInfo,
            juce::MidiMessage firstMessage,
            double currentSamples,
            double samplePerPpq,
            const int bufferSize,
            juce::MidiBuffer::Iterator& iterator,
            juce::MemoryAudioSource* sampleSource,
            const double bpm,
            double currentPpq);

    double sampleRate = 0.0;
    bool wasPlaying = false;
    int lastNoteNum = 0; /**< The number of the last musical note that was played. */

    juce::AudioFormatManager formatManager;
    std::unique_ptr<juce::MemoryAudioSource> audioSampleSource; /**< The source that holds the loaded sample data. */
    bool noteBeingHeld = false; /**< indicates whether a note is currently being held. */

    juce::AudioProcessorValueTreeState& parametersRef;
};
