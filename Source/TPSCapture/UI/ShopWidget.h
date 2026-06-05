#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "TPSStructTypes.h"
#include "ShopWidget.generated.h"

class AShopActor;
class UButton;
class UImage;
class UScrollBox;
class UTextBlock;
class UDataTable;
class UShopItemSlotWidget;
class UBuyConfirmWidget;
class UShopResultWidget;

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnShopCloseRequested);

UCLASS()
class TPSCAPTURE_API UShopWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;

	UFUNCTION(BlueprintCallable, Category = "Shop")
	void OpenShop(AShopActor* InShop);

	UFUNCTION(BlueprintCallable, Category = "Shop")
	void RefreshShopItems();

	UFUNCTION(BlueprintCallable, Category = "Shop")
	void RefreshCurrency();

	UPROPERTY(BlueprintAssignable, Category = "Shop")
	FOnShopCloseRequested OnShopCloseRequested;

protected:
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UButton> CloseButton;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UScrollBox> ShopItemScrollBox;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UImage> CurrencyIcon;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> CurrencyCountText;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Shop")
	TSubclassOf<UShopItemSlotWidget> ShopItemSlotWidgetClass;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Shop")
	TSubclassOf<UBuyConfirmWidget> BuyConfirmWidgetClass;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Shop")
	TSubclassOf<UShopResultWidget> ShopResultWidgetClass;

private:
	UFUNCTION()
	void HandleCloseButtonClicked();

	UFUNCTION()
	void HandleShopItemDoubleClicked(FShopItemData ShopItemData);

	UFUNCTION()
	void HandleBuySucceeded(FText ResultMessage);

	UFUNCTION()
	void HandleBuyFailed(FText ResultMessage);

	void ShowShopResult(const FText& ResultMessage);
	void ClearTransientShopWidgets();

private:
	UPROPERTY()
	TObjectPtr<AShopActor> OwningShop;

	UPROPERTY()
	TObjectPtr<UDataTable> ShopItemDataTable;

	UPROPERTY()
	FName CurrencyItemID = TEXT("Coin");

	UPROPERTY()
	TObjectPtr<UBuyConfirmWidget> ActiveBuyConfirmWidget;

	UPROPERTY()
	TObjectPtr<UShopResultWidget> ActiveShopResultWidget;
};
