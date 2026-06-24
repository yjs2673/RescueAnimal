#pragma once

#include "CoreMinimal.h"
#include "Engine/GameInstance.h"
#include "TPSStructTypes.h"
#include "TPSGameInstance.generated.h"

class UDataTable;
class ATPSCaptureCharacter;
class ADropItemActor;

#pragma region Runtime Data Structs
USTRUCT(BlueprintType)
struct FPlayerRuntimeData
{
	GENERATED_BODY()

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Runtime")
	bool bHasValidData = false;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Runtime")
	float CurrentHP = 0.0f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Runtime")
	int32 Level = 1;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Runtime")
	int32 CurrentExp = 0;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Runtime")
	TArray<FInventoryEntry> InventoryItems;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Runtime")
	int32 Coin = 0;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Runtime")
	int32 SpecialCurrency = 0;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Runtime")
	FName EquippedWeaponID = NAME_None;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Runtime")
	TArray<FName> QuickSlotItemIDs;
};

USTRUCT(BlueprintType)
struct FSpawnedDropItemRuntimeData
{
	GENERATED_BODY()

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Runtime")
	FName ItemSaveID = NAME_None;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Runtime")
	FName ItemID = NAME_None;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Runtime")
	int32 Count = 1;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Runtime")
	FTransform SpawnTransform = FTransform::Identity;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Runtime")
	TSubclassOf<ADropItemActor> DropItemActorClass;
};

USTRUCT(BlueprintType)
struct FMapRuntimeData
{
	GENERATED_BODY()

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Runtime")
	TArray<FName> DefeatedEnemyIDs;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Runtime")
	TArray<FName> RescuedAnimalIDs;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Runtime")
	TArray<FName> PickedItemIDs;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Runtime")
	TArray<FSpawnedDropItemRuntimeData> SpawnedDropItems;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Runtime")
	bool bMapCleared = false;
};
#pragma endregion

UCLASS()
class TPSCAPTURE_API UTPSGameInstance : public UGameInstance
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = "Data")
	bool GetItemDataByID(FName ItemID, FItemData& OutItemData) const;

	UFUNCTION(BlueprintCallable, Category = "Data")
	bool GetWeaponDataByID(FName WeaponID, FWeaponData& OutWeaponData) const;

	UFUNCTION(BlueprintCallable, Category = "Animal Collection")
	bool UnlockAnimal(FName AnimalID);

	UFUNCTION(BlueprintPure, Category = "Animal Collection")
	bool IsAnimalUnlocked(FName AnimalID) const;

	UFUNCTION(BlueprintPure, Category = "Animal Collection")
	TArray<FName> GetUnlockedAnimalIDs() const;

#pragma region Runtime Data Functions
	UFUNCTION(BlueprintCallable, Category = "Runtime")
	void SavePlayerRuntimeData(ATPSCaptureCharacter* PlayerCharacter);

	UFUNCTION(BlueprintCallable, Category = "Runtime")
	void LoadPlayerRuntimeData(ATPSCaptureCharacter* PlayerCharacter);

	UFUNCTION(BlueprintPure, Category = "Runtime")
	bool HasValidPlayerRuntimeData() const;

	UFUNCTION(BlueprintCallable, Category = "Runtime")
	void RegisterDefeatedEnemy(FName MapID, FName EnemyID);

	UFUNCTION(BlueprintCallable, Category = "Runtime")
	void RegisterRescuedAnimal(FName MapID, FName AnimalID);

	UFUNCTION(BlueprintCallable, Category = "Runtime")
	void RegisterPickedItem(FName MapID, FName ItemID);

	UFUNCTION(BlueprintCallable, Category = "Runtime")
	void RegisterSpawnedDropItem(FName MapID, const FSpawnedDropItemRuntimeData& DropItemData);

	UFUNCTION(BlueprintPure, Category = "Runtime")
	TArray<FSpawnedDropItemRuntimeData> GetSpawnedDropItems(FName MapID) const;

	UFUNCTION(BlueprintPure, Category = "Runtime")
	bool IsEnemyDefeated(FName MapID, FName EnemyID) const;

	UFUNCTION(BlueprintPure, Category = "Runtime")
	bool IsAnimalRescued(FName MapID, FName AnimalID) const;

	UFUNCTION(BlueprintPure, Category = "Runtime")
	bool IsItemPicked(FName MapID, FName ItemID) const;

	UFUNCTION(BlueprintCallable, Category = "Runtime")
	void SetMapCleared(FName MapID, bool bCleared = true);

	UFUNCTION(BlueprintPure, Category = "Runtime")
	bool IsMapCleared(FName MapID) const;
#pragma endregion

protected:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Data")
	UDataTable* ItemDataTable;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Data")
	UDataTable* WeaponDataTable;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Animal Collection")
	TSet<FName> UnlockedAnimalIDs;

#pragma region Runtime Data Variables
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Runtime")
	FPlayerRuntimeData PlayerRuntimeData;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Runtime")
	TMap<FName, FMapRuntimeData> MapRuntimeDataMap;
#pragma endregion

public:
	UPROPERTY()
	bool bPendingPortalTransition = false;

	UFUNCTION(BlueprintPure, Category = "Data")
	UDataTable* GetItemDataTable() const { return ItemDataTable; }

	UFUNCTION(BlueprintPure, Category = "Data")
	UDataTable* GetWeaponDataTable() const { return WeaponDataTable; }
};
