#include "QuickSlotWidget.h"

#include "Components/TextBlock.h"
#include "Components/Image.h"

void UQuickSlotWidget::NativeConstruct()
{
	Super::NativeConstruct();

	if (!SlotNumberText)
	{
		UE_LOG(LogTemp, Warning, TEXT("QuickSlotWidget: SlotNumberText is not bound."));
	}

	if (!ItemIcon)
	{
		UE_LOG(LogTemp, Warning, TEXT("QuickSlotWidget: ItemIcon is not bound."));
	}

	if (!ItemCountText)
	{
		UE_LOG(LogTemp, Warning, TEXT("QuickSlotWidget: ItemCountText is not bound."));
	}
}

void UQuickSlotWidget::SetupQuickSlot(int32 InSlotIndex, FName InItemID, int32 InItemCount)
{
	SlotIndex = InSlotIndex;
	ItemID = InItemID;
	ItemCount = FMath::Max(0, InItemCount);

	if (SlotNumberText)
	{
		SlotNumberText->SetText(FText::AsNumber(SlotIndex + 1));
	}

	const bool bHasItem = !ItemID.IsNone();

	if (ItemIcon)
	{
		ItemIcon->SetVisibility(bHasItem ? ESlateVisibility::Visible : ESlateVisibility::Hidden);
	}

	if (ItemCountText)
	{
		if (bHasItem && ItemCount > 0)
		{
			ItemCountText->SetText(FText::FromString(FString::Printf(TEXT("x%d"), ItemCount)));
			ItemCountText->SetVisibility(ESlateVisibility::Visible);
		}
		else
		{
			ItemCountText->SetText(FText::GetEmpty());
			ItemCountText->SetVisibility(ESlateVisibility::Hidden);
		}
	}
}