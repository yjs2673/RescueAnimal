#include "QuickSlotBarWidget.h"

#include "Components/UniformGridPanel.h"
#include "Components/UniformGridSlot.h"
#include "QuickSlotWidget.h"
#include "QuickSlotComponent.h"
#include "InventoryComponent.h"
#include "GameFramework/Pawn.h"

void UQuickSlotBarWidget::NativeConstruct()
{
	Super::NativeConstruct();

	if (!QuickSlotGrid)
	{
		UE_LOG(LogTemp, Warning, TEXT("QuickSlotBarWidget: QuickSlotGrid is not bound."));
	}

	if (!QuickSlotWidgetClass)
	{
		UE_LOG(LogTemp, Warning, TEXT("QuickSlotBarWidget: QuickSlotWidgetClass is not assigned."));
	}

	APawn* OwningPawn = GetOwningPlayerPawn();
	if (!OwningPawn)
	{
		UE_LOG(LogTemp, Warning, TEXT("QuickSlotBarWidget: OwningPawn is null."));
		return;
	}

	CachedQuickSlotComponent = OwningPawn->FindComponentByClass<UQuickSlotComponent>();
	CachedInventoryComponent = OwningPawn->FindComponentByClass<UInventoryComponent>();

	if (CachedQuickSlotComponent)
	{
		CachedQuickSlotComponent->OnQuickSlotChanged.AddDynamic(
			this,
			&UQuickSlotBarWidget::HandleQuickSlotChanged
		);
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("QuickSlotBarWidget: QuickSlotComponent not found."));
	}

	if (!CachedInventoryComponent)
	{
		UE_LOG(LogTemp, Warning, TEXT("QuickSlotBarWidget: InventoryComponent not found."));
	}

	RefreshQuickSlots();
}

void UQuickSlotBarWidget::HandleQuickSlotChanged(int32 SlotIndex, FName ItemID)
{
	RefreshQuickSlots();
}

void UQuickSlotBarWidget::RefreshQuickSlots()
{
	if (!QuickSlotGrid)
	{
		return;
	}

	QuickSlotGrid->ClearChildren();

	if (!QuickSlotWidgetClass)
	{
		return;
	}

	if (!CachedQuickSlotComponent)
	{
		APawn* OwningPawn = GetOwningPlayerPawn();
		if (OwningPawn)
		{
			CachedQuickSlotComponent = OwningPawn->FindComponentByClass<UQuickSlotComponent>();
		}
	}

	if (!CachedInventoryComponent)
	{
		APawn* OwningPawn = GetOwningPlayerPawn();
		if (OwningPawn)
		{
			CachedInventoryComponent = OwningPawn->FindComponentByClass<UInventoryComponent>();
		}
	}

	if (!CachedQuickSlotComponent)
	{
		return;
	}

	const int32 SlotCount = CachedQuickSlotComponent->GetSlotCount();

	for (int32 SlotIndex = 0; SlotIndex < SlotCount; ++SlotIndex)
	{
		const FName ItemID = CachedQuickSlotComponent->GetSlotItem(SlotIndex);
		const int32 ItemCount = CachedInventoryComponent && !ItemID.IsNone()
			? CachedInventoryComponent->GetItemCount(ItemID)
			: 0;

		UQuickSlotWidget* SlotWidget = CreateWidget<UQuickSlotWidget>(
			GetOwningPlayer(),
			QuickSlotWidgetClass
		);

		if (!SlotWidget)
		{
			continue;
		}

		SlotWidget->SetupQuickSlot(SlotIndex, ItemID, ItemCount);

		UUniformGridSlot* GridSlot = QuickSlotGrid->AddChildToUniformGrid(
			SlotWidget,
			0,
			SlotIndex
		);

		if (GridSlot)
		{
			GridSlot->SetHorizontalAlignment(HAlign_Fill);
			GridSlot->SetVerticalAlignment(VAlign_Fill);
		}
	}
}