#include "InventorySlotWidget.h"

#include "Components/Image.h"
#include "Components/TextBlock.h"

void UInventorySlotWidget::NativeConstruct()
{
	Super::NativeConstruct();

	if (!ItemIcon)
	{
		UE_LOG(LogTemp, Warning, TEXT("InventorySlotWidget: ItemIcon is not bound."));
	}

	if (!ItemNameText)
	{
		UE_LOG(LogTemp, Warning, TEXT("InventorySlotWidget: ItemNameText is not bound."));
	}

	if (!ItemCountText)
	{
		UE_LOG(LogTemp, Warning, TEXT("InventorySlotWidget: ItemCountText is not bound."));
	}
}

void UInventorySlotWidget::SetupSlot(FName InItemID, int32 InCount)
{
	ItemID = InItemID;
	Count = FMath::Max(0, InCount);

	if (ItemNameText)
	{
		ItemNameText->SetText(FText::FromName(ItemID));
	}

	if (ItemCountText)
	{
		ItemCountText->SetText(FText::FromString(FString::Printf(TEXT("x%d"), Count)));
	}

	if (ItemIcon)
	{
		ItemIcon->SetVisibility(ESlateVisibility::Visible);
	}
}