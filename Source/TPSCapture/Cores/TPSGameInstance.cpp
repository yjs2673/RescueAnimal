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