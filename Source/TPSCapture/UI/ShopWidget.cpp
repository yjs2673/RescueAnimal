#include "ShopWidget.h"
#include "ShopActor.h"
#include "BuyConfirmWidget.h"
#include "ShopResultWidget.h"
#include "ShopItemSlotWidget.h"
#include "InventoryComponent.h"
#include "TPSGameInstance.h"
#include "Components/Button.h"
#include "Components/Image.h"
#include "Components/ScrollBox.h"
#include "Components/TextBlock.h"
#include "GameFramework/Pawn.h"

UShopWidget::UShopWidget(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	bPlayOpenCloseSounds = true;
}

void UShopWidget::NativeConstruct()
{
	Super::NativeConstruct();

	if (CloseButton)
	{
		CloseButton->OnClicked.AddUniqueDynamic(this, &UShopWidget::HandleCloseButtonClicked);
	}
}

void UShopWidget::NativeDestruct()
{
	ClearTransientShopWidgets();

	Super::NativeDestruct();
}

void UShopWidget::OpenShop(AShopActor* InShop)
{
	OwningShop = InShop;

	if (!OwningShop)
	{
		return;
	}

	ShopItemDataTable = OwningShop->GetShopItemDataTable();
	CurrencyItemID = OwningShop->GetCurrencyItemID();

	RefreshCurrency();
	RefreshShopItems();
}

void UShopWidget::RefreshCurrency()
{
	UInventoryComponent* InventoryComponent = nullptr;

	if (APawn* OwningPawn = GetOwningPlayerPawn())
	{
		InventoryComponent = OwningPawn->FindComponentByClass<UInventoryComponent>();
	}

	const int32 CurrencyCount = InventoryComponent
		? InventoryComponent->GetItemCount(CurrencyItemID)
		: 0;

	if (CurrencyCountText)
	{
		CurrencyCountText->SetText(FText::AsNumber(CurrencyCount));
	}

	if (!CurrencyIcon)
	{
		return;
	}

	FItemData CurrencyData;
	const UTPSGameInstance* TPSGameInstance = GetGameInstance<UTPSGameInstance>();
	const bool bHasCurrencyData =
		TPSGameInstance && TPSGameInstance->GetItemDataByID(CurrencyItemID, CurrencyData);

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

void UShopWidget::RefreshShopItems()
{
	if (!ShopItemScrollBox)
	{
		UE_LOG(LogTemp, Warning, TEXT("ShopWidget: ShopItemScrollBox is not bound."));
		return;
	}

	ShopItemScrollBox->ClearChildren();

	if (!ShopItemSlotWidgetClass)
	{
		UE_LOG(LogTemp, Warning, TEXT("ShopWidget: ShopItemSlotWidgetClass is not assigned."));
		return;
	}

	if (!ShopItemDataTable)
	{
		UE_LOG(LogTemp, Warning, TEXT("ShopWidget: ShopItemDataTable is not assigned."));
		return;
	}

	TArray<FShopItemData*> ShopItemRows;
	ShopItemDataTable->GetAllRows<FShopItemData>(TEXT("ShopWidget"), ShopItemRows);

	for (const FShopItemData* ShopItemRow : ShopItemRows)
	{
		if (!ShopItemRow || ShopItemRow->ItemID.IsNone())
		{
			continue;
		}

		UShopItemSlotWidget* SlotWidget = CreateWidget<UShopItemSlotWidget>(
			GetOwningPlayer(),
			ShopItemSlotWidgetClass
		);

		if (!SlotWidget)
		{
			continue;
		}

		SlotWidget->SetupShopItem(*ShopItemRow, CurrencyItemID);
		SlotWidget->OnShopItemDoubleClicked.AddUniqueDynamic(
			this,
			&UShopWidget::HandleShopItemDoubleClicked
		);

		ShopItemScrollBox->AddChild(SlotWidget);
	}
}

void UShopWidget::HandleCloseButtonClicked()
{
	ClearTransientShopWidgets();
	OnShopCloseRequested.Broadcast();
}

void UShopWidget::HandleShopItemDoubleClicked(FShopItemData ShopItemData)
{
	if (!BuyConfirmWidgetClass)
	{
		UE_LOG(LogTemp, Warning, TEXT("ShopWidget: BuyConfirmWidgetClass is not assigned."));
		return;
	}

	if (ActiveBuyConfirmWidget)
	{
		ActiveBuyConfirmWidget->RemoveFromParent();
		ActiveBuyConfirmWidget = nullptr;
	}

	ActiveBuyConfirmWidget = CreateWidget<UBuyConfirmWidget>(
		GetOwningPlayer(),
		BuyConfirmWidgetClass
	);

	if (!ActiveBuyConfirmWidget)
	{
		return;
	}

	ActiveBuyConfirmWidget->SetupBuyConfirm(ShopItemData, CurrencyItemID);
	ActiveBuyConfirmWidget->OnBuySucceeded.AddUniqueDynamic(this, &UShopWidget::HandleBuySucceeded);
	ActiveBuyConfirmWidget->OnBuyFailed.AddUniqueDynamic(this, &UShopWidget::HandleBuyFailed);
	ActiveBuyConfirmWidget->AddToViewport();
}

void UShopWidget::HandleBuySucceeded(FText ResultMessage)
{
	ActiveBuyConfirmWidget = nullptr;
	RefreshCurrency();
	ShowShopResult(ResultMessage);
}

void UShopWidget::HandleBuyFailed(FText ResultMessage)
{
	ActiveBuyConfirmWidget = nullptr;
	ShowShopResult(ResultMessage);
}

void UShopWidget::ShowShopResult(const FText& ResultMessage)
{
	if (!ShopResultWidgetClass)
	{
		UE_LOG(LogTemp, Warning, TEXT("ShopWidget: ShopResultWidgetClass is not assigned."));
		return;
	}

	if (ActiveShopResultWidget)
	{
		ActiveShopResultWidget->RemoveFromParent();
		ActiveShopResultWidget = nullptr;
	}

	ActiveShopResultWidget = CreateWidget<UShopResultWidget>(
		GetOwningPlayer(),
		ShopResultWidgetClass
	);

	if (!ActiveShopResultWidget)
	{
		return;
	}

	ActiveShopResultWidget->SetupResult(ResultMessage);
	ActiveShopResultWidget->AddToViewport();
}

void UShopWidget::ClearTransientShopWidgets()
{
	if (ActiveBuyConfirmWidget)
	{
		ActiveBuyConfirmWidget->RemoveFromParent();
		ActiveBuyConfirmWidget = nullptr;
	}

	if (ActiveShopResultWidget)
	{
		ActiveShopResultWidget->RemoveFromParent();
		ActiveShopResultWidget = nullptr;
	}
}
