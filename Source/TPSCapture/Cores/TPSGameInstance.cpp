#include "TPSGameInstance.h"

bool UTPSGameInstance::GetItemDataByID(FName ItemID, FItemData& OutItemData) const
{
	if (!ItemDataTable || ItemID.IsNone())
		return false;

	const FItemData* FoundItemData = ItemDataTable->FindRow<FItemData>(ItemID, TEXT("GetItemDataByID"));

	if (!FoundItemData)
		return false;

	OutItemData = *FoundItemData;
	return true;
}

bool UTPSGameInstance::GetWeaponDataByID(FName WeaponID, FWeaponData& OutWeaponData) const
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

bool UTPSGameInstance::UnlockAnimal(FName AnimalID)
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

bool UTPSGameInstance::IsAnimalUnlocked(FName AnimalID) const
{
	return !AnimalID.IsNone() && UnlockedAnimalIDs.Contains(AnimalID);
}

TArray<FName> UTPSGameInstance::GetUnlockedAnimalIDs() const
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
void UTPSGameInstance::RegisterDefeatedEnemy(FName MapID, FName EnemyID)
{
	if (MapID.IsNone() || EnemyID.IsNone())
	{
		UE_LOG(LogTemp, Warning, TEXT("[RuntimeData] RegisterDefeatedEnemy failed: MapID or EnemyID is None."));
		return;
	}

	MapRuntimeDataMap.FindOrAdd(MapID).DefeatedEnemyIDs.AddUnique(EnemyID);
}

void UTPSGameInstance::RegisterRescuedAnimal(FName MapID, FName AnimalID)
{
	if (MapID.IsNone() || AnimalID.IsNone())
	{
		UE_LOG(LogTemp, Warning, TEXT("[RuntimeData] RegisterRescuedAnimal failed: MapID or AnimalID is None."));
		return;
	}

	MapRuntimeDataMap.FindOrAdd(MapID).RescuedAnimalIDs.AddUnique(AnimalID);
}

void UTPSGameInstance::RegisterPickedItem(FName MapID, FName ItemID)
{
	if (MapID.IsNone() || ItemID.IsNone())
	{
		UE_LOG(LogTemp, Warning, TEXT("[RuntimeData] RegisterPickedItem failed: MapID or ItemID is None."));
		return;
	}

	MapRuntimeDataMap.FindOrAdd(MapID).PickedItemIDs.AddUnique(ItemID);
}

bool UTPSGameInstance::IsEnemyDefeated(FName MapID, FName EnemyID) const
{
	if (MapID.IsNone() || EnemyID.IsNone())
	{
		return false;
	}

	const FMapRuntimeData* MapRuntimeData = MapRuntimeDataMap.Find(MapID);
	return MapRuntimeData && MapRuntimeData->DefeatedEnemyIDs.Contains(EnemyID);
}

bool UTPSGameInstance::IsAnimalRescued(FName MapID, FName AnimalID) const
{
	if (MapID.IsNone() || AnimalID.IsNone())
	{
		return false;
	}

	const FMapRuntimeData* MapRuntimeData = MapRuntimeDataMap.Find(MapID);
	return MapRuntimeData && MapRuntimeData->RescuedAnimalIDs.Contains(AnimalID);
}

bool UTPSGameInstance::IsItemPicked(FName MapID, FName ItemID) const
{
	if (MapID.IsNone() || ItemID.IsNone())
	{
		return false;
	}

	const FMapRuntimeData* MapRuntimeData = MapRuntimeDataMap.Find(MapID);
	return MapRuntimeData && MapRuntimeData->PickedItemIDs.Contains(ItemID);
}

void UTPSGameInstance::SetMapCleared(FName MapID, bool bCleared)
{
	if (MapID.IsNone())
	{
		UE_LOG(LogTemp, Warning, TEXT("[RuntimeData] SetMapCleared failed: MapID is None."));
		return;
	}

	MapRuntimeDataMap.FindOrAdd(MapID).bMapCleared = bCleared;
}

bool UTPSGameInstance::IsMapCleared(FName MapID) const
{
	if (MapID.IsNone())
	{
		return false;
	}

	const FMapRuntimeData* MapRuntimeData = MapRuntimeDataMap.Find(MapID);
	return MapRuntimeData && MapRuntimeData->bMapCleared;
}
#pragma endregion
