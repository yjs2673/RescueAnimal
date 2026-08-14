#pragma once

#include "CoreMinimal.h"
#include "UI/Audio/RAUserWidget.h"
#include "Characters/Player/Components/PlayerStatComponent.h"
#include "BuffIconWidget.generated.h"

class UImage;
class UBuffTooltipWidget;

UCLASS()
class RESCUEANIMAL_API UBuffIconWidget : public URAUserWidget
{
	GENERATED_BODY()

public:
	void InitializeBuffIcon(UPlayerStatComponent* InStatComponent, const FActiveBuffInfo& InBuffInfo);

protected:
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UImage> BuffIconImage = nullptr;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Buff")
	TSubclassOf<UBuffTooltipWidget> TooltipWidgetClass;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Buff", meta = (ClampMin = "0.0"))
	float BlinkStartTime = 5.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Buff", meta = (ClampMin = "0.1"))
	float BlinkSpeed = 6.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Buff", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float MinBlinkOpacity = 0.35f;

private:
	UPROPERTY()
	TObjectPtr<UPlayerStatComponent> CachedStatComponent = nullptr;

	FActiveBuffInfo BuffInfo;

	void SetupIcon();
	void SetupTooltip();
	void UpdateBlinkOpacity();
};
