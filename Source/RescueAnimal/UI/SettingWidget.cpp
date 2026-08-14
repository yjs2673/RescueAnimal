#include "SettingWidget.h"

#include "Components/Button.h"
#include "Components/Slider.h"
#include "InputCoreTypes.h"
#include "RAAudioSubsystem.h"

USettingWidget::USettingWidget(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	bPlayOpenCloseSounds = true;
}

void USettingWidget::NativeConstruct()
{
	Super::NativeConstruct();

	SetIsFocusable(true);

	if (MasterVolumeSlider)
	{
		MasterVolumeSlider->OnValueChanged.AddUniqueDynamic(
			this,
			&USettingWidget::HandleMasterVolumeChanged
		);
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("[SettingWidget] MasterVolumeSlider is not bound."));
	}

	if (BGMVolumeSlider)
	{
		BGMVolumeSlider->OnValueChanged.AddUniqueDynamic(
			this,
			&USettingWidget::HandleBGMVolumeChanged
		);
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("[SettingWidget] BGMVolumeSlider is not bound."));
	}

	if (SFXVolumeSlider)
	{
		SFXVolumeSlider->OnValueChanged.AddUniqueDynamic(
			this,
			&USettingWidget::HandleSFXVolumeChanged
		);
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("[SettingWidget] SFXVolumeSlider is not bound."));
	}

	if (BrightnessSlider)
	{
		BrightnessSlider->OnValueChanged.AddUniqueDynamic(
			this,
			&USettingWidget::HandleBrightnessChanged
		);
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("[SettingWidget] BrightnessSlider is not bound."));
	}

	if (CloseButton)
	{
		CloseButton->OnClicked.AddUniqueDynamic(
			this,
			&USettingWidget::HandleCloseButtonClicked
		);
	}

	RefreshSliderValues();
}

void USettingWidget::NativeDestruct()
{
	if (MasterVolumeSlider)
	{
		MasterVolumeSlider->OnValueChanged.RemoveDynamic(
			this,
			&USettingWidget::HandleMasterVolumeChanged
		);
	}

	if (BGMVolumeSlider)
	{
		BGMVolumeSlider->OnValueChanged.RemoveDynamic(
			this,
			&USettingWidget::HandleBGMVolumeChanged
		);
	}

	if (SFXVolumeSlider)
	{
		SFXVolumeSlider->OnValueChanged.RemoveDynamic(
			this,
			&USettingWidget::HandleSFXVolumeChanged
		);
	}

	if (BrightnessSlider)
	{
		BrightnessSlider->OnValueChanged.RemoveDynamic(
			this,
			&USettingWidget::HandleBrightnessChanged
		);
	}

	if (CloseButton)
	{
		CloseButton->OnClicked.RemoveDynamic(
			this,
			&USettingWidget::HandleCloseButtonClicked
		);
	}

	Super::NativeDestruct();
}

FReply USettingWidget::NativeOnKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent)
{
	if (InKeyEvent.GetKey() == EKeys::Escape)
	{
		OnSettingCloseRequested.Broadcast();
		return FReply::Handled();
	}

	return Super::NativeOnKeyDown(InGeometry, InKeyEvent);
}

void USettingWidget::HandleMasterVolumeChanged(float Value)
{
	if (bIsRefreshingSliders)
	{
		return;
	}

	if (UGameInstance* GameInstance = GetGameInstance())
	{
		if (URAAudioSubsystem* AudioSubsystem = GameInstance->GetSubsystem<URAAudioSubsystem>())
		{
			AudioSubsystem->SetMasterVolume(Value);
		}
	}
}

void USettingWidget::HandleBGMVolumeChanged(float Value)
{
	if (bIsRefreshingSliders)
	{
		return;
	}

	if (UGameInstance* GameInstance = GetGameInstance())
	{
		if (URAAudioSubsystem* AudioSubsystem = GameInstance->GetSubsystem<URAAudioSubsystem>())
		{
			AudioSubsystem->SetBGMVolume(Value);
		}
	}
}

void USettingWidget::HandleSFXVolumeChanged(float Value)
{
	if (bIsRefreshingSliders)
	{
		return;
	}

	if (UGameInstance* GameInstance = GetGameInstance())
	{
		if (URAAudioSubsystem* AudioSubsystem = GameInstance->GetSubsystem<URAAudioSubsystem>())
		{
			AudioSubsystem->SetSFXVolume(Value);
		}
	}
}

void USettingWidget::HandleBrightnessChanged(float Value)
{
	if (bIsRefreshingSliders)
	{
		return;
	}

	if (UGameInstance* GameInstance = GetGameInstance())
	{
		if (URAAudioSubsystem* AudioSubsystem = GameInstance->GetSubsystem<URAAudioSubsystem>())
		{
			AudioSubsystem->SetScreenBrightness(Value);
		}
	}
}

void USettingWidget::HandleCloseButtonClicked()
{
	OnSettingCloseRequested.Broadcast();
}

void USettingWidget::RefreshSliderValues()
{
	URAAudioSubsystem* AudioSubsystem = nullptr;

	if (UGameInstance* GameInstance = GetGameInstance())
	{
		AudioSubsystem = GameInstance->GetSubsystem<URAAudioSubsystem>();
	}

	if (!AudioSubsystem)
	{
		return;
	}

	bIsRefreshingSliders = true;

	if (MasterVolumeSlider)
	{
		MasterVolumeSlider->SetValue(AudioSubsystem->GetMasterVolume());
	}

	if (BGMVolumeSlider)
	{
		BGMVolumeSlider->SetValue(AudioSubsystem->GetBGMVolume());
	}

	if (SFXVolumeSlider)
	{
		SFXVolumeSlider->SetValue(AudioSubsystem->GetSFXVolume());
	}

	if (BrightnessSlider)
	{
		BrightnessSlider->SetValue(AudioSubsystem->GetScreenBrightness());
	}

	bIsRefreshingSliders = false;
}
