#include "MidiOscillator.h"
#include "Utilities.h"

MidiOscillator::MidiOscillator() = default;
MidiOscillator::~MidiOscillator() = default;

void MidiOscillator::processBlock(
        // const juce::AudioBuffer<float>& audioBuffer,
        const int bufferSize,
        juce::MidiBuffer& inputBuffer,
        juce::MidiBuffer& outputBuffer,
        const float speedScale,
        const juce::AudioPlayHead::PositionInfo* positionInfo,
        bool isTuned,
        double& nextQuarterNotePpq,
        double& nextNoteSample,
        float noteLength) {
    if (!isTuned) {
        const double currentPpq = *positionInfo->getPpqPosition();
        juce::MidiMessage firstMessage;
        int firstEventTime = 0; // The sample offset (relative to buffer start)

        juce::MidiBuffer::Iterator iterator(inputBuffer);
        // Call getNextEvent() - this returns true if an event was found
        const bool success = iterator.getNextEvent(firstMessage, firstEventTime);

        const auto currentSamples = static_cast<double>(*positionInfo->getTimeInSamples());
        const double bpm = *positionInfo->getBpm();
        // 60 seconds cancels out the minutes unit in bpm.  What's left is samples over beats, where a "beat" is basically a quarter note.
        const double samplePerPpq = (60 * sampleRate) / bpm;
        if (!positionInfo->getIsPlaying()) {
            outputBuffer.addEvent(juce::MidiMessage::noteOff(1, 30), currentSamples);
            nextQuarterNotePpq = 0.0;
        }

        if (success) {
            lastNoteNum = firstMessage.getNoteNumber();

            if (firstMessage.isNoteOn()) {
                outputBuffer.addEvent(juce::MidiMessage::noteOn(1, lastNoteNum, 1.0f), firstEventTime);
                noteBeingHeld = true;
                /*
                First event time is relative to the start of the buffer so it must be added to current 
                samples in order to get its absolute position.  Both of these are in units of samples 
                so they must be converted to ppq.
                Clearly the next quarter note is simply one quarter note after the current one, but speedScale
                must be accounted for since a higher speed effectively results in a shorter note and vice-versa.
            */
                nextQuarterNotePpq = ((currentSamples + firstEventTime) / samplePerPpq) + (1.0 / speedScale);
            } else {
                outputBuffer.addEvent(juce::MidiMessage::noteOff(1, lastNoteNum), firstEventTime);
                noteBeingHeld = false;
                nextQuarterNotePpq = 0.0;

                // If there are two notes immediately next to each other.
                juce::MidiMessage secondMessage;
                int secondEventTime = 0; // The sample offset (relative to buffer start)
                const bool success2 = iterator.getNextEvent(secondMessage, secondEventTime);
                lastNoteNum = secondMessage.getNoteNumber();
                if (success2 && secondMessage.isNoteOn()) {
                    outputBuffer.addEvent(juce::MidiMessage::noteOn(1, lastNoteNum, 1.0f), secondEventTime);
                    noteBeingHeld = true;
                    nextQuarterNotePpq = ((currentSamples + secondEventTime) / samplePerPpq) + (1.0 / speedScale);
                }
            }
        }

        double adjustedNoteEnd = nextQuarterNotePpq - (1.0 / speedScale) + (noteLength / speedScale);
        bool nextNoteInBlock = (nextQuarterNotePpq * samplePerPpq) <= (currentSamples + bufferSize)
                               && !compareFloat(nextQuarterNotePpq, 0.0);
        bool noteEndInBlock = (adjustedNoteEnd * samplePerPpq) <= (currentSamples + bufferSize);
        while ((noteBeingHeld && noteEndInBlock) || nextNoteInBlock) {
            if (noteBeingHeld && noteEndInBlock) {
                outputBuffer.addEvent(
                        juce::MidiMessage::noteOff(1, lastNoteNum),
                        floor((adjustedNoteEnd - currentPpq) * samplePerPpq));
                noteBeingHeld = false;
            }
            if (nextNoteInBlock) {
                // Sample position of the note relative to the start of the current block.
                const double noteOffset = (nextQuarterNotePpq - currentPpq) * samplePerPpq;

                // Add note off and note on at the same time to create a legato effect (continuous stream of notes).
                outputBuffer.addEvent(juce::MidiMessage::noteOff(1, lastNoteNum), floor(noteOffset));
                outputBuffer.addEvent(juce::MidiMessage::noteOn(1, lastNoteNum, 1.0f), floor(noteOffset));
                noteBeingHeld = true;

                // A higher speed means shorter quarter notes, so 1 / speedScale represents the length of a quarter
                // note relative to the baseline.
                // E.g. A speedScale of 2.0 results in a quarter note half the length of the baseline.
                nextQuarterNotePpq += (1.0 / speedScale);
                adjustedNoteEnd = nextQuarterNotePpq - (1.0 / speedScale) + (noteLength / speedScale);
                nextNoteInBlock = (nextQuarterNotePpq * samplePerPpq) <= (currentSamples + bufferSize)
                                  && !compareFloat(nextQuarterNotePpq, 0.0);
                noteEndInBlock = (adjustedNoteEnd * samplePerPpq) <= (currentSamples + bufferSize);
            }
        }
    } else {
        juce::MidiMessage firstMessage;
        int firstEventTime = 0; // The sample offset (relative to buffer start)

        juce::MidiBuffer::Iterator iterator(inputBuffer);
        // This returns true if an event was found
        bool success = iterator.getNextEvent(firstMessage, firstEventTime);

        const double currentSamples = static_cast<double>(*positionInfo->getTimeInSamples());

        // Ensures the note is always reset when playback starts
        if (!positionInfo->getIsPlaying() && (nextNoteSample != 0.0)) {
            outputBuffer.addEvent(juce::MidiMessage::noteOff(1, 30), currentSamples);
            noteBeingHeld = false;
            nextNoteSample = 0.0;
        }

        if (success) {
            lastNoteNum = firstMessage.getNoteNumber();

            // Hertz represents the number of notes per second.
            const double hertz = juce::MidiMessage::getMidiNoteInHertz(lastNoteNum);
            const double samplePerHz = sampleRate / hertz;

            // The note on and off of the input note needs to be effectively "copied" to the new buffer
            // since those events should still exist in the output.
            if (firstMessage.isNoteOn()) {
                // Right now they're all playing on note 30 but this is subject to change.  I'm thinking there could be an option
                // to have it match the input note, or there could be a dial in the plugin to dynamically control which note is played.
                outputBuffer.addEvent(juce::MidiMessage::noteOn(1, 30, 1.0f), firstEventTime);
                noteBeingHeld = true;

                // curr + firstTime gets the absolute position of the first note, and the next note is determined
                // simply by adding samplePerHz since 1Hz == 1 note.
                nextNoteSample = currentSamples + firstEventTime + samplePerHz / speedScale;
            } else {
                outputBuffer.addEvent(juce::MidiMessage::noteOff(1, 30), firstEventTime);
                noteBeingHeld = false;

                nextNoteSample = 0.0;

                // If there are two notes immediately next to each other.
                juce::MidiMessage secondMessage;
                int secondEventTime = 0; // The sample offset (relative to buffer start)
                bool success2 = iterator.getNextEvent(secondMessage, secondEventTime);
                if (success2 && secondMessage.isNoteOn()) {
                    lastNoteNum = secondMessage.getNoteNumber();
                    const double hertz = juce::MidiMessage::getMidiNoteInHertz(lastNoteNum);
                    const double samplePerHz = sampleRate / hertz;
                    outputBuffer.addEvent(juce::MidiMessage::noteOn(1, 30, 1.0f), secondEventTime);
                    noteBeingHeld = true;
                    nextNoteSample = currentSamples + secondEventTime + samplePerHz / speedScale;
                }
            }
        }

        const double hertz = juce::MidiMessage::getMidiNoteInHertz(lastNoteNum);
        const double samplePerHz = sampleRate / hertz;
        double adjustedNoteEnd =
                nextNoteSample - (samplePerHz / speedScale) + ((noteLength * samplePerHz) / speedScale);
        bool nextNoteInBlock = nextNoteSample <= (currentSamples + bufferSize) && !compareFloat(nextNoteSample, 0.0);
        bool noteEndInBlock = adjustedNoteEnd <= (currentSamples + bufferSize);

        while ((noteBeingHeld && noteEndInBlock) || nextNoteInBlock) {
            if (noteBeingHeld && noteEndInBlock) {
                outputBuffer.addEvent(juce::MidiMessage::noteOff(1, 30), floor(adjustedNoteEnd - currentSamples));
                noteBeingHeld = false;
            }
            if (nextNoteInBlock) {
                // Sample position of the note relative to the start of the current block.
                const double noteOffset = nextNoteSample - currentSamples;

                // Add note off and note on at the same time to create a legato effect (continuous stream of notes).
                outputBuffer.addEvent(juce::MidiMessage::noteOff(1, 30), floor(noteOffset));
                outputBuffer.addEvent(juce::MidiMessage::noteOn(1, 30, 1.0f), floor(noteOffset));
                noteBeingHeld = true;

                nextNoteSample += samplePerHz / speedScale;

                adjustedNoteEnd =
                        nextNoteSample - (samplePerHz / speedScale) + ((noteLength * samplePerHz) / speedScale);
                nextNoteInBlock = nextNoteSample <= (currentSamples + bufferSize) && !compareFloat(nextNoteSample, 0.0);
                noteEndInBlock = adjustedNoteEnd <= (currentSamples + bufferSize);
            }
        }
    }
}
