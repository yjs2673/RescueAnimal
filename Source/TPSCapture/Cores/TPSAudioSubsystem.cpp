#include "TPSAudioSubsystem.h"

#include "Components/AudioComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Sound/SoundBase.h"
#include "Sound/SoundClass.h"
#include "Sound/SoundMix.h"
#include "Engine/Engine.h"
#include "Engine/GameViewportClient.h"
#include "Styling/CoreStyle.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/SOverlay.h"

void UTPSAudioSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);

	LoadSoundClassSettings();
	LoadAudioSettings();
	ApplyRuntimeSettings();
}

void UTPSAudioSubsystem::Deinitialize()
{
	StopBGM(0.0f);
	RemoveBrightnessOverlay();

	Super::Deinitialize();
}

void UTPSAudioSubsystem::PlayBGM(USoundBase* BGM, float FadeInTime, float FadeOutTime)
{
	if (!BGM)
	{
		StopBGM(FadeOutTime);
		return;
	}

	const float ClampedFadeInTime = FMath::Max(0.0f, FadeInTime);
	const float ClampedFadeOutTime = FMath::Max(0.0f, FadeOutTime);

	if (CurrentBGMComponent && CurrentBGMSound == BGM)
	{
		ApplyCurrentBGMVolume();

		if (!CurrentBGMComponent->IsPlaying())
		{
			CurrentBGMComponent->FadeIn(ClampedFadeInTime, GetBGMComponentVolume());
		}

		return;
	}

	if (CurrentBGMComponent)
	{
		CurrentBGMComponent->FadeOut(ClampedFadeOutTime, 0.0f);
		CurrentBGMComponent = nullptr;
	}

	CurrentBGMSound = BGM;
	CurrentBGMComponent = UGameplayStatics::CreateSound2D(
		this,
		BGM,
		GetBGMComponentVolume(),
		1.0f,
		0.0f,
		nullptr,
		true,
		false
	);

	if (!CurrentBGMComponent)
	{
		UE_LOG(LogTemp, Warning, TEXT("[Audio] Failed to create BGM component. Sound=%s"), *BGM->GetName());
		CurrentBGMSound = nullptr;
		return;
	}

	CurrentBGMComponent->bIsUISound = true;
	CurrentBGMComponent->FadeIn(ClampedFadeInTime, GetBGMComponentVolume());
}

void UTPSAudioSubsystem::StopBGM(float FadeOutTime)
{
	if (!CurrentBGMComponent)
	{
		CurrentBGMSound = nullptr;
		return;
	}

	const float ClampedFadeOutTime = FMath::Max(0.0f, FadeOutTime);
	CurrentBGMComponent->FadeOut(ClampedFadeOutTime, 0.0f);
	CurrentBGMComponent = nullptr;
	CurrentBGMSound = nullptr;
}

void UTPSAudioSubsystem::SetMasterVolume(float NewVolume, bool bSaveImmediately)
{
	MasterVolume = FMath::Clamp(NewVolume, 0.0f, 1.0f);
	ApplySoundClassVolumes();
	ApplyCurrentBGMVolume();

	if (bSaveImmediately)
	{
		SaveAudioSettings();
	}
}

void UTPSAudioSubsystem::SetBGMVolume(float NewVolume, bool bSaveImmediately)
{
	BGMVolume = FMath::Clamp(NewVolume, 0.0f, 1.0f);
	ApplySoundClassVolumes();
	ApplyCurrentBGMVolume();

	if (bSaveImmediately)
	{
		SaveAudioSettings();
	}
}

void UTPSAudioSubsystem::SetSFXVolume(float NewVolume, bool bSaveImmediately)
{
	SFXVolume = FMath::Clamp(NewVolume, 0.0f, 1.0f);
	ApplySoundClassVolumes();

	if (bSaveImmediately)
	{
		SaveAudioSettings();
	}
}

void UTPSAudioSubsystem::SetScreenBrightness(float NewBrightness, bool bSaveImmediately)
{
	ScreenBrightness = FMath::Clamp(NewBrightness, 0.0f, 1.0f);
	ApplyBrightnessOverlay();

	if (bSaveImmediately)
	{
		SaveAudioSettings();
	}
}

void UTPSAudioSubsystem::ApplyRuntimeSettings()
{
	ApplySoundClassVolumes();
	ApplyCurrentBGMVolume();
	ApplyBrightnessOverlay();
}

void UTPSAudioSubsystem::LoadAudioSettings()
{
	USaveGame* LoadedSaveGame = nullptr;

	if (UGameplayStatics::DoesSaveGameExist(SettingsSaveSlotName, SettingsSaveUserIndex))
	{
		LoadedSaveGame = UGameplayStatics::LoadGameFromSlot(SettingsSaveSlotName, SettingsSaveUserIndex);
	}

	const UTPSAudioSettingsSaveGame* AudioSettings = Cast<UTPSAudioSettingsSaveGame>(LoadedSaveGame);
	if (!AudioSettings)
	{
		return;
	}

	MasterVolume = FMath::Clamp(AudioSettings->MasterVolume, 0.0f, 1.0f);
	BGMVolume = FMath::Clamp(AudioSettings->BGMVolume, 0.0f, 1.0f);
	SFXVolume = FMath::Clamp(AudioSettings->SFXVolume, 0.0f, 1.0f);
	ScreenBrightness = FMath::Clamp(AudioSettings->ScreenBrightness, 0.0f, 1.0f);

	ApplyRuntimeSettings();
}

void UTPSAudioSubsystem::SaveAudioSettings() const
{
	UTPSAudioSettingsSaveGame* AudioSettings = Cast<UTPSAudioSettingsSaveGame>(
		UGameplayStatics::CreateSaveGameObject(UTPSAudioSettingsSaveGame::StaticClass())
	);

	if (!AudioSettings)
	{
		UE_LOG(LogTemp, Warning, TEXT("[Audio] Failed to create audio settings save object."));
		return;
	}

	AudioSettings->MasterVolume = MasterVolume;
	AudioSettings->BGMVolume = BGMVolume;
	AudioSettings->SFXVolume = SFXVolume;
	AudioSettings->ScreenBrightness = ScreenBrightness;

	UGameplayStatics::SaveGameToSlot(AudioSettings, SettingsSaveSlotName, SettingsSaveUserIndex);
}

float UTPSAudioSubsystem::GetEffectiveBGMVolume() const
{
	return FMath::Clamp(MasterVolume * BGMVolume, 0.0f, 1.0f);
}

float UTPSAudioSubsystem::GetBGMComponentVolume() const
{
	return (VolumeSoundMix && BGMSoundClass) ? 1.0f : GetEffectiveBGMVolume();
}

void UTPSAudioSubsystem::ApplyCurrentBGMVolume() const
{
	if (CurrentBGMComponent)
	{
		CurrentBGMComponent->SetVolumeMultiplier(GetBGMComponentVolume());
	}
}

void UTPSAudioSubsystem::LoadSoundClassSettings()
{
	const UTPSAudioSubsystemSettings* AudioSettings = GetDefault<UTPSAudioSubsystemSettings>();
	if (!AudioSettings)
	{
		return;
	}

	VolumeSoundMix = AudioSettings->VolumeSoundMix.LoadSynchronous();
	MasterSoundClass = AudioSettings->MasterSoundClass.LoadSynchronous();
	BGMSoundClass = AudioSettings->BGMSoundClass.LoadSynchronous();
	SFXSoundClass = AudioSettings->SFXSoundClass.LoadSynchronous();

	if (VolumeSoundMix)
	{
		UGameplayStatics::PushSoundMixModifier(this, VolumeSoundMix);
	}
}

void UTPSAudioSubsystem::ApplySoundClassVolumes()
{
	if (!VolumeSoundMix)
	{
		return;
	}

	ApplySoundClassVolume(MasterSoundClass, MasterVolume);
	ApplySoundClassVolume(BGMSoundClass, BGMVolume);
	ApplySoundClassVolume(SFXSoundClass, SFXVolume);
}

void UTPSAudioSubsystem::ApplySoundClassVolume(USoundClass* SoundClass, float Volume, bool bApplyToChildren)
{
	if (!SoundClass || !VolumeSoundMix)
	{
		return;
	}

	UGameplayStatics::SetSoundMixClassOverride(
		this,
		VolumeSoundMix,
		SoundClass,
		FMath::Clamp(Volume, 0.0f, 1.0f),
		1.0f,
		0.0f,
		bApplyToChildren
	);
}

void UTPSAudioSubsystem::ApplyBrightnessOverlay()
{
	const float DimOpacity = 1.0f - FMath::Clamp(ScreenBrightness, 0.0f, 1.0f);

	if (DimOpacity <= KINDA_SMALL_NUMBER)
	{
		RemoveBrightnessOverlay();
		return;
	}

	EnsureBrightnessOverlay();

	if (BrightnessOverlayRootWidget.IsValid())
	{
		BrightnessOverlayRootWidget->SetRenderOpacity(DimOpacity);
	}
}

void UTPSAudioSubsystem::EnsureBrightnessOverlay()
{
	if (BrightnessOverlayRootWidget.IsValid())
	{
		return;
	}

	if (!GEngine || !GEngine->GameViewport)
	{
		return;
	}

	BrightnessOverlayRootWidget =
		SNew(SOverlay)
		.Visibility(EVisibility::HitTestInvisible)
		.RenderOpacity(1.0f - ScreenBrightness)
		+ SOverlay::Slot()
		.HAlign(HAlign_Fill)
		.VAlign(VAlign_Fill)
		[
			SAssignNew(BrightnessOverlayBorderWidget, SBorder)
			.BorderImage(FCoreStyle::Get().GetBrush(TEXT("WhiteBrush")))
			.BorderBackgroundColor(FLinearColor::Black)
		];

	GEngine->GameViewport->AddViewportWidgetContent(
		BrightnessOverlayRootWidget.ToSharedRef(),
		50
	);
}

void UTPSAudioSubsystem::RemoveBrightnessOverlay()
{
	if (GEngine && GEngine->GameViewport && BrightnessOverlayRootWidget.IsValid())
	{
		GEngine->GameViewport->RemoveViewportWidgetContent(BrightnessOverlayRootWidget.ToSharedRef());
	}

	BrightnessOverlayRootWidget.Reset();
	BrightnessOverlayBorderWidget.Reset();
}
