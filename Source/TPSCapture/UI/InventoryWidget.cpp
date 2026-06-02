#include "InventoryWidget.h"
#include "QuickSlotBarWidget.h"
#include "Components/Button.h"
#include "Components/Border.h"
#include "Components/SizeBox.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/UniformGridPanel.h"
#include "Components/UniformGridSlot.h"
#include "InventorySlotWidget.h"
#include "InventoryComponent.h"
#include "GameFramework/Pawn.h"
#include "Input/Reply.h"

bool UInventoryWidget::bHasSavedInventoryPosition = false;
FVector2D UInventoryWidget::SavedInventoryPosition = FVector2D::ZeroVector;

void UInventoryWidget::NativeConstruct()
{
	Super::NativeConstruct();

	if (CloseButton)
	{
		CloseButton->OnClicked.AddUniqueDynamic(
			this,
			&UInventoryWidget::HandleCloseButtonClicked
		);
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("InventoryWidget: CloseButton is not bound."));
	}

	if (!InventoryRootSizeBox)
	{
		UE_LOG(LogTemp, Warning, TEXT("InventoryWidget: InventoryRootSizeBox is not bound."));
	}

	if (!Upper_Border)
	{
		UE_LOG(LogTemp, Warning, TEXT("InventoryWidget: Upper_Border is not bound."));
	}

	if (!InventoryGrid)
	{
		UE_LOG(LogTemp, Warning, TEXT("InventoryWidget: InventoryGrid is not bound."));
	}

	if (!InventorySlotWidgetClass)
	{
		UE_LOG(LogTemp, Warning, TEXT("InventoryWidget: InventorySlotWidgetClass is not assigned."));
	}

	if (bHasSavedInventoryPosition)
	{
		SetInventoryPosition(SavedInventoryPosition);
	}

	BindInventoryComponent();

	BuildEmptySlots();
	RefreshInventory();
}

void UInventoryWidget::NativeDestruct()
{
	UnbindInventoryComponent();

	Super::NativeDestruct();
}

void UInventoryWidget::BindInventoryComponent()
{
	CachedInventoryComponent = GetInventoryComponent();

	if (CachedInventoryComponent)
	{
		CachedInventoryComponent->OnItemChanged.AddUniqueDynamic(
			this,
			&UInventoryWidget::HandleInventoryItemChanged
		);
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("InventoryWidget: Failed to bind InventoryComponent."));
	}
}

void UInventoryWidget::UnbindInventoryComponent()
{
	if (CachedInventoryComponent)
	{
		CachedInventoryComponent->OnItemChanged.RemoveDynamic(
			this,
			&UInventoryWidget::HandleInventoryItemChanged
		);

		CachedInventoryComponent = nullptr;
	}
}

UInventoryComponent* UInventoryWidget::GetInventoryComponent() const
{
	APawn* OwningPawn = GetOwningPlayerPawn();
	if (!OwningPawn)
	{
		return nullptr;
	}

	return OwningPawn->FindComponentByClass<UInventoryComponent>();
}

void UInventoryWidget::HandleInventoryItemChanged(FName ItemID, int32 NewCount)
{
	RefreshInventory();
}

FReply UInventoryWidget::NativeOnMouseButtonDown(
	const FGeometry& InGeometry,
	const FPointerEvent& InMouseEvent
)
{
	if (InMouseEvent.GetEffectingButton() == EKeys::LeftMouseButton &&
		IsMouseOverUpperBorder(InMouseEvent))
	{
		bIsDraggingInventory = true;

		const FVector2D MouseLocalPosition =
			InGeometry.AbsoluteToLocal(InMouseEvent.GetScreenSpacePosition());

		DragOffset = MouseLocalPosition - GetInventoryPosition();

		return FReply::Handled().CaptureMouse(TakeWidget());
	}

	if (InMouseEvent.GetEffectingButton() == EKeys::LeftMouseButton)
	{
		return FReply::Handled();
	}

	return Super::NativeOnMouseButtonDown(InGeometry, InMouseEvent);
}

FReply UInventoryWidget::NativeOnMouseButtonDoubleClick(
	const FGeometry& InGeometry,
	const FPointerEvent& InMouseEvent
)
{
	if (InMouseEvent.GetEffectingButton() == EKeys::LeftMouseButton)
	{
		return FReply::Handled();
	}

	return Super::NativeOnMouseButtonDoubleClick(InGeometry, InMouseEvent);
}

FReply UInventoryWidget::NativeOnMouseMove(
	const FGeometry& InGeometry,
	const FPointerEvent& InMouseEvent
)
{
	if (bIsDraggingInventory)
	{
		const FVector2D MouseLocalPosition =
			InGeometry.AbsoluteToLocal(InMouseEvent.GetScreenSpacePosition());

		const FVector2D NewPosition = MouseLocalPosition - DragOffset;

		SetInventoryPosition(NewPosition);

		SavedInventoryPosition = NewPosition;
		bHasSavedInventoryPosition = true;

		return FReply::Handled();
	}

	return Super::NativeOnMouseMove(InGeometry, InMouseEvent);
}

FReply UInventoryWidget::NativeOnMouseButtonUp(
	const FGeometry& InGeometry,
	const FPointerEvent& InMouseEvent
)
{
	if (bIsDraggingInventory &&
		InMouseEvent.GetEffectingButton() == EKeys::LeftMouseButton)
	{
		bIsDraggingInventory = false;

		return FReply::Handled().ReleaseMouseCapture();
	}

	if (InMouseEvent.GetEffectingButton() == EKeys::LeftMouseButton)
	{
		return FReply::Handled();
	}

	return Super::NativeOnMouseButtonUp(InGeometry, InMouseEvent);
}

bool UInventoryWidget::IsMouseOverUpperBorder(const FPointerEvent& InMouseEvent) const
{
	if (!Upper_Border)
	{
		return false;
	}

	return Upper_Border->GetCachedGeometry().IsUnderLocation(
		InMouseEvent.GetScreenSpacePosition()
	);
}

FVector2D UInventoryWidget::GetInventoryPosition() const
{
	if (!InventoryRootSizeBox)
	{
		return FVector2D::ZeroVector;
	}

	const UCanvasPanelSlot* CanvasSlot =
		Cast<UCanvasPanelSlot>(InventoryRootSizeBox->Slot);

	if (!CanvasSlot)
	{
		return FVector2D::ZeroVector;
	}

	return CanvasSlot->GetPosition();
}

void UInventoryWidget::SetInventoryPosition(const FVector2D& NewPosition)
{
	if (!InventoryRootSizeBox)
	{
		return;
	}

	UCanvasPanelSlot* CanvasSlot =
		Cast<UCanvasPanelSlot>(InventoryRootSizeBox->Slot);

	if (!CanvasSlot)
	{
		return;
	}

	CanvasSlot->SetPosition(NewPosition);
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

	InventoryGrid->SetSlotPadding(FMargin(3.0f));

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
			GridSlot->SetHorizontalAlignment(HAlign_Center);
			GridSlot->SetVerticalAlignment(VAlign_Center);
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

	UInventoryComponent* InventoryComponent = CachedInventoryComponent.Get();

	if (!InventoryComponent)
	{
		InventoryComponent = GetInventoryComponent();
		CachedInventoryComponent = InventoryComponent;

		if (CachedInventoryComponent)
		{
			CachedInventoryComponent->OnItemChanged.AddUniqueDynamic(
				this,
				&UInventoryWidget::HandleInventoryItemChanged
			);
		}
	}

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
