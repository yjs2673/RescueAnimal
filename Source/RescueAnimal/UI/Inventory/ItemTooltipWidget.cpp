#include "ItemTooltipWidget.h"

#include "Components/TextBlock.h"

void UItemTooltipWidget::SetupTooltip(const FItemData& ItemData)
{
	if (ItemNameText)
	{
		ItemNameText->SetText(ItemData.ItemName);
	}

	if (ItemTypeText)
	{
		ItemTypeText->SetText(GetItemTypeText(ItemData.ItemType));
	}

	if (DescriptionText)
	{
		DescriptionText->SetText(ItemData.Description);
	}
}

FText UItemTooltipWidget::GetItemTypeText(EItemType ItemType) const
{
	switch (ItemType)
	{
	case EItemType::Consumable:
		return FText::FromString(TEXT("소비"));

	case EItemType::Weapon:
		return FText::FromString(TEXT("무기"));

	case EItemType::Material:
		return FText::FromString(TEXT("재료"));

	default:
		return FText::FromString(TEXT("기타"));
	}
}