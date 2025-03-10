// Copyright Epic Games, Inc. All Rights Reserved.

#include "CoreMinimal.h"
#include "MetasoundExecutableOperator.h"
#include "MetasoundFacade.h"
#include "MetasoundNodeInterface.h"
#include "MetasoundParamHelper.h"
#include "MetasoundSampleCounter.h"
#include "MetasoundStandardNodesCategories.h"
#include "MetasoundNodeInterface.h"
#include "MetasoundVertex.h"

#include "HarmonixMetasound/DataTypes/MidiStream.h"
#include "HarmonixMetasound/DataTypes/MusicTransport.h"
#include "HarmonixDsp/AudioUtility.h"
#include "HarmonixMetasound/Common.h"

#include "HarmonixMetasound/MidiOps/StuckNoteGuard.h"
#include "DSP/Chorus.h"
#include "MetasoundEpicSynth1Types.h"
#include "MetasoundEpicSynth1.h"
#include "DSP/Envelope.h"
#include "DSP/Amp.h"
#include "DSP/DelayStereo.h"
#include "DSP/Chorus.h"	
#include "Engine/DataTable.h"
#include "Components/SynthComponent.h"
#include "HarmonixDsp/Ramper.h"
#include "MetasoundEnumRegistrationMacro.h"
#include "MetasoundEnum.h"
#include "Containers/Array.h"
#include "Containers/ArrayView.h"
#include "IAudioProxyInitializer.h"
#include "MetasoundDataReference.h"

#include <vector>



DEFINE_LOG_CATEGORY_STATIC(LogEpicSynth1Node, VeryVerbose, All);

#define LOCTEXT_NAMESPACE "EpicSynthsMetasounds_Epic1SynthNode"

DECLARE_METASOUND_ENUM(EMetasoundSynth1OscType, EMetasoundSynth1OscType::Sine, EPICSYNTHSMETASOUNDS_API, FEnumEpicsynth1Osc, FEnumEpicsynth1OscTypeInfo, FEnumEpicsynth1OscReadRef, FEnumEpicsynth1OscWriteRef);

DEFINE_METASOUND_ENUM_BEGIN(EMetasoundSynth1OscType, FEnumEpicsynth1Osc, "Oscillator Type")
DEFINE_METASOUND_ENUM_ENTRY(EMetasoundSynth1OscType::Sine, "SineDescription", "Sine", "SineDescriptionTT", "Sine tooltip"),
DEFINE_METASOUND_ENUM_ENTRY(EMetasoundSynth1OscType::Saw, "SawDescription", "Saw", "SawDescriptionTT", "Saw tooltip"),
DEFINE_METASOUND_ENUM_ENTRY(EMetasoundSynth1OscType::Triangle, "TriangleDescription", "Triangle", "TriangleDescriptionTT", "Triangle tooltip"),
DEFINE_METASOUND_ENUM_ENTRY(EMetasoundSynth1OscType::Square, "SquareDescription", "Square", "SquareDescriptionTT", "Square tooltip"),
DEFINE_METASOUND_ENUM_ENTRY(EMetasoundSynth1OscType::Noise, "NoiseDescription", "Noise", "NoiseDescriptionTT", "Noise tooltip"),
DEFINE_METASOUND_ENUM_END()

DECLARE_METASOUND_ENUM(EMetasoundSynthFilterType, EMetasoundSynthFilterType::LowPass, EPICSYNTHSMETASOUNDS_API, FEnumMetasoundSynthFilterType, FEnumMetasoundSynthFilterTypeInfo, FEnumMetasoundSynthFilterTypeReadRef, FEnumMetasoundSynthFilterTypeWriteRef);


DEFINE_METASOUND_ENUM_BEGIN(EMetasoundSynthFilterType, FEnumMetasoundSynthFilterType, "Filter Type")
DEFINE_METASOUND_ENUM_ENTRY(EMetasoundSynthFilterType::LowPass, "LowPassDescription", "Low Pass", "LowPassDescriptionTT", "Low Pass Filter"),
DEFINE_METASOUND_ENUM_ENTRY(EMetasoundSynthFilterType::HighPass, "HighPassDescription", "High Pass", "HighPassDescriptionTT", "High Pass Filter"),
DEFINE_METASOUND_ENUM_ENTRY(EMetasoundSynthFilterType::BandPass, "BandPassDescription", "Band Pass", "BandPassDescriptionTT", "Band Pass Filter"),
DEFINE_METASOUND_ENUM_ENTRY(EMetasoundSynthFilterType::BandStop, "BandStopDescription", "Band Stop", "BandStopDescriptionTT", "Band Stop Filter"),
DEFINE_METASOUND_ENUM_END()

DECLARE_METASOUND_ENUM(EMetasoundSynthLFOType, EMetasoundSynthLFOType::Sine, EPICSYNTHSMETASOUNDS_API, FEnumMetasoundSynthLFOType, FEnumMetasoundSynthLFOTypeInfo, FEnumMetasoundSynthLFOTypeReadRef, FEnumMetasoundSynthLFOTypeWriteRef);

DEFINE_METASOUND_ENUM_BEGIN(EMetasoundSynthLFOType, FEnumMetasoundSynthLFOType, "LFO Type")
DEFINE_METASOUND_ENUM_ENTRY(EMetasoundSynthLFOType::Sine, "SineDescription", "Sine", "SineDescriptionTT", "Sine waveform"),
DEFINE_METASOUND_ENUM_ENTRY(EMetasoundSynthLFOType::UpSaw, "UpSawDescription", "Up Saw", "UpSawDescriptionTT", "Up Saw waveform"),
DEFINE_METASOUND_ENUM_ENTRY(EMetasoundSynthLFOType::DownSaw, "DownSawDescription", "Down Saw", "DownSawDescriptionTT", "Down Saw waveform"),
DEFINE_METASOUND_ENUM_ENTRY(EMetasoundSynthLFOType::Square, "SquareDescription", "Square", "SquareDescriptionTT", "Square waveform"),
DEFINE_METASOUND_ENUM_ENTRY(EMetasoundSynthLFOType::Triangle, "TriangleDescription", "Triangle", "TriangleDescriptionTT", "Triangle waveform"),
DEFINE_METASOUND_ENUM_ENTRY(EMetasoundSynthLFOType::Exponential, "ExponentialDescription", "Exponential", "ExponentialDescriptionTT", "Exponential waveform"),
DEFINE_METASOUND_ENUM_ENTRY(EMetasoundSynthLFOType::RandomSampleHold, "RandomSampleHoldDescription", "Random Sample & Hold", "RandomSampleHoldDescriptionTT", "Random Sample & Hold waveform"),
DEFINE_METASOUND_ENUM_END()

DECLARE_METASOUND_ENUM(EMetasoundSynthLFOMode, EMetasoundSynthLFOMode::Sync, EPICSYNTHSMETASOUNDS_API, FEnumMetasoundSynthLFOMode, FEnumMetasoundSynthLFOModeInfo, FEnumMetasoundSynthLFOModeReadRef, FEnumMetasoundSynthLFOModeWriteRef);

DEFINE_METASOUND_ENUM_BEGIN(EMetasoundSynthLFOMode, FEnumMetasoundSynthLFOMode, "LFO Mode")
DEFINE_METASOUND_ENUM_ENTRY(EMetasoundSynthLFOMode::Sync, "SyncDescription", "Sync", "SyncDescriptionTT", "LFO syncs with note on"),
DEFINE_METASOUND_ENUM_ENTRY(EMetasoundSynthLFOMode::OneShot, "OneShotDescription", "One Shot", "OneShotDescriptionTT", "LFO performs one cycle"),
DEFINE_METASOUND_ENUM_ENTRY(EMetasoundSynthLFOMode::Free, "FreeDescription", "Free", "FreeDescriptionTT", "LFO runs continuously"),
DEFINE_METASOUND_ENUM_END()

DECLARE_METASOUND_ENUM(EMetasoundSynthLFOPatchType, EMetasoundSynthLFOPatchType::PatchToNone, EPICSYNTHSMETASOUNDS_API, FEnumMetasoundSynthLFOPatchType, FEnumMetasoundSynthLFOPatchTypeInfo, FEnumMetasoundSynthLFOPatchTypeReadRef, FEnumMetasoundSynthLFOPatchTypeWriteRef);

DEFINE_METASOUND_ENUM_BEGIN(EMetasoundSynthLFOPatchType, FEnumMetasoundSynthLFOPatchType, "LFO Patch Type")
DEFINE_METASOUND_ENUM_ENTRY(EMetasoundSynthLFOPatchType::PatchToNone, "PatchToNoneDescription", "Patch To None", "PatchToNoneDescriptionTT", "No LFO patch"),
DEFINE_METASOUND_ENUM_ENTRY(EMetasoundSynthLFOPatchType::PatchToGain, "PatchToGainDescription", "Patch To Gain", "PatchToGainDescriptionTT", "Patch LFO to gain"),
DEFINE_METASOUND_ENUM_ENTRY(EMetasoundSynthLFOPatchType::PatchToOscFreq, "PatchToOscFreqDescription", "Patch To Oscillator Freq", "PatchToOscFreqDescriptionTT", "Patch LFO to oscillator frequency"),
DEFINE_METASOUND_ENUM_ENTRY(EMetasoundSynthLFOPatchType::PatchToFilterFreq, "PatchToFilterFreqDescription", "Patch To Filter Freq", "PatchToFilterFreqDescriptionTT", "Patch LFO to filter frequency"),
DEFINE_METASOUND_ENUM_ENTRY(EMetasoundSynthLFOPatchType::PatchToFilterQ, "PatchToFilterQDescription", "Patch To Filter Q", "PatchToFilterQDescriptionTT", "Patch LFO to filter Q"),
DEFINE_METASOUND_ENUM_ENTRY(EMetasoundSynthLFOPatchType::PatchToOscPulseWidth, "PatchToOscPulseWidthDescription", "Patch To Oscillator Pulse Width", "PatchToOscPulseWidthDescriptionTT", "Patch LFO to oscillator pulse width"),
DEFINE_METASOUND_ENUM_ENTRY(EMetasoundSynthLFOPatchType::PatchToOscPan, "PatchToOscPanDescription", "Patch To Oscillator Pan", "PatchToOscPanDescriptionTT", "Patch LFO to oscillator pan"),
DEFINE_METASOUND_ENUM_ENTRY(EMetasoundSynthLFOPatchType::PatchLFO1ToLFO2Frequency, "PatchLFO1ToLFO2FrequencyDescription", "Patch LFO1 To LFO2 Frequency", "PatchLFO1ToLFO2FrequencyDescriptionTT", "Patch LFO1 to LFO2 frequency"),
DEFINE_METASOUND_ENUM_ENTRY(EMetasoundSynthLFOPatchType::PatchLFO1ToLFO2Gain, "PatchLFO1ToLFO2GainDescription", "Patch LFO1 To LFO2 Gain", "PatchLFO1ToLFO2GainDescriptionTT", "Patch LFO1 to LFO2 gain"),
DEFINE_METASOUND_ENUM_END()

DECLARE_METASOUND_ENUM(EMetasoundSynthFilterAlgorithm, EMetasoundSynthFilterAlgorithm::OnePole, EPICSYNTHSMETASOUNDS_API, FEnumMetasoundSynthFilterAlgorithm, FEnumMetasoundSynthFilterAlgorithmInfo, FEnumMetasoundSynthFilterAlgorithmReadRef, FEnumMetasoundSynthFilterAlgorithmWriteRef);

DEFINE_METASOUND_ENUM_BEGIN(EMetasoundSynthFilterAlgorithm, FEnumMetasoundSynthFilterAlgorithm, "Filter Algorithm")
DEFINE_METASOUND_ENUM_ENTRY(EMetasoundSynthFilterAlgorithm::OnePole, "OnePoleDescription", "One Pole", "OnePoleDescriptionTT", "One Pole filter"),
DEFINE_METASOUND_ENUM_ENTRY(EMetasoundSynthFilterAlgorithm::StateVariable, "StateVariableDescription", "State Variable", "StateVariableDescriptionTT", "State Variable filter"),
DEFINE_METASOUND_ENUM_ENTRY(EMetasoundSynthFilterAlgorithm::Ladder, "LadderDescription", "Ladder", "LadderDescriptionTT", "Ladder filter"),
DEFINE_METASOUND_ENUM_END()

DECLARE_METASOUND_ENUM(EMetasoundSynthStereoDelayMode, EMetasoundSynthStereoDelayMode::Normal, EPICSYNTHSMETASOUNDS_API, FEnumMetasoundSynthStereoDelayMode, FEnumMetasoundSynthStereoDelayModeInfo, FEnumMetasoundSynthStereoDelayModeReadRef, FEnumMetasoundSynthStereoDelayModeWriteRef);

DEFINE_METASOUND_ENUM_BEGIN(EMetasoundSynthStereoDelayMode, FEnumMetasoundSynthStereoDelayMode, "Stereo Delay Mode")
DEFINE_METASOUND_ENUM_ENTRY(EMetasoundSynthStereoDelayMode::Normal, "NormalDescription", "Normal", "NormalDescriptionTT", "Normal stereo delay"),
DEFINE_METASOUND_ENUM_ENTRY(EMetasoundSynthStereoDelayMode::Cross, "CrossDescription", "Cross", "CrossDescriptionTT", "Cross stereo delay"),
DEFINE_METASOUND_ENUM_ENTRY(EMetasoundSynthStereoDelayMode::PingPong, "PingPongDescription", "Ping Pong", "PingPongDescriptionTT", "Ping pong stereo delay"),
DEFINE_METASOUND_ENUM_END()

DECLARE_METASOUND_ENUM(EMetasoundSynth1PatchSource, EMetasoundSynth1PatchSource::LFO1, EPICSYNTHSMETASOUNDS_API, FEnumMetasoundSynth1PatchSource, FEnumMetasoundSynth1PatchSourceInfo, FEnumMetasoundSynth1PatchSourceReadRef, FEnumMetasoundSynth1PatchSourceWriteRef);

DEFINE_METASOUND_ENUM_BEGIN(EMetasoundSynth1PatchSource, FEnumMetasoundSynth1PatchSource, "Patch Source")
DEFINE_METASOUND_ENUM_ENTRY(EMetasoundSynth1PatchSource::LFO1, "LFO1Description", "LFO 1", "LFO1DescriptionTT", "LFO 1 as patch source"),
DEFINE_METASOUND_ENUM_ENTRY(EMetasoundSynth1PatchSource::LFO2, "LFO2Description", "LFO 2", "LFO2DescriptionTT", "LFO 2 as patch source"),
DEFINE_METASOUND_ENUM_ENTRY(EMetasoundSynth1PatchSource::Envelope, "EnvelopeDescription", "Envelope", "EnvelopeDescriptionTT", "Envelope as patch source"),
DEFINE_METASOUND_ENUM_ENTRY(EMetasoundSynth1PatchSource::BiasEnvelope, "BiasEnvelopeDescription", "Bias Envelope", "BiasEnvelopeDescriptionTT", "Bias Envelope as patch source"),
DEFINE_METASOUND_ENUM_END()

DECLARE_METASOUND_ENUM(EMetasoundSynth1PatchDestination, EMetasoundSynth1PatchDestination::Osc1Gain, EPICSYNTHSMETASOUNDS_API, FEnumMetasoundSynth1PatchDestination, FEnumMetasoundSynth1PatchDestinationInfo, FEnumMetasoundSynth1PatchDestinationReadRef, FEnumMetasoundSynth1PatchDestinationWriteRef);

DEFINE_METASOUND_ENUM_BEGIN(EMetasoundSynth1PatchDestination, FEnumMetasoundSynth1PatchDestination, "Patch Destination")
DEFINE_METASOUND_ENUM_ENTRY(EMetasoundSynth1PatchDestination::Osc1Gain, "Osc1GainDescription", "Osc1 Gain", "Osc1GainDescriptionTT", "Oscillator 1 Gain"),
DEFINE_METASOUND_ENUM_ENTRY(EMetasoundSynth1PatchDestination::Osc1Frequency, "Osc1FrequencyDescription", "Osc1 Frequency", "Osc1FrequencyDescriptionTT", "Oscillator 1 Frequency"),
DEFINE_METASOUND_ENUM_ENTRY(EMetasoundSynth1PatchDestination::Osc1Pulsewidth, "Osc1PulsewidthDescription", "Osc1 Pulsewidth", "Osc1PulsewidthDescriptionTT", "Oscillator 1 Pulsewidth"),
DEFINE_METASOUND_ENUM_ENTRY(EMetasoundSynth1PatchDestination::Osc2Gain, "Osc2GainDescription", "Osc2 Gain", "Osc2GainDescriptionTT", "Oscillator 2 Gain"),
DEFINE_METASOUND_ENUM_ENTRY(EMetasoundSynth1PatchDestination::Osc2Frequency, "Osc2FrequencyDescription", "Osc2 Frequency", "Osc2FrequencyDescriptionTT", "Oscillator 2 Frequency"),
DEFINE_METASOUND_ENUM_ENTRY(EMetasoundSynth1PatchDestination::Osc2Pulsewidth, "Osc2PulsewidthDescription", "Osc2 Pulsewidth", "Osc2PulsewidthDescriptionTT", "Oscillator 2 Pulsewidth"),
DEFINE_METASOUND_ENUM_ENTRY(EMetasoundSynth1PatchDestination::FilterFrequency, "FilterFrequencyDescription", "Filter Frequency", "FilterFrequencyDescriptionTT", "Filter Frequency"),
DEFINE_METASOUND_ENUM_ENTRY(EMetasoundSynth1PatchDestination::FilterQ, "FilterQDescription", "Filter Q", "FilterQDescriptionTT", "Filter Q"),
DEFINE_METASOUND_ENUM_ENTRY(EMetasoundSynth1PatchDestination::Gain, "GainDescription", "Gain", "GainDescriptionTT", "Gain"),
DEFINE_METASOUND_ENUM_ENTRY(EMetasoundSynth1PatchDestination::Pan, "PanDescription", "Pan", "PanDescriptionTT", "Pan"),
DEFINE_METASOUND_ENUM_ENTRY(EMetasoundSynth1PatchDestination::LFO1Frequency, "LFO1FrequencyDescription", "LFO1 Frequency", "LFO1FrequencyDescriptionTT", "LFO1 Frequency"),
DEFINE_METASOUND_ENUM_ENTRY(EMetasoundSynth1PatchDestination::LFO1Gain, "LFO1GainDescription", "LFO1 Gain", "LFO1GainDescriptionTT", "LFO1 Gain"),
DEFINE_METASOUND_ENUM_ENTRY(EMetasoundSynth1PatchDestination::LFO2Frequency, "LFO2FrequencyDescription", "LFO2 Frequency", "LFO2FrequencyDescriptionTT", "LFO2 Frequency"),
DEFINE_METASOUND_ENUM_ENTRY(EMetasoundSynth1PatchDestination::LFO2Gain, "LFO2GainDescription", "LFO2 Gain", "LFO2GainDescriptionTT", "LFO2 Gain"),
DEFINE_METASOUND_ENUM_END()

DECLARE_METASOUND_ENUM(EMetasoundSynthModEnvPatch, EMetasoundSynthModEnvPatch::PatchToNone, EPICSYNTHSMETASOUNDS_API, FEnumMetasoundSynthModEnvPatch, FEnumMetasoundSynthModEnvPatchInfo, FEnumMetasoundSynthModEnvPatchReadRef, FEnumMetasoundSynthModEnvPatchWriteRef);

DEFINE_METASOUND_ENUM_BEGIN(EMetasoundSynthModEnvPatch, FEnumMetasoundSynthModEnvPatch, "Mod Env Patch")
DEFINE_METASOUND_ENUM_ENTRY(EMetasoundSynthModEnvPatch::PatchToNone, "PatchToNoneDescription", "No Patch", "PatchToNoneDescriptionTT", "No modulation envelope patch"),
DEFINE_METASOUND_ENUM_ENTRY(EMetasoundSynthModEnvPatch::PatchToOscFreq, "PatchToOscFreqDescription", "Patch To Osc Freq", "PatchToOscFreqDescriptionTT", "Patch modulation envelope to oscillator frequency"),
DEFINE_METASOUND_ENUM_ENTRY(EMetasoundSynthModEnvPatch::PatchToFilterFreq, "PatchToFilterFreqDescription", "Patch To Filter Freq", "PatchToFilterFreqDescriptionTT", "Patch modulation envelope to filter frequency"),
DEFINE_METASOUND_ENUM_ENTRY(EMetasoundSynthModEnvPatch::PatchToFilterQ, "PatchToFilterQDescription", "Patch To Filter Q", "PatchToFilterQDescriptionTT", "Patch modulation envelope to filter Q"),
DEFINE_METASOUND_ENUM_ENTRY(EMetasoundSynthModEnvPatch::PatchToLFO1Gain, "PatchToLFO1GainDescription", "Patch To LFO1 Gain", "PatchToLFO1GainDescriptionTT", "Patch modulation envelope to LFO1 gain"),
DEFINE_METASOUND_ENUM_ENTRY(EMetasoundSynthModEnvPatch::PatchToLFO2Gain, "PatchToLFO2GainDescription", "Patch To LFO2 Gain", "PatchToLFO2GainDescriptionTT", "Patch modulation envelope to LFO2 gain"),
DEFINE_METASOUND_ENUM_ENTRY(EMetasoundSynthModEnvPatch::PatchToLFO1Freq, "PatchToLFO1FreqDescription", "Patch To LFO1 Freq", "PatchToLFO1FreqDescriptionTT", "Patch modulation envelope to LFO1 frequency"),
DEFINE_METASOUND_ENUM_ENTRY(EMetasoundSynthModEnvPatch::PatchToLFO2Freq, "PatchToLFO2FreqDescription", "Patch To LFO2 Freq", "PatchToLFO2FreqDescriptionTT", "Patch modulation envelope to LFO2 frequency"),
DEFINE_METASOUND_ENUM_END()

DECLARE_METASOUND_ENUM(EMetasoundSynthModEnvBiasPatch, EMetasoundSynthModEnvBiasPatch::PatchToNone, EPICSYNTHSMETASOUNDS_API, FEnumMetasoundSynthModEnvBiasPatch, FEnumMetasoundSynthModEnvBiasPatchInfo, FEnumMetasoundSynthModEnvBiasPatchReadRef, FEnumMetasoundSynthModEnvBiasPatchWriteRef);

DEFINE_METASOUND_ENUM_BEGIN(EMetasoundSynthModEnvBiasPatch, FEnumMetasoundSynthModEnvBiasPatch, "Mod Env Bias Patch")
DEFINE_METASOUND_ENUM_ENTRY(EMetasoundSynthModEnvBiasPatch::PatchToNone, "PatchToNoneDescription", "No Patch", "PatchToNoneDescriptionTT", "No modulation envelope bias patch"),
DEFINE_METASOUND_ENUM_ENTRY(EMetasoundSynthModEnvBiasPatch::PatchToOscFreq, "PatchToOscFreqDescription", "Patch To Osc Freq", "PatchToOscFreqDescriptionTT", "Patch modulation envelope bias to oscillator frequency"),
DEFINE_METASOUND_ENUM_ENTRY(EMetasoundSynthModEnvBiasPatch::PatchToFilterFreq, "PatchToFilterFreqDescription", "Patch To Filter Freq", "PatchToFilterFreqDescriptionTT", "Patch modulation envelope bias to filter frequency"),
DEFINE_METASOUND_ENUM_ENTRY(EMetasoundSynthModEnvBiasPatch::PatchToFilterQ, "PatchToFilterQDescription", "Patch To Filter Q", "PatchToFilterQDescriptionTT", "Patch modulation envelope bias to filter Q"),
DEFINE_METASOUND_ENUM_ENTRY(EMetasoundSynthModEnvBiasPatch::PatchToLFO1Gain, "PatchToLFO1GainDescription", "Patch To LFO1 Gain", "PatchToLFO1GainDescriptionTT", "Patch modulation envelope bias to LFO1 gain"),
DEFINE_METASOUND_ENUM_ENTRY(EMetasoundSynthModEnvBiasPatch::PatchToLFO2Gain, "PatchToLFO2GainDescription", "Patch To LFO2 Gain", "PatchToLFO2GainDescriptionTT", "Patch modulation envelope bias to LFO2 gain"),
DEFINE_METASOUND_ENUM_ENTRY(EMetasoundSynthModEnvBiasPatch::PatchToLFO1Freq, "PatchToLFO1FreqDescription", "Patch To LFO1 Freq", "PatchToLFO1FreqDescriptionTT", "Patch modulation envelope bias to LFO1 frequency"),
DEFINE_METASOUND_ENUM_ENTRY(EMetasoundSynthModEnvBiasPatch::PatchToLFO2Freq, "PatchToLFO2FreqDescription", "Patch To LFO2 Freq", "PatchToLFO2FreqDescriptionTT", "Patch modulation envelope bias to LFO2 frequency"),
DEFINE_METASOUND_ENUM_END()

namespace EpicSynthsMetasounds::Epic1SynthNode
{
	
	
	
	using namespace Metasound;
	using namespace HarmonixMetasound;

	const FNodeClassName& GetClassName()
	{
		static FNodeClassName ClassName
		{
			"EpicSynthsMetasounds",
			"Epic1SynthNode",
			""
		};
		return ClassName;
	}

	int32 GetCurrentMajorVersion()
	{
		return 1;
	}

	namespace Inputs
	{
		DEFINE_INPUT_METASOUND_PARAM(Enable, "Enable", "Enable");
		DEFINE_INPUT_METASOUND_PARAM(MidiStream, "MidiStream", "MidiStream");
		DEFINE_INPUT_METASOUND_PARAM(MinTrackIndex, "Track Index", "Track");
//	DEFINE_INPUT_METASOUND_PARAM(MaxTrackIndex, "Channel Index", "Channel");
		DEFINE_INPUT_METASOUND_PARAM(Voice1OscType, "Voice 1 Oscillator Type", "Oscillator Type for Voice 1");
		DEFINE_INPUT_METASOUND_PARAM(Voice2OscType, "Voice 2 Oscillator Type", "Oscillator Type for Voice 2");
		DEFINE_INPUT_METASOUND_PARAM(Monophonic, "Monophonic", "Monophonic");
		//DEFINE_INPUT_METASOUND_PARAM(Osc1Cents, "Osc1 Cents", "Osc1 Cents");
		//DEFINE_INPUT_METASOUND_PARAM(Osc1PulseWidth, "Osc1 Pulse Width", "Osc1 Pulse Width");
		//DEFINE_INPUT_METASOUND_PARAM(Osc2Cents, "Osc2 Cents", "Osc2 Cents");
		//DEFINE_INPUT_METASOUND_PARAM(Osc2PulseWidth, "Osc2 Pulse Width", "Osc2 Pulse Width");
		//DEFINE_INPUT_METASOUND_PARAM(IncludeConductorTrack, "Include Conductor Track", "Enable to include the conductor track (AKA track 0)");

		// New inputs
		DEFINE_INPUT_METASOUND_PARAM(Osc1Gain, "Osc1 Gain", "Oscillator 1 Gain [0.0, 1.0]");
		DEFINE_INPUT_METASOUND_PARAM(Osc1Octave, "Osc1 Octave", "Oscillator 1 Octave [-8.0, 8.0]");
		DEFINE_INPUT_METASOUND_PARAM(Osc1Semitones, "Osc1 Semitones", "Oscillator 1 Semitones [-12.0, 12.0]");
		DEFINE_INPUT_METASOUND_PARAM(Osc1Cents, "Osc1 Cents", "Oscillator 1 Cents [-100.0, 100.0]");
		DEFINE_INPUT_METASOUND_PARAM(Osc1PulseWidth, "Osc1 Pulse Width", "Oscillator 1 Pulse Width [0.0, 1.0]");
		DEFINE_INPUT_METASOUND_PARAM(Osc2Gain, "Osc2 Gain", "Oscillator 2 Gain [0.0, 1.0]");
		DEFINE_INPUT_METASOUND_PARAM(Osc2Octave, "Osc2 Octave", "Oscillator 2 Octave [-8.0, 8.0]");
		DEFINE_INPUT_METASOUND_PARAM(Osc2Semitones, "Osc2 Semitones", "Oscillator 2 Semitones [-12.0, 12.0]");
		DEFINE_INPUT_METASOUND_PARAM(Osc2Cents, "Osc2 Cents", "Oscillator 2 Cents [-100.0, 100.0]");
		DEFINE_INPUT_METASOUND_PARAM(Osc2PulseWidth, "Osc2 Pulse Width", "Oscillator 2 Pulse Width [0.0, 1.0]");
		DEFINE_INPUT_METASOUND_PARAM(Portamento, "Portamento", "Portamento amount [0.0, 1.0]");
		DEFINE_INPUT_METASOUND_PARAM(EnableUnison, "Enable Unison", "Enable oscillator unison");
		DEFINE_INPUT_METASOUND_PARAM(EnableOscSync, "Enable Osc Sync", "Enable oscillator sync");
		DEFINE_INPUT_METASOUND_PARAM(Spread, "Spread", "Stereo spread amount [0.0, 1.0]");
		DEFINE_INPUT_METASOUND_PARAM(Pan, "Pan", "Stereo pan position [-1.0, 1.0]");
		DEFINE_INPUT_METASOUND_PARAM(GainDb, "Gain (dB)", "Output gain in dB [-90.0, 20.0]");
		DEFINE_INPUT_METASOUND_PARAM(AttackTime, "Attack Time", "Amplitude envelope attack time (ms) [0.0, 10000.0]");
		DEFINE_INPUT_METASOUND_PARAM(DecayTime, "Decay Time", "Amplitude envelope decay time (ms) [0.0, 10000.0]");
		DEFINE_INPUT_METASOUND_PARAM(SustainGain, "Sustain Gain", "Amplitude envelope sustain gain [0.0, 1.0]");
		DEFINE_INPUT_METASOUND_PARAM(ReleaseTime, "Release Time", "Amplitude envelope release time (ms) [0.0, 10000.0]");
		DEFINE_INPUT_METASOUND_PARAM(EnableLegato, "Enable Legato", "Enable legato mode");
		DEFINE_INPUT_METASOUND_PARAM(EnableRetrigger, "Enable Retrigger", "Enable retrigger mode");
		DEFINE_INPUT_METASOUND_PARAM(FilterFrequency, "Filter Frequency", "Filter cutoff frequency (Hz) [0.0, 20000.0]");
		DEFINE_INPUT_METASOUND_PARAM(FilterQ, "Filter Q", "Filter resonance [0.5, 10.0]");
		DEFINE_INPUT_METASOUND_PARAM(FilterType, "Filter Type", "Type of filter (lowpass, highpass, etc.)");
		DEFINE_INPUT_METASOUND_PARAM(FilterAlgorithm, "Filter Algorithm", "Filter circuit/algorithm type");
		DEFINE_INPUT_METASOUND_PARAM(LFO1Frequency, "LFO1 Frequency", "LFO1 frequency (Hz) [0.0, 50.0]");
		DEFINE_INPUT_METASOUND_PARAM(LFO1Gain, "LFO1 Gain", "LFO1 gain [0.0, 1.0]");
		DEFINE_INPUT_METASOUND_PARAM(LFO1Type, "LFO1 Type", "LFO1 waveform type");
		DEFINE_INPUT_METASOUND_PARAM(LFO1Mode, "LFO1 Mode", "LFO1 mode (Sync, OneShot, Free)");
		DEFINE_INPUT_METASOUND_PARAM(LFO1PatchType, "LFO1 Patch Type", "LFO1 built-in patch routing");
		DEFINE_INPUT_METASOUND_PARAM(LFO2Frequency, "LFO2 Frequency", "LFO2 frequency (Hz) [0.0, 50.0]");
		DEFINE_INPUT_METASOUND_PARAM(LFO2Gain, "LFO2 Gain", "LFO2 gain [0.0, 1.0]");
		DEFINE_INPUT_METASOUND_PARAM(LFO2Type, "LFO2 Type", "LFO2 waveform type");
		DEFINE_INPUT_METASOUND_PARAM(LFO2Mode, "LFO2 Mode", "LFO2 mode (Sync, OneShot, Free)");
		DEFINE_INPUT_METASOUND_PARAM(LFO2PatchType, "LFO2 Patch Type", "LFO2 built-in patch routing");
		DEFINE_INPUT_METASOUND_PARAM(EnableStereoDelay, "Enable Stereo Delay", "Enable stereo delay effect");
		DEFINE_INPUT_METASOUND_PARAM(StereoDelayMode, "Stereo Delay Mode", "Stereo delay mode");
		DEFINE_INPUT_METASOUND_PARAM(StereoDelayTime, "Stereo Delay Time", "Stereo delay time (ms) [0.0, 2000.0]");
		DEFINE_INPUT_METASOUND_PARAM(StereoDelayFeedback, "Stereo Delay Feedback", "Stereo delay feedback [0.0, 1.0]");
		DEFINE_INPUT_METASOUND_PARAM(StereoDelayWetLevel, "Stereo Delay Wet Level", "Stereo delay wet level [0.0, 1.0]");
		DEFINE_INPUT_METASOUND_PARAM(StereoDelayRatio, "Stereo Delay Ratio", "Stereo delay ratio [0.0, 1.0]");
		DEFINE_INPUT_METASOUND_PARAM(EnableChorus, "Enable Chorus", "Enable chorus effect");
		DEFINE_INPUT_METASOUND_PARAM(ChorusDepth, "Chorus Depth", "Chorus depth [0.0, 1.0]");
		DEFINE_INPUT_METASOUND_PARAM(ChorusFeedback, "Chorus Feedback", "Chorus feedback [0.0, 1.0]");
		DEFINE_INPUT_METASOUND_PARAM(ChorusFrequency, "Chorus Frequency", "Chorus frequency (Hz) [0.0, 20.0]");
	}

	namespace Outputs
	{
		DEFINE_OUTPUT_METASOUND_PARAM(AudioOutLeft, "Audio Out Left", "Left output of SFizz Synth");
		DEFINE_OUTPUT_METASOUND_PARAM(AudioOutRight, "Audio Out Right", "Right output of Sfizz Synth");
	}

	class FEpic1SynthMetasoundOperator final : public TExecutableOperator<FEpic1SynthMetasoundOperator>
	{
	public:
		static const FNodeClassMetadata& GetNodeInfo()
		{
			auto InitNodeInfo = []() -> FNodeClassMetadata
				{
					FNodeClassMetadata Info;
					Info.ClassName = GetClassName();
					Info.MajorVersion = 1;
					Info.MinorVersion = 0;
					Info.DisplayName = INVTEXT("Epic1Synth Node");
					Info.Description = INVTEXT("Renders MIDI Stream to audio data using Epic1Synth");
					Info.Author = PluginAuthor;
					Info.PromptIfMissing = PluginNodeMissingPrompt;
					Info.DefaultInterface = GetVertexInterface();
					Info.CategoryHierarchy = { INVTEXT("Synthesis"), NodeCategories::Music };
					return Info;
				};

			static const FNodeClassMetadata Info = InitNodeInfo();

			return Info;
		}

		static const FVertexInterface& GetVertexInterface()
		{
			static const FVertexInterface Interface(
				FInputVertexInterface(
					// Existing inputs
					TInputDataVertex<bool>(METASOUND_GET_PARAM_NAME_AND_METADATA(Inputs::Enable), true),
					TInputDataVertex<FMidiStream>(METASOUND_GET_PARAM_NAME_AND_METADATA(Inputs::MidiStream)),
					TInputDataVertex<int32>(METASOUND_GET_PARAM_NAME_AND_METADATA(Inputs::MinTrackIndex), 1),
					TInputDataVertex<FEnumEpicsynth1Osc>(METASOUND_GET_PARAM_NAME_AND_METADATA(Inputs::Voice1OscType)),
					TInputDataVertex<FEnumEpicsynth1Osc>(METASOUND_GET_PARAM_NAME_AND_METADATA(Inputs::Voice2OscType)),
					TInputDataVertex<bool>(METASOUND_GET_PARAM_NAME_AND_METADATA(Inputs::Monophonic), false),
					
					// Oscillator 1 controls
					TInputDataVertex<float>(METASOUND_GET_PARAM_NAME_AND_METADATA(Inputs::Osc1Gain), 1.0f),
					TInputDataVertex<float>(METASOUND_GET_PARAM_NAME_AND_METADATA(Inputs::Osc1Octave), 0.0f),
					TInputDataVertex<float>(METASOUND_GET_PARAM_NAME_AND_METADATA(Inputs::Osc1Semitones), 0.0f),
					TInputDataVertex<float>(METASOUND_GET_PARAM_NAME_AND_METADATA(Inputs::Osc1Cents), 0.0f),
					TInputDataVertex<float>(METASOUND_GET_PARAM_NAME_AND_METADATA(Inputs::Osc1PulseWidth), 0.5f),
					
					// Oscillator 2 controls
					TInputDataVertex<float>(METASOUND_GET_PARAM_NAME_AND_METADATA(Inputs::Osc2Gain), 1.0f),
					TInputDataVertex<float>(METASOUND_GET_PARAM_NAME_AND_METADATA(Inputs::Osc2Octave), 0.0f),
					TInputDataVertex<float>(METASOUND_GET_PARAM_NAME_AND_METADATA(Inputs::Osc2Semitones), 0.0f),
					TInputDataVertex<float>(METASOUND_GET_PARAM_NAME_AND_METADATA(Inputs::Osc2Cents), 0.0f),
					TInputDataVertex<float>(METASOUND_GET_PARAM_NAME_AND_METADATA(Inputs::Osc2PulseWidth), 0.5f),
					
					// Global synth controls
					TInputDataVertex<float>(METASOUND_GET_PARAM_NAME_AND_METADATA(Inputs::Portamento), 0.0f),
					TInputDataVertex<bool>(METASOUND_GET_PARAM_NAME_AND_METADATA(Inputs::EnableUnison), false),
					TInputDataVertex<bool>(METASOUND_GET_PARAM_NAME_AND_METADATA(Inputs::EnableOscSync), false),
					TInputDataVertex<float>(METASOUND_GET_PARAM_NAME_AND_METADATA(Inputs::Spread), 0.5f),
					TInputDataVertex<float>(METASOUND_GET_PARAM_NAME_AND_METADATA(Inputs::Pan), 0.0f),
					TInputDataVertex<float>(METASOUND_GET_PARAM_NAME_AND_METADATA(Inputs::GainDb), -3.0f),
					
					// Envelope controls
					TInputDataVertex<float>(METASOUND_GET_PARAM_NAME_AND_METADATA(Inputs::AttackTime), 10.0f),
					TInputDataVertex<float>(METASOUND_GET_PARAM_NAME_AND_METADATA(Inputs::DecayTime), 100.0f),
					TInputDataVertex<float>(METASOUND_GET_PARAM_NAME_AND_METADATA(Inputs::SustainGain), 0.707f),
					TInputDataVertex<float>(METASOUND_GET_PARAM_NAME_AND_METADATA(Inputs::ReleaseTime), 5000.0f),
					TInputDataVertex<bool>(METASOUND_GET_PARAM_NAME_AND_METADATA(Inputs::EnableLegato), true),
					TInputDataVertex<bool>(METASOUND_GET_PARAM_NAME_AND_METADATA(Inputs::EnableRetrigger), false),
					
					// Filter controls
					TInputDataVertex<float>(METASOUND_GET_PARAM_NAME_AND_METADATA(Inputs::FilterFrequency), 8000.0f),
					TInputDataVertex<float>(METASOUND_GET_PARAM_NAME_AND_METADATA(Inputs::FilterQ), 2.0f),
					TInputDataVertex<FEnumMetasoundSynthFilterType>(METASOUND_GET_PARAM_NAME_AND_METADATA(Inputs::FilterType)),
					TInputDataVertex<FEnumMetasoundSynthFilterAlgorithm>(METASOUND_GET_PARAM_NAME_AND_METADATA(Inputs::FilterAlgorithm)),
					
					// LFO1 controls
					TInputDataVertex<float>(METASOUND_GET_PARAM_NAME_AND_METADATA(Inputs::LFO1Frequency), 1.0f),
					TInputDataVertex<float>(METASOUND_GET_PARAM_NAME_AND_METADATA(Inputs::LFO1Gain), 0.0f),
					TInputDataVertex<FEnumMetasoundSynthLFOType>(METASOUND_GET_PARAM_NAME_AND_METADATA(Inputs::LFO1Type)),
					TInputDataVertex<FEnumMetasoundSynthLFOMode>(METASOUND_GET_PARAM_NAME_AND_METADATA(Inputs::LFO1Mode)),
					TInputDataVertex<FEnumMetasoundSynthLFOPatchType>(METASOUND_GET_PARAM_NAME_AND_METADATA(Inputs::LFO1PatchType)),
					
					// LFO2 controls
					TInputDataVertex<float>(METASOUND_GET_PARAM_NAME_AND_METADATA(Inputs::LFO2Frequency), 1.0f),
					TInputDataVertex<float>(METASOUND_GET_PARAM_NAME_AND_METADATA(Inputs::LFO2Gain), 0.0f),
					TInputDataVertex<FEnumMetasoundSynthLFOType>(METASOUND_GET_PARAM_NAME_AND_METADATA(Inputs::LFO2Type)),
					TInputDataVertex<FEnumMetasoundSynthLFOMode>(METASOUND_GET_PARAM_NAME_AND_METADATA(Inputs::LFO2Mode)),
					TInputDataVertex<FEnumMetasoundSynthLFOPatchType>(METASOUND_GET_PARAM_NAME_AND_METADATA(Inputs::LFO2PatchType)),
					
					// Stereo delay controls
					TInputDataVertex<bool>(METASOUND_GET_PARAM_NAME_AND_METADATA(Inputs::EnableStereoDelay), true),
					TInputDataVertex<FEnumMetasoundSynthStereoDelayMode>(METASOUND_GET_PARAM_NAME_AND_METADATA(Inputs::StereoDelayMode)),
					TInputDataVertex<float>(METASOUND_GET_PARAM_NAME_AND_METADATA(Inputs::StereoDelayTime), 700.0f),
					TInputDataVertex<float>(METASOUND_GET_PARAM_NAME_AND_METADATA(Inputs::StereoDelayFeedback), 0.7f),
					TInputDataVertex<float>(METASOUND_GET_PARAM_NAME_AND_METADATA(Inputs::StereoDelayWetLevel), 0.3f),
					TInputDataVertex<float>(METASOUND_GET_PARAM_NAME_AND_METADATA(Inputs::StereoDelayRatio), 0.2f),
					
					// Chorus controls
					TInputDataVertex<bool>(METASOUND_GET_PARAM_NAME_AND_METADATA(Inputs::EnableChorus), false),
					TInputDataVertex<float>(METASOUND_GET_PARAM_NAME_AND_METADATA(Inputs::ChorusDepth), 0.2f),
					TInputDataVertex<float>(METASOUND_GET_PARAM_NAME_AND_METADATA(Inputs::ChorusFeedback), 0.5f),
					TInputDataVertex<float>(METASOUND_GET_PARAM_NAME_AND_METADATA(Inputs::ChorusFrequency), 2.0f)
				),
				FOutputVertexInterface(
					TOutputDataVertex<FAudioBuffer>(METASOUND_GET_PARAM_NAME_AND_METADATA(Outputs::AudioOutLeft)),
					TOutputDataVertex<FAudioBuffer>(METASOUND_GET_PARAM_NAME_AND_METADATA(Outputs::AudioOutRight))
				)				
			);
			return Interface;
		}

		struct FInputs
		{
			// Existing inputs
			FBoolReadRef Enabled;
			FMidiStreamReadRef MidiStream;
			FInt32ReadRef MinTrackIndex;
			FEnumEpicsynth1OscReadRef Voice1OscillatorTypeRef;
			FEnumEpicsynth1OscReadRef Voice2OscillatorTypeRef;
			FBoolReadRef bIsMonoRef;

			// Oscillator 1 controls
			FFloatReadRef Osc1Gain;
			FFloatReadRef Osc1Octave;
			FFloatReadRef Osc1Semitones;
			FFloatReadRef Osc1Cents;
			FFloatReadRef Osc1PulseWidth;
			
			// Oscillator 2 controls
			FFloatReadRef Osc2Gain;
			FFloatReadRef Osc2Octave;
			FFloatReadRef Osc2Semitones;
			FFloatReadRef Osc2Cents;
			FFloatReadRef Osc2PulseWidth;
			
			// Global synth controls
			FFloatReadRef Portamento;
			FBoolReadRef EnableUnison;
			FBoolReadRef EnableOscSync;
			FFloatReadRef Spread;
			FFloatReadRef Pan;
			FFloatReadRef GainDb;
			
			// Envelope controls
			FFloatReadRef AttackTime;
			FFloatReadRef DecayTime;
			FFloatReadRef SustainGain;
			FFloatReadRef ReleaseTime;
			FBoolReadRef EnableLegato;
			FBoolReadRef EnableRetrigger;
			
			// Filter controls
			FFloatReadRef FilterFrequency;
			FFloatReadRef FilterQ;
			FEnumMetasoundSynthFilterTypeReadRef FilterType;
			FEnumMetasoundSynthFilterAlgorithmReadRef FilterAlgorithm;
			
			// LFO1 controls
			FFloatReadRef LFO1Frequency;
			FFloatReadRef LFO1Gain;
			FEnumMetasoundSynthLFOTypeReadRef LFO1Type;
			FEnumMetasoundSynthLFOModeReadRef LFO1Mode;
			FEnumMetasoundSynthLFOPatchTypeReadRef LFO1PatchType;
			
			// LFO2 controls
			FFloatReadRef LFO2Frequency;
			FFloatReadRef LFO2Gain;
			FEnumMetasoundSynthLFOTypeReadRef LFO2Type;
			FEnumMetasoundSynthLFOModeReadRef LFO2Mode;
			FEnumMetasoundSynthLFOPatchTypeReadRef LFO2PatchType;
			
			// Stereo delay controls
			FBoolReadRef EnableStereoDelay;
			FEnumMetasoundSynthStereoDelayModeReadRef StereoDelayMode;
			FFloatReadRef StereoDelayTime;
			FFloatReadRef StereoDelayFeedback;
			FFloatReadRef StereoDelayWetLevel;
			FFloatReadRef StereoDelayRatio;
			
			// Chorus controls
			FBoolReadRef EnableChorus;
			FFloatReadRef ChorusDepth;
			FFloatReadRef ChorusFeedback;
			FFloatReadRef ChorusFrequency;
		};

		static TUniquePtr<IOperator> CreateOperator(const FBuildOperatorParams& InParams, FBuildResults& OutResults)
		{
			const FInputVertexInterfaceData& InputData = InParams.InputData;

			FInputs Inputs
			{
				// Existing inputs
				InputData.GetOrCreateDefaultDataReadReference<bool>(Inputs::EnableName, InParams.OperatorSettings),
				InputData.GetOrConstructDataReadReference<FMidiStream>(Inputs::MidiStreamName),
				InputData.GetOrCreateDefaultDataReadReference<int32>(Inputs::MinTrackIndexName, InParams.OperatorSettings),
				InputData.GetOrConstructDataReadReference<FEnumEpicsynth1Osc>(Inputs::Voice1OscTypeName),
				InputData.GetOrConstructDataReadReference<FEnumEpicsynth1Osc>(Inputs::Voice2OscTypeName),
				InputData.GetOrCreateDefaultDataReadReference<bool>(Inputs::MonophonicName, InParams.OperatorSettings),
				
				// Oscillator 1 controls
				InputData.GetOrCreateDefaultDataReadReference<float>(Inputs::Osc1GainName, InParams.OperatorSettings),
				InputData.GetOrCreateDefaultDataReadReference<float>(Inputs::Osc1OctaveName, InParams.OperatorSettings),
				InputData.GetOrCreateDefaultDataReadReference<float>(Inputs::Osc1SemitonesName, InParams.OperatorSettings),
				InputData.GetOrCreateDefaultDataReadReference<float>(Inputs::Osc1CentsName, InParams.OperatorSettings),
				InputData.GetOrCreateDefaultDataReadReference<float>(Inputs::Osc1PulseWidthName, InParams.OperatorSettings),
				
				// Oscillator 2 controls
				InputData.GetOrCreateDefaultDataReadReference<float>(Inputs::Osc2GainName, InParams.OperatorSettings),
				InputData.GetOrCreateDefaultDataReadReference<float>(Inputs::Osc2OctaveName, InParams.OperatorSettings),
				InputData.GetOrCreateDefaultDataReadReference<float>(Inputs::Osc2SemitonesName, InParams.OperatorSettings),
				InputData.GetOrCreateDefaultDataReadReference<float>(Inputs::Osc2CentsName, InParams.OperatorSettings),
				InputData.GetOrCreateDefaultDataReadReference<float>(Inputs::Osc2PulseWidthName, InParams.OperatorSettings),
				
				// Global synth controls
				InputData.GetOrCreateDefaultDataReadReference<float>(Inputs::PortamentoName, InParams.OperatorSettings),
				InputData.GetOrCreateDefaultDataReadReference<bool>(Inputs::EnableUnisonName, InParams.OperatorSettings),
				InputData.GetOrCreateDefaultDataReadReference<bool>(Inputs::EnableOscSyncName, InParams.OperatorSettings),
				InputData.GetOrCreateDefaultDataReadReference<float>(Inputs::SpreadName, InParams.OperatorSettings),
				InputData.GetOrCreateDefaultDataReadReference<float>(Inputs::PanName, InParams.OperatorSettings),
				InputData.GetOrCreateDefaultDataReadReference<float>(Inputs::GainDbName, InParams.OperatorSettings),
				
				// Envelope controls
				InputData.GetOrCreateDefaultDataReadReference<float>(Inputs::AttackTimeName, InParams.OperatorSettings),
				InputData.GetOrCreateDefaultDataReadReference<float>(Inputs::DecayTimeName, InParams.OperatorSettings),
				InputData.GetOrCreateDefaultDataReadReference<float>(Inputs::SustainGainName, InParams.OperatorSettings),
				InputData.GetOrCreateDefaultDataReadReference<float>(Inputs::ReleaseTimeName, InParams.OperatorSettings),
				InputData.GetOrCreateDefaultDataReadReference<bool>(Inputs::EnableLegatoName, InParams.OperatorSettings),
				InputData.GetOrCreateDefaultDataReadReference<bool>(Inputs::EnableRetriggerName, InParams.OperatorSettings),
				
				// Filter controls
				InputData.GetOrCreateDefaultDataReadReference<float>(Inputs::FilterFrequencyName, InParams.OperatorSettings),
				InputData.GetOrCreateDefaultDataReadReference<float>(Inputs::FilterQName, InParams.OperatorSettings),
				InputData.GetOrConstructDataReadReference<FEnumMetasoundSynthFilterType>(Inputs::FilterTypeName),
				InputData.GetOrConstructDataReadReference<FEnumMetasoundSynthFilterAlgorithm>(Inputs::FilterAlgorithmName),
				
				// LFO1 controls
				InputData.GetOrCreateDefaultDataReadReference<float>(Inputs::LFO1FrequencyName, InParams.OperatorSettings),
				InputData.GetOrCreateDefaultDataReadReference<float>(Inputs::LFO1GainName, InParams.OperatorSettings),
				InputData.GetOrConstructDataReadReference<FEnumMetasoundSynthLFOType>(Inputs::LFO1TypeName),
				InputData.GetOrConstructDataReadReference<FEnumMetasoundSynthLFOMode>(Inputs::LFO1ModeName),
				InputData.GetOrConstructDataReadReference<FEnumMetasoundSynthLFOPatchType>(Inputs::LFO1PatchTypeName),
				
				// LFO2 controls
				InputData.GetOrCreateDefaultDataReadReference<float>(Inputs::LFO2FrequencyName, InParams.OperatorSettings),
				InputData.GetOrCreateDefaultDataReadReference<float>(Inputs::LFO2GainName, InParams.OperatorSettings),
				InputData.GetOrConstructDataReadReference<FEnumMetasoundSynthLFOType>(Inputs::LFO2TypeName),
				InputData.GetOrConstructDataReadReference<FEnumMetasoundSynthLFOMode>(Inputs::LFO2ModeName),
				InputData.GetOrConstructDataReadReference<FEnumMetasoundSynthLFOPatchType>(Inputs::LFO2PatchTypeName),
				
				// Stereo delay controls
				InputData.GetOrCreateDefaultDataReadReference<bool>(Inputs::EnableStereoDelayName, InParams.OperatorSettings),
				InputData.GetOrConstructDataReadReference<FEnumMetasoundSynthStereoDelayMode>(Inputs::StereoDelayModeName),
				InputData.GetOrCreateDefaultDataReadReference<float>(Inputs::StereoDelayTimeName, InParams.OperatorSettings),
				InputData.GetOrCreateDefaultDataReadReference<float>(Inputs::StereoDelayFeedbackName, InParams.OperatorSettings),
				InputData.GetOrCreateDefaultDataReadReference<float>(Inputs::StereoDelayWetLevelName, InParams.OperatorSettings),
				InputData.GetOrCreateDefaultDataReadReference<float>(Inputs::StereoDelayRatioName, InParams.OperatorSettings),
				
				// Chorus controls
				InputData.GetOrCreateDefaultDataReadReference<bool>(Inputs::EnableChorusName, InParams.OperatorSettings),
				InputData.GetOrCreateDefaultDataReadReference<float>(Inputs::ChorusDepthName, InParams.OperatorSettings),
				InputData.GetOrCreateDefaultDataReadReference<float>(Inputs::ChorusFeedbackName, InParams.OperatorSettings),
				InputData.GetOrCreateDefaultDataReadReference<float>(Inputs::ChorusFrequencyName, InParams.OperatorSettings)
			};

			return MakeUnique<FEpic1SynthMetasoundOperator>(InParams, MoveTemp(Inputs));
		}

		FEpic1SynthMetasoundOperator(const FBuildOperatorParams& InParams, FInputs&& InInputs)
			: Inputs(MoveTemp(InInputs))
			, SampleRate(InParams.OperatorSettings.GetSampleRate())
			, AudioOutLeft(FAudioBufferWriteRef::CreateNew(InParams.OperatorSettings))
			, AudioOutRight(FAudioBufferWriteRef::CreateNew(InParams.OperatorSettings))
		{
			Reset(InParams);
		}
		virtual void BindInputs(FInputVertexInterfaceData& InVertexData) override
		{
			// Existing inputs
			InVertexData.BindReadVertex(Inputs::EnableName, Inputs.Enabled);
			InVertexData.BindReadVertex(Inputs::MidiStreamName, Inputs.MidiStream);
			InVertexData.BindReadVertex(Inputs::MinTrackIndexName, Inputs.MinTrackIndex);
			InVertexData.BindReadVertex(Inputs::Voice1OscTypeName, Inputs.Voice1OscillatorTypeRef);
			InVertexData.BindReadVertex(Inputs::Voice2OscTypeName, Inputs.Voice2OscillatorTypeRef);
			InVertexData.BindReadVertex(Inputs::MonophonicName, Inputs.bIsMonoRef);
			
			// Oscillator 1 controls
			InVertexData.BindReadVertex(Inputs::Osc1GainName, Inputs.Osc1Gain);
			InVertexData.BindReadVertex(Inputs::Osc1OctaveName, Inputs.Osc1Octave);
			InVertexData.BindReadVertex(Inputs::Osc1SemitonesName, Inputs.Osc1Semitones);
			InVertexData.BindReadVertex(Inputs::Osc1CentsName, Inputs.Osc1Cents);
			InVertexData.BindReadVertex(Inputs::Osc1PulseWidthName, Inputs.Osc1PulseWidth);
			
			// Oscillator 2 controls
			InVertexData.BindReadVertex(Inputs::Osc2GainName, Inputs.Osc2Gain);
			InVertexData.BindReadVertex(Inputs::Osc2OctaveName, Inputs.Osc2Octave);
			InVertexData.BindReadVertex(Inputs::Osc2SemitonesName, Inputs.Osc2Semitones);
			InVertexData.BindReadVertex(Inputs::Osc2CentsName, Inputs.Osc2Cents);
			InVertexData.BindReadVertex(Inputs::Osc2PulseWidthName, Inputs.Osc2PulseWidth);
			
			// Global synth controls
			InVertexData.BindReadVertex(Inputs::PortamentoName, Inputs.Portamento);
			InVertexData.BindReadVertex(Inputs::EnableUnisonName, Inputs.EnableUnison);
			InVertexData.BindReadVertex(Inputs::EnableOscSyncName, Inputs.EnableOscSync);
			InVertexData.BindReadVertex(Inputs::SpreadName, Inputs.Spread);
			InVertexData.BindReadVertex(Inputs::PanName, Inputs.Pan);
			InVertexData.BindReadVertex(Inputs::GainDbName, Inputs.GainDb);
			
			// Envelope controls
			InVertexData.BindReadVertex(Inputs::AttackTimeName, Inputs.AttackTime);
			InVertexData.BindReadVertex(Inputs::DecayTimeName, Inputs.DecayTime);
			InVertexData.BindReadVertex(Inputs::SustainGainName, Inputs.SustainGain);
			InVertexData.BindReadVertex(Inputs::ReleaseTimeName, Inputs.ReleaseTime);
			InVertexData.BindReadVertex(Inputs::EnableLegatoName, Inputs.EnableLegato);
			InVertexData.BindReadVertex(Inputs::EnableRetriggerName, Inputs.EnableRetrigger);
			
			// Filter controls
			InVertexData.BindReadVertex(Inputs::FilterFrequencyName, Inputs.FilterFrequency);
			InVertexData.BindReadVertex(Inputs::FilterQName, Inputs.FilterQ);
			InVertexData.BindReadVertex(Inputs::FilterTypeName, Inputs.FilterType);
			InVertexData.BindReadVertex(Inputs::FilterAlgorithmName, Inputs.FilterAlgorithm);
			
			// LFO1 controls
			InVertexData.BindReadVertex(Inputs::LFO1FrequencyName, Inputs.LFO1Frequency);
			InVertexData.BindReadVertex(Inputs::LFO1GainName, Inputs.LFO1Gain);
			InVertexData.BindReadVertex(Inputs::LFO1TypeName, Inputs.LFO1Type);
			InVertexData.BindReadVertex(Inputs::LFO1ModeName, Inputs.LFO1Mode);
			InVertexData.BindReadVertex(Inputs::LFO1PatchTypeName, Inputs.LFO1PatchType);
			
			// LFO2 controls
			InVertexData.BindReadVertex(Inputs::LFO2FrequencyName, Inputs.LFO2Frequency);
			InVertexData.BindReadVertex(Inputs::LFO2GainName, Inputs.LFO2Gain);
			InVertexData.BindReadVertex(Inputs::LFO2TypeName, Inputs.LFO2Type);
			InVertexData.BindReadVertex(Inputs::LFO2ModeName, Inputs.LFO2Mode);
			InVertexData.BindReadVertex(Inputs::LFO2PatchTypeName, Inputs.LFO2PatchType);
			
			// Stereo delay controls
			InVertexData.BindReadVertex(Inputs::EnableStereoDelayName, Inputs.EnableStereoDelay);
			InVertexData.BindReadVertex(Inputs::StereoDelayModeName, Inputs.StereoDelayMode);
			InVertexData.BindReadVertex(Inputs::StereoDelayTimeName, Inputs.StereoDelayTime);
			InVertexData.BindReadVertex(Inputs::StereoDelayFeedbackName, Inputs.StereoDelayFeedback);
			InVertexData.BindReadVertex(Inputs::StereoDelayWetLevelName, Inputs.StereoDelayWetLevel);
			InVertexData.BindReadVertex(Inputs::StereoDelayRatioName, Inputs.StereoDelayRatio);
			
			// Chorus controls
			InVertexData.BindReadVertex(Inputs::EnableChorusName, Inputs.EnableChorus);
			InVertexData.BindReadVertex(Inputs::ChorusDepthName, Inputs.ChorusDepth);
			InVertexData.BindReadVertex(Inputs::ChorusFeedbackName, Inputs.ChorusFeedback);
			InVertexData.BindReadVertex(Inputs::ChorusFrequencyName, Inputs.ChorusFrequency);
		}

		virtual void BindOutputs(FOutputVertexInterfaceData& InVertexData) override
		{
			InVertexData.BindWriteVertex(Outputs::AudioOutLeftName, AudioOutLeft);
			InVertexData.BindWriteVertex(Outputs::AudioOutRightName, AudioOutRight);
		}

		void Reset(const FResetParams&)
		{

		}


		//destructor
		virtual ~FEpic1SynthMetasoundOperator()
		{
			UE_LOG(LogEpicSynth1Node, VeryVerbose, TEXT("Epic Synth Node Destructor"));
		}
			




		void HandleMidiMessage(FMidiVoiceId InVoiceId, int8 InStatus, int8 InData1, int8 InData2, int32 InEventTick, int32 InCurrentTick, float InMsOffset)
		{
			using namespace Harmonix::Midi::Constants;
			int8 InChannel = InStatus & 0xF;
			FScopeLock Lock(&sNoteActionCritSec);
			switch (InStatus & 0xF0)
			{
			case GNoteOff:
				if (bIsSustainPedalDown)
				{
					NoteStatus[InData1].KeyedOn = false;
				}
				else {
					EpicSynth1.NoteOff(InData1);

				}
				//UE_LOG(LogEpicSynth1Node, VeryVerbose, TEXT("Note Off: %d"), InData1);
				break;
			case GNoteOn:
				NoteStatus[InData1].KeyedOn = true;

				EpicSynth1.NoteOn(InData1, (float) InData2);
				break;
			case GPolyPres:
		
				break;
			case GChanPres:
				break;
			case GControl:
				//if is sustain pedal

				if (InData1 == 64)
				{
					if (InData2 > 63)
					{
						bIsSustainPedalDown = true;
					}
					else
					{
						bIsSustainPedalDown = false;
						for (int i = 0; i < 128; i++)
						{
							if (!NoteStatus[i].KeyedOn)
							{
								EpicSynth1.NoteOff(i);
							}
						}
					}
				}

				break;
			case GPitch:
				//UE_LOG(LogEpicSynth1Node, VeryVerbose, TEXT("Pitch Bend: %d"), InData1);
				PitchBendRamper.SetTarget(FMidiMsg::GetPitchBendFromData(InData1, InData2));
				break;
			}
		}

		void Execute()
		{
			const int32 BlockSizeFrames = AudioOutLeft->Num();
			PendingNoteActions.Empty();
			//if (*Inputs.SfzLibPath != LibPath)
			{
				if (!bEpic1SynthCreated)
				{
					Oscillator1TypeEnum = *Inputs.Voice1OscillatorTypeRef.Get();
					Oscillator2TypeEnum = *Inputs.Voice2OscillatorTypeRef.Get();
					bIsMono = *Inputs.bIsMonoRef;

					// cents, pulse width
					Oscillator1Cents = *Inputs.Osc1Cents;
					Oscillator1PulseWidth = *Inputs.Osc1PulseWidth;
					Oscillator2Cents = *Inputs.Osc2Cents;
					Oscillator2PulseWidth = *Inputs.Osc2PulseWidth;
					
					EpicSynth1.Init(SampleRate, VoiceCount);		


					EpicSynth1.SetMonoMode(bIsMono);
					EpicSynth1.SetOscType(0, (Audio::EOsc::Type)Oscillator1TypeEnum);
					EpicSynth1.SetOscType(1, (Audio::EOsc::Type)Oscillator2TypeEnum);
					EpicSynth1.SetOscCents(0, Oscillator1Cents);
					EpicSynth1.SetOscCents(1, Oscillator2Cents);
					EpicSynth1.SetOscPulseWidth(0, Oscillator1PulseWidth);
					EpicSynth1.SetOscPulseWidth(1, Oscillator2PulseWidth);
					EpicSynth1.SetOscUnison(false);
					EpicSynth1.SetOscSpread(0.5f);
					EpicSynth1.SetGainDb(-3.0f);
					EpicSynth1.SetEnvAttackTime(10.0f);
					EpicSynth1.SetEnvDecayTime(10.0f);
					EpicSynth1.SetEnvSustainGain(0.707f);
					EpicSynth1.SetEnvReleaseTime(50.0f);
					EpicSynth1.SetEnvLegatoEnabled(false);
					EpicSynth1.SetEnvRetriggerMode(false);
					EpicSynth1.SetFilterFrequency(1200.0f);
					EpicSynth1.SetFilterQ(2.0f);
					EpicSynth1.SetFilterAlgorithm(EMetasoundSynthFilterAlgorithm::Ladder);
					EpicSynth1.SetStereoDelayIsEnabled(false);
					EpicSynth1.SetStereoDelayMode((Audio::EStereoDelayMode::Type)EMetasoundSynthStereoDelayMode::PingPong);
					EpicSynth1.SetStereoDelayRatio(0.2f);
					EpicSynth1.SetStereoDelayFeedback(0.7f);
					EpicSynth1.SetStereoDelayWetLevel(0.3f);
					EpicSynth1.SetChorusEnabled(false);

					const float RampCallRateHz = (float) (1 /SampleRate) / (float) BlockSizeFrames;
					PitchBendRamper.SetRampTimeMs(RampCallRateHz, 5.0f);
					PitchBendRamper.SetTarget(0.0f);
					PitchBendRamper.SnapToTarget();



					DeinterleavedBuffer.resize(2 * BlockSizeFrames);
					DecodedAudioDataBuffer.resize(2 * BlockSizeFrames);
					//DeinterleavedBuffer[0] = AudioOutLeft->GetData();
					//DeinterleavedBuffer[1] = AudioOutRight->GetData();

					CurrentTrackNumber = *Inputs.MinTrackIndex;


					bEpic1SynthCreated = true;
				}

			}

			if (!bEpic1SynthCreated)
			{
				return;
				// send note on test
				//sfizz_send_note_on(SfizzSynth, 0, 60, 100);
			}
			
			StuckNoteGuard.UnstickNotes(*Inputs.MidiStream, [this](const FMidiStreamEvent& Event)
				{
					EpicSynth1.NoteOff(Event.GetVoiceId());
				});

			if (*Inputs.Voice1OscillatorTypeRef.Get() != Oscillator1TypeEnum)
			{
				Oscillator1TypeEnum = *Inputs.Voice1OscillatorTypeRef.Get();
				EpicSynth1.SetOscType(0, (Audio::EOsc::Type)Oscillator1TypeEnum);
			}


			if (*Inputs.Voice2OscillatorTypeRef.Get() != Oscillator2TypeEnum)
			{
				Oscillator2TypeEnum = *Inputs.Voice2OscillatorTypeRef.Get();
				EpicSynth1.SetOscType(1, (Audio::EOsc::Type)Oscillator2TypeEnum);
			}

			if (*Inputs.bIsMonoRef != bIsMono)
			{
				bIsMono = *Inputs.bIsMonoRef;
				EpicSynth1.SetMonoMode(bIsMono);
			}

			if (*Inputs.Osc1Cents != Oscillator1Cents)
			{
				Oscillator1Cents = *Inputs.Osc1Cents;
				EpicSynth1.SetOscCents(0, Oscillator1Cents);
			}

			if (*Inputs.Osc1PulseWidth != Oscillator1PulseWidth)
			{
				Oscillator1PulseWidth = *Inputs.Osc1PulseWidth;
				EpicSynth1.SetOscPulseWidth(0, Oscillator1PulseWidth);
			}

			if (*Inputs.Osc2Cents != Oscillator2Cents)
			{
				Oscillator2Cents = *Inputs.Osc2Cents;
				EpicSynth1.SetOscCents(1, Oscillator2Cents);
			}

			if (*Inputs.Osc2PulseWidth != Oscillator2PulseWidth)
			{
				Oscillator2PulseWidth = *Inputs.Osc2PulseWidth;
				EpicSynth1.SetOscPulseWidth(1, Oscillator2PulseWidth);
			}

			 // Check for changes in oscillator parameters
			if (*Inputs.Osc1Gain != Oscillator1Gain)
			{
				Oscillator1Gain = *Inputs.Osc1Gain;
				EpicSynth1.SetOscGain(0, Oscillator1Gain);
			}

			if (*Inputs.Osc1Octave != Oscillator1Octave)
			{
				Oscillator1Octave = *Inputs.Osc1Octave;
				EpicSynth1.SetOscOctave(0, Oscillator1Octave);
			}

			if (*Inputs.Osc1Semitones != Oscillator1Semitones)
			{
				Oscillator1Semitones = *Inputs.Osc1Semitones;
				EpicSynth1.SetOscSemitones(0, Oscillator1Semitones);
			}

			if (*Inputs.Osc2Gain != Oscillator2Gain)
			{
				Oscillator2Gain = *Inputs.Osc2Gain;
				EpicSynth1.SetOscGain(1, Oscillator2Gain);
			}

			if (*Inputs.Osc2Octave != Oscillator2Octave)
			{
				Oscillator2Octave = *Inputs.Osc2Octave;
				EpicSynth1.SetOscOctave(1, Oscillator2Octave);
			}

			if (*Inputs.Osc2Semitones != Oscillator2Semitones)
			{
				Oscillator2Semitones = *Inputs.Osc2Semitones;
				EpicSynth1.SetOscSemitones(1, Oscillator2Semitones);
			}

			// Global synth parameters
			if (*Inputs.Portamento != SynthPortamento)
			{
				SynthPortamento = *Inputs.Portamento;
				// EpicSynth1.SetPortamento(SynthPortamento);
			}

			if (*Inputs.EnableUnison != bEnableUnison)
			{
				bEnableUnison = *Inputs.EnableUnison;
				EpicSynth1.SetOscUnison(bEnableUnison);
			}

			if (*Inputs.EnableOscSync != bEnableOscSync)
			{
				bEnableOscSync = *Inputs.EnableOscSync;
				// EpicSynth1.SetEnableOscillatorSync(bEnableOscSync);
			}

			if (*Inputs.Spread != OscSpread)
			{
				OscSpread = *Inputs.Spread;
				EpicSynth1.SetOscSpread(OscSpread);
			}

			if (*Inputs.Pan != SynthPan)
			{
				SynthPan = *Inputs.Pan;
				EpicSynth1.SetPan(SynthPan);
			}

			if (*Inputs.GainDb != SynthGainDb)
			{
				SynthGainDb = *Inputs.GainDb;
				EpicSynth1.SetGainDb(SynthGainDb);
			}

			// Envelope parameters
			if (*Inputs.AttackTime != EnvAttackTime)
			{
				EnvAttackTime = *Inputs.AttackTime;
				EpicSynth1.SetEnvAttackTime(EnvAttackTime);
			}

			if (*Inputs.DecayTime != EnvDecayTime)
			{
				EnvDecayTime = *Inputs.DecayTime;
				EpicSynth1.SetEnvDecayTime(EnvDecayTime);
			}

			if (*Inputs.SustainGain != EnvSustainGain)
			{
				EnvSustainGain = *Inputs.SustainGain;
				EpicSynth1.SetEnvSustainGain(EnvSustainGain);
			}

			if (*Inputs.ReleaseTime != EnvReleaseTime)
			{
				EnvReleaseTime = *Inputs.ReleaseTime;
				EpicSynth1.SetEnvReleaseTime(EnvReleaseTime);
			}

			if (*Inputs.EnableLegato != bEnableLegato)
			{
				bEnableLegato = *Inputs.EnableLegato;
				EpicSynth1.SetEnvLegatoEnabled(bEnableLegato);
			}

			if (*Inputs.EnableRetrigger != bEnableRetrigger)
			{
				bEnableRetrigger = *Inputs.EnableRetrigger;
				EpicSynth1.SetEnvRetriggerMode(bEnableRetrigger);
			}

			// Filter parameters
			if (*Inputs.FilterFrequency != FilterFrequency)
			{
				FilterFrequency = *Inputs.FilterFrequency;
				EpicSynth1.SetFilterFrequency(FilterFrequency);
			}

			if (*Inputs.FilterQ != FilterQ)
			{
				FilterQ = *Inputs.FilterQ;
				EpicSynth1.SetFilterQ(FilterQ);
			}

			if (*Inputs.FilterType.Get() != FilterTypeEnum)
			{
				FilterTypeEnum = *Inputs.FilterType.Get();
				// EpicSynth1.SetFilterType(FilterTypeEnum);
				EpicSynth1.SetFilterType(Audio::EFilter::LowPass);
			}

			if (*Inputs.FilterAlgorithm.Get() != FilterAlgorithmEnum)
			{
				FilterAlgorithmEnum = *Inputs.FilterAlgorithm.Get();
				EpicSynth1.SetFilterAlgorithm(FilterAlgorithmEnum);
			}

			// LFO1 parameters
			if (*Inputs.LFO1Frequency != LFO1Frequency)
			{
				LFO1Frequency = *Inputs.LFO1Frequency;
				// EpicSynth1.SetLFO1Frequency(LFO1Frequency);
			}

			if (*Inputs.LFO1Gain != LFO1Gain)
			{
				LFO1Gain = *Inputs.LFO1Gain;
				// EpicSynth1.SetLFO1Gain(LFO1Gain);
			}

			if (*Inputs.LFO1Type.Get() != LFO1TypeEnum)
			{
				LFO1TypeEnum = *Inputs.LFO1Type.Get();
				// EpicSynth1.SetLFO1Type((Audio::ELFO::Type)LFO1TypeEnum);
			}

			if (*Inputs.LFO1Mode.Get() != LFO1ModeEnum)
			{
				LFO1ModeEnum = *Inputs.LFO1Mode.Get();
				// EpicSynth1.SetLFO1Mode((Audio::ELFO::Mode)LFO1ModeEnum);
			}

			if (*Inputs.LFO1PatchType.Get() != LFO1PatchTypeEnum)
			{
				LFO1PatchTypeEnum = *Inputs.LFO1PatchType.Get();
				// EpicSynth1.SetLFO1PatchType((Audio::ELFOPatchType::Type)LFO1PatchTypeEnum);
			}

			// LFO2 parameters
			if (*Inputs.LFO2Frequency != LFO2Frequency)
			{
				LFO2Frequency = *Inputs.LFO2Frequency;
				// EpicSynth1.SetLFO2Frequency(LFO2Frequency);
			}

			if (*Inputs.LFO2Gain != LFO2Gain)
			{
				LFO2Gain = *Inputs.LFO2Gain;
				// EpicSynth1.SetLFO2Gain(LFO2Gain);
			}

			if (*Inputs.LFO2Type.Get() != LFO2TypeEnum)
			{
				LFO2TypeEnum = *Inputs.LFO2Type.Get();
				// EpicSynth1.SetLFO2Type((Audio::ELFO::Type)LFO2TypeEnum);
			}

			if (*Inputs.LFO2Mode.Get() != LFO2ModeEnum)
			{
				LFO2ModeEnum = *Inputs.LFO2Mode.Get();
				// EpicSynth1.SetLFO2Mode((Audio::ELFO::Mode)LFO2ModeEnum);
			}

			if (*Inputs.LFO2PatchType.Get() != LFO2PatchTypeEnum)
			{
				LFO2PatchTypeEnum = *Inputs.LFO2PatchType.Get();
				// EpicSynth1.SetLFO2PatchType((Audio::ELFOPatchType::Type)LFO2PatchTypeEnum);
			}

			// Stereo Delay parameters
			if (*Inputs.EnableStereoDelay != bEnableStereoDelay)
			{
				bEnableStereoDelay = *Inputs.EnableStereoDelay;
				EpicSynth1.SetStereoDelayIsEnabled(bEnableStereoDelay);
			}

			if (*Inputs.StereoDelayMode.Get() != StereoDelayModeEnum)
			{
				StereoDelayModeEnum = *Inputs.StereoDelayMode.Get();
				EpicSynth1.SetStereoDelayMode((Audio::EStereoDelayMode::Type)StereoDelayModeEnum);
			}

			if (*Inputs.StereoDelayTime != StereoDelayTime)
			{
				StereoDelayTime = *Inputs.StereoDelayTime;
				// EpicSynth1.SetStereoDelayTime(StereoDelayTime);
			}

			if (*Inputs.StereoDelayFeedback != StereoDelayFeedback)
			{
				StereoDelayFeedback = *Inputs.StereoDelayFeedback;
				EpicSynth1.SetStereoDelayFeedback(StereoDelayFeedback);
			}

			if (*Inputs.StereoDelayWetLevel != StereoDelayWetLevel)
			{
				StereoDelayWetLevel = *Inputs.StereoDelayWetLevel;
				EpicSynth1.SetStereoDelayWetLevel(StereoDelayWetLevel);
			}

			if (*Inputs.StereoDelayRatio != StereoDelayRatio)
			{
				StereoDelayRatio = *Inputs.StereoDelayRatio;
				EpicSynth1.SetStereoDelayRatio(StereoDelayRatio);
			}

			// Chorus parameters
			if (*Inputs.EnableChorus != bEnableChorus)
			{
				bEnableChorus = *Inputs.EnableChorus;
				EpicSynth1.SetChorusEnabled(bEnableChorus);
			}

			if (*Inputs.ChorusDepth != ChorusDepth)
			{
				ChorusDepth = *Inputs.ChorusDepth;
				// EpicSynth1.SetChorusDepth(ChorusDepth);
			}

			if (*Inputs.ChorusFeedback != ChorusFeedback)
			{
				ChorusFeedback = *Inputs.ChorusFeedback;
				// EpicSynth1.SetChorusFeedback(ChorusFeedback);
			}

			if (*Inputs.ChorusFrequency != ChorusFrequency)
			{
				ChorusFrequency = *Inputs.ChorusFrequency;
				// EpicSynth1.SetChorusFrequency(ChorusFrequency);
			}

			//Filter.SetFilterValues(*Inputs.MinTrackIndex, *Inputs.MaxTrackIndex, false);
			//Filter.SetFilterValues(*Inputs.MinTrackIndex, *Inputs.MaxTrackIndex, false);
			//Outputs.MidiStream->PrepareBlock();

			// create an iterator for midi events in the block
			const TArray<FMidiStreamEvent>& MidiEvents = Inputs.MidiStream->GetEventsInBlock();
			auto MidiEventIterator = MidiEvents.begin();
			// create an iterator for the midi clock 
			const TSharedPtr<const FMidiClock, ESPMode::NotThreadSafe> MidiClock = Inputs.MidiStream->GetClock();
			int32 FramesRequired = 1;
			//while (FramesRequired > 0)
			{
				while (MidiEventIterator != MidiEvents.end())
				{
					{
						const FMidiMsg& MidiMessage = (*MidiEventIterator).MidiMessage;
						if (MidiMessage.IsStd()  && (*MidiEventIterator).TrackIndex == CurrentTrackNumber)
						{
							
							HandleMidiMessage(
								(*MidiEventIterator).GetVoiceId(),
								MidiMessage.GetStdStatus(),
								MidiMessage.GetStdData1(),
								MidiMessage.GetStdData2(),
								(*MidiEventIterator).AuthoredMidiTick,
								(*MidiEventIterator).CurrentMidiTick,
								0.0f);
						}
						else if (MidiMessage.IsAllNotesOff())
						{
							//AllNotesOff();
						}
						else if (MidiMessage.IsAllNotesKill())
						{
							//KillAllVoices();
						}
						++MidiEventIterator;

					}

				}
			}
			if (MidiClock.IsValid())
			{
				const float ClockSpeed = MidiClock->GetSpeedAtBlockSampleFrame(0);
				//SetSpeed(ClockSpeed, !(*ClockSpeedAffectsPitchInPin));
				const float ClockTempo = MidiClock->GetTempoAtBlockSampleFrame(0);
				//sfizz_send_bpm_tempo(SfizzSynth, 0, ClockTempo);
				//SetTempo(ClockTempo);
				//const float Beat = MidiClock->GetQuarterNoteIncludingCountIn();
				//SetBeat(Beat);
			}

			

			FScopeLock Lock(&EpicSynth1NodeCritSection);
			//apply pitchbend

			PitchBendRamper.Ramp();
			EpicSynth1.SetOscPitchBend(0, PitchBendRamper.GetCurrent());
			EpicSynth1.SetOscPitchBend(1, PitchBendRamper.GetCurrent());
			//UE_LOG(LogEpicSynth1Node, VeryVerbose, TEXT("Pitch Bend: %f"), PitchBendRamper.GetCurrent());
			//acquire samples from synth
			for (int32 SampleIndex = 0; SampleIndex < BlockSizeFrames; ++SampleIndex)
			{
				EpicSynth1.GenerateFrame(&DecodedAudioDataBuffer.data()[SampleIndex]);
				AudioOutLeft->GetData()[SampleIndex] = DecodedAudioDataBuffer.data()[SampleIndex];
				AudioOutRight->GetData()[SampleIndex] = DecodedAudioDataBuffer.data()[SampleIndex + 1];
			}




		}
	private:
		FInputs Inputs;
	//	FOutputs Outputs;


		struct FPendingNoteAction
		{
			int8  MidiNote = 0;
			int8  Velocity = 0;
			int32 EventTick = 0;
			int32 TriggerTick = 0;
			float OffsetMs = 0.0f;
			int32 FrameOffset = 0;
			FMidiVoiceId VoiceId;
		};
		struct FMIDINoteStatus
		{
			// is the key pressed down?
			bool KeyedOn = false;

			// is there any sound coming out of this note? (release could mean key off but voices active)
			int32 NumActiveVoices = 0;
		};


		//stuff copied from the fusion sampler...
		FCriticalSection sNoteActionCritSec;
		FCriticalSection EpicSynth1NodeCritSection;
		FCriticalSection sNoteStatusCritSec;
		static const int8 kNoteIgnore = -1;
		static const int8 kNoteOff = 0;
		static const int32 kMaxLayersPerNote = 128;
		Harmonix::Midi::Ops::FStuckNoteGuard StuckNoteGuard;
		FSampleRate SampleRate;
		//** DATA
		int32 FramesPerBlock = 0;
		int32 CurrentTrackNumber = 0;
		int32 CurrentChannelNumber = 0;
		bool MadeAudioLastFrame = false;
		TArray<FPendingNoteAction> PendingNoteActions;
		FMIDINoteStatus NoteStatus[Harmonix::Midi::Constants::GMaxNumNotes];

		bool bIsSustainPedalDown = false;

		//pitch bend
		// on range [-1, 1]
		TLinearRamper<float> PitchBendRamper;
		// extra pitch bend in semitones
		float ExtraPitchBend = 0.0f;
		float PitchBendFactor = 1.0f;
		float FineTuneCents = 0.0f;
		
		//sfizz stuff
		//sfizz_synth_t* SfizzSynth;

		std::vector<float>   DecodedAudioDataBuffer;
		std::vector<float*>  DeinterleavedBuffer;
	


		int32 VoiceCount = 32;

		
		FAudioBufferWriteRef AudioOutLeft;
		FAudioBufferWriteRef AudioOutRight;
		//unDAWMetasounds::TrackIsolatorOP::FMidiTrackIsolator Filter;



		protected:
			Audio::FEpicSynth1 EpicSynth1;

			bool bIsMono = false;
			bool bIsLegato = false;

			EMetasoundSynth1OscType Oscillator1TypeEnum = EMetasoundSynth1OscType::Saw;
			EMetasoundSynth1OscType Oscillator2TypeEnum = EMetasoundSynth1OscType::Saw;

			float Oscillator1Cents = 0.0f;
			float Oscillator2Cents = 0.0f;

			float Oscillator1PulseWidth = 0.5f;
			float Oscillator2PulseWidth = 0.5f;

			float Oscillator1Gain = 1.0f;
			float Oscillator1Octave = 0.0f;
			float Oscillator1Semitones = 0.0f;
			float Oscillator2Gain = 1.0f;
			float Oscillator2Octave = 0.0f;
			float Oscillator2Semitones = 0.0f;
			float SynthPortamento = 0.0f;
			bool bEnableUnison = false;
			bool bEnableOscSync = false;
			float OscSpread = 0.5f;
			float SynthPan = 0.0f;
			float SynthGainDb = -3.0f;
			float EnvAttackTime = 10.0f;
			float EnvDecayTime = 100.0f;
			float EnvSustainGain = 0.707f;
			float EnvReleaseTime = 5000.0f;
			bool bEnableLegato = true;
			bool bEnableRetrigger = false;
			float FilterFrequency = 8000.0f;
			float FilterQ = 2.0f;
			EMetasoundSynthFilterType FilterTypeEnum = EMetasoundSynthFilterType::LowPass;
			EMetasoundSynthFilterAlgorithm FilterAlgorithmEnum = EMetasoundSynthFilterAlgorithm::OnePole;
			float LFO1Frequency = 1.0f;
			float LFO1Gain = 0.0f;
			EMetasoundSynthLFOType LFO1TypeEnum = EMetasoundSynthLFOType::Sine;
			EMetasoundSynthLFOMode LFO1ModeEnum = EMetasoundSynthLFOMode::Sync;
			EMetasoundSynthLFOPatchType LFO1PatchTypeEnum = EMetasoundSynthLFOPatchType::PatchToNone;
			float LFO2Frequency = 1.0f;
			float LFO2Gain = 0.0f;
			EMetasoundSynthLFOType LFO2TypeEnum = EMetasoundSynthLFOType::Sine;
			EMetasoundSynthLFOMode LFO2ModeEnum = EMetasoundSynthLFOMode::Sync;
			EMetasoundSynthLFOPatchType LFO2PatchTypeEnum = EMetasoundSynthLFOPatchType::PatchToNone;
			bool bEnableStereoDelay = true;
			EMetasoundSynthStereoDelayMode StereoDelayModeEnum = EMetasoundSynthStereoDelayMode::Normal;
			float StereoDelayTime = 700.0f;
			float StereoDelayFeedback = 0.7f;
			float StereoDelayWetLevel = 0.3f;
			float StereoDelayRatio = 0.2f;
			bool bEnableChorus = false;
			float ChorusDepth = 0.2f;
			float ChorusFeedback = 0.5f;
			float ChorusFrequency = 2.0f;

			bool bEpic1SynthCreated = false;


		
	

	};

	class FEpic1SynthNode final : public FNodeFacade
	{
	public:
		explicit FEpic1SynthNode(const FNodeInitData& InInitData)
			: FNodeFacade(InInitData.InstanceName, InInitData.InstanceID, TFacadeOperatorClass<FEpic1SynthMetasoundOperator>())
		{}
		virtual ~FEpic1SynthNode() override = default;
	};

	METASOUND_REGISTER_NODE(FEpic1SynthNode)
}

#undef LOCTEXT_NAMESPACE // "HarmonixMetaSound"