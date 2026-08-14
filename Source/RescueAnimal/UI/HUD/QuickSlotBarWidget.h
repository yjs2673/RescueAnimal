#pragma once

#include "CoreMinimal.h"
#include "UI/Audio/RAUserWidget.h"
#include "QuickSlotBarWidget.generated.h"

class UUniformGridPanel;
class UQuickSlotWidget;
class UQuickSlotComponent;
class UInventoryComponent;

UCLASS()
class RESCUEANIMAL_API UQuickSlotBarWidget : public URAUserWidget
{
	GENERATED_BODY()

public:
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;

public:
	UFUNCTION(BlueprintCallable, Category = "QuickSlot")
	void RefreshQuickSlots();

protected:
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UUniformGridPanel> QuickSlotGrid;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "QuickSlot")
	TSubclassOf<UQuickSlotWidget> QuickSlotWidgetClass;

private:
	UFUNCTION()
	void HandleQuickSlotChanged(int32 SlotIndex, FName ItemID);
	
	UFUNCTION()
	void HandleInventoryItemChanged(FName ItemID, int32 NewCount);

private:
	UPROPERTY()
	TObjectPtr<UQuickSlotComponent> CachedQuickSlotComponent;

	UPROPERTY()
	TObjectPtr<UInventoryComponent> CachedInventoryComponent;
};
