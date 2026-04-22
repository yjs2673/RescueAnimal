// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class TPSCapture : ModuleRules
{
	public TPSCapture(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

        PublicDependencyModuleNames.AddRange(new string[] {
            "Core",
            "CoreUObject",
            "Engine",
            "InputCore",
            "EnhancedInput",
            "UMG",
            "Niagara",
            "AIModule",
            "NavigationSystem"
        });

        PrivateDependencyModuleNames.AddRange(new string[] {
            "Slate",
            "SlateCore",
        });

        PublicIncludePaths.AddRange(new string[] {
            "TPSCapture/Characters",
            "TPSCapture/Characters/Player",
            "TPSCapture/Characters/Enemies",
            "TPSCapture/Components",
            "TPSCapture/Cores",
            "TPSCapture/Creatures",
            "TPSCapture/Creatures/Animals",
            "TPSCapture/Creatures/Enemies",
            "TPSCapture/Data",
            "TPSCapture/Interactables",
            "TPSCapture/UI",
            "TPSCapture/Weapons"
        });
    }
}
