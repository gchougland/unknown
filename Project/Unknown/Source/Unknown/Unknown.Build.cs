// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class Unknown : ModuleRules
{
	public Unknown(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
	
  PublicDependencyModuleNames.AddRange(new string[] { "Core", "CoreUObject", "Engine", "InputCore", "EnhancedInput", "GameplayTags", "DeveloperSettings", "UMG", "Slate", "SlateCore", "Niagara" });

  PrivateDependencyModuleNames.AddRange(new string[] { "PhysicsCore", "ImageWrapper", "ImageCore", "RenderCore" });

  // UI dependencies for runtime drawing of selection box via UMG/Slate
  PrivateDependencyModuleNames.AddRange(new string[] { });
		
		// Uncomment if you are using online features
		// PrivateDependencyModuleNames.Add("OnlineSubsystem");

		// To include OnlineSubsystemSteam, add it to the plugins section in your uproject file with the Enabled attribute set to true
	}
}
