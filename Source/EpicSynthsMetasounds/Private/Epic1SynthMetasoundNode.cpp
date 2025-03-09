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
		DEFINE_INPUT_METASOUND_PARAM(Osc1Cents, "Osc1 Cents", "Osc1 Cents");
		DEFINE_INPUT_METASOUND_PARAM(Osc1PulseWidth, "Osc1 Pulse Width", "Osc1 Pulse Width");
		DEFINE_INPUT_METASOUND_PARAM(Osc2Cents, "Osc2 Cents", "Osc2 Cents");
		DEFINE_INPUT_METASOUND_PARAM(Osc2PulseWidth, "Osc2 Pulse Width", "Osc2 Pulse Width");
		//DEFINE_INPUT_METASOUND_PARAM(IncludeConductorTrack, "Include Conductor Track", "Enable to include the conductor track (AKA track 0)");
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
					TInputDataVertex<bool>(METASOUND_GET_PARAM_NAME_AND_METADATA(Inputs::Enable), true),
					TInputDataVertex<FMidiStream>(METASOUND_GET_PARAM_NAME_AND_METADATA(Inputs::MidiStream)),
					TInputDataVertex<int32>(METASOUND_GET_PARAM_NAME_AND_METADATA(Inputs::MinTrackIndex), 1),
					TInputDataVertex<FEnumEpicsynth1Osc>(METASOUND_GET_PARAM_NAME_AND_METADATA(Inputs::Voice1OscType)),
					TInputDataVertex<FEnumEpicsynth1Osc>(METASOUND_GET_PARAM_NAME_AND_METADATA(Inputs::Voice2OscType)),
					TInputDataVertex<bool>(METASOUND_GET_PARAM_NAME_AND_METADATA(Inputs::Monophonic), false),
					TInputDataVertex<float>(METASOUND_GET_PARAM_NAME_AND_METADATA(Inputs::Osc1Cents), 0.0f),
					TInputDataVertex<float>(METASOUND_GET_PARAM_NAME_AND_METADATA(Inputs::Osc1PulseWidth), 0.5f),
					TInputDataVertex<float>(METASOUND_GET_PARAM_NAME_AND_METADATA(Inputs::Osc2Cents), 0.0f),
					TInputDataVertex<float>(METASOUND_GET_PARAM_NAME_AND_METADATA(Inputs::Osc2PulseWidth), 0.5f)

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
			FBoolReadRef Enabled;
			FMidiStreamReadRef MidiStream;
			FInt32ReadRef MinTrackIndex;

			FEnumEpicsynth1OscReadRef Voice1OscillatorTypeRef;
			FEnumEpicsynth1OscReadRef Voice2OscillatorTypeRef;
			FBoolReadRef bIsMonoRef;

			FFloatReadRef Osc1Cents;
			FFloatReadRef Osc1PulseWidth;
			FFloatReadRef Osc2Cents;
			FFloatReadRef Osc2PulseWidth;


		};


		static TUniquePtr<IOperator> CreateOperator(const FBuildOperatorParams& InParams, FBuildResults& OutResults)
		{
			const FInputVertexInterfaceData& InputData = InParams.InputData;

			FInputs Inputs
			{
				InputData.GetOrCreateDefaultDataReadReference<bool>(Inputs::EnableName, InParams.OperatorSettings),
				InputData.GetOrConstructDataReadReference<FMidiStream>(Inputs::MidiStreamName),
				InputData.GetOrCreateDefaultDataReadReference<int32>(Inputs::MinTrackIndexName, InParams.OperatorSettings),
				InputData.GetOrConstructDataReadReference<FEnumEpicsynth1Osc>(Inputs::Voice1OscTypeName),
				InputData.GetOrConstructDataReadReference<FEnumEpicsynth1Osc>(Inputs::Voice2OscTypeName),
				InputData.GetOrCreateDefaultDataReadReference<bool>(Inputs::MonophonicName, InParams.OperatorSettings),
				InputData.GetOrCreateDefaultDataReadReference<float>(Inputs::Osc1CentsName, InParams.OperatorSettings),
				InputData.GetOrCreateDefaultDataReadReference<float>(Inputs::Osc1PulseWidthName, InParams.OperatorSettings),
				InputData.GetOrCreateDefaultDataReadReference<float>(Inputs::Osc2CentsName, InParams.OperatorSettings),
				InputData.GetOrCreateDefaultDataReadReference<float>(Inputs::Osc2PulseWidthName, InParams.OperatorSettings)


			};

			// outputs
			FOutputVertexInterface OutputInterface;


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
			InVertexData.BindReadVertex(Inputs::EnableName, Inputs.Enabled);
			InVertexData.BindReadVertex(Inputs::MidiStreamName, Inputs.MidiStream);
			InVertexData.BindReadVertex(Inputs::MinTrackIndexName, Inputs.MinTrackIndex);
			InVertexData.BindReadVertex(Inputs::Voice1OscTypeName, Inputs.Voice1OscillatorTypeRef);
			InVertexData.BindReadVertex(Inputs::MonophonicName, Inputs.bIsMonoRef);
			InVertexData.BindReadVertex(Inputs::Osc1CentsName, Inputs.Osc1Cents);
			InVertexData.BindReadVertex(Inputs::Osc1PulseWidthName, Inputs.Osc1PulseWidth);
			InVertexData.BindReadVertex(Inputs::Osc2CentsName, Inputs.Osc2Cents);
			InVertexData.BindReadVertex(Inputs::Osc2PulseWidthName, Inputs.Osc2PulseWidth);

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
		
				EpicSynth1.NoteOff(InData1);
				//UE_LOG(LogEpicSynth1Node, VeryVerbose, TEXT("Note Off: %d"), InData1);
				break;
			case GNoteOn:
				EpicSynth1.NoteOn(InData1, (float) InData2);
				break;
			case GPolyPres:

		
				break;
			case GChanPres:
				break;
			case GControl:
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
					//NoteOff(Event.GetVoiceId(), Event.MidiMessage.GetStdData1(), Event.MidiMessage.GetStdChannel());
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