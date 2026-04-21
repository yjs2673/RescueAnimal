#pragma once

#include "CoreMinimal.h"
#include "Engine/DataTable.h"
#include "TPSGameEnums.h"
#include "TPSStructTypes.generated.h"

class UTexture2D;
class AWeaponBase;
class AArrowProjectile;

USTRUCT(BlueprintType)
struct FInventoryEntry
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Inventory")
	FName ItemID = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Inventory")
	int32 Count = 0;
};

USTRUCT(BlueprintType)
struct FRewardItem
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Reward")
	FName ItemID = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Reward")
	int32 Count = 1;
};

USTRUCT(BlueprintType)
struct FItemData : public FTableRowBase
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Item")
	FName ItemID = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Item")
	FText ItemName;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Item")
	EItemType ItemType = EItemType::None;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Item")
	ERarity Rarity = ERarity::Common;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Item")
	int32 MaxStack = 99;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Item")
	UTexture2D* Icon = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Item")
	FText Description;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Item|Consumable")
	EConsumableType ConsumableType = EConsumableType::None;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Item|Consumable")
	float HealAmount = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Item|Consumable")
	float CapturePower = 0.f;
};

USTRUCT(BlueprintType)
struct FWeaponData : public FTableRowBase
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weapon")
	FName WeaponID = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weapon")
	FText WeaponName;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weapon")
	EWeaponType WeaponType = EWeaponType::None;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weapon")
	EAttackType AttackType = EAttackType::Unarmed;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weapon")
	ERarity Rarity = ERarity::Common;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weapon")
	UTexture2D* Icon = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weapon")
	FText Description;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weapon")
	TSubclassOf<class AWeaponBase> WeaponClass;
};