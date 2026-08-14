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
            "RescueAnimal",
            "RescueAnimal/Animals",
            "RescueAnimal/Animals/Components",
            "RescueAnimal/Camp",
            "RescueAnimal/Characters",
            "RescueAnimal/Characters/Player",
            "RescueAnimal/Characters/Player/Components",
            "RescueAnimal/Characters/Enemies",
            "RescueAnimal/Characters/Enemies/Components",
            "RescueAnimal/Cores",
            "RescueAnimal/Cores/Audio",
            "RescueAnimal/Cores/GameInstance",
            "RescueAnimal/Cores/GameMode",
            "RescueAnimal/Cores/PlayerController",
            "RescueAnimal/Cores/PlayerController/Components",
            "RescueAnimal/Cores/WorldState",
            "RescueAnimal/Creatures",
            "RescueAnimal/Data",
            "RescueAnimal/Interactables",
            "RescueAnimal/Interactables/Items",
            "RescueAnimal/Interactables/NPC",
            "RescueAnimal/Interactables/Portal",
            "RescueAnimal/Interactables/Shop",
            "RescueAnimal/UI",
            "RescueAnimal/UI/Animal",
            "RescueAnimal/UI/Audio",
            "RescueAnimal/UI/Buff",
            "RescueAnimal/UI/Dialogue",
            "RescueAnimal/UI/Enemy",
            "RescueAnimal/UI/GameFlow",
            "RescueAnimal/UI/HUD",
            "RescueAnimal/UI/Inventory",
            "RescueAnimal/UI/Settings",
            "RescueAnimal/UI/Shop",
            "RescueAnimal/UI/Skill",
            "RescueAnimal/Weapons",
            "RescueAnimal/Weapons/Projectiles"
        });
    }
}
