#pragma once

#include "CoreMinimal.h"
#include "TPSUserWidget.h"
#include "TPSStructTypes.h"
#include "ItemTooltipWidget.generated.h"

class UTextBlock;

UCLASS()
class TPSCAPTURE_API UItemTooltipWidget : public UTPSUserWidget
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = "Tooltip")
	void SetupTooltip(const FItemData& ItemData);

private:
	FText GetItemTypeText(EItemType ItemType) const;

protected:
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> ItemNameText;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> ItemTypeText;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> DescriptionText;
};
