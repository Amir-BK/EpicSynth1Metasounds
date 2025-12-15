// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once
#include "MetasoundDataTypeRegistrationMacro.h"
#include "EpicSynth1Types.generated.h"

UENUM(BlueprintType)
enum class EMetasoundSynth1OscType : uint8
{
	Sine = 0,
	Saw,
	Triangle,
	Square,
	Noise,
	Count UMETA(Hidden)
};

UENUM(BlueprintType)
enum class EMetasoundSynthLFOType : uint8
{
	Sine = 0,
	UpSaw,
	DownSaw,
	Square,
	Triangle,
	Exponential,
	RandomSampleHold,
	Count UMETA(Hidden)
};

UENUM(BlueprintType)
enum class EMetasoundSynthLFOMode : uint8
{
	Sync = 0,
	OneShot,
	Free,
	Count UMETA(Hidden)
};

UENUM(BlueprintType)
enum class EMetasoundSynthLFOPatchType : uint8
{
	PatchToNone = 0,

	PatchToGain,
	PatchToOscFreq,
	PatchToFilterFreq,
	PatchToFilterQ,
	PatchToOscPulseWidth,
	PatchToOscPan,
	PatchLFO1ToLFO2Frequency,
	PatchLFO1ToLFO2Gain,
	Count UMETA(Hidden)
};

UENUM(BlueprintType)
enum class EMetasoundSynthModEnvPatch : uint8
{
	PatchToNone = 0,

	PatchToOscFreq,
	PatchToFilterFreq,
	PatchToFilterQ,
	PatchToLFO1Gain,
	PatchToLFO2Gain,
	PatchToLFO1Freq,
	PatchToLFO2Freq,
	Count UMETA(Hidden)
};

UENUM(BlueprintType)
enum class EMetasoundSynthModEnvBiasPatch : uint8
{
	PatchToNone = 0,

	PatchToOscFreq,
	PatchToFilterFreq,
	PatchToFilterQ,
	PatchToLFO1Gain,
	PatchToLFO2Gain,
	PatchToLFO1Freq,
	PatchToLFO2Freq,
	Count UMETA(Hidden)
};

UENUM(BlueprintType)
enum class EMetasoundSynthFilterType : uint8
{
	LowPass = 0,
	HighPass,
	BandPass,
	BandStop,
	Count UMETA(Hidden)
};

UENUM(BlueprintType)
enum class EMetasoundSynthFilterAlgorithm : uint8
{
	OnePole = 0,
	StateVariable,
	Ladder,
	Count UMETA(Hidden)
};

UENUM(BlueprintType)
enum class EMetasoundSynthStereoDelayMode : uint8
{
	Normal = 0,
	Cross,
	PingPong,
	Count UMETA(Hidden)
};

UENUM(BlueprintType)
enum class EMetasoundSynth1PatchSource : uint8
{
	LFO1,
	LFO2,
	Envelope,
	BiasEnvelope,
	Count UMETA(Hidden)
};

UENUM(BlueprintType)
enum class EMetasoundSynth1PatchDestination : uint8
{
	Osc1Gain,
	Osc1Frequency,
	Osc1Pulsewidth,
	Osc2Gain,
	Osc2Frequency,
	Osc2Pulsewidth,
	FilterFrequency,
	FilterQ,
	Gain,
	Pan,
	LFO1Frequency,
	LFO1Gain,
	LFO2Frequency,
	LFO2Gain,
	Count UMETA(Hidden)
};


namespace Metasound
{
	struct EPICSYNTHSMETASOUNDS_API FMetasoundSynth1PatchCable
	{
		
	public:
		FMetasoundSynth1PatchCable() = default;
		FMetasoundSynth1PatchCable(const FMetasoundSynth1PatchCable& InOther) = default;
		FMetasoundSynth1PatchCable& operator=(const FMetasoundSynth1PatchCable& InOther) = default;

		FMetasoundSynth1PatchCable(EMetasoundSynth1PatchSource InSource, EMetasoundSynth1PatchDestination InDestination, float InDepth)
			: Source(InSource)
			, Destination(InDestination)
			, Depth(InDepth)
		{
		}

		// The patch depth (how much the modulator modulates the destination)

		float Depth = 0.0f;

		// The patch destination type

		EMetasoundSynth1PatchDestination Destination = EMetasoundSynth1PatchDestination::Osc1Gain;
		EMetasoundSynth1PatchSource Source = EMetasoundSynth1PatchSource::LFO1;

	};

	DECLARE_METASOUND_DATA_REFERENCE_TYPES(FMetasoundSynth1PatchCable, EPICSYNTHSMETASOUNDS_API, FMetasoundSynth1PatchCableTypeInfo, FMetasoundSynth1PatchCableReadRef, FMetasoundSynth1PatchCableWriteRef)
}

//REGISTER_METASOUND_DATATYPE(Metasound::FMetasoundSynth1PatchCable, "Synth Patch", ::Metasound::ELiteralType::Invalid)

struct FMetasoundPatchId
{

	int32 Id = INDEX_NONE;
};

