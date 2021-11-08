/*  ===========================================================================
*
*   This file is part of HISE.
*   Copyright 2016 Christoph Hart
*
*   HISE is free software: you can redistribute it and/or modify
*   it under the terms of the GNU General Public License as published by
*   the Free Software Foundation, either version 3 of the License, or
*   (at your option) any later version.
*
*   HISE is distributed in the hope that it will be useful,
*   but WITHOUT ANY WARRANTY; without even the implied warranty of
*   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
*   GNU General Public License for more details.
*
*   You should have received a copy of the GNU General Public License
*   along with HISE.  If not, see <http://www.gnu.org/licenses/>.
*
*   Commercial licenses for using HISE in an closed source project are
*   available on request. Please visit the project's website to get more
*   information about commercial licensing:
*
*   http://www.hise.audio/
*
*   HISE is based on the JUCE library,
*   which must be separately licensed for closed source applications:
*
*   http://www.juce.com
*
*   ===========================================================================
*/

namespace hise { using namespace juce;

SET_DOCUMENTATION(AudioLooper)
{
	SET_DOC_NAME(AudioLooper);

	ADD_PARAMETER_DOC_WITH_NAME(SyncMode, "Sync Mode", "Syncs the looper to the host tempo");
	ADD_PARAMETER_DOC_WITH_NAME(LoopEnabled, "Loop Enabled", "Enables looped playback");
	ADD_PARAMETER_DOC_WITH_NAME(PitchTracking, "Pitch Tracking", "Repitches the sample based on the note and the root note.");
	ADD_PARAMETER_DOC_WITH_NAME(RootNote, "Root Note", "Sets the root note when pitch tracking is enabled");
	ADD_PARAMETER_DOC_WITH_NAME(SampleStartMod, "Sample Start modulation", "Modulates the sample start");
	ADD_PARAMETER_DOC_WITH_NAME(Reversed, "Reversed", "Reverses the sample");
}

AudioLooperVoice::AudioLooperVoice(ModulatorSynth *ownerSynth) :
ModulatorSynthVoice(ownerSynth),
syncFactor(1.0f)
{
}

void AudioLooperVoice::startNote(int midiNoteNumber, float /*velocity*/, SynthesiserSound*, int /*currentPitchWheelPosition*/)
{
	ModulatorSynthVoice::startNote(midiNoteNumber, 0.0f, nullptr, -1);

	midiNoteNumber += getTransposeAmount();

	voiceUptime = (double)getCurrentHiseEvent().getStartOffset();

	auto maxStartModMs = (double)getOwnerSynth()->getAttribute(AudioLooper::SpecialParameters::SampleStartMod);
	auto maxStartModSamples = maxStartModMs / 1000.0 * getSampleRate();
	auto randomized = (double)r.nextFloat() * maxStartModSamples;

	voiceUptime += randomized;

	AudioLooper *looper = static_cast<AudioLooper*>(getOwnerSynth());

	SimpleReadWriteLock::ScopedReadLock sl(looper->getBuffer().getDataLock());

	uptimeDelta = looper->getBuffer().isNotEmpty() ? 1.0 : 0.0;

	const double resampleFactor = looper->getSampleRateForLoadedFile() / getSampleRate();

	uptimeDelta *= resampleFactor;
    uptimeDelta *= looper->getMainController()->getGlobalPitchFactor();
    
	if (looper->pitchTrackingEnabled)
	{
		const double noteDelta = midiNoteNumber - looper->rootNote;
        const double pitchDelta = pow(2, noteDelta / 12.0);
        uptimeDelta *= pitchDelta;
	}
}

int getSamplePos(int uptime, int loopLength, int loopOffset, bool reversed, int totalLength)
{
	if (reversed)
	{
		if (uptime > loopLength)
			return totalLength - uptime % loopLength;
		else
			return totalLength - uptime;
	}
	else
	{
		if (uptime < loopOffset)
			return uptime;
		else
			return ((uptime - loopOffset) % loopLength) + loopOffset;
	}
}

void AudioLooperVoice::calculateBlock(int startSample, int numSamples)
{
	const int startIndex = startSample;
	const int samplesToCopy = numSamples;

	const float *voicePitchValues = getOwnerSynth()->getPitchValuesForVoice();

	AudioLooper *looper = static_cast<AudioLooper*>(getOwnerSynth());

	SimpleReadWriteLock::ScopedReadLock sl(looper->getBuffer().getDataLock());
	auto sampleRange = looper->getBuffer().getCurrentRange();

	auto buffer = &looper->getAudioSampleBuffer();
	auto length = sampleRange.getLength();

	const bool noBuffer = buffer->getNumChannels() == 0;
	const bool sampleFinished = !looper->isUsingLoop() && (voiceUptime > length);
	
	const bool isLastVoice = getOwnerSynth()->isLastStartedVoice(this);
	const bool isReversed = looper->reversed;

	if (sampleFinished || noBuffer)
	{
		voiceBuffer.clear(startSample, numSamples);
		resetVoice();
		return;
	}
    
	int offset = sampleRange.getStart();

	const float *leftSamples = buffer->getReadPointer(0, 0);
	const float *rightSamples = buffer->getNumChannels() > 1 ? buffer->getReadPointer(1, 0) : leftSamples;

	auto loopRange = looper->getBuffer().getLoopRange();

	int loopStart = jmax<int>(offset, loopRange.getStart());
	int loopEnd = jmin<int>(loopRange.getEnd(), sampleRange.getEnd());

	length = looper->isUsingLoop() ? loopEnd - loopStart : length;

	auto end = sampleRange.getLength()-1;

	auto loopOffset = jmax<int>(0, loopStart - offset);

	bool resetAfterBlock = false;
	bool checkReset = !looper->isUsingLoop();

	while (--numSamples >= 0)
	{
		int uptime = (int)voiceUptime;

		const int samplePos = getSamplePos(uptime, length, loopOffset, isReversed, end);
		const int nextSamplePos = getSamplePos(uptime+1, length, loopOffset, isReversed, end);
		
		//const int samplePos = (int)voiceUptime % looper->length + looper->sampleRange.getStart();
		//const int nextSamplePos = ((int)voiceUptime + 1) % looper->length + looper->sampleRange.getStart();

        if(checkReset && (uptime+2) > length)
        {
			voiceBuffer.clear(startSample, numSamples+1);

			resetAfterBlock = true;
			break;
        }
        
		const double alpha = fmod(voiceUptime, 1.0);

		const float leftPrevSample = leftSamples[samplePos];
		const float rightPrevSample = rightSamples[samplePos];

		const float leftNextSample = leftSamples[nextSamplePos];
		const float rightNextSample = rightSamples[nextSamplePos];

		const float leftSample = Interpolator::interpolateLinear(leftPrevSample, leftNextSample, (float)alpha);
		const float rightSample = Interpolator::interpolateLinear(rightPrevSample, rightNextSample, (float)alpha);

		//const float currentSample = invAlpha * v1 + alpha * v2;

		// Stereo mode assumed
		voiceBuffer.setSample(0, startSample, leftSample);
		voiceBuffer.setSample(1, startSample, rightSample);

		jassert(voicePitchValues == nullptr || voicePitchValues[startSample] > 0.0f);

		const double pitchDelta = (uptimeDelta * syncFactor * (voicePitchValues == nullptr ? 1.0f : voicePitchValues[startSample]));

		
		voiceUptime += pitchDelta;

		++startSample;
	}

	getOwnerSynth()->effectChain->renderVoice(voiceIndex, voiceBuffer, startIndex, samplesToCopy);

	if (auto modValues = getOwnerSynth()->getVoiceGainValues())
	{
		FloatVectorOperations::multiply(voiceBuffer.getWritePointer(0, startIndex), modValues + startIndex, samplesToCopy);
		FloatVectorOperations::multiply(voiceBuffer.getWritePointer(1, startIndex), modValues + startIndex, samplesToCopy);
	}
	else
	{
		const float constantGainValue = getOwnerSynth()->getConstantGainModValue();

		FloatVectorOperations::multiply(voiceBuffer.getWritePointer(0, startIndex), constantGainValue, samplesToCopy);
		FloatVectorOperations::multiply(voiceBuffer.getWritePointer(1, startIndex), constantGainValue, samplesToCopy);
	}

	if (isLastVoice && length != 0)
	{
		const int samplePos = getSamplePos((int)voiceUptime, length, loopOffset, isReversed, length);

		looper->getBuffer().sendDisplayIndexMessage((float)samplePos);
	}

	if (resetAfterBlock)
		resetVoice();
}

void AudioLooperVoice::resetVoice()
{
	if (getOwnerSynth()->isLastStartedVoice(this))
		static_cast<AudioLooper*>(getOwnerSynth())->setInputValue(-1.0f);

	ModulatorSynthVoice::resetVoice();
}

AudioLooper::AudioLooper(MainController *mc, const String &id, int numVoices) :
ModulatorSynth(mc, id, numVoices),
AudioSampleProcessor(mc),
syncMode(AudioSampleProcessor::SyncToHostMode::FreeRunning),
pitchTrackingEnabled(false),
rootNote(64)
{
	getBuffer().addListener(this);
	finaliseModChains();

	parameterNames.add("SyncMode");
	parameterNames.add("LoopEnabled");
	parameterNames.add("PitchTracking");
	parameterNames.add("RootNote");
	parameterNames.add("SampleStartMod");
	parameterNames.add("Reversed");

	inputMerger.setManualCountLimit(5);

	for (int i = 0; i < numVoices; i++)
	{
		addVoice(new AudioLooperVoice(this));

	}

	addSound(new AudioLooperSound());
}

void AudioLooper::restoreFromValueTree(const ValueTree &v)
{
	ModulatorSynth::restoreFromValueTree(v);
	AudioSampleProcessor::restoreFromValueTree(v);

	loadAttribute(SyncMode, "SyncMode");
	loadAttribute(PitchTracking, "PitchTracking");
	loadAttribute(LoopEnabled, "LoopEnabled");
	loadAttribute(RootNote, "RootNote");
	loadAttribute(SampleStartMod, "SampleStartMod");
	loadAttribute(Reversed, "Reversed");
}

ValueTree AudioLooper::exportAsValueTree() const
{
	ValueTree v = ModulatorSynth::exportAsValueTree();

	saveAttribute(SyncMode, "SyncMode");
	saveAttribute(PitchTracking, "PitchTracking");
	saveAttribute(LoopEnabled, "LoopEnabled");
	saveAttribute(RootNote, "RootNote");
	saveAttribute(SampleStartMod, "SampleStartMod");
	saveAttribute(Reversed, "Reversed");

	AudioSampleProcessor::saveToValueTree(v);

	return v;
}

float AudioLooper::getAttribute(int parameterIndex) const
{
	if (parameterIndex < ModulatorSynth::numModulatorSynthParameters) return ModulatorSynth::getAttribute(parameterIndex);

	switch (parameterIndex)
	{
	case SyncMode:		return (float)(int)syncMode;
	case LoopEnabled:	return isUsingLoop() ? 1.0f : 0.0f;
	case RootNote:		return (float)rootNote;
	case PitchTracking:	return pitchTrackingEnabled ? 1.0f : 0.0f;
	case SampleStartMod: return (float)sampleStartMod;
	case Reversed:		return reversed ? 1.0f : 0.0f;
	default:					jassertfalse; return -1.0f;
	}
}

float AudioLooper::getDefaultValue(int parameterIndex) const
{
	if (parameterIndex < ModulatorSynth::numModulatorSynthParameters) return ModulatorSynth::getDefaultValue(parameterIndex);

	switch (parameterIndex)
	{
	case SyncMode:		return (float)(int)0;
	case LoopEnabled:	return 1.0f;
	case RootNote:		return 64.0f;
	case PitchTracking:	return 0.0f;
	case SampleStartMod: return 0.0f;
	case Reversed:		return 0.0f;
	default: jassertfalse; return -1.0f;
	}
}

void AudioLooper::setInternalAttribute(int parameterIndex, float newValue)
{
	if (parameterIndex < ModulatorSynth::numModulatorSynthParameters)
	{
		ModulatorSynth::setInternalAttribute(parameterIndex, newValue);
		return;
	}

	switch (parameterIndex)
	{
	case SyncMode:		setSyncMode((int)newValue); break;
	case LoopEnabled:	setUseLoop(newValue > 0.5f);
						break;
	case RootNote:		rootNote = (int)newValue; break;
	case PitchTracking:	pitchTrackingEnabled = newValue > 0.5f; break;
	case SampleStartMod: sampleStartMod = jmax<int>(0, (int)newValue); break;
	case Reversed:		reversed = newValue > 0.5f; break;
	default:			jassertfalse; break;
	}
}

void AudioLooper::bufferWasLoaded()
{
	if (!pitchTrackingEnabled)
		return;

	AudioSampleBuffer copy;
	double sr = 0.0;

	{
		SimpleReadWriteLock::ScopedReadLock sl(getBuffer().getDataLock());
		auto& b = getAudioSampleBuffer();
		sr = getSampleRate();
		copy.makeCopyOf(b);
	}

	if (copy.getNumSamples() > 0)
	{
		auto freq = PitchDetection::detectPitch(copy, 0, copy.getNumSamples(), sr);
		
		if (freq == 0.0)
			return;

		Array<Range<double>> freqRanges;

		freqRanges.add(Range<double>(0, MidiMessage::getMidiNoteInHertz(1) / 2));

		for (int i = 1; i < 126; i++)
		{
			const double thisPitch = MidiMessage::getMidiNoteInHertz(i);
			const double nextPitch = MidiMessage::getMidiNoteInHertz(i + 1);
			const double prevPitch = MidiMessage::getMidiNoteInHertz(i - 1);

			const double lowerLimit = thisPitch - (thisPitch - prevPitch) * 0.5;
			const double upperLimit = thisPitch + (nextPitch - thisPitch) * 0.5;

			freqRanges.add(Range<double>(lowerLimit, upperLimit));
		}

		for (int j = 0; j < freqRanges.size(); j++)
		{
			if (freqRanges[j].contains(freq))
			{
				setAttribute(AudioLooper::SpecialParameters::RootNote, (float)(j), sendNotification);
				return;
			}
		}
	}

	
	
}

void AudioLooper::bufferWasModified()
{
	
}

ProcessorEditorBody* AudioLooper::createEditor(ProcessorEditor *parentEditor)
{
#if USE_BACKEND
	return new AudioLooperEditor(parentEditor);
#else

	ignoreUnused(parentEditor);
	jassertfalse;
	return nullptr;
#endif
}

void AudioLooper::setSyncMode(int newSyncMode)
{
	SimpleReadWriteLock::ScopedReadLock sl(getBuffer().getDataLock());

	syncMode = (SyncToHostMode)newSyncMode;

	const double globalBpm = getMainController()->getBpm();

	const bool tempoOK = globalBpm > 0.0 && globalBpm < 1000.0;
	const bool sampleRateOK = getSampleRate() != -1.0;
	const bool lengthOK = getBuffer().isNotEmpty();

	if (!tempoOK || !sampleRateOK || !lengthOK)
	{
		dynamic_cast<AudioLooperVoice*>(getVoice(0))->syncFactor = 1.0;
		return;
	}

	int multiplier = 1;

	switch (syncMode)
	{
	case SyncToHostMode::FreeRunning:		dynamic_cast<AudioLooperVoice*>(getVoice(0))->syncFactor = 1.0; return;
	case SyncToHostMode::OneBeat:			multiplier = 1; break;
	case SyncToHostMode::TwoBeats:			multiplier = 2; break;
	case SyncToHostMode::OneBar:			multiplier = 4; break;
	case SyncToHostMode::TwoBars:			multiplier = 8; break;
	case SyncToHostMode::FourBars:			multiplier = 16; break;
	default:								jassertfalse; multiplier = 1;
	}

	const int lengthForOneBeat = TempoSyncer::getTempoInSamples(getMainController()->getBpm(), getSampleRate(), TempoSyncer::Quarter);

	if (lengthForOneBeat == 0)
	{
		dynamic_cast<AudioLooperVoice*>(getVoice(0))->syncFactor = 1.0;
	}
	else
	{
		dynamic_cast<AudioLooperVoice*>(getVoice(0))->syncFactor = (float)getBuffer().getCurrentRange().getLength() / (float)(lengthForOneBeat * multiplier);
	}
}

} // namespace hise
