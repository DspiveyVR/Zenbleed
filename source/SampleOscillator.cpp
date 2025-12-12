#include "SampleOscillator.h"

SampleOscillator::SampleOscillator(juce::AudioProcessorValueTreeState& paramsRef) :
    formatManager(), parametersRef(paramsRef) {
    formatManager.registerBasicFormats();
};
SampleOscillator::~SampleOscillator() = default;

void SampleOscillator::loadSampleFromFile(const juce::File& file) {
    parametersRef.state.setProperty("SamplePath", file.getFullPathName(), nullptr);

    auto* reader = formatManager.createReaderFor(file);
    if (reader == nullptr)
        return;

    auto totalLength = (int)reader->lengthInSamples;
    auto numChannels = (int)reader->numChannels;

    auto sampleBuffer = std::make_unique<juce::AudioBuffer<float>>(numChannels, totalLength);
    reader->read(sampleBuffer.get(), 0, totalLength, 0, true, true);

    audioSampleSource = std::make_unique<juce::MemoryAudioSource>(*sampleBuffer.release(), true);
}

void SampleOscillator::processBlock(
        juce::MidiBuffer& inputBuffer,
        juce::AudioBuffer<float>& outputBuffer,
        const float speedScale,
        const juce::AudioPlayHead::PositionInfo* positionInfo) {
    juce::MemoryAudioSource* sampleSource = audioSampleSource.get();
    if (!sampleSource)
        return;

    juce::AudioSourceChannelInfo sampleInfo(outputBuffer);

    const double currentPpq = *positionInfo->getPpqPosition();
    juce::MidiMessage firstMessage;
    int firstEventTime = 0; // The sample offset (relative to buffer start)

    juce::MidiBuffer::Iterator iterator(inputBuffer);
    // Call getNextEvent() - this returns true if an event was found
    bool success = iterator.getNextEvent(firstMessage, firstEventTime);

    const double currentSamples = static_cast<double>(*positionInfo->getTimeInSamples());
    const double bpm = *positionInfo->getBpm();
    // 60 seconds cancels out the minutes unit in bpm.  What's left is samples over beats, where a "beat" is basically a quarter note.
    const double samplePerPpq = (60 * sampleRate) / bpm;
    const int bufferSize = outputBuffer.getNumSamples();

    if (success) {
        lastNoteNum = firstMessage.getNoteNumber();

        if (firstMessage.isNoteOn()) {
            noteBeingHeld = true;
            sampleInfo.startSample = firstEventTime;
            /*
                First event time is relative to the start of the buffer so it must be added to current 
                samples in order to get its absolute position.  Both of these are in units of samples 
                so they must be converted to ppq.
                Clearly the next quarter note is simply one quarter note after the current one, but speedScale
                must be accounted for since a higher speed effectively results in a shorter note and vice-versa.
            */
            nextQuarterNotePpq = ((currentSamples + firstEventTime) / samplePerPpq) + (1.0 / speedScale);
        } else {
            noteBeingHeld = false;
            sampleInfo.numSamples = firstEventTime - sampleInfo.startSample;
            sampleSource->setNextReadPosition(0);
            nextQuarterNotePpq = 0.0;
            sampleSource->getNextAudioBlock(sampleInfo); // Play the first note

            // If there are two notes immediately next to each other.
            juce::MidiMessage secondMessage;
            int secondEventTime = 0; // The sample offset (relative to buffer start)
            bool success2 = iterator.getNextEvent(secondMessage, secondEventTime);
            lastNoteNum = secondMessage.getNoteNumber();
            if (success2 && secondMessage.isNoteOn()) {
                noteBeingHeld = true;
                sampleInfo.startSample = secondEventTime;
                sampleSource->setNextReadPosition(0);

                const double samplePerPpq = (60 * sampleRate) / bpm;
                nextQuarterNotePpq = ((currentSamples + secondEventTime) / samplePerPpq) + (1.0 / speedScale);
            }
        }
    }

    if (!noteBeingHeld)
        return;

    // If next note is within the current block.
    while ((nextQuarterNotePpq * samplePerPpq) <= (currentSamples + bufferSize) && (nextQuarterNotePpq != 0.0)) {
        // Sample position of the note relative to the start of the current block.
        const double noteOffset = (nextQuarterNotePpq - currentPpq) * samplePerPpq;
        sampleInfo.numSamples = noteOffset - sampleInfo.startSample - 1;
        sampleSource->getNextAudioBlock(sampleInfo); // play the remainder of the current note

        sampleSource->setNextReadPosition(0);
        sampleInfo.startSample = noteOffset;

        // A higher speed means shorter quarter notes, so 1 / speedScale represents the length of a quarter
        // note relative to the baseline.
        // E.g. A speedScale of 2.0 results in a quarter note half the length of the baseline.
        nextQuarterNotePpq += (1.0 / speedScale);
    }
    sampleInfo.numSamples = outputBuffer.getNumSamples() - sampleInfo.startSample;
    sampleSource->getNextAudioBlock(sampleInfo); // play the remainder of the current note
}
