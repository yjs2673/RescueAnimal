#include "InventoryWidget.h"
#include "QuickSlotBarWidget.h"
#include "Components/Button.h"
#include "Components/UniformGridPanel.h"
#include "Components/UniformGridSlot.h"
#include "InventorySlotWidget.h"
#include "InventoryComponent.h"
#include "GameFramework/Pawn.h"

void UInventoryWidget::NativeConstruct()
{
	Super::NativeConstruct();

	if (CloseButton)
	{
		CloseButton->OnClicked.AddDynamic(
			this,
			&UInventoryWidget::HandleCloseButtonClicked
		);
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("InventoryWidget: CloseButton is not bound."));
	}

	if (!InventoryGrid)
	{
		UE_LOG(LogTemp, Warning, TEXT("InventoryWidget: InventoryGrid is not bound."));
	}

	if (!InventorySlotWidgetClass)
	{
		UE_LOG(LogTemp, Warning, TEXT("InventoryWidget: InventorySlotWidgetClass is not assigned."));
	}

	BuildEmptySlots();
	RefreshInventory();
}

void UInventoryWidget::HandleCloseButtonClicked()
{
	OnInventoryCloseRequested.Broadcast();
}

void UInventoryWidget::BuildEmptySlots()
{
	if (!InventoryGrid)
	{
		return;
	}

	InventoryGrid->ClearChildren();
	SlotWidgets.Empty();

	if (!InventorySlotWidgetClass)
	{
		return;
	}

	for (int32 Index = 0; Index < MaxSlotCount; ++Index)
	{
		UInventorySlotWidget* SlotWidget = CreateWidget<UInventorySlotWidget>(
			GetOwningPlayer(),
			InventorySlotWidgetClass
		);

		if (!SlotWidget)
		{
			continue;
		}

		SlotWidget->SetEmptySlot();

		const int32 Row = Index / SlotColumnCount;
		const int32 Column = Index % SlotColumnCount;

		UUniformGridSlot* GridSlot = InventoryGrid->AddChildToUniformGrid(
			SlotWidget,
			Row,
			Column
		);

		if (GridSlot)
		{
			GridSlot->SetHorizontalAlignment(HAlign_Fill);
			GridSlot->SetVerticalAlignment(VAlign_Fill);
		}

		SlotWidgets.Add(SlotWidget);
	}
}

void UInventoryWidget::RefreshInventory()
{
	if (!InventoryGrid)
	{
		UE_LOG(LogTemp, Warning, TEXT("InventoryWidget: InventoryGrid is null."));
		return;
	}

	if (SlotWidgets.Num() == 0)
	{
		BuildEmptySlots();
	}

	APawn* OwningPawn = GetOwningPlayerPawn();
	if (!OwningPawn)
	{
		UE_LOG(LogTemp, Warning, TEXT("InventoryWidget: OwningPawn is null."));
		return;
	}

	UInventoryComponent* InventoryComponent = OwningPawn->FindComponentByClass<UInventoryComponent>();
	if (!InventoryComponent)
	{
		UE_LOG(LogTemp, Warning, TEXT("InventoryWidget: InventoryComponent not found."));
		return;
	}

	const TArray<FInventoryEntry>& InventoryItems = InventoryComponent->GetAllItems();

	int32 VisibleIndex = 0;

	for (const FInventoryEntry& Entry : InventoryItems)
	{
		if (Entry.ItemID.IsNone() || Entry.Count <= 0)
		{
			continue;
		}

		if (!SlotWidgets.IsValidIndex(VisibleIndex))
		{
			break;
		}

		SlotWidgets[VisibleIndex]->SetupSlot(Entry.ItemID, Entry.Count);
		VisibleIndex++;
	}

	for (int32 Index = VisibleIndex; Index < SlotWidgets.Num(); ++Index)
	{
		if (SlotWidgets[Index])
		{
			SlotWidgets[Index]->SetEmptySlot();
		}
	}

	if (QuickSlotBar)
	{
		QuickSlotBar->RefreshQuickSlots();
	}
}