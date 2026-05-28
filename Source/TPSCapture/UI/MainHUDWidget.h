#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "MainHUDWidget.generated.h"

class UButton;
class UQuickSlotBarWidget;

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnInventoryButtonClicked);

UCLASS()
class TPSCAPTURE_API UMainHUDWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	virtual void NativeConstruct() override;

public:
	UPROPERTY(BlueprintAssignable, Category = "MainHUD")
	FOnInventoryButtonClicked OnInventoryButtonClicked;

protected:
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UButton> InventoryButton;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UQuickSlotBarWidget> QuickSlotBar;

private:
	UFUNCTION()
	void HandleInventoryButtonClicked();
};