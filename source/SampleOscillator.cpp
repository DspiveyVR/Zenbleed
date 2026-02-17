#include "SampleOscillator.h"
#include "Utilities.h"

inline float
getEtetScale(float inputSpeedScale, float etetNumerator, float etetDenominator, int lastNoteNum, int etetRootNote);

void getNextSampleBlock(
        const int writeStart,
        const int writeLength,
        double& nextReadPosition,
        const float samplePitchBendRatio,
        const int inSamplesLength,
        const int outSamplesLength,
        const float* inSamplesL,
        const float* inSamplesR,
        float* outSamplesL,
        float* outSamplesR,
        bool& currentSampleEnded);

SampleOscillator::SampleOscillator(juce::AudioProcessorValueTreeState& paramsRef) :
    formatManager(), parametersRef(paramsRef) {
    formatManager.registerBasicFormats();
};
SampleOscillator::~SampleOscillator() = default;

void SampleOscillator::loadSampleFromFile(const juce::File& file) {
    parametersRef.state.setProperty("SamplePath", file.getFullPathName(), nullptr);

    auto* reader = formatManager.createReaderFor(file);
    if (reader == nullptr) {
        return;
    }

    auto totalLength = (int)reader->lengthInSamples;
    auto numChannels = (int)reader->numChannels;

    loadedSampleBuffer = std::make_unique<juce::AudioBuffer<float>>(2, totalLength + 1);
    reader->read(loadedSampleBuffer.get(), 0, totalLength, 0, true, true);

    for (int ch = 0; ch < numChannels; ++ch)
        loadedSampleBuffer->setSample(ch, totalLength, loadedSampleBuffer->getSample(ch, 0));

    delete reader;
}

void SampleOscillator::processBlock(
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
        float etetDenominator) {
    juce::AudioBuffer<float>* sampleBuffer = loadedSampleBuffer.get();
    if (!sampleBuffer)
        return;

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
        nextNoteSample = nextQuarterNotePpq * samplePerPpq;
        processUntuned(
                inputBuffer,
                outputBuffer,
                speedScale,
                positionInfo,
                nextQuarterNotePpq,
                nextNoteSample,
                noteLength,
                samplePitchBendRatio,
                success,
                firstEventTime,
                firstMessage,
                currentSamples,
                samplePerPpq,
                bufferSize,
                iterator,
                sampleBuffer,
                bpm,
                currentPpq,
                isEtet,
                etetRootNote,
                etetNumerator,
                etetDenominator);
    } else {
        nextQuarterNotePpq = nextNoteSample / samplePerPpq;
        processTuned(
                inputBuffer,
                outputBuffer,
                speedScale,
                positionInfo,
                nextQuarterNotePpq,
                nextNoteSample,
                noteLength,
                samplePitchBendRatio,
                success,
                firstEventTime,
                firstMessage,
                currentSamples,
                samplePerPpq,
                bufferSize,
                iterator,
                sampleBuffer,
                bpm,
                currentPpq);
    }
}

void SampleOscillator::processUntuned(
        juce::MidiBuffer& inputBuffer,
        juce::AudioBuffer<float>& outputBuffer,
        const float inputSpeedScale,
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
        float etetDenominator) {
    const float* inSamplesL = sampleBuffer->getReadPointer(0);
    const float* inSamplesR = sampleBuffer->getReadPointer(1);
    const int inSamplesLength = sampleBuffer->getNumSamples();
    const int outSamplesLength = outputBuffer.getNumSamples();
    float* outSamplesL = outputBuffer.getWritePointer(0);
    float* outSamplesR = outputBuffer.getWritePointer(1);

    int writeStart = 0;
    int writeLength = 0;

    float speedScale =
            isEtet ? inputSpeedScale
                             * getEtetScale(inputSpeedScale, etetNumerator, etetDenominator, lastNoteNum, etetRootNote)
                   : inputSpeedScale;

    if (success) {
        currentSampleEnded = false;
        lastNoteNum = firstMessage.getNoteNumber();

        if (isEtet) {
            speedScale = inputSpeedScale
                         * getEtetScale(inputSpeedScale, etetNumerator, etetDenominator, lastNoteNum, etetRootNote);
        }

        if (firstMessage.isNoteOn()) {
            noteBeingHeld = true;
            writeStart = firstEventTime;
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
            writeLength = firstEventTime - writeStart;
            nextQuarterNotePpq = 0.0;
            nextReadPosition = 0;

            getNextSampleBlock(
                    writeStart,
                    writeLength,
                    nextReadPosition,
                    samplePitchBendRatio,
                    inSamplesLength,
                    outSamplesLength,
                    inSamplesL,
                    inSamplesR,
                    outSamplesL,
                    outSamplesR,
                    currentSampleEnded);

            // If there are two notes immediately next to each other.
            juce::MidiMessage secondMessage;
            int secondEventTime = 0; // The sample offset (relative to buffer start)
            bool success2 = iterator.getNextEvent(secondMessage, secondEventTime);
            lastNoteNum = secondMessage.getNoteNumber();

            if (isEtet) {
                speedScale = inputSpeedScale
                             * getEtetScale(inputSpeedScale, etetNumerator, etetDenominator, lastNoteNum, etetRootNote);
            }

            if (success2 && secondMessage.isNoteOn()) {
                noteBeingHeld = true;
                writeStart = secondEventTime;
                nextReadPosition = 0;

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
            double fragmentSamples = (adjustedNoteEnd * samplePerPpq) - currentSamples - writeStart;

            writeLength = fragmentSamples;
            getNextSampleBlock(
                    writeStart,
                    writeLength,
                    nextReadPosition,
                    samplePitchBendRatio,
                    inSamplesLength,
                    outSamplesLength,
                    inSamplesL,
                    inSamplesR,
                    outSamplesL,
                    outSamplesR,
                    currentSampleEnded);

            // Apply a micro-fade (e.g., last 44 samples is ~1ms at 44.1kHz).
            // This prevents a click at the end of the note.
            // The fade length changes based on speed since we don't want to fade the short fragments too early but we also want long
            // fragments to have a fade long enough to eliminate the click.
            // The call to min is there to ensure that the fadeLength is never longer than the fragment length (this would cause a memory error),
            // though I haven't checked if this is mathematically possible anyway.
            const int fadeLength = (int)std::min(6.4 / speedScale, fragmentSamples);
            const int fadeStart = writeStart + fragmentSamples - fadeLength;

            if (fadeStart >= 0) {
                for (int ch = 0; ch < 2; ++ch) {
                    outputBuffer.applyGainRamp(ch, fadeStart, fadeLength, 1.0f, 0.0f);
                }
            }

            noteBeingHeld = false;
        }
        if (nextNoteInBlock) {
            // Sample position of the note relative to the start of the current block.
            const double noteOffset = (nextQuarterNotePpq - currentPpq) * samplePerPpq;
            writeLength = noteOffset - writeStart - 1;

            if (!noteEndInBlock) {
                getNextSampleBlock(
                        writeStart,
                        writeLength,
                        nextReadPosition,
                        samplePitchBendRatio,
                        inSamplesLength,
                        outSamplesLength,
                        inSamplesL,
                        inSamplesR,
                        outSamplesL,
                        outSamplesR,
                        currentSampleEnded);
            }
            noteBeingHeld = true;
            currentSampleEnded = false;

            nextReadPosition = 0;
            writeStart = noteOffset;

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
        writeLength = outputBuffer.getNumSamples() - writeStart;
        getNextSampleBlock(
                writeStart,
                writeLength,
                nextReadPosition,
                samplePitchBendRatio,
                inSamplesLength,
                outSamplesLength,
                inSamplesL,
                inSamplesR,
                outSamplesL,
                outSamplesR,
                currentSampleEnded);
    }
}

inline float
getEtetScale(float inputSpeedScale, float etetNumerator, float etetDenominator, int lastNoteNum, int etetRootNote) {
    const int noteDiff = lastNoteNum - etetRootNote;
    const float interval = etetNumerator / etetDenominator;
    if (noteDiff == 0) {
        return 1.0f;
    } else {
        return std::pow(interval, noteDiff);
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
        double currentPpq) {
    const float* inSamplesL = sampleBuffer->getReadPointer(0);
    const float* inSamplesR = sampleBuffer->getReadPointer(1);
    const int inSamplesLength = sampleBuffer->getNumSamples();
    const int outSamplesLength = outputBuffer.getNumSamples();
    float* outSamplesL = outputBuffer.getWritePointer(0);
    float* outSamplesR = outputBuffer.getWritePointer(1);

    int writeStart = 0;
    int writeLength = 0;

    if (success) {
        currentSampleEnded = false;
        lastNoteNum = firstMessage.getNoteNumber();

        // Hertz represents the number of notes per second.
        const double hertz = juce::MidiMessage::getMidiNoteInHertz(lastNoteNum);
        const double samplePerHz = sampleRate / hertz;

        if (firstMessage.isNoteOn()) {
            noteBeingHeld = true;
            writeStart = firstEventTime;
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
            writeLength = firstEventTime - writeStart;
            nextReadPosition = 0;
            nextNoteSample = 0.0;

            // Play the first note
            getNextSampleBlock(
                    writeStart,
                    writeLength,
                    nextReadPosition,
                    samplePitchBendRatio,
                    inSamplesLength,
                    outSamplesLength,
                    inSamplesL,
                    inSamplesR,
                    outSamplesL,
                    outSamplesR,
                    currentSampleEnded);

            // If there are two notes immediately next to each other.
            juce::MidiMessage secondMessage;
            int secondEventTime = 0; // The sample offset (relative to buffer start)
            bool success2 = iterator.getNextEvent(secondMessage, secondEventTime);
            lastNoteNum = secondMessage.getNoteNumber();
            if (success2 && secondMessage.isNoteOn()) {
                noteBeingHeld = true;
                writeStart = secondEventTime;
                nextReadPosition = 0;
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
            double fragmentSamples = (adjustedNoteEnd - currentSamples) - writeStart;

            writeLength = fragmentSamples;
            getNextSampleBlock(
                    writeStart,
                    writeLength,
                    nextReadPosition,
                    samplePitchBendRatio,
                    inSamplesLength,
                    outSamplesLength,
                    inSamplesL,
                    inSamplesR,
                    outSamplesL,
                    outSamplesR,
                    currentSampleEnded);

            // Apply a micro-fade (e.g., last 44 samples is ~1ms at 44.1kHz).
            // This prevents a click at the end of the note.
            // The fade length changes based on speed since we don't want to fade the short fragments too early but we also want long
            // fragments to have a fade long enough to eliminate the click.
            // The call to min is there to ensure that the fadeLength is never longer than the fragment length (this would cause a memory error),
            // though I haven't checked if this is mathematically possible anyway.
            const int fadeLength = (int)std::min(6.4 / speedScale, fragmentSamples);
            const int fadeStart = writeStart + fragmentSamples - fadeLength;

            if (fadeStart >= 0) {
                for (int ch = 0; ch < 2; ++ch) {
                    outputBuffer.applyGainRamp(ch, fadeStart, fadeLength, 1.0f, 0.0f);
                }
            }

            noteBeingHeld = false;
        }
        if (nextNoteInBlock) {
            const double noteOffset = nextNoteSample - currentSamples;
            writeLength = noteOffset - writeStart - 1;

            if (!noteEndInBlock) {
                // play the remainder of the current note
                getNextSampleBlock(
                        writeStart,
                        writeLength,
                        nextReadPosition,
                        samplePitchBendRatio,
                        inSamplesLength,
                        outSamplesLength,
                        inSamplesL,
                        inSamplesR,
                        outSamplesL,
                        outSamplesR,
                        currentSampleEnded);
            }
            noteBeingHeld = true;
            currentSampleEnded = false;

            nextReadPosition = 0;
            writeStart = noteOffset;

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
        writeLength = outputBuffer.getNumSamples() - writeStart;
        // play the remainder of the current note
        getNextSampleBlock(
                writeStart,
                writeLength,
                nextReadPosition,
                samplePitchBendRatio,
                inSamplesLength,
                outSamplesLength,
                inSamplesL,
                inSamplesR,
                outSamplesL,
                outSamplesR,
                currentSampleEnded);
    }
}

void getNextSampleBlock(
        const int writeStart,
        const int writeLength,
        double& nextReadPosition,
        const float samplePitchBendRatio,
        const int inSamplesLength,
        const int outSamplesLength,
        const float* inSamplesL,
        const float* inSamplesR,
        float* outSamplesL,
        float* outSamplesR,
        bool& currentSampleEnded) {
    if (currentSampleEnded || writeStart < 0 || writeLength <= 0) {
        return;
    }
    // Handle pitch changes with linear interpolation between samples.
    for (int i = 0; i < writeLength; ++i) {
        int idxA = (int)nextReadPosition;
        float fraction = (float)(nextReadPosition - (double)idxA);
        if (idxA + 1 >= inSamplesLength) {
            currentSampleEnded = true;
            return;
        }
        if (writeStart + i >= outSamplesLength) {
            return;
        }
        outSamplesL[writeStart + i] = inSamplesL[(idxA)] + fraction * (inSamplesL[(idxA + 1)] - inSamplesL[(idxA)]);
        outSamplesR[writeStart + i] = inSamplesR[(idxA)] + fraction * (inSamplesR[(idxA + 1)] - inSamplesR[(idxA)]);
        nextReadPosition += samplePitchBendRatio;
    }
}
