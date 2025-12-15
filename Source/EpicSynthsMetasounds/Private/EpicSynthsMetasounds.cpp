// Copyright Epic Games, Inc. All Rights Reserved.

#include "EpicSynthsMetasounds.h"
#include "MetasoundEpicSynth1Types.h"
#include "MetasoundDataTypeRegistrationMacro.h"

#define LOCTEXT_NAMESPACE "FEpicSynthsMetasoundsModule"

void FEpicSynthsMetasoundsModule::StartupModule()
{
	// This code will execute after your module is loaded into memory; the exact timing is specified in the .uplugin file per-module
	using namespace Metasound;

	//RegisterDataTypeWithFrontend<FMetasoundSynth1PatchCable>();
	//RegisterDataTypeArrayWithFrontend<FMetasoundSynth1PatchCable>();


}

void FEpicSynthsMetasoundsModule::ShutdownModule()
{
	// This function may be called during shutdown to clean up your module.  For modules that support dynamic reloading,
	// we call this function before unloading the module.
}

#undef LOCTEXT_NAMESPACE
	
IMPLEMENT_MODULE(FEpicSynthsMetasoundsModule, EpicSynthsMetasounds)