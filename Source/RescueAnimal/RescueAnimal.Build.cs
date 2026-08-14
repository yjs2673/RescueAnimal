// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class RescueAnimal : ModuleRules
{
	public RescueAnimal(ReadOnlyTargetRules Target) : base(Target)
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
            "NavigationSystem",
            "DeveloperSettings"
        });

        PrivateDependencyModuleNames.AddRange(new string[] {
            "Slate",
            "SlateCore",
        });

        PublicIncludePaths.AddRange(new string[] {
            "RescueAnimal/Animals",
            "RescueAnimal/Camp",
            "RescueAnimal/Characters",
            "RescueAnimal/Characters/Player",
            "RescueAnimal/Characters/Enemies",
            "RescueAnimal/Components",
            "RescueAnimal/Cores",
            "RescueAnimal/Creatures",
            "RescueAnimal/Data",
            "RescueAnimal/Interactables",
            "RescueAnimal/UI",
            "RescueAnimal/Weapons"
        });
    }
}
