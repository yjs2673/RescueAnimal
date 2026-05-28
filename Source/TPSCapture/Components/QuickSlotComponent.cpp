#include "QuickSlotComponent.h"

UQuickSlotComponent::UQuickSlotComponent()
{
	PrimaryComponentTick.bCanEverTick = false;

	InitializeSlots();
}

void UQuickSlotComponent::BeginPlay()
{
	Super::BeginPlay();

	InitializeSlots();
}

void UQuickSlotComponent::InitializeSlots()
{
	if (QuickSlots.Num() != MaxQuickSlotCount)
	{
		QuickSlots.SetNum(MaxQuickSlotCount);

		for (FName& SlotItem : QuickSlots)
		{
			if (SlotItem.IsNone())
			{
				SlotItem = NAME_None;
			}
		}
	}
}

int32 UQuickSlotComponent::GetSlotCount() const
{
	return MaxQuickSlotCount;
}

FName UQuickSlotComponent::GetSlotItem(int32 SlotIndex) const
{
	if (!IsValidSlotIndex(SlotIndex))
	{
		return NAME_None;
	}

	return QuickSlots[SlotIndex];
}

bool UQuickSlotComponent::SetSlotItem(int32 SlotIndex, FName ItemID)
{
	if (!IsValidSlotIndex(SlotIndex))
	{
		UE_LOG(LogTemp, Warning, TEXT("SetSlotItem failed. Invalid SlotIndex: %d"), SlotIndex);
		return false;
	}

	QuickSlots[SlotIndex] = ItemID;

	OnQuickSlotChanged.Broadcast(SlotIndex, ItemID);

	UE_LOG(LogTemp, Log, TEXT("QuickSlot %d set to %s"), SlotIndex + 1, *ItemID.ToString());

	return true;
}

bool UQuickSlotComponent::ClearSlot(int32 SlotIndex)
{
	if (!IsValidSlotIndex(SlotIndex))
	{
		UE_LOG(LogTemp, Warning, TEXT("ClearSlot failed. Invalid SlotIndex: %d"), SlotIndex);
		return false;
	}

	QuickSlots[SlotIndex] = NAME_None;

	OnQuickSlotChanged.Broadcast(SlotIndex, NAME_None);

	UE_LOG(LogTemp, Log, TEXT("QuickSlot %d cleared"), SlotIndex + 1);

	return true;
}

bool UQuickSlotComponent::IsValidSlotIndex(int32 SlotIndex) const
{
	return SlotIndex >= 0 && SlotIndex < MaxQuickSlotCount;
}