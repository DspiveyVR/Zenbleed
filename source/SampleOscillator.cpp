#include "SampleOscillator.h"
#include "Utilities.h"

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
        const juce::AudioPlayHead::PositionInfo* positionInfo,
        bool isTuned,
        double& nextQuarterNotePpq,
        double& nextNoteSample,
        float noteLength) {
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

    if (!isTuned) {
        processUntuned(
                inputBuffer,
                outputBuffer,
                speedScale,
                positionInfo,
                nextQuarterNotePpq,
                nextNoteSample,
                noteLength,
                success,
                firstEventTime,
                sampleInfo,
                firstMessage,
                currentSamples,
                samplePerPpq,
                bufferSize,
                iterator,
                sampleSource,
                bpm,
                currentPpq);
    } else {
        processTuned(
                inputBuffer,
                outputBuffer,
                speedScale,
                positionInfo,
                nextQuarterNotePpq,
                nextNoteSample,
                noteLength,
                success,
                firstEventTime,
                sampleInfo,
                firstMessage,
                currentSamples,
                samplePerPpq,
                bufferSize,
                iterator,
                sampleSource,
                bpm,
                currentPpq);
    }
}

void SampleOscillator::processUntuned(
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
        double currentPpq) {
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

    double adjustedNoteEnd = nextQuarterNotePpq - (1.0 / speedScale) + (noteLength / speedScale);
    bool nextNoteInBlock = (nextQuarterNotePpq * samplePerPpq) <= (currentSamples + bufferSize)
                           && !compareFloat(nextQuarterNotePpq, 0.0);
    bool noteEndInBlock = (adjustedNoteEnd * samplePerPpq) <= (currentSamples + bufferSize);

    while ((noteBeingHeld && noteEndInBlock) || nextNoteInBlock) {
        if (noteBeingHeld && noteEndInBlock) {
            // Calculate the length of the note fragment.
            double fragmentSamples = adjustedNoteEnd - currentSamples - sampleInfo.startSample;

            sampleInfo.numSamples = fragmentSamples;
            sampleSource->getNextAudioBlock(sampleInfo);

            // Apply a micro-fade (e.g., last 44 samples is ~1ms at 44.1kHz).
            // This prevents a click at the end of the note.
            // The fade length changes based on speed since we don't want to fade the short fragments too early but we also want long
            // fragments to have a fade long enough to eliminate the click.
            // The call to min is there to ensure that the fadeLength is never longer than the fragment length (this would cause a memory error),
            // though I haven't checked if this is mathematically possible anyway.
            const int fadeLength = std::min(6.4 / speedScale, fragmentSamples);
            const int fadeStart = sampleInfo.startSample + fragmentSamples - fadeLength;

            for (int ch = 0; ch < outputBuffer.getNumChannels(); ++ch) {
                outputBuffer.applyGainRamp(ch, fadeStart, fadeLength, 1.0f, 0.0f);
            }

            noteBeingHeld = false;
        }
        if (nextNoteInBlock) {
            // Sample position of the note relative to the start of the current block.
            const double noteOffset = (nextQuarterNotePpq - currentPpq) * samplePerPpq;
            sampleInfo.numSamples = noteOffset - sampleInfo.startSample - 1;

            if (!noteEndInBlock) {
                sampleSource->getNextAudioBlock(sampleInfo); // play the remainder of the current note
            }
            noteBeingHeld = true;

            sampleSource->setNextReadPosition(0);

            sampleInfo.startSample = noteOffset;

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
    if (noteBeingHeld) {
        sampleInfo.numSamples = outputBuffer.getNumSamples() - sampleInfo.startSample;
        sampleSource->getNextAudioBlock(sampleInfo); // play the remainder of the current note
    }
}

void SampleOscillator::processTuned(
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
        double currentPpq) {
    if (success) {
        lastNoteNum = firstMessage.getNoteNumber();

        // Hertz represents the number of notes per second.
        const double hertz = juce::MidiMessage::getMidiNoteInHertz(lastNoteNum);
        const double samplePerHz = sampleRate / hertz;

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
            nextNoteSample = currentSamples + firstEventTime + samplePerHz / speedScale;
        } else {
            noteBeingHeld = false;
            sampleInfo.numSamples = firstEventTime - sampleInfo.startSample;
            sampleSource->setNextReadPosition(0);
            nextNoteSample = 0.0;
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
                const double hertz = juce::MidiMessage::getMidiNoteInHertz(lastNoteNum);
                const double samplePerHz = sampleRate / hertz;

                nextNoteSample = currentSamples + secondEventTime + samplePerHz / speedScale;
            }
        }
    }

    const double hertz = juce::MidiMessage::getMidiNoteInHertz(lastNoteNum);
    const double samplePerHz = sampleRate / hertz;
    double adjustedNoteEnd = nextNoteSample - (samplePerHz / speedScale) + ((noteLength * samplePerHz) / speedScale);
    bool nextNoteInBlock = nextNoteSample <= (currentSamples + bufferSize) && (nextNoteSample != 0.0);
    bool noteEndInBlock = adjustedNoteEnd <= (currentSamples + bufferSize);

    while ((noteBeingHeld && noteEndInBlock) || nextNoteInBlock) {
        if (noteBeingHeld && noteEndInBlock) {
            // Calculate the length of the note fragment.
            double fragmentSamples = (adjustedNoteEnd - currentSamples) - sampleInfo.startSample;

            sampleInfo.numSamples = fragmentSamples;
            sampleSource->getNextAudioBlock(sampleInfo);

            // Apply a micro-fade (e.g., last 44 samples is ~1ms at 44.1kHz).
            // This prevents a click at the end of the note.
            // The fade length changes based on speed since we don't want to fade the short fragments too early but we also want long
            // fragments to have a fade long enough to eliminate the click.
            // The call to min is there to ensure that the fadeLength is never longer than the fragment length (this would cause a memory error),
            // though I haven't checked if this is mathematically possible anyway.
            const int fadeLength = std::min(6.4 / speedScale, fragmentSamples);
            const int fadeStart = sampleInfo.startSample + fragmentSamples - fadeLength;

            for (int ch = 0; ch < outputBuffer.getNumChannels(); ++ch) {
                outputBuffer.applyGainRamp(ch, fadeStart, fadeLength, 1.0f, 0.0f);
            }

            noteBeingHeld = false;
        }
        if (nextNoteInBlock) {
            const double noteOffset = nextNoteSample - currentSamples;
            sampleInfo.numSamples = noteOffset - sampleInfo.startSample - 1;

            if (!noteEndInBlock) {
                sampleSource->getNextAudioBlock(sampleInfo); // play the remainder of the current note
            }
            noteBeingHeld = true;

            sampleSource->setNextReadPosition(0);
            sampleInfo.startSample = noteOffset;

            // A higher speed means shorter quarter notes, so 1 / speedScale represents the length of a quarter
            // note relative to the baseline.
            // E.g. A speedScale of 2.0 results in a quarter note half the length of the baseline.
            nextNoteSample += samplePerHz / speedScale;

            adjustedNoteEnd = nextNoteSample - (samplePerHz / speedScale) + ((noteLength * samplePerHz) / speedScale);
            nextNoteInBlock = nextNoteSample <= (currentSamples + bufferSize) && (nextNoteSample != 0.0);
            noteEndInBlock = adjustedNoteEnd <= (currentSamples + bufferSize);
        }
    }

    if (noteBeingHeld) {
        sampleInfo.numSamples = outputBuffer.getNumSamples() - sampleInfo.startSample;
        sampleSource->getNextAudioBlock(sampleInfo); // play the remainder of the current note
    }
}
