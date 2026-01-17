#include "PluginProcessor.h"

#include "MidiOscillator.h"
#include "PluginEditor.h"

//==============================================================================
PluginProcessor::PluginProcessor() :
    AudioProcessor(BusesProperties().withOutput("Output", juce::AudioChannelSet::stereo(), true)),
    parameters(
            *this,
            nullptr,
            juce::Identifier("ZenParameters"),
            {
                    std::make_unique<juce::AudioParameterFloat>(
                            "lowSpeed",
                            "Low Speed",
                            juce::NormalisableRange<float> { 0.1f, 16.0f, 0.01, 0.7 },
                            1.0f,
                            juce::AudioParameterFloatAttributes {}.withCategory(
                                    juce::AudioParameterFloat::genericParameter)),
                    std::make_unique<juce::AudioParameterFloat>(
                            "midSpeed",
                            "Mid Speed",
                            juce::NormalisableRange<float> { 0.1f, 64.0f, 0.01, 0.7 },
                            1.0f,
                            juce::AudioParameterFloatAttributes {}.withCategory(
                                    juce::AudioParameterFloat::genericParameter)),
                    std::make_unique<juce::AudioParameterFloat>(
                            "highSpeed",
                            "High Speed",
                            juce::NormalisableRange<float> { 0.1f, 256.0f, 0.01, 0.7 },
                            1.0f,
                            juce::AudioParameterFloatAttributes {}.withCategory(
                                    juce::AudioParameterFloat::genericParameter)),
                    std::make_unique<juce::AudioParameterChoice>(
                            "speedRange",
                            "Speed Range",
                            speedRangeChoices,
                            static_cast<int>(SpeedRange::Medium),
                            juce::AudioParameterChoiceAttributes {}.withCategory(
                                    juce::AudioParameterFloat::genericParameter)),
                    std::make_unique<juce::AudioParameterBool>(
                            "isMidiMode",
                            "Midi mode",
                            false,
                            juce::AudioParameterBoolAttributes {}.withCategory(
                                    juce::AudioParameterBool::genericParameter)),
                    std::make_unique<juce::AudioParameterBool>(
                            "isTuned",
                            "Tuned",
                            false,
                            juce::AudioParameterBoolAttributes {}.withCategory(
                                    juce::AudioParameterBool::genericParameter)),
            }),
    sampleOscillator(std::make_unique<SampleOscillator>(parameters)),
    midiOscillator(std::make_unique<MidiOscillator>()) {
    lowSpeedParameter = parameters.getRawParameterValue("lowSpeed");
    midSpeedParameter = parameters.getRawParameterValue("midSpeed");
    highSpeedParameter = parameters.getRawParameterValue("highSpeed");
    speedRangeParameter = parameters.getRawParameterValue("speedRange");
    isMidiModeParameter = parameters.getRawParameterValue("isMidiMode");
    isTunedParameter = parameters.getRawParameterValue("isTuned");
}

PluginProcessor::~PluginProcessor() {}

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
    sampleOscillator.get()->setSampleRate(sampleRate);
    midiOscillator.get()->setSampleRate(sampleRate);
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
    juce::ScopedNoDenormals noDenormals;

    const juce::AudioPlayHead* playhead = getPlayHead();
    if (!playhead)
        return;

    const juce::AudioPlayHead::PositionInfo* positionInfo = &*(playhead->getPosition());
    if (!positionInfo || !positionInfo->getBpm())
        return;

    double speedScale = 0.0;
    const SpeedRange rangeChoice = static_cast<SpeedRange>(static_cast<int>(speedRangeParameter->load()));
    switch (rangeChoice) {
        case SpeedRange::Low:
            speedScale = lowSpeedParameter->load();
            break;
        case SpeedRange::Medium:
            speedScale = midSpeedParameter->load();
            break;
        case SpeedRange::High:
            speedScale = highSpeedParameter->load();
            break;
        default:
            break;
    }

    bool isMidiMode = isMidiModeParameter->load();
    if (isMidiMode) {
        juce::MidiBuffer outputBuffer;

        midiOscillator->processBlock(
                buffer.getNumSamples(),
                midiMessages,
                outputBuffer,
                speedScale,
                positionInfo,
                isTunedParameter->load(),
                nextQuarterNotePpq,
                nextNoteSample);
        midiMessages.swapWith(outputBuffer);
    } else {
        sampleOscillator.get()->processBlock(
                midiMessages,
                buffer,
                speedScale,
                positionInfo,
                isTunedParameter->load(),
                nextQuarterNotePpq,
                nextNoteSample);
    }
}

//==============================================================================
bool PluginProcessor::hasEditor() const {
    return true; // (change this to false if you choose to not supply an editor)
}

juce::AudioProcessorEditor* PluginProcessor::createEditor() {
    auto editor = new PluginEditor(*this);
    return editor;
    // return new juce::GenericAudioProcessorEditor(*this);
}

void PluginProcessor::editorBeingDeleted(juce::AudioProcessorEditor* editor) noexcept {
    // This function is called by the framework when the host closes the GUI.
    juce::ignoreUnused(editor);
}

//==============================================================================
void PluginProcessor::getStateInformation(juce::MemoryBlock& destData) {
    // You should use this method to store your parameters in the memory block.
    // You could do that either as raw data, or use the XML or ValueTree classes
    // as intermediaries to make it easy to save and load complex data.
    auto state = parameters.copyState();
    const auto* path = parameters.state.getPropertyPointer("SamplePath");
    std::unique_ptr<juce::XmlElement> xml(state.createXml());
    copyXmlToBinary(*xml, destData);
}

void PluginProcessor::setStateInformation(const void* data, int sizeInBytes) {
    // You should use this method to restore your parameters from this memory block,
    // whose contents will have been created by the getStateInformation() call.
    std::unique_ptr<juce::XmlElement> xmlState(getXmlFromBinary(data, sizeInBytes));
    if (xmlState.get() != nullptr)
        if (xmlState->hasTagName(parameters.state.getType()))
            parameters.replaceState(juce::ValueTree::fromXml(*xmlState));

    const auto* path = parameters.state.getPropertyPointer("SamplePath");
    if (path) {
        juce::File sample(*path);
        sampleOscillator.get()->loadSampleFromFile(sample);
    }
}

//==============================================================================
// This creates new instances of the plugin..
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() { return new PluginProcessor(); }
