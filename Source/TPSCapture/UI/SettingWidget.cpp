#include "SettingWidget.h"

#include "Components/Button.h"
#include "Components/Slider.h"
#include "InputCoreTypes.h"
#include "TPSAudioSubsystem.h"

void USettingWidget::NativeConstruct()
{
	Super::NativeConstruct();

	SetIsFocusable(true);
	SetIsEnabled(true);

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
		CloseButton->SetIsEnabled(true);
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

void USettingWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);

	EnsureCloseButtonEnabled();
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
		if (UTPSAudioSubsystem* AudioSubsystem = GameInstance->GetSubsystem<UTPSAudioSubsystem>())
		{
			AudioSubsystem->SetMasterVolume(Value);
		}
	}

	EnsureCloseButtonEnabled();
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

	EnsureCloseButtonEnabled();
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

	EnsureCloseButtonEnabled();
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

	EnsureCloseButtonEnabled();
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
	EnsureCloseButtonEnabled();
}

void USettingWidget::EnsureCloseButtonEnabled()
{
	SetIsEnabled(true);

	if (CloseButton)
	{
		CloseButton->SetIsEnabled(true);
	}
}
