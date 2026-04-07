#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "CrosshairBowWidget.generated.h"

UCLASS()
class TPSCAPTURE_API UCrosshairBowWidget : public UUserWidget
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
