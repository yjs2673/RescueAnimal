#include "InventoryComponent.h"

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

	FInventoryEntry* FoundEntry = FindEntry(ItemID);
	if (!FoundEntry || FoundEntry->Count < Count)
	{
		return false;
	}

	FoundEntry->Count -= Count;
	const int32 NewCount = FoundEntry->Count;

	if (FoundEntry->Count <= 0)
	{
		Items.RemoveAll([&](const FInventoryEntry& Entry)
			{
				return Entry.ItemID == ItemID;
			});
	}

	OnItemChanged.Broadcast(ItemID, FMath::Max(NewCount, 0));
	return true;
}

int32 UInventoryComponent::GetItemCount(FName ItemID) const
{
	if (const FInventoryEntry* FoundEntry = FindEntry(ItemID))
	{
		return FoundEntry->Count;
	}

	return 0;
}

bool UInventoryComponent::HasItem(FName ItemID, int32 Count) const
{
	return GetItemCount(ItemID) >= Count;
}