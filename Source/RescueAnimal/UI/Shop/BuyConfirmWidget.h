#pragma once

#include "CoreMinimal.h"
#include "UI/Audio/RAUserWidget.h"
#include "RAStructTypes.h"
#include "BuyConfirmWidget.generated.h"

class UButton;
class UImage;
class UTextBlock;
class UInventoryComponent;

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnBuyConfirmClosed);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnBuySucceeded, FText, ResultMessage);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnBuyFailed, FText, ResultMessage);

UCLASS()
class RESCUEANIMAL_API UBuyConfirmWidget : public URAUserWidget
{
	GENERATED_BODY()

public:
	UBuyConfirmWidget(const FObjectInitializer& ObjectInitializer);

	virtual void NativeConstruct() override;

	UFUNCTION(BlueprintCallable, Category = "Shop")
	void SetupBuyConfirm(const FShopItemData& InShopItemData, FName InCurrencyItemID);

	UPROPERTY(BlueprintAssignable, Category = "Shop")
	FOnBuyConfirmClosed OnBuyConfirmClosed;

	UPROPERTY(BlueprintAssignable, Category = "Shop")
	FOnBuySucceeded OnBuySucceeded;

	UPROPERTY(BlueprintAssignable, Category = "Shop")
	FOnBuyFailed OnBuyFailed;

protected:
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UImage> ItemIcon;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> ItemNameText;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> DescriptionText;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> UnitPriceText;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> TotalPriceText;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UImage> CurrencyIcon;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UButton> DecreaseButton;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UButton> IncreaseButton;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> CountText;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UButton> BuyButton;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UButton> CancelButton;

private:
	UFUNCTION()
	void HandleDecreaseClicked();

	UFUNCTION()
	void HandleIncreaseClicked();

	UFUNCTION()
	void HandleBuyClicked();

	UFUNCTION()
	void HandleCancelClicked();

	void RefreshCountText();
	void RefreshPriceText();
	UInventoryComponent* GetInventoryComponent() const;

private:
	UPROPERTY()
	FShopItemData ShopItemData;

	UPROPERTY()
	FName CurrencyItemID = TEXT("Coin");

	int32 CurrentBuyCount = 1;
	int32 MinBuyCount = 1;
	int32 MaxBuyCount = 99;
};
