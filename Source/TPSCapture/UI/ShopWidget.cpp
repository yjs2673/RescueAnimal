#include "ShopWidget.h"
#include "ShopActor.h"
#include "ShopItemSlotWidget.h"
#include "InventoryComponent.h"
#include "TPSGameInstance.h"
#include "Components/Button.h"
#include "Components/Image.h"
#include "Components/ScrollBox.h"
#include "Components/TextBlock.h"
#include "GameFramework/Pawn.h"

void UShopWidget::NativeConstruct()
{
	Super::NativeConstruct();

	if (CloseButton)
	{
		CloseButton->OnClicked.AddUniqueDynamic(this, &UShopWidget::HandleCloseButtonClicked);
	}
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
	OnShopCloseRequested.Broadcast();
}

void UShopWidget::HandleShopItemDoubleClicked(FShopItemData ShopItemData)
{
	// Next step:
	// Open buy confirm/count widget with ShopItemData and CurrencyItemID.
	UE_LOG(LogTemp, Warning, TEXT("Shop item double clicked: %s"), *ShopItemData.ItemID.ToString());
}