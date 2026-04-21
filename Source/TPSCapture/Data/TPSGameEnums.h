#pragma once

#include "CoreMinimal.h"
#include "TPSGameEnums.generated.h"

UENUM(BlueprintType)
enum class EItemType : uint8
{
	None			UMETA(DisplayName = "None"),
	Material		UMETA(DisplayName = "Material"),
	Consumable		UMETA(DisplayName = "Consumable"),
	Weapon			UMETA(DisplayName = "Weapon"),
	CaptureTool		UMETA(DisplayName = "Capture Tool"),
	Quest			UMETA(DisplayName = "Quest")
};

UENUM(BlueprintType)
enum class EWeaponType : uint8
{
	None			UMETA(DisplayName = "None"),
	Stick			UMETA(DisplayName = "Stick"),
	Spear			UMETA(DisplayName = "Spear"),
	Sword			UMETA(DisplayName = "Sword"),
	Bow				UMETA(DisplayName = "Bow"),
	Pickaxe			UMETA(DisplayName = "Pickaxe"),
	Axe				UMETA(DisplayName = "Axe")
};

UENUM(BlueprintType)
enum class EAttackType : uint8
{
	Unarmed			UMETA(DisplayName = "Unarmed"),
	Melee			UMETA(DisplayName = "Melee"),
	Ranged			UMETA(DisplayName = "Ranged"),
	Throw			UMETA(DisplayName = "Throw")
};

UENUM(BlueprintType)
enum class ERarity : uint8
{
	Common			UMETA(DisplayName = "Common"),
	Uncommon		UMETA(DisplayName = "Uncommon"),
	Rare			UMETA(DisplayName = "Rare"),
	Epic			UMETA(DisplayName = "Epic"),
	Legendary		UMETA(DisplayName = "Legendary")
};

UENUM(BlueprintType)
enum class EConsumableType : uint8
{
	None			UMETA(DisplayName = "None"),
	Heal			UMETA(DisplayName = "Heal"),
	Buff			UMETA(DisplayName = "Buff"),
	Capture			UMETA(DisplayName = "Capture")
};