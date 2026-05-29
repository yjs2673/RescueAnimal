#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "InventoryWidget.generated.h"

class UButton;
class UUniformGridPanel;
class UInventorySlotWidget;
class UQuickSlotBarWidget;

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnInventoryCloseRequested);

UCLASS()
class TPSCAPTURE_API UInventoryWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	virtual void NativeConstruct() override;

public:
	UFUNCTION(BlueprintCallable, Category = "Inventory")
	void RefreshInventory();

public:
	UPROPERTY(BlueprintAssignable, Category = "Inventory")
	FOnInventoryCloseRequested OnInventoryCloseRequested;

protected:
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UButton> CloseButton;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UUniformGridPanel> InventoryGrid;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Inventory")
	TSubclassOf<UInventorySlotWidget> InventorySlotWidgetClass;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Inventory")
	int32 SlotColumnCount = 4;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Inventory")
	int32 MaxSlotCount = 20;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UQuickSlotBarWidget> QuickSlotBar;

private:
	UFUNCTION()
	void HandleCloseButtonClicked();

	void BuildEmptySlots();

private:
	UPROPERTY()
	TArray<TObjectPtr<UInventorySlotWidget>> SlotWidgets;
};