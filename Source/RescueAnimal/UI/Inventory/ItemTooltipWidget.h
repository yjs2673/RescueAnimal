#pragma once

#include "CoreMinimal.h"
#include "UI/Audio/RAUserWidget.h"
#include "RAStructTypes.h"
#include "ItemTooltipWidget.generated.h"

class UTextBlock;

UCLASS()
class RESCUEANIMAL_API UItemTooltipWidget : public URAUserWidget
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
