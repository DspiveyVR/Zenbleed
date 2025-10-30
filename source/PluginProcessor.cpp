#include "PluginProcessor.h"

#include "MidiOscillator.h"
#include "PluginEditor.h"
using namespace std;

//==============================================================================
PluginProcessor::PluginProcessor() :
    AudioProcessor(
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
            }) {
    speedParameter = parameters.getRawParameterValue("speed");
    midiOscillator = new MidiOscillator();
}

PluginProcessor::~PluginProcessor() { delete midiOscillator; }

//==============================================================================
const juce::String PluginProcessor::getName() const { return JucePlugin_Name; }

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

void PluginProcessor::setCurrentProgram(int index) { juce::ignoreUnused(index); }

const juce::String PluginProcessor::getProgramName(int index) {
    juce::ignoreUnused(index);
    return {};
}

void PluginProcessor::changeProgramName(int index, const juce::String& newName) { juce::ignoreUnused(index, newName); }

//==============================================================================
void PluginProcessor::prepareToPlay(const double sampleRate, int) {
    if (midiOscillator) midiOscillator->setSampleRate(sampleRate);
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

void PluginProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages) {
    buffer.clear();
    // midiMessages.clear();

    const juce::AudioPlayHead* playhead = getPlayHead();
    if (!playhead) return;

    const juce::AudioPlayHead::PositionInfo *positionInfo = &*(playhead->getPosition());
    if (!positionInfo || !positionInfo->getBpm()) return;

    juce::MidiBuffer outputBuffer;

    midiOscillator->processBlock(buffer.getNumSamples(), midiMessages, outputBuffer, *speedParameter, positionInfo);
    midiMessages.swapWith(outputBuffer);
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
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() { return new PluginProcessor(); }
