#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "InventorySlotWidget.generated.h"

class UImage;
class UTextBlock;
class UTexture2D;

UCLASS()
class TPSCAPTURE_API UInventorySlotWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	virtual void NativeConstruct() override;

public:
	UFUNCTION(BlueprintCallable, Category = "Inventory")
	void SetupSlot(FName InItemID, int32 InCount);

protected:
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UImage> ItemIcon;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> ItemNameText;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> ItemCountText;

protected:
	UPROPERTY(BlueprintReadOnly, Category = "Inventory")
	FName ItemID;

	UPROPERTY(BlueprintReadOnly, Category = "Inventory")
	int32 Count = 0;
};