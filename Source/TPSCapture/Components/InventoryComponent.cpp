#include "InventoryComponent.h"
#include "TPSGameInstance.h"
#include "Engine/World.h"

UInventoryComponent::UInventoryComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UInventoryComponent::BeginPlay()
{
	Super::BeginPlay();
}

FInventoryEntry* UInventoryComponent::FindEntry(FName ItemID)
{
	return Items.FindByPredicate([&](const FInventoryEntry& Entry)
		{
			return Entry.ItemID == ItemID;
		});
}

const FInventoryEntry* UInventoryComponent::FindEntry(FName ItemID) const
{
	return Items.FindByPredicate([&](const FInventoryEntry& Entry)
		{
			return Entry.ItemID == ItemID;
		});
}

void UInventoryComponent::AddItem(FName ItemID, int32 Count)
{
	if (ItemID.IsNone() || Count <= 0)
	{
		return;
	}

	FItemData ItemData;
	const UTPSGameInstance* TPSGameInstance = GetWorld() ? GetWorld()->GetGameInstance<UTPSGameInstance>() : nullptr;
	const bool bHasItemData = TPSGameInstance && TPSGameInstance->GetItemDataByID(ItemID, ItemData);
	const bool bShouldStack = !bHasItemData || (ItemData.ItemType != EItemType::Weapon && ItemData.MaxStack > 1);

	if (!bShouldStack)
	{
		for (int32 Index = 0; Index < Count; ++Index)
		{
			FInventoryEntry NewEntry;
			NewEntry.ItemID = ItemID;
			NewEntry.Count = 1;
			Items.Add(NewEntry);
		}

		OnItemChanged.Broadcast(ItemID, GetItemCount(ItemID));
		return;
	}

	if (FInventoryEntry* FoundEntry = FindEntry(ItemID))
	{
		FoundEntry->Count += Count;

		OnItemChanged.Broadcast(ItemID, FoundEntry->Count);
		return;
	}

	FInventoryEntry NewEntry;
	NewEntry.ItemID = ItemID;
	NewEntry.Count = Count;
	Items.Add(NewEntry);

	OnItemChanged.Broadcast(ItemID, Count);
}

bool UInventoryComponent::RemoveItem(FName ItemID, int32 Count)
{
	if (ItemID.IsNone() || Count <= 0)
	{
		return false;
	}

	if (GetItemCount(ItemID) < Count)
	{
		return false;
	}

	int32 RemainingCount = Count;

	for (int32 Index = 0; Index < Items.Num() && RemainingCount > 0;)
	{
		FInventoryEntry& Entry = Items[Index];
		if (Entry.ItemID != ItemID)
		{
			++Index;
			continue;
		}

		const int32 RemoveCount = FMath::Min(Entry.Count, RemainingCount);
		Entry.Count -= RemoveCount;
		RemainingCount -= RemoveCount;

		if (Entry.Count <= 0)
		{
			Items.RemoveAt(Index);
			continue;
		}

		++Index;
	}

	OnItemChanged.Broadcast(ItemID, GetItemCount(ItemID));
	return true;
}

int32 UInventoryComponent::GetItemCount(FName ItemID) const
{
	int32 TotalCount = 0;

	for (const FInventoryEntry& Entry : Items)
	{
		if (Entry.ItemID == ItemID)
		{
			TotalCount += Entry.Count;
		}
	}

	return TotalCount;
}

bool UInventoryComponent::HasItem(FName ItemID, int32 Count) const
{
	return GetItemCount(ItemID) >= Count;
}
