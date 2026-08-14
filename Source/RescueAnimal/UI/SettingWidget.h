#pragma once

#include "CoreMinimal.h"
#include "RAUserWidget.h"
#include "SettingWidget.generated.h"

class UButton;
class USlider;

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnSettingCloseRequested);

UCLASS()
class RESCUEANIMAL_API USettingWidget : public URAUserWidget
{
	GENERATED_BODY()

public:
	USettingWidget(const FObjectInitializer& ObjectInitializer);

	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;
	virtual FReply NativeOnKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent) override;

	UPROPERTY(BlueprintAssignable, Category = "Setting")
	FOnSettingCloseRequested OnSettingCloseRequested;

protected:
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<USlider> MasterVolumeSlider;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<USlider> BGMVolumeSlider;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<USlider> SFXVolumeSlider;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<USlider> BrightnessSlider;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UButton> CloseButton;

private:
	UFUNCTION()
	void HandleMasterVolumeChanged(float Value);

	UFUNCTION()
	void HandleBGMVolumeChanged(float Value);

	UFUNCTION()
	void HandleSFXVolumeChanged(float Value);

	UFUNCTION()
	void HandleBrightnessChanged(float Value);

	UFUNCTION()
	void HandleCloseButtonClicked();

	void RefreshSliderValues();

	bool bIsRefreshingSliders = false;
};
