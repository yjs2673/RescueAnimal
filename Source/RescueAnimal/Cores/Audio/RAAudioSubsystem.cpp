#include "RAAudioSubsystem.h"

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

void URAAudioSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);

	LoadSoundClassSettings();
	LoadAudioSettings();
	ApplyRuntimeSettings();
}

void URAAudioSubsystem::Deinitialize()
{
	StopBGM(0.0f);
	RemoveBrightnessOverlay();

	Super::Deinitialize();
}

void URAAudioSubsystem::PlayBGM(USoundBase* BGM, float FadeInTime, float FadeOutTime)
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

void URAAudioSubsystem::StopBGM(float FadeOutTime)
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

void URAAudioSubsystem::SetMasterVolume(float NewVolume, bool bSaveImmediately)
{
	MasterVolume = FMath::Clamp(NewVolume, 0.0f, 1.0f);
	ApplySoundClassVolumes();
	ApplyCurrentBGMVolume();

	if (bSaveImmediately)
	{
		SaveAudioSettings();
	}
}

void URAAudioSubsystem::SetBGMVolume(float NewVolume, bool bSaveImmediately)
{
	BGMVolume = FMath::Clamp(NewVolume, 0.0f, 1.0f);
	ApplySoundClassVolumes();
	ApplyCurrentBGMVolume();

	if (bSaveImmediately)
	{
		SaveAudioSettings();
	}
}

void URAAudioSubsystem::SetSFXVolume(float NewVolume, bool bSaveImmediately)
{
	SFXVolume = FMath::Clamp(NewVolume, 0.0f, 1.0f);
	ApplySoundClassVolumes();

	if (bSaveImmediately)
	{
		SaveAudioSettings();
	}
}

void URAAudioSubsystem::SetScreenBrightness(float NewBrightness, bool bSaveImmediately)
{
	ScreenBrightness = FMath::Clamp(NewBrightness, 0.0f, 1.0f);
	ApplyBrightnessOverlay();

	if (bSaveImmediately)
	{
		SaveAudioSettings();
	}
}

void URAAudioSubsystem::ApplyRuntimeSettings()
{
	ApplySoundClassVolumes();
	ApplyCurrentBGMVolume();
	ApplyBrightnessOverlay();
}

void URAAudioSubsystem::LoadAudioSettings()
{
	USaveGame* LoadedSaveGame = nullptr;

	if (UGameplayStatics::DoesSaveGameExist(SettingsSaveSlotName, SettingsSaveUserIndex))
	{
		LoadedSaveGame = UGameplayStatics::LoadGameFromSlot(SettingsSaveSlotName, SettingsSaveUserIndex);
	}

	const URAAudioSettingsSaveGame* AudioSettings = Cast<URAAudioSettingsSaveGame>(LoadedSaveGame);
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

void URAAudioSubsystem::SaveAudioSettings() const
{
	URAAudioSettingsSaveGame* AudioSettings = Cast<URAAudioSettingsSaveGame>(
		UGameplayStatics::CreateSaveGameObject(URAAudioSettingsSaveGame::StaticClass())
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

float URAAudioSubsystem::GetEffectiveBGMVolume() const
{
	return FMath::Clamp(MasterVolume * BGMVolume, 0.0f, 1.0f);
}

float URAAudioSubsystem::GetBGMComponentVolume() const
{
	return (VolumeSoundMix && BGMSoundClass) ? 1.0f : GetEffectiveBGMVolume();
}

float URAAudioSubsystem::GetAppliedVolume(float Volume) const
{
	const float ClampedVolume = FMath::Clamp(Volume, 0.0f, 1.0f);
	return ClampedVolume <= KINDA_SMALL_NUMBER ? 0.0f : ClampedVolume;
}

float URAAudioSubsystem::GetAppliedMasterVolume() const
{
	const float ClampedMasterVolume = FMath::Clamp(MasterVolume, 0.0f, 1.0f);
	const float ClampedMinMasterVolume = FMath::Clamp(MinMasterVolume, 0.0f, 1.0f);
	return FMath::Lerp(ClampedMinMasterVolume, 1.0f, ClampedMasterVolume);
}

float URAAudioSubsystem::GetAppliedBGMVolume() const
{
	const float ClampedBGMVolume = FMath::Clamp(BGMVolume, 0.0f, 1.0f);
	const float ClampedMinBGMVolume = FMath::Clamp(MinBGMVolume, 0.0f, 1.0f);
	return FMath::Lerp(ClampedMinBGMVolume, 1.0f, ClampedBGMVolume);
}

float URAAudioSubsystem::GetBrightnessDimOpacity() const
{
	const float NormalizedBrightness = FMath::Clamp(ScreenBrightness, 0.0f, 1.0f);
	return (1.0f - NormalizedBrightness) * FMath::Clamp(MaxDimOpacity, 0.0f, 1.0f);
}

void URAAudioSubsystem::ApplyCurrentBGMVolume() const
{
	if (CurrentBGMComponent)
	{
		CurrentBGMComponent->SetVolumeMultiplier(GetBGMComponentVolume());
	}
}

void URAAudioSubsystem::LoadSoundClassSettings()
{
	const URAAudioSubsystemSettings* AudioSettings = GetDefault<URAAudioSubsystemSettings>();
	if (!AudioSettings)
	{
		return;
	}

	VolumeSoundMix = AudioSettings->VolumeSoundMix.LoadSynchronous();
	MasterSoundClass = AudioSettings->MasterSoundClass.LoadSynchronous();
	BGMSoundClass = AudioSettings->BGMSoundClass.LoadSynchronous();
	SFXSoundClass = AudioSettings->SFXSoundClass.LoadSynchronous();
	MaxDimOpacity = FMath::Clamp(AudioSettings->MaxDimOpacity, 0.0f, 1.0f);
	MinMasterVolume = FMath::Clamp(AudioSettings->MinMasterVolume, 0.0f, 1.0f);
	MinBGMVolume = FMath::Clamp(AudioSettings->MinBGMVolume, 0.0f, 1.0f);

	if (VolumeSoundMix)
	{
		UGameplayStatics::PushSoundMixModifier(this, VolumeSoundMix);
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("[Audio] VolumeSoundMix is not assigned or failed to load."));
	}

	if (!MasterSoundClass)
	{
		UE_LOG(LogTemp, Warning, TEXT("[Audio] MasterSoundClass is not assigned or failed to load."));
	}

	if (!BGMSoundClass)
	{
		UE_LOG(LogTemp, Warning, TEXT("[Audio] BGMSoundClass is not assigned or failed to load."));
	}

	if (!SFXSoundClass)
	{
		UE_LOG(LogTemp, Warning, TEXT("[Audio] SFXSoundClass is not assigned or failed to load."));
	}
}

void URAAudioSubsystem::ApplySoundClassVolumes()
{
	if (!VolumeSoundMix)
	{
		return;
	}

	UGameplayStatics::PushSoundMixModifier(this, VolumeSoundMix);

	ApplySoundClassVolume(MasterSoundClass, GetAppliedMasterVolume());
	ApplySoundClassVolume(BGMSoundClass, GetAppliedBGMVolume());
	ApplySoundClassVolume(SFXSoundClass, SFXVolume);
}

void URAAudioSubsystem::ApplySoundClassVolume(USoundClass* SoundClass, float Volume, bool bApplyToChildren)
{
	if (!SoundClass || !VolumeSoundMix)
	{
		return;
	}

	UGameplayStatics::SetSoundMixClassOverride(
		this,
		VolumeSoundMix,
		SoundClass,
		GetAppliedVolume(Volume),
		1.0f,
		0.0f,
		bApplyToChildren
	);
}

void URAAudioSubsystem::ApplyBrightnessOverlay()
{
	const float DimOpacity = GetBrightnessDimOpacity();

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

void URAAudioSubsystem::EnsureBrightnessOverlay()
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
		.RenderOpacity(GetBrightnessDimOpacity())
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

void URAAudioSubsystem::RemoveBrightnessOverlay()
{
	if (GEngine && GEngine->GameViewport && BrightnessOverlayRootWidget.IsValid())
	{
		GEngine->GameViewport->RemoveViewportWidgetContent(BrightnessOverlayRootWidget.ToSharedRef());
	}

	BrightnessOverlayRootWidget.Reset();
	BrightnessOverlayBorderWidget.Reset();
}
