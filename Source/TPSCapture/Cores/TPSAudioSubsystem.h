#pragma once

#include "CoreMinimal.h"
#include "GameFramework/SaveGame.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "TPSAudioSubsystem.generated.h"

class UAudioComponent;
class USoundBase;

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

	UPROPERTY(BlueprintReadWrite, Category = "Audio Settings", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float UIVolume = 1.0f;
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

	UFUNCTION(BlueprintCallable, Category = "Audio|Settings")
	void SetUIVolume(float NewVolume, bool bSaveImmediately = true);

	UFUNCTION(BlueprintPure, Category = "Audio|Settings")
	float GetMasterVolume() const { return MasterVolume; }

	UFUNCTION(BlueprintPure, Category = "Audio|Settings")
	float GetBGMVolume() const { return BGMVolume; }

	UFUNCTION(BlueprintPure, Category = "Audio|Settings")
	float GetSFXVolume() const { return SFXVolume; }

	UFUNCTION(BlueprintPure, Category = "Audio|Settings")
	float GetUIVolume() const { return UIVolume; }

	UFUNCTION(BlueprintCallable, Category = "Audio|Settings")
	void LoadAudioSettings();

	UFUNCTION(BlueprintCallable, Category = "Audio|Settings")
	void SaveAudioSettings() const;

private:
	float GetEffectiveBGMVolume() const;
	void ApplyCurrentBGMVolume() const;

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
	float UIVolume = 1.0f;

	UPROPERTY()
	FString SettingsSaveSlotName = TEXT("TPSAudioSettings");

	UPROPERTY()
	int32 SettingsSaveUserIndex = 0;
};