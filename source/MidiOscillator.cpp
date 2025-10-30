#include "MidiOscillator.h"

MidiOscillator::MidiOscillator() = default;
MidiOscillator::~MidiOscillator() = default;

void MidiOscillator::processBlock(
    const juce::AudioBuffer<float>& audioBuffer,
    juce::MidiBuffer& newBuffer,
    const float speedScale,
    const juce::AudioPlayHead::PositionInfo *positionInfo
) {
    const double currentPpq = *positionInfo->getPpqPosition();
    juce::MidiMessage firstMessage;
    int firstEventTime = 0; // The sample offset (relative to buffer start)

    juce::MidiBuffer::Iterator iterator(newBuffer);
    // Call getNextEvent() - this returns true if an event was found
    const bool success = iterator.getNextEvent(firstMessage, firstEventTime);

    const auto currentSamples = static_cast<double>(*positionInfo->getTimeInSamples());
    const double bpm = *positionInfo->getBpm();
    const double samplePerPpq = (60 * sampleRate) / bpm;
    const int bufferSize = audioBuffer.getNumSamples();
    if (!positionInfo->getIsPlaying()) {
        newBuffer.addEvent(juce::MidiMessage::noteOff(1, 30), currentSamples);
        nextQuarterNotePpq = 0.0;
    }

    if (success) {
        lastNoteNum = firstMessage.getNoteNumber();

        if (firstMessage.isNoteOn()) {
            newBuffer.addEvent(juce::MidiMessage::noteOn(1, lastNoteNum, 1.0f), firstEventTime);
            nextQuarterNotePpq = ((currentSamples + firstEventTime) / samplePerPpq) + (1.0 / speedScale);
        } else {
            newBuffer.addEvent(juce::MidiMessage::noteOff(1, lastNoteNum), firstEventTime);
            nextQuarterNotePpq = 0.0;

            // If there are two notes immediately next to each other.
            juce::MidiMessage secondMessage;
            int secondEventTime = 0; // The sample offset (relative to buffer start)
            const bool success2 = iterator.getNextEvent(secondMessage, secondEventTime);
            lastNoteNum = secondMessage.getNoteNumber();
            if (success2 && secondMessage.isNoteOn()) {
                const double hertz = juce::MidiMessage::getMidiNoteInHertz(lastNoteNum);
                const double samplePerPpq = (60 * sampleRate) / bpm;
                newBuffer.addEvent(juce::MidiMessage::noteOn(1, lastNoteNum, 1.0f), secondEventTime);
                nextQuarterNotePpq = ((currentSamples + secondEventTime) / samplePerPpq) + (1.0 / speedScale);
            }
        }
    }

    // If next note is within the current block.
    if ((nextQuarterNotePpq * samplePerPpq) <= (currentSamples + bufferSize) && (nextQuarterNotePpq != 0.0)) {
        // Sample position of the note relative to the start of the current block.
        const double noteOffset = (nextQuarterNotePpq - currentPpq) * samplePerPpq;

        // Add note off and note on at the same time to create a legato effect (continuous stream of notes).
        newBuffer.addEvent(juce::MidiMessage::noteOff(1, lastNoteNum), floor(noteOffset));
        newBuffer.addEvent(juce::MidiMessage::noteOn(1, lastNoteNum, 1.0f), floor(noteOffset));

        // A higher speed means shorter quarter notes, so 1 / speedScale represents the length of a quarter
        // note relative to the baseline.
        // E.g. A speedScale of 2.0 results in a quarter note half the length of the baseline.
        nextQuarterNotePpq += (1.0 / speedScale);
    }
}
