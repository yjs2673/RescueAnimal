#pragma once

#include "CoreMinimal.h"
#include "RAUserWidget.h"
#include "InventorySlotWidget.generated.h"

class UImage;
class UTextBlock;
class UDragDropOperation;
class UItemTooltipWidget;
struct FItemData;
struct FGeometry;
struct FPointerEvent;

UCLASS()
class RESCUEANIMAL_API UInventorySlotWidget : public URAUserWidget
{
	GENERATED_BODY()

public:
	virtual void NativeConstruct() override;

	virtual FReply NativeOnMouseButtonDown(
		const FGeometry& InGeometry,
		const FPointerEvent& InMouseEvent
	) override;

	virtual FReply NativeOnMouseButtonDoubleClick(
		const FGeometry& InGeometry,
		const FPointerEvent& InMouseEvent
	) override;

	virtual void NativeOnDragDetected(
		const FGeometry& InGeometry,
		const FPointerEvent& InMouseEvent,
		UDragDropOperation*& OutOperation
	) override;

public:
	UFUNCTION(BlueprintCallable, Category = "Inventory")
	void SetupSlot(FName InItemID, int32 InCount);

	UFUNCTION(BlueprintCallable, Category = "Inventory")
	void SetEmptySlot();

protected:
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UImage> ItemIcon;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> ItemNameText;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> ItemCountText;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Tooltip")
	TSubclassOf<UItemTooltipWidget> ItemTooltipWidgetClass;

protected:
	UPROPERTY(BlueprintReadOnly, Category = "Inventory")
	FName ItemID = NAME_None;

	UPROPERTY(BlueprintReadOnly, Category = "Inventory")
	int32 Count = 0;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Inventory")
	float DoubleClickUseThreshold = 0.3f;

private:
	void UpdateTooltip();
	void ClearTooltip();
	bool GetSlotItemData(FItemData& OutItemData) const;
	bool UseSlotItem();
	bool TryUseItemOnDoubleClick();

	FName LastClickedItemID = NAME_None;
	float LastClickTime = -1.0f;
};
