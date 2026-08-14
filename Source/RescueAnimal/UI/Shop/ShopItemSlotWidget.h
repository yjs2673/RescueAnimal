#pragma once

#include "CoreMinimal.h"
#include "UI/Audio/RAUserWidget.h"
#include "RAStructTypes.h"
#include "ShopItemSlotWidget.generated.h"

class UButton;
class UImage;
class UTextBlock;
class UItemTooltipWidget;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(
	FOnShopItemDoubleClicked,
	FShopItemData,
	ShopItemData
);

UCLASS()
class RESCUEANIMAL_API UShopItemSlotWidget : public URAUserWidget
{
	GENERATED_BODY()

public:
	virtual void NativeConstruct() override;

	virtual FReply NativeOnMouseButtonDoubleClick(
		const FGeometry& InGeometry,
		const FPointerEvent& InMouseEvent
	) override;

	UFUNCTION(BlueprintCallable, Category = "Shop")
	void SetupShopItem(const FShopItemData& InShopItemData, FName InCurrencyItemID);

	UPROPERTY(BlueprintAssignable, Category = "Shop")
	FOnShopItemDoubleClicked OnShopItemDoubleClicked;

protected:
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UButton> ItemButton;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UImage> ItemIcon;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> ItemNameText;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> PriceText;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> DescriptionText;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UImage> CurrencyIcon;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Shop")
	TSubclassOf<UItemTooltipWidget> ItemTooltipWidgetClass;

private:
	UFUNCTION()
	void HandleItemButtonClicked();

	void UpdateTooltip();
	void ClearTooltip();

private:
	UPROPERTY()
	FShopItemData ShopItemData;

	UPROPERTY()
	FName CurrencyItemID = TEXT("Coin");
};
