#pragma once

#include "CoreMinimal.h"
#include "RAUserWidget.h"
#include "RAGameEnums.h"
#include "BuffTooltipWidget.generated.h"

class UTextBlock;
class UPlayerStatComponent;

UCLASS()
class RESCUEANIMAL_API UBuffTooltipWidget : public URAUserWidget
{
	GENERATED_BODY()

public:
	void InitializeTooltip(UPlayerStatComponent* InStatComponent, EBuffType InBuffType, const FText& InBuffName, float InBuffValue, bool bInBuffValueIsPercent);

protected:
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> BuffNameText = nullptr;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> BuffValueText = nullptr;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> RemainingTimeText = nullptr;

private:
	UPROPERTY()
	TObjectPtr<UPlayerStatComponent> CachedStatComponent = nullptr;

	EBuffType BuffType = EBuffType::None;
	FText BuffName;
	float BuffValue = 0.0f;
	bool bBuffValueIsPercent = false;

	void RefreshTexts();
	FText MakeBuffValueText() const;
};
