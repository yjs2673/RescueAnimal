#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "TPSGameEnums.h"
#include "PlayerSkillComponent.generated.h"

class ATPSCaptureCharacter;
class AArrowProjectile;
class UAnimMontage;
class UNiagaraSystem;
class USoundBase;
class UStaticMesh;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnSkillCooldownStarted, EWeaponType, SkillWeaponType);

USTRUCT(BlueprintType)
struct FPlayerSkillCommonInfo
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Skill", meta = (ClampMin = "0.0"))
	float Cooldown = 5.0f;
};

USTRUCT(BlueprintType)
struct FUnarmedSkillInfo
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Skill")
	FPlayerSkillCommonInfo Common;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Skill|Combat", meta = (ClampMin = "0.0"))
	float Damage = 35.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Skill|Combat", meta = (ClampMin = "0.0"))
	float Range = 220.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Skill|Combat", meta = (ClampMin = "0.0"))
	float Radius = 90.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Skill|Movement")
	float ForwardLaunchStrength = 850.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Skill|Movement")
	float UpwardLaunchStrength = 320.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Skill|Combat", meta = (ClampMin = "0.0"))
	float HitDelay = 0.25f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Skill|Combat", meta = (ClampMin = "0.0"))
	float EndDelayWhenNoMontage = 0.75f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Skill|Knockback")
	float KnockbackStrength = 700.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Skill|Knockback")
	float KnockbackUpwardStrength = 120.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Skill|Animation")
	UAnimMontage* SkillMontage = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Skill|SFX")
	USoundBase* HitSound = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Skill|VFX")
	UNiagaraSystem* HitVFX = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Skill|VFX")
	UNiagaraSystem* ShockwaveVFX = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Skill|VFX")
	FLinearColor HitVFXColor = FLinearColor(1.f, 1.f, 1.f, 1.f);

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Skill|VFX", meta = (ClampMin = "0.0"))
	float HitVFXScale = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Skill|VFX", meta = (ClampMin = "0.0"))
	float HitVFXLifetime = 0.35f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Skill|VFX")
	FLinearColor ShockwaveVFXColor = FLinearColor(1.f, 1.f, 1.f, 1.f);

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Skill|VFX", meta = (ClampMin = "0.0"))
	float ShockwaveVFXScale = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Skill|VFX", meta = (ClampMin = "0.0"))
	float ShockwaveVFXLifetime = 0.6f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Skill|VFX")
	FName FootSocketName = TEXT("foot_r");
};

USTRUCT(BlueprintType)
struct FSwordSkillInfo
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Skill")
	FPlayerSkillCommonInfo Common;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Skill|Combat", meta = (ClampMin = "0.0"))
	float Damage = 55.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Skill|Combat", meta = (ClampMin = "0.0"))
	float Range = 260.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Skill|Combat", meta = (ClampMin = "0.0"))
	float Radius = 130.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Skill|Movement")
	float ForwardLaunchStrength = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Skill|Combat", meta = (ClampMin = "0.0"))
	float HitDelay = 0.25f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Skill|Combat", meta = (ClampMin = "0.0"))
	float EndDelayWhenNoMontage = 0.75f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Skill|Animation")
	UAnimMontage* SkillMontage = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Skill|SFX")
	USoundBase* HitSound = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Skill|VFX")
	UNiagaraSystem* HitVFX = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Skill|VFX")
	FLinearColor HitVFXColor = FLinearColor(1.f, 0.2f, 0.2f, 1.f);

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Skill|VFX", meta = (ClampMin = "0.0"))
	float HitVFXScale = 1.2f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Skill|VFX", meta = (ClampMin = "0.0"))
	float HitVFXLifetime = 0.45f;
};

USTRUCT(BlueprintType)
struct FBowSkillInfo
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Skill")
	FPlayerSkillCommonInfo Common;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Skill|Bow")
	TSubclassOf<AArrowProjectile> FireArrowProjectileClass;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Skill|Bow")
	UStaticMesh* FirePreviewArrowStaticMesh = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Skill|Bow")
	UNiagaraSystem* FirePreviewArrowVFX = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Skill|Bow")
	FVector FirePreviewVFXRelativeLocation = FVector::ZeroVector;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Skill|Bow")
	FRotator FirePreviewVFXRelativeRotation = FRotator::ZeroRotator;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Skill|Bow")
	FVector FirePreviewVFXRelativeScale = FVector(1.0f);

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Skill|SFX")
	USoundBase* SkillReleaseSound = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Skill|SFX")
	USoundBase* HitSound = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Skill|VFX")
	UNiagaraSystem* HitVFX = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Skill|VFX")
	FLinearColor HitVFXColor = FLinearColor(1.0f, 0.25f, 0.05f, 1.0f);

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Skill|VFX", meta = (ClampMin = "0.0"))
	float HitVFXScale = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Skill|VFX", meta = (ClampMin = "0.0"))
	float HitVFXLifetime = 0.45f;
};

UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class TPSCAPTURE_API UPlayerSkillComponent : public UActorComponent
{
	GENERATED_BODY()

public:	
	UPlayerSkillComponent();

#pragma region Cooldown Events
	UPROPERTY(BlueprintAssignable, Category = "Skill|Cooldown")
	FOnSkillCooldownStarted OnSkillCooldownStarted;
#pragma endregion Cooldown Events

protected:
	virtual void BeginPlay() override;

public:	
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	UFUNCTION(BlueprintCallable, Category = "Skill")
	bool TryActivateSkill();

	UFUNCTION(BlueprintCallable, Category = "Skill")
	void HandleSkillInput();

	UFUNCTION(BlueprintCallable, Category = "Skill|Unarmed")
	void TriggerUnarmedSkillHit();

	UFUNCTION(BlueprintCallable, Category = "Skill|Sword")
	void TriggerSwordSkillHit();

	UFUNCTION(BlueprintCallable, Category = "Skill")
	void TriggerSkillHit();

	UFUNCTION(BlueprintPure, Category = "Skill|Bow")
	bool IsBowSkillPrepared() const { return bBowSkillPrepared; }

	UFUNCTION(BlueprintPure, Category = "Skill|Bow")
	TSubclassOf<AArrowProjectile> GetPreparedBowProjectileClass() const;

	UFUNCTION(BlueprintCallable, Category = "Skill|Bow")
	bool CommitBowSkillRelease();

	UFUNCTION(BlueprintCallable, Category = "Skill|Bow")
	void CancelBowSkillPreparation();

	UFUNCTION(BlueprintCallable, Category = "Skill|Bow")
	void PlayBowSkillReleaseSound() const;

	UFUNCTION(BlueprintCallable, Category = "Skill|Bow")
	void ApplyBowSkillHitEffects(AArrowProjectile* Arrow) const;

	UFUNCTION(BlueprintPure, Category = "Skill")
	float GetCooldownRemaining(EWeaponType SkillWeaponType) const;

	UFUNCTION(BlueprintPure, Category = "Skill")
	float GetSkillCooldown(EWeaponType SkillWeaponType) const;

	UFUNCTION(BlueprintPure, Category = "Skill")
	float GetCooldownPercent(EWeaponType SkillWeaponType) const;

protected:
	bool ActivateUnarmedSkill();
	bool ActivateSwordSkill();
	bool ToggleBowSkillPreparation();
	bool CanActivateSkill(EWeaponType SkillWeaponType, const FPlayerSkillCommonInfo& SkillInfo) const;
	void StartSkillCooldown(EWeaponType SkillWeaponType);
	void EndActiveSkill();
	void PerformUnarmedSkillHit();
	void PerformSwordSkillHit();
	void SpawnSkillVFX(UNiagaraSystem* NiagaraSystem, const FVector& SpawnLocation, const FRotator& SpawnRotation, const FLinearColor& Color, float Scale, float Lifetime) const;
	FVector GetFootSocketLocation() const;
	bool ShouldIgnoreSkillTarget(AActor* TargetActor) const;

	UFUNCTION()
	void OnSkillMontageEnded(UAnimMontage* Montage, bool bInterrupted);

protected:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Skill|Unarmed")
	FUnarmedSkillInfo UnarmedSkill;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Skill|Sword")
	FSwordSkillInfo SwordSkill;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Skill|Bow")
	FBowSkillInfo BowSkill;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Skill")
	bool bIsSkillActive = false;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Skill|Unarmed")
	bool bUnarmedSkillHitApplied = false;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Skill|Sword")
	bool bSwordSkillHitApplied = false;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Skill")
	EWeaponType ActiveSkillWeaponType = EWeaponType::None;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Skill|Bow")
	bool bBowSkillPrepared = false;

	UPROPERTY(Transient)
	TObjectPtr<ATPSCaptureCharacter> OwnerCharacter = nullptr;

	UPROPERTY(Transient)
	TMap<EWeaponType, float> LastSkillUseTimes;

	FTimerHandle SkillHitTimerHandle;
	FTimerHandle SkillEndTimerHandle;
};
