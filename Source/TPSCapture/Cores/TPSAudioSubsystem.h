#pragma once

#include "CoreMinimal.h"
#include "Engine/DeveloperSettings.h"
#include "GameFramework/SaveGame.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "TPSAudioSubsystem.generated.h"

class UAudioComponent;
class USoundClass;
class USoundBase;
class USoundMix;

UCLASS(Config = Game, DefaultConfig, meta = (DisplayName = "TPS Audio"))
class TPSCAPTURE_API UTPSAudioSubsystemSettings : public UDeveloperSettings
{
	GENERATED_BODY()

public:
	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "Sound Mix")
	TSoftObjectPtr<USoundMix> VolumeSoundMix;

	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "Sound Classes")
	TSoftObjectPtr<USoundClass> MasterSoundClass;

	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "Sound Classes")
	TSoftObjectPtr<USoundClass> BGMSoundClass;

	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "Sound Classes")
	TSoftObjectPtr<USoundClass> SFXSoundClass;
};

UCLASS()
class TPSCAPTURE_API UTPSAudioSettingsSaveGame : public USaveGame
{
	GENERATED_BODY()

public:
	UPROPERTY(BlueprintReadWrite, Category = "Audio Settings", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float MasterVolume = 1.0f;

	UPROPERTY(BlueprintReadWrite, Category = "Audio Settings", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float BGMVolume = 1.0f;

	UPROPERTY(BlueprintReadWrite, Category = "Audio Settings", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float SFXVolume = 1.0f;
};

UCLASS()
class TPSCAPTURE_API UTPSAudioSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

	UFUNCTION(BlueprintCallable, Category = "Audio|BGM")
	void PlayBGM(USoundBase* BGM, float FadeInTime = 1.0f, float FadeOutTime = 1.0f);

	UFUNCTION(BlueprintCallable, Category = "Audio|BGM")
	void StopBGM(float FadeOutTime = 1.0f);

	UFUNCTION(BlueprintCallable, Category = "Audio|Settings")
	void SetMasterVolume(float NewVolume, bool bSaveImmediately = true);

	UFUNCTION(BlueprintCallable, Category = "Audio|Settings")
	void SetBGMVolume(float NewVolume, bool bSaveImmediately = true);

	UFUNCTION(BlueprintCallable, Category = "Audio|Settings")
	void SetSFXVolume(float NewVolume, bool bSaveImmediately = true);

	UFUNCTION(BlueprintPure, Category = "Audio|Settings")
	float GetMasterVolume() const { return MasterVolume; }

	UFUNCTION(BlueprintPure, Category = "Audio|Settings")
	float GetBGMVolume() const { return BGMVolume; }

	UFUNCTION(BlueprintPure, Category = "Audio|Settings")
	float GetSFXVolume() const { return SFXVolume; }

	UFUNCTION(BlueprintCallable, Category = "Audio|Settings")
	void LoadAudioSettings();

	UFUNCTION(BlueprintCallable, Category = "Audio|Settings")
	void SaveAudioSettings() const;

private:
	float GetEffectiveBGMVolume() const;
	float GetBGMComponentVolume() const;
	void ApplyCurrentBGMVolume() const;
	void LoadSoundClassSettings();
	void ApplySoundClassVolumes();
	void ApplySoundClassVolume(USoundClass* SoundClass, float Volume, bool bApplyToChildren = true);

private:
	UPROPERTY(Transient)
	TObjectPtr<UAudioComponent> CurrentBGMComponent = nullptr;

	UPROPERTY(Transient)
	TObjectPtr<USoundBase> CurrentBGMSound = nullptr;

	UPROPERTY()
	float MasterVolume = 1.0f;

	UPROPERTY()
	float BGMVolume = 1.0f;

	UPROPERTY()
	float SFXVolume = 1.0f;

	UPROPERTY()
	FString SettingsSaveSlotName = TEXT("TPSAudioSettings");

	UPROPERTY()
	int32 SettingsSaveUserIndex = 0;

	UPROPERTY(Transient)
	TObjectPtr<USoundMix> VolumeSoundMix = nullptr;

	UPROPERTY(Transient)
	TObjectPtr<USoundClass> MasterSoundClass = nullptr;

	UPROPERTY(Transient)
	TObjectPtr<USoundClass> BGMSoundClass = nullptr;

	UPROPERTY(Transient)
	TObjectPtr<USoundClass> SFXSoundClass = nullptr;
};
