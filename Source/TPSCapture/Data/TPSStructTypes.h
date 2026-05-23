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
struct FDropItemData
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Drop")
	FName ItemID = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Drop")
	int32 MinCount = 1;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Drop")
	int32 MaxCount = 1;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Drop", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float DropRate = 1.0f;
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

USTRUCT(BlueprintType)
struct FAnimalData : public FTableRowBase
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Animal")
	FName AnimalID; // 고유 ID

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Animal")
	FText DisplayName; // 인게임 이름

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Animal")
	float MaxHP = 30.0f; // 최대 체력

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Animal")
	float CaptureDifficulty = 1.0f; // 캡처 난이도 (높을수록 캡처하기 어려움)

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Animal")
	TArray<FName> DropItemIDs; // 드롭 아이템 ID 목록

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Animal")
	UTexture2D* Icon = nullptr; // UI 아이콘
};