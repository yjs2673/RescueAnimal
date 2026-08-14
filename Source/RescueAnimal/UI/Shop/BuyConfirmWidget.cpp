#include "BuyConfirmWidget.h"
#include "InventoryComponent.h"
#include "RAGameInstance.h"
#include "Components/Button.h"
#include "Components/Image.h"
#include "Components/TextBlock.h"
#include "GameFramework/Pawn.h"

UBuyConfirmWidget::UBuyConfirmWidget(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	bPlayOpenCloseSounds = true;
}

void UBuyConfirmWidget::NativeConstruct()
{
	Super::NativeConstruct();

	if (DecreaseButton)
	{
		DecreaseButton->OnClicked.AddUniqueDynamic(this, &UBuyConfirmWidget::HandleDecreaseClicked);
	}

	if (IncreaseButton)
	{
		IncreaseButton->OnClicked.AddUniqueDynamic(this, &UBuyConfirmWidget::HandleIncreaseClicked);
	}

	if (BuyButton)
	{
		BuyButton->OnClicked.AddUniqueDynamic(this, &UBuyConfirmWidget::HandleBuyClicked);
	}

	if (CancelButton)
	{
		CancelButton->OnClicked.AddUniqueDynamic(this, &UBuyConfirmWidget::HandleCancelClicked);
	}
}

void UBuyConfirmWidget::SetupBuyConfirm(const FShopItemData& InShopItemData, FName InCurrencyItemID)
{
	ShopItemData = InShopItemData;
	CurrencyItemID = ShopItemData.CurrencyItemID.IsNone()
		? InCurrencyItemID
		: ShopItemData.CurrencyItemID;

	MinBuyCount = FMath::Clamp(ShopItemData.MinBuyCount, 1, 99);
	MaxBuyCount = FMath::Clamp(ShopItemData.MaxBuyCount, MinBuyCount, 99);
	CurrentBuyCount = MinBuyCount;

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

	if (UnitPriceText)
	{
		UnitPriceText->SetText(FText::AsNumber(ShopItemData.Price));
	}

	RefreshCountText();
	RefreshPriceText();
}

void UBuyConfirmWidget::HandleDecreaseClicked()
{
	CurrentBuyCount = CurrentBuyCount <= MinBuyCount
		? MaxBuyCount
		: CurrentBuyCount - 1;

	RefreshCountText();
	RefreshPriceText();
}

void UBuyConfirmWidget::HandleIncreaseClicked()
{
	CurrentBuyCount = CurrentBuyCount >= MaxBuyCount
		? MinBuyCount
		: CurrentBuyCount + 1;

	RefreshCountText();
	RefreshPriceText();
}

void UBuyConfirmWidget::HandleBuyClicked()
{
	UInventoryComponent* InventoryComponent = GetInventoryComponent();
	if (!InventoryComponent || ShopItemData.ItemID.IsNone())
	{
		OnBuyFailed.Broadcast(FText::FromString(TEXT("\uAD6C\uB9E4 \uC2E4\uD328")));
		RemoveFromParent();
		return;
	}

	const int32 TotalPrice = ShopItemData.Price * CurrentBuyCount;

	if (!InventoryComponent->HasItem(CurrencyItemID, TotalPrice))
	{
		OnBuyFailed.Broadcast(FText::FromString(TEXT("\uAD6C\uB9E4 \uC2E4\uD328: \uC7AC\uD654 \uBD80\uC871")));
		RemoveFromParent();
		return;
	}

	if (!InventoryComponent->CanAddItem(ShopItemData.ItemID, CurrentBuyCount))
	{
		OnBuyFailed.Broadcast(FText::FromString(TEXT("\uAD6C\uB9E4 \uC2E4\uD328: \uC778\uBCA4\uD1A0\uB9AC \uCE78 \uBD80\uC871")));
		RemoveFromParent();
		return;
	}

	if (!InventoryComponent->RemoveItem(CurrencyItemID, TotalPrice))
	{
		OnBuyFailed.Broadcast(FText::FromString(TEXT("\uAD6C\uB9E4 \uC2E4\uD328: \uC7AC\uD654 \uBD80\uC871")));
		RemoveFromParent();
		return;
	}

	InventoryComponent->AddItem(ShopItemData.ItemID, CurrentBuyCount);

	OnBuySucceeded.Broadcast(FText::FromString(TEXT("\uAD6C\uB9E4 \uC131\uACF5")));
	RemoveFromParent();
}

void UBuyConfirmWidget::HandleCancelClicked()
{
	OnBuyConfirmClosed.Broadcast();
	RemoveFromParent();
}

void UBuyConfirmWidget::RefreshCountText()
{
	if (CountText)
	{
		CountText->SetText(FText::AsNumber(CurrentBuyCount));
	}
}

void UBuyConfirmWidget::RefreshPriceText()
{
	if (TotalPriceText)
	{
		TotalPriceText->SetText(FText::AsNumber(ShopItemData.Price * CurrentBuyCount));
	}
}

UInventoryComponent* UBuyConfirmWidget::GetInventoryComponent() const
{
	APawn* OwningPawn = GetOwningPlayerPawn();
	if (!OwningPawn)
	{
		return nullptr;
	}

	return OwningPawn->FindComponentByClass<UInventoryComponent>();
}
