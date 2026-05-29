#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "MainHUDWidget.generated.h"

class UButton;
class UQuickSlotBarWidget;
class UCrosshairBowWidget;

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnInventoryButtonClicked);

UCLASS()
class TPSCAPTURE_API UMainHUDWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	virtual void NativeConstruct() override;

public:
	UCrosshairBowWidget* GetCrosshairBowWidget() const;

	UPROPERTY(BlueprintAssignable, Category = "MainHUD")
	FOnInventoryButtonClicked OnInventoryButtonClicked;

protected:
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UButton> InventoryButton;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UQuickSlotBarWidget> QuickSlotBar;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UCrosshairBowWidget> CrosshairBowWidget;

private:
	UFUNCTION()
	void HandleInventoryButtonClicked();
};
