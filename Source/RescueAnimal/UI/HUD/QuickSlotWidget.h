#pragma once

#include "CoreMinimal.h"
#include "UI/Audio/RAUserWidget.h"
#include "QuickSlotWidget.generated.h"

class UTextBlock;
class UImage;
class UDragDropOperation;
struct FGeometry;
struct FPointerEvent;

UCLASS()
class RESCUEANIMAL_API UQuickSlotWidget : public URAUserWidget
{
	GENERATED_BODY()

public:
	virtual void NativeConstruct() override;

	virtual FReply NativeOnMouseButtonDown(
		const FGeometry& InGeometry,
		const FPointerEvent& InMouseEvent
	) override;

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
