#include "SettingWidget.h"

#include "Components/Button.h"
#include "Components/Slider.h"
#include "TPSAudioSubsystem.h"

void USettingWidget::NativeConstruct()
{
	Super::NativeConstruct();

	if (MasterVolumeSlider)
	{
		MasterVolumeSlider->OnValueChanged.AddUniqueDynamic(
			this,
			&USettingWidget::HandleMasterVolumeChanged
		);
	}

	if (BGMVolumeSlider)
	{
		BGMVolumeSlider->OnValueChanged.AddUniqueDynamic(
			this,
			&USettingWidget::HandleBGMVolumeChanged
		);
	}

	if (SFXVolumeSlider)
	{
		SFXVolumeSlider->OnValueChanged.AddUniqueDynamic(
			this,
			&USettingWidget::HandleSFXVolumeChanged
		);
	}

	if (BrightnessSlider)
	{
		BrightnessSlider->OnValueChanged.AddUniqueDynamic(
			this,
			&USettingWidget::HandleBrightnessChanged
		);
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

void USettingWidget::HandleMasterVolumeChanged(float Value)
{
	if (bIsRefreshingSliders)
	{
		return;
	}

	if (UGameInstance* GameInstance = GetGameInstance())
	{
		if (UTPSAudioSubsystem* AudioSubsystem = GameInstance->GetSubsystem<UTPSAudioSubsystem>())
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
		if (UTPSAudioSubsystem* AudioSubsystem = GameInstance->GetSubsystem<UTPSAudioSubsystem>())
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
		if (UTPSAudioSubsystem* AudioSubsystem = GameInstance->GetSubsystem<UTPSAudioSubsystem>())
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
		if (UTPSAudioSubsystem* AudioSubsystem = GameInstance->GetSubsystem<UTPSAudioSubsystem>())
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
	UTPSAudioSubsystem* AudioSubsystem = nullptr;

	if (UGameInstance* GameInstance = GetGameInstance())
	{
		AudioSubsystem = GameInstance->GetSubsystem<UTPSAudioSubsystem>();
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
