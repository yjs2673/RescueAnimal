#include "InventorySlotWidget.h"
#include "ItemDragDropOperation.h"
#include "Blueprint/WidgetBlueprintLibrary.h"
#include "Components/Image.h"
#include "Components/TextBlock.h"
#include "Input/Reply.h"
#include "TPSGameInstance.h"

void UInventorySlotWidget::NativeConstruct()
{
	Super::NativeConstruct();
}

void UInventorySlotWidget::SetupSlot(FName InItemID, int32 InCount)
{
	ItemID = InItemID;
	Count = FMath::Max(0, InCount);

	FItemData ItemData;
	const UTPSGameInstance* TPSGameInstance = GetGameInstance<UTPSGameInstance>();
	const bool bHasItemData = TPSGameInstance && TPSGameInstance->GetItemDataByID(ItemID, ItemData);

	if (ItemNameText)
	{
		ItemNameText->SetText(bHasItemData && !ItemData.ItemName.IsEmpty()
			? ItemData.ItemName
			: FText::FromName(ItemID));
	}

	if (ItemCountText)
	{
		ItemCountText->SetText(FText::FromString(FString::Printf(TEXT("x%d"), Count)));
	}

	if (ItemIcon)
	{
		UTexture2D* ItemTexture = nullptr;

		if (bHasItemData)
		{
			ItemTexture = ItemData.Image ? ItemData.Image : ItemData.Icon;
		}

		if (ItemTexture)
		{
			ItemIcon->SetBrushFromTexture(ItemTexture, true);
			ItemIcon->SetVisibility(ESlateVisibility::Visible);
		}
		else
		{
			ItemIcon->SetVisibility(ESlateVisibility::Hidden);
		}
	}
}

FReply UInventorySlotWidget::NativeOnMouseButtonDown(
	const FGeometry& InGeometry,
	const FPointerEvent& InMouseEvent
)
{
	if (InMouseEvent.IsMouseButtonDown(EKeys::LeftMouseButton) && !ItemID.IsNone())
	{
		return UWidgetBlueprintLibrary::DetectDragIfPressed(
			InMouseEvent,
			this,
			EKeys::LeftMouseButton
		).NativeReply;
	}

	return Super::NativeOnMouseButtonDown(InGeometry, InMouseEvent);
}

void UInventorySlotWidget::NativeOnDragDetected(
	const FGeometry& InGeometry,
	const FPointerEvent& InMouseEvent,
	UDragDropOperation*& OutOperation
)
{
	UE_LOG(LogTemp, Warning, TEXT("DragDetected ItemID: %s"), *ItemID.ToString());

	UItemDragDropOperation* DragOperation = NewObject<UItemDragDropOperation>();

	if (!DragOperation)
	{
		return;
	}

	DragOperation->ItemID = ItemID;
	DragOperation->DefaultDragVisual = this;
	DragOperation->Pivot = EDragPivot::MouseDown;

	OutOperation = DragOperation;
}
