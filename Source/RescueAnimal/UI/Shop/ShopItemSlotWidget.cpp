#include "ShopItemSlotWidget.h"
#include "ItemTooltipWidget.h"
#include "RAGameInstance.h"
#include "Components/Button.h"
#include "Components/Image.h"
#include "Components/TextBlock.h"
#include "Input/Reply.h"

void UShopItemSlotWidget::NativeConstruct()
{
	Super::NativeConstruct();

	if (ItemButton)
	{
		ItemButton->OnClicked.AddUniqueDynamic(
			this,
			&UShopItemSlotWidget::HandleItemButtonClicked
		);
	}
}

FReply UShopItemSlotWidget::NativeOnMouseButtonDoubleClick(
	const FGeometry& InGeometry,
	const FPointerEvent& InMouseEvent
)
{
	if (InMouseEvent.GetEffectingButton() == EKeys::LeftMouseButton &&
		!ShopItemData.ItemID.IsNone())
	{
		OnShopItemDoubleClicked.Broadcast(ShopItemData);
		return FReply::Handled();
	}

	return Super::NativeOnMouseButtonDoubleClick(InGeometry, InMouseEvent);
}

void UShopItemSlotWidget::SetupShopItem(
	const FShopItemData& InShopItemData,
	FName InCurrencyItemID
)
{
	ShopItemData = InShopItemData;
	CurrencyItemID = ShopItemData.CurrencyItemID.IsNone()
		? InCurrencyItemID
		: ShopItemData.CurrencyItemID;

	FItemData ItemData;
	FItemData CurrencyData;

	const URAGameInstance* RAGameInstance = GetGameInstance<URAGameInstance>();
	const bool bHasItemData =
		RAGameInstance && RAGameInstance->GetItemDataByID(ShopItemData.ItemID, ItemData);
	const bool bHasCurrencyData =
		RAGameInstance && RAGameInstance->GetItemDataByID(CurrencyItemID, CurrencyData);

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

	if (ItemNameText)
	{
		FText DisplayName = ShopItemData.ItemName;

		if (DisplayName.IsEmpty() && bHasItemData)
		{
			DisplayName = ItemData.ItemName;
		}

		if (DisplayName.IsEmpty())
		{
			DisplayName = FText::FromName(ShopItemData.ItemID);
		}

		ItemNameText->SetText(DisplayName);
	}

	if (PriceText)
	{
		PriceText->SetText(FText::AsNumber(ShopItemData.Price));
	}

	if (DescriptionText)
	{
		if (bHasItemData && !ItemData.Description.IsEmpty())
		{
			DescriptionText->SetText(ItemData.Description);
			DescriptionText->SetVisibility(ESlateVisibility::Visible);
		}
		else
		{
			DescriptionText->SetText(FText::GetEmpty());
			DescriptionText->SetVisibility(ESlateVisibility::Collapsed);
		}
	}

	if (CurrencyIcon)
	{
		UTexture2D* CurrencyTexture = nullptr;

		if (bHasCurrencyData)
		{
			CurrencyTexture = CurrencyData.Image ? CurrencyData.Image : CurrencyData.Icon;
		}

		if (CurrencyTexture)
		{
			CurrencyIcon->SetBrushFromTexture(CurrencyTexture, true);
			CurrencyIcon->SetVisibility(ESlateVisibility::Visible);
		}
		else
		{
			CurrencyIcon->SetVisibility(ESlateVisibility::Hidden);
		}
	}

	UpdateTooltip();
}

void UShopItemSlotWidget::HandleItemButtonClicked()
{
	// Optional: selection highlight can be handled here later.
}

void UShopItemSlotWidget::UpdateTooltip()
{
	if (ShopItemData.ItemID.IsNone() || !ItemTooltipWidgetClass)
	{
		ClearTooltip();
		return;
	}

	FItemData ItemData;
	const URAGameInstance* RAGameInstance = GetGameInstance<URAGameInstance>();
	const bool bHasItemData =
		RAGameInstance && RAGameInstance->GetItemDataByID(ShopItemData.ItemID, ItemData);

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

void UShopItemSlotWidget::ClearTooltip()
{
	SetToolTip(nullptr);
}
