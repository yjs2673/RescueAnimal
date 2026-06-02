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
