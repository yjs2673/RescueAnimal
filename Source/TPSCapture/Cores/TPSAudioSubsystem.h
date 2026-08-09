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
class SBorder;
class SWidget;

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

	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "Display", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float MaxDimOpacity = 0.75f;

	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "Audio", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float MinMasterVolume = 0.05f;

	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "Audio", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float MinBGMVolume = 0.05f;
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

	UPROPERTY(BlueprintReadWrite, Category = "Display Settings", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float ScreenBrightness = 1.0f;
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

	UFUNCTION(BlueprintCallable, Category = "Display|Settings")
	void SetScreenBrightness(float NewBrightness, bool bSaveImmediately = true);

	UFUNCTION(BlueprintPure, Category = "Audio|Settings")
	float GetMasterVolume() const { return MasterVolume; }

	UFUNCTION(BlueprintPure, Category = "Audio|Settings")
	float GetBGMVolume() const { return BGMVolume; }

	UFUNCTION(BlueprintPure, Category = "Audio|Settings")
	float GetSFXVolume() const { return SFXVolume; }

	UFUNCTION(BlueprintPure, Category = "Display|Settings")
	float GetScreenBrightness() const { return ScreenBrightness; }

	UFUNCTION(BlueprintCallable, Category = "Settings")
	void ApplyRuntimeSettings();

	UFUNCTION(BlueprintCallable, Category = "Audio|Settings")
	void LoadAudioSettings();

	UFUNCTION(BlueprintCallable, Category = "Audio|Settings")
	void SaveAudioSettings() const;

private:
	float GetEffectiveBGMVolume() const;
	float GetBGMComponentVolume() const;
	float GetAppliedVolume(float Volume) const;
	float GetAppliedMasterVolume() const;
	float GetAppliedBGMVolume() const;
	float GetBrightnessDimOpacity() const;
	void ApplyCurrentBGMVolume() const;
	void LoadSoundClassSettings();
	void ApplySoundClassVolumes();
	void ApplySoundClassVolume(USoundClass* SoundClass, float Volume, bool bApplyToChildren = true);
	void ApplyBrightnessOverlay();
	void EnsureBrightnessOverlay();
	void RemoveBrightnessOverlay();

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
	float ScreenBrightness = 1.0f;

	UPROPERTY()
	float MaxDimOpacity = 0.75f;

	UPROPERTY()
	float MinMasterVolume = 0.05f;

	UPROPERTY()
	float MinBGMVolume = 0.05f;

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

	TSharedPtr<SWidget> BrightnessOverlayRootWidget;
	TSharedPtr<SBorder> BrightnessOverlayBorderWidget;
};
