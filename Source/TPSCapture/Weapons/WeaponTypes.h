#pragma once

#include "CoreMinimal.h"
#include "WeaponTypes.generated.h"

UENUM(BlueprintType)
enum class EWeaponType : uint8
{
	None		UMETA(DisplayName = "None"),
	Stick		UMETA(DisplayName = "Stick"),
	Spear		UMETA(DisplayName = "Spear"),
	Sword		UMETA(DisplayName = "Sword"),
	Bow			UMETA(DisplayName = "Bow"),
	Pickaxe		UMETA(DisplayName = "Pickaxe"),
	Axe			UMETA(DisplayName = "Axe")
};

UENUM(BlueprintType)
enum class EAttackType : uint8
{
	Unarmed		UMETA(DisplayName = "Unarmed"),
	Melee		UMETA(DisplayName = "Melee"),
	Ranged		UMETA(DisplayName = "Ranged"),
	Throw		UMETA(DisplayName = "Throw")
};