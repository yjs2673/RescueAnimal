#include "InventorySlotWidget.h"

#include "Components/Image.h"
#include "Components/TextBlock.h"
#include "TPSGameInstance.h"

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
		if (bHasItemData && ItemData.Icon)
		{
			ItemIcon->SetBrushFromTexture(ItemData.Icon, true);
			ItemIcon->SetVisibility(ESlateVisibility::Visible);
		}
		else
		{
			ItemIcon->SetVisibility(ESlateVisibility::Hidden);
		}
	}
}
