#pragma once

#include "CoreMinimal.h"
#include "RAUserWidget.h"
#include "CrosshairBowWidget.generated.h"

UCLASS()
class RESCUEANIMAL_API UCrosshairBowWidget : public URAUserWidget
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintImplementableEvent, Category = "Crosshair")
	void SetChargeAlpha(float Alpha);

	UFUNCTION(BlueprintImplementableEvent, Category = "Crosshair")
	void SetCrosshairVisible(bool bVisible);

	UFUNCTION(BlueprintImplementableEvent, Category = "Crosshair")
	void ResetCrosshair();

	UFUNCTION(BlueprintImplementableEvent, Category = "Crosshair")
	void PlayFullChargeEffect();
};
