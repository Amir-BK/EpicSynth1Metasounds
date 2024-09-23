// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;
using System.IO;

public class EpicSynthsMetasounds : ModuleRules
{
	public EpicSynthsMetasounds(ReadOnlyTargetRules Target) : base(Target)
	{

		PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;
		IWYUSupport = IWYUSupport.None;

		PublicIncludePaths.AddRange(
			new string[] {
				// ... add public include paths required here ...
			}
			);
				
		
		PrivateIncludePaths.AddRange(
			new string[] {
				// ... add other private include paths required here ...
			}
			);
			
		
		PublicDependencyModuleNames.AddRange(
			new string[]
			{
				"Core",
				"Synthesis",
				"AudioExtensions",
				"CoreUObject",
				// ... add other public dependencies that you statically link with here ...
			}
			);
			
		
		PrivateDependencyModuleNames.AddRange(
			new string[]
			{
			
				"Engine",
				"Slate",
				"SlateCore",
			
				"AudioMixer",
				"AudioMixerCore",
				"SignalProcessing",
				"MetasoundStandardNodes",
                "HarmonixDsp",
                "HarmonixMidi",
                "Harmonix",
                "HarmonixMetasound",
				"MetasoundEngine",
				"MetasoundGraphCore",
				"MetasoundFrontend",
				"MetasoundGenerator",
				"InputCore",
				"Projects",
				// ... add private dependencies that you statically link with here ...	
			}
			);


		if (Target.Type == TargetType.Editor)
		{
			PrivateDependencyModuleNames.Add("AudioSynesthesiaCore");
		}

		DynamicallyLoadedModuleNames.AddRange(
			new string[]
			{
				// ... add any modules that your module loads dynamically here ...
			}
			);
	}
}
