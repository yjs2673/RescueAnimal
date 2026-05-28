#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "QuickSlotWidget.generated.h"

class UTextBlock;
class UImage;
class UDragDropOperation;

UCLASS()
class TPSCAPTURE_API UQuickSlotWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	virtual void NativeConstruct() override;

	virtual bool NativeOnDrop(
		const FGeometry& InGeometry,
		const FDragDropEvent& InDragDropEvent,
		UDragDropOperation* InOperation
	) override;

public:
	UFUNCTION(BlueprintCallable, Category = "QuickSlot")
	void SetupQuickSlot(int32 InSlotIndex, FName InItemID, int32 InItemCount);

protected:
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> SlotNumberText;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UImage> ItemIcon;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> ItemCountText;

protected:
	UPROPERTY(BlueprintReadOnly, Category = "QuickSlot")
	int32 SlotIndex = INDEX_NONE;

	UPROPERTY(BlueprintReadOnly, Category = "QuickSlot")
	FName ItemID = NAME_None;

	UPROPERTY(BlueprintReadOnly, Category = "QuickSlot")
	int32 ItemCount = 0;
};