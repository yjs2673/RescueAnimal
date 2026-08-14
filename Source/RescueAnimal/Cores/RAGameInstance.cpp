#include "RAGameInstance.h"

#include "InventoryComponent.h"
#include "PlayerStatComponent.h"
#include "QuickSlotComponent.h"
#include "RACharacter.h"

URAGameInstance::URAGameInstance()
{
	RequiredClearMapIDs =
	{
		TEXT("MAP_Plain"),
		TEXT("MAP_Snow"),
		TEXT("MAP_Desert")
	};
}

bool URAGameInstance::GetItemDataByID(FName ItemID, FItemData& OutItemData) const
{
	if (!ItemDataTable || ItemID.IsNone())
		return false;

	const FItemData* FoundItemData = ItemDataTable->FindRow<FItemData>(ItemID, TEXT("GetItemDataByID"));

	if (!FoundItemData)
		return false;

	OutItemData = *FoundItemData;
	return true;
}

bool URAGameInstance::GetWeaponDataByID(FName WeaponID, FWeaponData& OutWeaponData) const
{
	if (WeaponID.IsNone())
		return false;

	if (!WeaponDataTable)
	{
		UE_LOG(LogTemp, Warning, TEXT("GetWeaponDataByID failed: WeaponDataTable is null. WeaponID=%s"), *WeaponID.ToString());
		return false;
	}

	const FWeaponData* FoundWeaponData = WeaponDataTable->FindRow<FWeaponData>(WeaponID, TEXT("GetWeaponDataByID"));

	if (!FoundWeaponData)
	{
		static const FString ContextString(TEXT("GetWeaponDataByID fallback"));
		TArray<FWeaponData*> AllWeaponRows;
		WeaponDataTable->GetAllRows<FWeaponData>(ContextString, AllWeaponRows);

		for (const FWeaponData* WeaponRow : AllWeaponRows)
		{
			if (WeaponRow && WeaponRow->WeaponID == WeaponID)
			{
				FoundWeaponData = WeaponRow;
				break;
			}
		}
	}

	if (!FoundWeaponData)
	{
		UE_LOG(LogTemp, Warning, TEXT("GetWeaponDataByID failed: row not found. WeaponID=%s Table=%s"),
			*WeaponID.ToString(),
			*GetNameSafe(WeaponDataTable));
		return false;
	}

	OutWeaponData = *FoundWeaponData;
	return true;
}

bool URAGameInstance::UnlockAnimal(FName AnimalID)
{
	if (AnimalID.IsNone())
	{
		UE_LOG(LogTemp, Warning, TEXT("[AnimalCollection] Unlock failed: AnimalID is None."));
		return false;
	}

	if (UnlockedAnimalIDs.Contains(AnimalID))
	{
		UE_LOG(LogTemp, Warning, TEXT("[AnimalCollection] Already unlocked animal species: %s"), *AnimalID.ToString());
		return false;
	}

	UnlockedAnimalIDs.Add(AnimalID);

	UE_LOG(LogTemp, Warning, TEXT("[AnimalCollection] New animal species unlocked: %s / Total=%d"),
		*AnimalID.ToString(),
		UnlockedAnimalIDs.Num());

	return true;
}

bool URAGameInstance::IsAnimalUnlocked(FName AnimalID) const
{
	return !AnimalID.IsNone() && UnlockedAnimalIDs.Contains(AnimalID);
}

TArray<FName> URAGameInstance::GetUnlockedAnimalIDs() const
{
	TArray<FName> UnlockedIDs;
	UnlockedIDs.Reserve(UnlockedAnimalIDs.Num());

	for (const FName& AnimalID : UnlockedAnimalIDs)
	{
		UnlockedIDs.Add(AnimalID);
	}

	return UnlockedIDs;
}

#pragma region Runtime Data Functions
void URAGameInstance::SavePlayerRuntimeData(ARACharacter* PlayerCharacter)
{
	if (!PlayerCharacter)
	{
		UE_LOG(LogTemp, Warning, TEXT("[RuntimeData] SavePlayerRuntimeData failed: PlayerCharacter is null."));
		return;
	}

	PlayerRuntimeData.bHasValidData = true;

	if (const UPlayerStatComponent* StatComponent = PlayerCharacter->GetPlayerStatComponent())
	{
		PlayerRuntimeData.CurrentHP = StatComponent->GetCurrentHP();
		PlayerRuntimeData.Level = StatComponent->GetLevel();
		PlayerRuntimeData.CurrentExp = StatComponent->GetCurrentEXP();
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("[RuntimeData] TODO: PlayerStatComponent is missing during save."));
	}

	if (const UInventoryComponent* InventoryComponent = PlayerCharacter->GetInventoryComponent())
	{
		PlayerRuntimeData.InventoryItems = InventoryComponent->GetAllItems();
		PlayerRuntimeData.Coin = InventoryComponent->GetItemCount(TEXT("Coin"));
		PlayerRuntimeData.SpecialCurrency = InventoryComponent->GetItemCount(TEXT("SpecialCurrency"));
	}
	else
	{
		PlayerRuntimeData.InventoryItems.Reset();
		PlayerRuntimeData.Coin = 0;
		PlayerRuntimeData.SpecialCurrency = 0;
		UE_LOG(LogTemp, Warning, TEXT("[RuntimeData] TODO: InventoryComponent is missing during save."));
	}

	PlayerRuntimeData.EquippedWeaponID = PlayerCharacter->GetCurrentWeaponItemID();
	PlayerRuntimeData.QuickSlotItemIDs.Reset();

	if (const UQuickSlotComponent* QuickSlotComponent = PlayerCharacter->GetQuickSlotComponent())
	{
		const int32 SlotCount = QuickSlotComponent->GetSlotCount();
		PlayerRuntimeData.QuickSlotItemIDs.Reserve(SlotCount);

		for (int32 SlotIndex = 0; SlotIndex < SlotCount; ++SlotIndex)
		{
			PlayerRuntimeData.QuickSlotItemIDs.Add(QuickSlotComponent->GetSlotItem(SlotIndex));
		}
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("[RuntimeData] TODO: QuickSlotComponent is missing during save."));
	}
}

void URAGameInstance::LoadPlayerRuntimeData(ARACharacter* PlayerCharacter)
{
	if (!PlayerCharacter)
	{
		UE_LOG(LogTemp, Warning, TEXT("[RuntimeData] LoadPlayerRuntimeData failed: PlayerCharacter is null."));
		return;
	}

	if (!HasValidPlayerRuntimeData())
	{
		return;
	}

	if (UPlayerStatComponent* StatComponent = PlayerCharacter->GetPlayerStatComponent())
	{
		StatComponent->SetRuntimeStats(
			PlayerRuntimeData.CurrentHP,
			PlayerRuntimeData.Level,
			PlayerRuntimeData.CurrentExp
		);
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("[RuntimeData] TODO: PlayerStatComponent is missing during load."));
	}

	if (UInventoryComponent* InventoryComponent = PlayerCharacter->GetInventoryComponent())
	{
		InventoryComponent->SetItems(PlayerRuntimeData.InventoryItems);

		const FName CoinItemID = TEXT("Coin");
		const int32 CurrentCoin = InventoryComponent->GetItemCount(CoinItemID);
		if (PlayerRuntimeData.Coin > CurrentCoin)
		{
			InventoryComponent->AddItem(CoinItemID, PlayerRuntimeData.Coin - CurrentCoin);
		}
		else if (PlayerRuntimeData.Coin < CurrentCoin)
		{
			InventoryComponent->RemoveItem(CoinItemID, CurrentCoin - PlayerRuntimeData.Coin);
		}

		const FName SpecialCurrencyItemID = TEXT("SpecialCurrency");
		const int32 CurrentSpecialCurrency = InventoryComponent->GetItemCount(SpecialCurrencyItemID);
		if (PlayerRuntimeData.SpecialCurrency > CurrentSpecialCurrency)
		{
			InventoryComponent->AddItem(SpecialCurrencyItemID, PlayerRuntimeData.SpecialCurrency - CurrentSpecialCurrency);
		}
		else if (PlayerRuntimeData.SpecialCurrency < CurrentSpecialCurrency)
		{
			InventoryComponent->RemoveItem(SpecialCurrencyItemID, CurrentSpecialCurrency - PlayerRuntimeData.SpecialCurrency);
		}
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("[RuntimeData] TODO: InventoryComponent is missing during load."));
	}

	if (UQuickSlotComponent* QuickSlotComponent = PlayerCharacter->GetQuickSlotComponent())
	{
		const int32 SlotCount = QuickSlotComponent->GetSlotCount();
		for (int32 SlotIndex = 0; SlotIndex < SlotCount; ++SlotIndex)
		{
			const FName ItemID = PlayerRuntimeData.QuickSlotItemIDs.IsValidIndex(SlotIndex)
				? PlayerRuntimeData.QuickSlotItemIDs[SlotIndex]
				: NAME_None;

			QuickSlotComponent->SetSlotItem(SlotIndex, ItemID);
		}
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("[RuntimeData] TODO: QuickSlotComponent is missing during load."));
	}

	if (!PlayerRuntimeData.EquippedWeaponID.IsNone())
	{
		if (UInventoryComponent* InventoryComponent = PlayerCharacter->GetInventoryComponent())
		{
			if (!InventoryComponent->HasItem(PlayerRuntimeData.EquippedWeaponID, 1))
			{
				InventoryComponent->AddItem(PlayerRuntimeData.EquippedWeaponID, 1);
			}
		}

		if (!PlayerCharacter->EquipWeaponFromInventory(PlayerRuntimeData.EquippedWeaponID))
		{
			UE_LOG(LogTemp, Warning, TEXT("[RuntimeData] TODO: Failed to restore equipped weapon. WeaponID=%s"),
				*PlayerRuntimeData.EquippedWeaponID.ToString());
		}
	}
	else
	{
		PlayerCharacter->OnWeaponChanged.Broadcast(PlayerCharacter->GetCurrentWeaponType());
	}
}

bool URAGameInstance::HasValidPlayerRuntimeData() const
{
	return PlayerRuntimeData.bHasValidData;
}

void URAGameInstance::RegisterDefeatedEnemy(FName MapID, FName EnemyID)
{
	if (MapID.IsNone() || EnemyID.IsNone())
	{
		UE_LOG(LogTemp, Warning, TEXT("[RuntimeData] RegisterDefeatedEnemy failed: MapID or EnemyID is None."));
		return;
	}

	MapRuntimeDataMap.FindOrAdd(MapID).DefeatedEnemyIDs.AddUnique(EnemyID);
}

void URAGameInstance::RegisterRescuedAnimal(FName MapID, FName AnimalID)
{
	if (MapID.IsNone() || AnimalID.IsNone())
	{
		UE_LOG(LogTemp, Warning, TEXT("[RuntimeData] RegisterRescuedAnimal failed: MapID or AnimalID is None."));
		return;
	}

	MapRuntimeDataMap.FindOrAdd(MapID).RescuedAnimalIDs.AddUnique(AnimalID);
}

void URAGameInstance::RegisterPickedItem(FName MapID, FName ItemID)
{
	if (MapID.IsNone() || ItemID.IsNone())
	{
		UE_LOG(LogTemp, Warning, TEXT("[RuntimeData] RegisterPickedItem failed: MapID or ItemID is None."));
		return;
	}

	FMapRuntimeData& MapRuntimeData = MapRuntimeDataMap.FindOrAdd(MapID);
	MapRuntimeData.PickedItemIDs.AddUnique(ItemID);
	MapRuntimeData.SpawnedDropItems.RemoveAll(
		[ItemID](const FSpawnedDropItemRuntimeData& DropItemData)
		{
			return DropItemData.ItemSaveID == ItemID;
		}
	);
}

#pragma region Runtime Spawned Drop Items
void URAGameInstance::RegisterSpawnedDropItem(FName MapID, const FSpawnedDropItemRuntimeData& DropItemData)
{
	if (MapID.IsNone() || DropItemData.ItemSaveID.IsNone() || DropItemData.ItemID.IsNone())
	{
		UE_LOG(LogTemp, Warning, TEXT("[RuntimeData] RegisterSpawnedDropItem failed: MapID, ItemSaveID, or ItemID is None."));
		return;
	}

	FMapRuntimeData& MapRuntimeData = MapRuntimeDataMap.FindOrAdd(MapID);
	FSpawnedDropItemRuntimeData* ExistingData = MapRuntimeData.SpawnedDropItems.FindByPredicate(
		[&DropItemData](const FSpawnedDropItemRuntimeData& Existing)
		{
			return Existing.ItemSaveID == DropItemData.ItemSaveID;
		}
	);

	if (ExistingData)
	{
		*ExistingData = DropItemData;
		return;
	}

	MapRuntimeData.SpawnedDropItems.Add(DropItemData);
}

TArray<FSpawnedDropItemRuntimeData> URAGameInstance::GetSpawnedDropItems(FName MapID) const
{
	if (MapID.IsNone())
	{
		return {};
	}

	const FMapRuntimeData* MapRuntimeData = MapRuntimeDataMap.Find(MapID);
	return MapRuntimeData ? MapRuntimeData->SpawnedDropItems : TArray<FSpawnedDropItemRuntimeData>();
}
#pragma endregion Runtime Spawned Drop Items

bool URAGameInstance::IsEnemyDefeated(FName MapID, FName EnemyID) const
{
	if (MapID.IsNone() || EnemyID.IsNone())
	{
		return false;
	}

	const FMapRuntimeData* MapRuntimeData = MapRuntimeDataMap.Find(MapID);
	return MapRuntimeData && MapRuntimeData->DefeatedEnemyIDs.Contains(EnemyID);
}

bool URAGameInstance::IsAnimalRescued(FName MapID, FName AnimalID) const
{
	if (MapID.IsNone() || AnimalID.IsNone())
	{
		return false;
	}

	const FMapRuntimeData* MapRuntimeData = MapRuntimeDataMap.Find(MapID);
	return MapRuntimeData && MapRuntimeData->RescuedAnimalIDs.Contains(AnimalID);
}

bool URAGameInstance::IsItemPicked(FName MapID, FName ItemID) const
{
	if (MapID.IsNone() || ItemID.IsNone())
	{
		return false;
	}

	const FMapRuntimeData* MapRuntimeData = MapRuntimeDataMap.Find(MapID);
	return MapRuntimeData && MapRuntimeData->PickedItemIDs.Contains(ItemID);
}

void URAGameInstance::SetMapCleared(FName MapID, bool bCleared)
{
	if (MapID.IsNone())
	{
		UE_LOG(LogTemp, Warning, TEXT("[RuntimeData] SetMapCleared failed: MapID is None."));
		return;
	}

	FMapRuntimeData& MapRuntimeData = MapRuntimeDataMap.FindOrAdd(MapID);
	const bool bWasCleared = MapRuntimeData.bMapCleared;
	MapRuntimeData.bMapCleared = bCleared;

	if (bCleared && !bWasCleared)
	{
		UE_LOG(LogTemp, Warning, TEXT("[GameProgress] Field map cleared. MapID=%s"), *MapID.ToString());

		if (IsGameCleared())
		{
			UE_LOG(LogTemp, Warning, TEXT("[GameProgress] GAME CLEAR: All required field maps are cleared."));
		}
	}
}

bool URAGameInstance::IsMapCleared(FName MapID) const
{
	if (MapID.IsNone())
	{
		return false;
	}

	const FMapRuntimeData* MapRuntimeData = MapRuntimeDataMap.Find(MapID);
	return MapRuntimeData && MapRuntimeData->bMapCleared;
}

#pragma region Game Progress
bool URAGameInstance::HasPlayedIntroDialogue() const
{
	return bHasPlayedIntroDialogue;
}

void URAGameInstance::SetHasPlayedIntroDialogue(bool bValue)
{
	if (bHasPlayedIntroDialogue == bValue)
	{
		return;
	}

	bHasPlayedIntroDialogue = bValue;
	UE_LOG(LogTemp, Log, TEXT("[GameProgress] Intro dialogue state changed: %s"),
		bHasPlayedIntroDialogue ? TEXT("true") : TEXT("false"));
}

bool URAGameInstance::IsGameStarted() const
{
	return bIsGameStarted;
}

void URAGameInstance::SetGameStarted(bool bValue)
{
	if (bIsGameStarted == bValue)
	{
		return;
	}

	bIsGameStarted = bValue;
	UE_LOG(LogTemp, Log, TEXT("[GameProgress] Game started state changed: %s"),
		bIsGameStarted ? TEXT("true") : TEXT("false"));
}

bool URAGameInstance::IsEndingTriggered() const
{
	return bIsEndingTriggered;
}

void URAGameInstance::SetEndingTriggered(bool bValue)
{
	if (bIsEndingTriggered == bValue)
	{
		return;
	}

	bIsEndingTriggered = bValue;
	UE_LOG(LogTemp, Log, TEXT("[GameProgress] Ending triggered state changed: %s"),
		bIsEndingTriggered ? TEXT("true") : TEXT("false"));
}

bool URAGameInstance::AreAllFieldMapsCleared() const
{
	if (RequiredClearMapIDs.IsEmpty())
	{
		UE_LOG(LogTemp, Warning, TEXT("[GameProgress] RequiredClearMapIDs is empty."));
		return false;
	}

	for (const FName& MapID : RequiredClearMapIDs)
	{
		if (!IsMapCleared(MapID))
		{
			return false;
		}
	}

	return true;
}

bool URAGameInstance::IsGameCleared() const
{
	return AreAllFieldMapsCleared();
}
#pragma endregion Game Progress
#pragma endregion
