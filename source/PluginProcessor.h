#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include "SampleOscillator.h"

#if (MSVC)
#include "ipps.h"
#endif

#include "MidiOscillator.h"

class PluginEditor;

class PluginProcessor final : public juce::AudioProcessor {
public:
    /**
     * @brief Construct a new Plugin Processor object

     * Uses RAII to create MidiOscillator and SampleOscillator as child objects,
     * which are used to modularize the DSP logic.
     */
    PluginProcessor();
    ~PluginProcessor() override;

    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;

    bool isBusesLayoutSupported(const BusesLayout& layouts) const override;

    using AudioProcessor::processBlock;
    void processBlock(juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    void editorBeingDeleted(juce::AudioProcessorEditor* editor) noexcept;
    bool hasEditor() const override;

    const juce::String getName() const override;

    bool acceptsMidi() const override;
    bool producesMidi() const override;
    bool isMidiEffect() const override;
    double getTailLengthSeconds() const override;

    int getNumPrograms() override;
    int getCurrentProgram() override;
    void setCurrentProgram(int index) override;
    const juce::String getProgramName(int index) override;
    void changeProgramName(int index, const juce::String& newName) override;

    const juce::StringArray getSpeedRangeChoices() { return speedRangeChoices; }
    juce::AudioProcessorValueTreeState& getParametersApvts() { return parameters; }

    void getStateInformation(juce::MemoryBlock& destData) override;
    void setStateInformation(const void* data, int sizeInBytes) override;

    std::atomic<float> lastNoteSamplePos = 0.0;
    std::unique_ptr<SampleOscillator>
            sampleOscillator; /**< Child instance of SampleOscillator created in the constructor of PluginProcessor */

private:
    enum class SpeedRange { Low, Medium, High };
    const juce::StringArray speedRangeChoices = { "Low", "Medium", "High" };

    juce::AudioProcessorValueTreeState parameters;
    std::atomic<float>* lowSpeedParameter = nullptr; /**< Speed parameter (low range): acts as a pitch bend */
    std::atomic<float>* midSpeedParameter = nullptr; /**< Speed parameter (mid range): acts as a pitch bend */
    std::atomic<float>* highSpeedParameter = nullptr; /**< Speed parameter (high range): acts as a pitch bend */
    std::atomic<float>* speedRangeParameter =
            nullptr; /**< Speed range parameter: selects which speed range to choose */
    std::atomic<float>* isMidiModeParameter =
            nullptr; /**< Midi mode parameter: toggles between MIDI mode and sampler mode */
    std::atomic<float>* isTunedParameter =
            nullptr; /**< Tuned mode parameter: determines whether or not the extratone is tuned to the input note */
    std::atomic<float>* isEtetParameter =
            nullptr; /**< Extratone Equal Temperament (ETET) mode parameter: determines whether or not the "Extratone Equal Temperament (ETET) mode is enabled" */
    std::atomic<float>* etetRootNoteParameter = nullptr; /**< ETET root note parameter: The root note for Extratone Equal Temperament (ETET) */
    std::atomic<float>* etetNumeratorParameter = nullptr; /**< Extratone Equal Temperament (ETET) numerator parameter: Controls the numerator of the harmonic interval used in Extratone Equal Temperament (ETET) */
    std::atomic<float>* etetDenominatorParameter = nullptr; /**< Extratone Equal Temperament (ETET) denominator parameter: Controls the denominator of the harmonic interval used in Extratone Equal Temperament (ETET) */
    std::atomic<float>* noteLengthParameter = nullptr; /**< Note length parameter: Controls how long a note is held before the next note is played */
    std::atomic<float>* samplePitchBendParameter = nullptr; /**< Sample pitch bend parameter: Changes the pitch of the loaded sample (in semitones) */
    std::atomic<float>* velocityParameter = nullptr; /**< Velocity parameter: Controls the velocity of the MIDI note being played */
    std::atomic<float>* isKeytrackParameter = nullptr; /**< Is keytrack parameter: Dictates wheter the MIDI note triggered is equal to the input note or whether it is fixed. */
    std::atomic<float>* fixedNoteNumberParameter = nullptr; /**< Fixed note number parameter: Dictates the MIDI note to be triggered when keytrack is false */
    double nextQuarterNotePpq = 0; /**< The ppq position of the next note to be played. */
    double nextNoteSample = 0; /**< The sample position of the next note to be played. */
    bool killswitch = false;

    std::unique_ptr<MidiOscillator>
            midiOscillator; /**< Child instance of MidiOscillator created in the constructor of PluginProcessor */

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PluginProcessor)
};
