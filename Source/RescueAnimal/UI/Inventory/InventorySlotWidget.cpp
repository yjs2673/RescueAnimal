#include "InventorySlotWidget.h"

#include "PlayerInteractionComponent.h"
#include "ItemTooltipWidget.h"
#include "ItemDragDropOperation.h"
#include "Blueprint/WidgetBlueprintLibrary.h"
#include "Components/Image.h"
#include "Components/TextBlock.h"
#include "Input/Reply.h"
#include "RAGameInstance.h"
#include "RACharacter.h"
#include "Engine/World.h"

void UInventorySlotWidget::NativeConstruct()
{
	Super::NativeConstruct();

	SetEmptySlot();
}

void UInventorySlotWidget::SetEmptySlot()
{
	ItemID = NAME_None;
	Count = 0;
	LastClickedItemID = NAME_None;
	LastClickTime = -1.0f;

	if (ItemIcon)
	{
		ItemIcon->SetBrushFromTexture(nullptr);
		ItemIcon->SetVisibility(ESlateVisibility::Hidden);
	}

	if (ItemNameText)
	{
		ItemNameText->SetText(FText::GetEmpty());
		ItemNameText->SetVisibility(ESlateVisibility::Collapsed);
	}

	if (ItemCountText)
	{
		ItemCountText->SetText(FText::GetEmpty());
		ItemCountText->SetVisibility(ESlateVisibility::Collapsed);
	}

	ClearTooltip();
}

void UInventorySlotWidget::SetupSlot(FName InItemID, int32 InCount)
{
	if (ItemID != InItemID)
	{
		LastClickedItemID = NAME_None;
		LastClickTime = -1.0f;
	}

	ItemID = InItemID;
	Count = FMath::Max(0, InCount);

	if (ItemID.IsNone() || Count <= 0)
	{
		SetEmptySlot();
		return;
	}

	FItemData ItemData;
	const URAGameInstance* RAGameInstance = GetGameInstance<URAGameInstance>();
	const bool bHasItemData = RAGameInstance && RAGameInstance->GetItemDataByID(ItemID, ItemData);

	if (ItemNameText)
	{
		// 일반 게임 인벤토리처럼 슬롯 안에서는 이름을 숨김.
		// 나중에 툴팁으로 보여주는 방식 추천.
		ItemNameText->SetText(bHasItemData && !ItemData.ItemName.IsEmpty()
			? ItemData.ItemName
			: FText::FromName(ItemID));

		ItemNameText->SetVisibility(ESlateVisibility::Collapsed);
	}

	if (ItemCountText)
	{
		ItemCountText->SetText(FText::FromString(FString::Printf(TEXT("x%d"), Count)));
		ItemCountText->SetVisibility(ESlateVisibility::Visible);
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

	UpdateTooltip();
}

FReply UInventorySlotWidget::NativeOnMouseButtonDown(
	const FGeometry& InGeometry,
	const FPointerEvent& InMouseEvent
)
{
	if (InMouseEvent.IsMouseButtonDown(EKeys::LeftMouseButton) && !ItemID.IsNone() && Count > 0)
	{
		if (TryUseItemOnDoubleClick())
		{
			return FReply::Handled();
		}

		if (UWorld* World = GetWorld())
		{
			LastClickedItemID = ItemID;
			LastClickTime = World->GetTimeSeconds();
		}

		return UWidgetBlueprintLibrary::DetectDragIfPressed(
			InMouseEvent,
			this,
			EKeys::LeftMouseButton
		).NativeReply;
	}

	return Super::NativeOnMouseButtonDown(InGeometry, InMouseEvent);
}

FReply UInventorySlotWidget::NativeOnMouseButtonDoubleClick(
	const FGeometry& InGeometry,
	const FPointerEvent& InMouseEvent
)
{
	if (InMouseEvent.GetEffectingButton() == EKeys::LeftMouseButton && !ItemID.IsNone() && Count > 0)
	{
		UseSlotItem();
		LastClickedItemID = NAME_None;
		LastClickTime = -1.0f;
		return FReply::Handled();
	}

	return Super::NativeOnMouseButtonDoubleClick(InGeometry, InMouseEvent);
}
void UInventorySlotWidget::NativeOnDragDetected(
	const FGeometry& InGeometry,
	const FPointerEvent& InMouseEvent,
	UDragDropOperation*& OutOperation
)
{
	if (ItemID.IsNone() || Count <= 0)
	{
		return;
	}

	LastClickedItemID = NAME_None;
	LastClickTime = -1.0f;

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

bool UInventorySlotWidget::GetSlotItemData(FItemData& OutItemData) const
{
	if (ItemID.IsNone() || Count <= 0)
		return false;

	const URAGameInstance* RAGameInstance = GetGameInstance<URAGameInstance>();
	return RAGameInstance && RAGameInstance->GetItemDataByID(ItemID, OutItemData);
}

bool UInventorySlotWidget::UseSlotItem()
{
	FItemData ItemData;
	if (!GetSlotItemData(ItemData))
		return false;

	if (ItemData.ItemType != EItemType::Consumable && ItemData.ItemType != EItemType::Weapon)
		return false;

	ARACharacter* PlayerCharacter = Cast<ARACharacter>(GetOwningPlayerPawn());
	if (!PlayerCharacter)
		return false;

	UPlayerInteractionComponent* PlayerInteractionComponent = PlayerCharacter->GetPlayerInteractionComponent();
	return PlayerInteractionComponent && PlayerInteractionComponent->UseInventoryItem(ItemID);
}
bool UInventorySlotWidget::TryUseItemOnDoubleClick()
{
	UWorld* World = GetWorld();
	if (!World)
		return false;

	const float CurrentTime = World->GetTimeSeconds();
	const bool bIsDoubleClick = LastClickedItemID == ItemID &&
		CurrentTime - LastClickTime <= DoubleClickUseThreshold;

	if (!bIsDoubleClick)
		return false;

	LastClickedItemID = NAME_None;
	LastClickTime = -1.0f;

	return UseSlotItem();
}
void UInventorySlotWidget::UpdateTooltip()
{
	if (ItemID.IsNone() || Count <= 0)
	{
		ClearTooltip();
		return;
	}

	if (!ItemTooltipWidgetClass)
	{
		ClearTooltip();
		return;
	}

	FItemData ItemData;
	const URAGameInstance* RAGameInstance = GetGameInstance<URAGameInstance>();
	const bool bHasItemData = RAGameInstance && RAGameInstance->GetItemDataByID(ItemID, ItemData);

	if (!bHasItemData)
	{
		ClearTooltip();
		return;
	}

	UItemTooltipWidget* TooltipWidget = CreateWidget<UItemTooltipWidget>(
		GetOwningPlayer(),
		ItemTooltipWidgetClass
	);

	if (!TooltipWidget)
	{
		ClearTooltip();
		return;
	}

	TooltipWidget->SetupTooltip(ItemData);

	SetToolTip(TooltipWidget);
}

void UInventorySlotWidget::ClearTooltip()
{
	SetToolTip(nullptr);
}
