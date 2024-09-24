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
//#include "Sfizz.h"
#include <vector>
//#include "MidiStreamTrackIsolatorNode.h"

//#include "SfizzSynthNode.h"
//#include "MidiTrackIsolator.h"

DEFINE_LOG_CATEGORY_STATIC(LogEpicSynth1Node, VeryVerbose, All);

#define LOCTEXT_NAMESPACE "EpicSynthsMetasounds_Epic1SynthNode"

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
		DEFINE_INPUT_METASOUND_PARAM(MaxTrackIndex, "Channel Index", "Channel");
		DEFINE_INPUT_METASOUND_PARAM(SfzLibPath, "Sfz Lib Path", "Absolute path to the Sfz lib, passed to the Sfizz synth")
		DEFINE_INPUT_METASOUND_PARAM(ScalaFilePath, "Scala File Path", "Optional path to Scala file")
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
					TInputDataVertex<int32>(METASOUND_GET_PARAM_NAME_AND_METADATA(Inputs::MinTrackIndex), 0),
					TInputDataVertex<int32>(METASOUND_GET_PARAM_NAME_AND_METADATA(Inputs::MaxTrackIndex), 0),
					TInputDataVertex<FString>(METASOUND_GET_PARAM_NAME_AND_METADATA(Inputs::SfzLibPath)),
					TInputDataVertex<FString>(METASOUND_GET_PARAM_NAME_AND_METADATA(Inputs::ScalaFilePath))
	
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
			FInt32ReadRef MaxTrackIndex;
			FStringReadRef SfzLibPath;
			FStringReadRef ScalaFilePath;
		};


		static TUniquePtr<IOperator> CreateOperator(const FBuildOperatorParams& InParams, FBuildResults& OutResults)
		{
			const FInputVertexInterfaceData& InputData = InParams.InputData;

			FInputs Inputs
			{
				InputData.GetOrCreateDefaultDataReadReference<bool>(Inputs::EnableName, InParams.OperatorSettings),
				InputData.GetOrConstructDataReadReference<FMidiStream>(Inputs::MidiStreamName),
				InputData.GetOrCreateDefaultDataReadReference<int32>(Inputs::MinTrackIndexName, InParams.OperatorSettings),
				InputData.GetOrCreateDefaultDataReadReference<int32>(Inputs::MaxTrackIndexName, InParams.OperatorSettings),
				InputData.GetOrCreateDefaultDataReadReference<FString>(Inputs::SfzLibPathName, InParams.OperatorSettings),
				InputData.GetOrCreateDefaultDataReadReference<FString>(Inputs::ScalaFilePathName, InParams.OperatorSettings)
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
			InVertexData.BindReadVertex(Inputs::MaxTrackIndexName, Inputs.MaxTrackIndex);
			InVertexData.BindReadVertex(Inputs::SfzLibPathName, Inputs.SfzLibPath);
			InVertexData.BindReadVertex(Inputs::ScalaFilePathName, Inputs.ScalaFilePath);
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
				UE_LOG(LogEpicSynth1Node, VeryVerbose, TEXT("Pitch Bend: %d"), InData1);
				
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
				
					
					EpicSynth1.Init(SampleRate, VoiceCount);


					EpicSynth1.SetMonoMode(false);
					EpicSynth1.SetOscType(0, (Audio::EOsc::Type)EMetasoundSynth1OscType::Saw);
					EpicSynth1.SetOscType(1, (Audio::EOsc::Type)EMetasoundSynth1OscType::Saw);
					EpicSynth1.SetOscCents(0, 0.0f);
					EpicSynth1.SetOscPulseWidth(0, 0.5f);
					EpicSynth1.SetOscPulseWidth(1, 0.5f);
					EpicSynth1.SetOscUnison(false);
					EpicSynth1.SetOscSpread(0.5f);
					EpicSynth1.SetGainDb(-3.0f);
					EpicSynth1.SetEnvAttackTime(10.0f);
					EpicSynth1.SetEnvDecayTime(100.0f);
					EpicSynth1.SetEnvSustainGain(0.707f);
					EpicSynth1.SetEnvReleaseTime(5000.0f);
					EpicSynth1.SetEnvLegatoEnabled(true);
					EpicSynth1.SetEnvRetriggerMode(false);
					EpicSynth1.SetFilterFrequency(1200.0f);
					EpicSynth1.SetFilterQ(2.0f);
					EpicSynth1.SetFilterAlgorithm(EMetasoundSynthFilterAlgorithm::Ladder);
					EpicSynth1.SetStereoDelayIsEnabled(true);
					EpicSynth1.SetStereoDelayMode((Audio::EStereoDelayMode::Type)EMetasoundSynthStereoDelayMode::PingPong);
					EpicSynth1.SetStereoDelayRatio(0.2f);
					EpicSynth1.SetStereoDelayFeedback(0.7f);
					EpicSynth1.SetStereoDelayWetLevel(0.3f);
					EpicSynth1.SetChorusEnabled(false);

					const float RampCallRateHz = SampleRate / (float) BlockSizeFrames;

					PitchBendRamper.SetRampTimeMs(RampCallRateHz, 5.0f);
					PitchBendRamper.SetTarget(0.0f);
					PitchBendRamper.SnapToTarget();



					DeinterleavedBuffer.resize(2 * BlockSizeFrames);
					DecodedAudioDataBuffer.resize(2 * BlockSizeFrames);
					//DeinterleavedBuffer[0] = AudioOutLeft->GetData();
					//DeinterleavedBuffer[1] = AudioOutRight->GetData();
	

					CurrentTrackNumber = *Inputs.MinTrackIndex;
					CurrentChannelNumber = *Inputs.MaxTrackIndex;

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
			UE_LOG(LogEpicSynth1Node, VeryVerbose, TEXT("Pitch Bend: %f"), PitchBendRamper.GetCurrent());

			//acquire samples from synth
			for (int32 SampleIndex = 0; SampleIndex < BlockSizeFrames; ++SampleIndex)
			{
			
				//UE_LOG(LogEpicSynth1Node, VeryVerbose, TEXT("Pitch Bend: %f"), PitchBendRamper.GetCurrent());

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
	
		bool bSuccessLoadSFZFile = false;
		bool bEpic1SynthCreated = false;
		FString LibPath;
		FString ScalaPath;

		int32 VoiceCount = 8;

		
		FAudioBufferWriteRef AudioOutLeft;
		FAudioBufferWriteRef AudioOutRight;
		//unDAWMetasounds::TrackIsolatorOP::FMidiTrackIsolator Filter;

		protected:
			Audio::FEpicSynth1 EpicSynth1;

	

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