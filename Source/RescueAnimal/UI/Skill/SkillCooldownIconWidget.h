#pragma once

#include "CoreMinimal.h"
#include "RAGameEnums.h"
#include "UI/Audio/RAUserWidget.h"
#include "SkillCooldownIconWidget.generated.h"

class ARACharacter;
class UImage;
class UMaterialInstanceDynamic;
class UMaterialInterface;
class UPlayerSkillComponent;
class UTexture2D;

UCLASS()
class RESCUEANIMAL_API USkillCooldownIconWidget : public URAUserWidget
{
	GENERATED_BODY()

public:
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;

	UFUNCTION(BlueprintCallable, Category = "Skill Cooldown")
	void SetupSkillCooldownIcon(EWeaponType InSkillWeaponType, UTexture2D* InSkillIconTexture, UPlayerSkillComponent* InSkillComponent = nullptr);

	UFUNCTION(BlueprintCallable, Category = "Skill Cooldown")
	void SetSkillWeaponType(EWeaponType InSkillWeaponType);

	UFUNCTION(BlueprintCallable, Category = "Skill Cooldown")
	void SetSkillIconTexture(UTexture2D* InSkillIconTexture);

	UFUNCTION(BlueprintCallable, Category = "Skill Cooldown")
	void BindSkillComponent(UPlayerSkillComponent* InSkillComponent);

	UFUNCTION(BlueprintCallable, Category = "Skill Cooldown")
	void RefreshCooldown();

	UFUNCTION(BlueprintPure, Category = "Skill Cooldown")
	float GetDisplayedCooldownPercent() const { return DisplayedCooldownPercent; }

protected:
#pragma region Cooldown Material
	void InitializeCooldownMaterial();
#pragma endregion Cooldown Material

#pragma region Skill Component Binding
	void ResolveSkillComponent();
	void BindCooldownEvents();
	void UnbindCooldownEvents();
#pragma endregion Skill Component Binding

	void ApplyCooldownPercent(float CooldownPercent);
	void ApplyIconTexture();

	ARACharacter* GetPlayerCharacter() const;

	UFUNCTION()
	void HandleSkillCooldownStarted(EWeaponType CooldownSkillWeaponType);

#pragma region Cooldown Refresh Timer
	void StartCooldownRefreshTimer();
	void StopCooldownRefreshTimer();
#pragma endregion Cooldown Refresh Timer

#pragma region Skill Component Binding Retry
	void StartSkillComponentBindRetryTimer();
	void StopSkillComponentBindRetryTimer();
#pragma endregion Skill Component Binding Retry

protected:
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UImage> SkillIconImage;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UImage> CooldownOverlayImage;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Skill Cooldown")
	EWeaponType SkillWeaponType = EWeaponType::None;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Skill Cooldown")
	TObjectPtr<UTexture2D> SkillIconTexture = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Skill Cooldown")
	TObjectPtr<UMaterialInterface> CooldownOverlayMaterial = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Skill Cooldown")
	FName CooldownPercentParameterName = TEXT("CooldownPercent");

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Skill Cooldown")
	bool bAutoBindOwningPlayerSkillComponent = true;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Skill Cooldown")
	bool bHideOverlayWhenReady = true;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Skill Cooldown", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float ReadyOverlayOpacity = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Skill Cooldown", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float CooldownOverlayOpacity = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Skill Cooldown", meta = (ClampMin = "0.01"))
	float CooldownRefreshInterval = 0.02f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Skill Cooldown", meta = (ClampMin = "0.05"))
	float SkillComponentBindRetryInterval = 0.1f;

	UPROPERTY(Transient)
	TObjectPtr<UPlayerSkillComponent> BoundSkillComponent = nullptr;

	UPROPERTY(Transient)
	TObjectPtr<UMaterialInstanceDynamic> CooldownOverlayMID = nullptr;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Skill Cooldown")
	float DisplayedCooldownPercent = 0.0f;

	FTimerHandle CooldownRefreshTimerHandle;
	FTimerHandle SkillComponentBindRetryTimerHandle;
};
