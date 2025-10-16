#include "PluginProcessor.h"
#include "PluginEditor.h"

//==============================================================================
PluginProcessor::PluginProcessor()
    : AudioProcessor(
          BusesProperties()
#if !JucePlugin_IsMidiEffect
#if !JucePlugin_IsSynth
              .withInput("Input", juce::AudioChannelSet::stereo(), true)
#endif
              .withOutput("Output", juce::AudioChannelSet::stereo(), true)
#endif
      ),
      parameters(
          *this,
          nullptr,
          juce::Identifier("ZenParameters"),
          {
              std::make_unique<juce::AudioParameterFloat>("speed", "Speed", 0.0f, 4.0f, 1.0f),
          }
      ) {
    speedParameter = parameters.getRawParameterValue("speed");
}

PluginProcessor::~PluginProcessor() = default;

//==============================================================================
const juce::String PluginProcessor::getName() const {
    return JucePlugin_Name;
}

bool PluginProcessor::acceptsMidi() const {
#if JucePlugin_WantsMidiInput
    return true;
#else
    return false;
#endif
}

bool PluginProcessor::producesMidi() const {
#if JucePlugin_ProducesMidiOutput
    return true;
#else
    return false;
#endif
}

bool PluginProcessor::isMidiEffect() const {
#if JucePlugin_IsMidiEffect
    return true;
#else
    return false;
#endif
}

double PluginProcessor::getTailLengthSeconds() const { return 0.0; }

int PluginProcessor::getNumPrograms() {
    return 1; // NB: some hosts don't cope very well if you tell them there are 0 programs,
    // so this should be at least 1, even if you're not really implementing programs.
}

int PluginProcessor::getCurrentProgram() { return 0; }

void PluginProcessor::setCurrentProgram(int index) {
    juce::ignoreUnused(index);
}

const juce::String PluginProcessor::getProgramName(int index) {
    juce::ignoreUnused(index);
    return {};
}

void PluginProcessor::changeProgramName(int index, const juce::String& newName) {
    juce::ignoreUnused(index, newName);
}

//==============================================================================
void PluginProcessor::prepareToPlay(double sampleRate, int) {
    this->sampleRate = sampleRate;
}

void PluginProcessor::releaseResources() {
    // When playback stops, you can use this as an opportunity to free up any
    // spare memory, etc.
}

bool PluginProcessor::isBusesLayoutSupported(const BusesLayout& layouts) const {
#if JucePlugin_IsMidiEffect
    juce::ignoreUnused(layouts);
    return true;
#else
    // This is the place where you check if the layout is supported.
    // In this template code we only support mono or stereo.
    if (layouts.getMainOutputChannelSet() != juce::AudioChannelSet::mono()
        && layouts.getMainOutputChannelSet() != juce::AudioChannelSet::stereo())
        return false;

    // This checks if the input layout matches the output layout
#if !JucePlugin_IsSynth
    if (layouts.getMainOutputChannelSet() != layouts.getMainInputChannelSet())
        return false;
#endif

    return true;
#endif
}

void PluginProcessor::processBlock(
    juce::AudioBuffer<float>& buffer,
    juce::MidiBuffer& midiMessages
) {
    buffer.clear();
    midiMessages.clear();

    const juce::AudioPlayHead* playhead = getPlayHead();
    if (!playhead) return;

    const auto positionInfo = playhead->getPosition();
    if (!positionInfo || !positionInfo->getBpm()) return;

    const double currentPpq = *positionInfo->getPpqPosition();

    // Resets the next note counter when playback starts so that it's not way off in the distance.
    const bool isPlaying = positionInfo->getIsPlaying();
    if (isPlaying && !wasPlaying) {
        // Ceil ensures that the first note lies on a quarter note boundary.
        // The integer part of ppq represents quarter notes.
        nextQuarterNotePpq = ceil(currentPpq);
    }
    wasPlaying = isPlaying;

    const double currentSamples = static_cast<double>(*positionInfo->getTimeInSamples());
    const double bpm = *positionInfo->getBpm();
    const double samplePerPpq = (60 * sampleRate) / bpm;
    const int bufferSize = buffer.getNumSamples();
    // If next note is within the current block.

    if ((nextQuarterNotePpq * samplePerPpq) <= currentSamples + bufferSize) {
        // Sample position of the note relative to the start of the current block.
        const double noteOffset = (nextQuarterNotePpq - currentPpq) * samplePerPpq;
        const double speedScale = *speedParameter;

        // Add note off and note on at the same time to create a legato effect (continuous stream of notes).
        midiMessages.addEvent(juce::MidiMessage::noteOff(1, 30), floor(noteOffset));
        midiMessages.addEvent(juce::MidiMessage::noteOn(1, 30, 1.0f), floor(noteOffset));

        // A higher speed means shorter quarter notes, so 1 / speedScale represents the length of a quarter
        // note relative to the baseline.
        // E.g. A speedScale of 2.0 results in a quarter note half the length of the baseline.
        nextQuarterNotePpq += (1.0 / speedScale);
    }
}

//==============================================================================
bool PluginProcessor::hasEditor() const {
    return true; // (change this to false if you choose to not supply an editor)
}

juce::AudioProcessorEditor* PluginProcessor::createEditor() {
    // auto editor = new PluginEditor(*this);
    // activeEditor = editor;
    return new juce::GenericAudioProcessorEditor(*this);
}

void PluginProcessor::editorBeingDeleted(juce::AudioProcessorEditor* editor) noexcept {
    // This function is called by the framework when the host closes the GUI.
    juce::ignoreUnused(editor);
    activeEditor = nullptr; // <--- THIS IS THE LINE THAT CLEARS THE REFERENCE
}

//==============================================================================
void PluginProcessor::getStateInformation(juce::MemoryBlock& destData) {
    // You should use this method to store your parameters in the memory block.
    // You could do that either as raw data, or use the XML or ValueTree classes
    // as intermediaries to make it easy to save and load complex data.
    juce::ignoreUnused(destData);
}

void PluginProcessor::setStateInformation(const void* data, int sizeInBytes) {
    // You should use this method to restore your parameters from this memory block,
    // whose contents will have been created by the getStateInformation() call.
    juce::ignoreUnused(data, sizeInBytes);
}

//==============================================================================
// This creates new instances of the plugin..
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() {
    return new PluginProcessor();
}
