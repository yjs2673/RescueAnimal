#include "TPSAudioSubsystem.h"

#include "Components/AudioComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Sound/SoundBase.h"

void UTPSAudioSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);

	LoadAudioSettings();
}

void UTPSAudioSubsystem::Deinitialize()
{
	StopBGM(0.0f);

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
			CurrentBGMComponent->FadeIn(ClampedFadeInTime, GetEffectiveBGMVolume());
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
		GetEffectiveBGMVolume(),
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
	CurrentBGMComponent->FadeIn(ClampedFadeInTime, GetEffectiveBGMVolume());
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
	ApplyCurrentBGMVolume();

	if (bSaveImmediately)
	{
		SaveAudioSettings();
	}
}

void UTPSAudioSubsystem::SetBGMVolume(float NewVolume, bool bSaveImmediately)
{
	BGMVolume = FMath::Clamp(NewVolume, 0.0f, 1.0f);
	ApplyCurrentBGMVolume();

	if (bSaveImmediately)
	{
		SaveAudioSettings();
	}
}

void UTPSAudioSubsystem::SetSFXVolume(float NewVolume, bool bSaveImmediately)
{
	SFXVolume = FMath::Clamp(NewVolume, 0.0f, 1.0f);

	if (bSaveImmediately)
	{
		SaveAudioSettings();
	}
}

void UTPSAudioSubsystem::SetUIVolume(float NewVolume, bool bSaveImmediately)
{
	UIVolume = FMath::Clamp(NewVolume, 0.0f, 1.0f);

	if (bSaveImmediately)
	{
		SaveAudioSettings();
	}
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
	UIVolume = FMath::Clamp(AudioSettings->UIVolume, 0.0f, 1.0f);

	ApplyCurrentBGMVolume();
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
	AudioSettings->UIVolume = UIVolume;

	UGameplayStatics::SaveGameToSlot(AudioSettings, SettingsSaveSlotName, SettingsSaveUserIndex);
}

float UTPSAudioSubsystem::GetEffectiveBGMVolume() const
{
	return FMath::Clamp(MasterVolume * BGMVolume, 0.0f, 1.0f);
}

void UTPSAudioSubsystem::ApplyCurrentBGMVolume() const
{
	if (CurrentBGMComponent)
	{
		CurrentBGMComponent->SetVolumeMultiplier(GetEffectiveBGMVolume());
	}
}