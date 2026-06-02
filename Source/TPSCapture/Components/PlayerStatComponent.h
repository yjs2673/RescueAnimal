#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "TimerManager.h"
#include "PlayerStatComponent.generated.h"

class USoundBase;
class UNiagaraComponent;
class UNiagaraSystem;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnHPChangedSignature, float, CurrentHP, float, MaxHP);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnDeathSignature);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnEXPChangedSignature, int32, CurrentEXP, int32, RequiredEXP);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnLevelChangedSignature, int32, NewLevel);

UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class TPSCAPTURE_API UPlayerStatComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UPlayerStatComponent();

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

#pragma region Bonus Stats
protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Stats")
	float LevelBonusAttack = 0.f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Stats")
	float LevelBonusMaxHP = 0.f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Stats")
	float LevelBonusDefense = 0.f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Stats")
	float LevelBonusJumpZVelocity = 0.f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Stats")
	float LevelBonusMoveSpeed = 0.f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Stats|Buff")
	float AttackBuffMultiplier = 1.0f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Stats|Buff")
	float DefenseBuffMultiplier = 1.0f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Stats|Buff")
	float JumpBuffMultiplier = 1.0f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Stats|Buff")
	float MoveSpeedBuffMultiplier = 1.0f;

public:
	UFUNCTION(BlueprintPure, Category = "Stats")
	float GetBonusAttack() const { return LevelBonusAttack; }

	UFUNCTION(BlueprintPure, Category = "Stats")
	float GetBonusMaxHP() const { return LevelBonusMaxHP; }

	UFUNCTION(BlueprintPure, Category = "Stats")
	float GetBonusDefense() const { return LevelBonusDefense; }

	UFUNCTION(BlueprintPure, Category = "Stats")
	float GetBonusJumpZVelocity() const { return LevelBonusJumpZVelocity; }

	UFUNCTION(BlueprintPure, Category = "Stats")
	float GetBonusMoveSpeed() const { return LevelBonusMoveSpeed; }

	UFUNCTION(BlueprintPure, Category = "Stats|Buff")
	float GetAttackBuffMultiplier() const { return AttackBuffMultiplier; }

	UFUNCTION(BlueprintPure, Category = "Stats|Buff")
	float GetDefenseBuffMultiplier() const { return DefenseBuffMultiplier; }

	UFUNCTION(BlueprintPure, Category = "Stats|Buff")
	float GetJumpBuffMultiplier() const { return JumpBuffMultiplier; }

	UFUNCTION(BlueprintPure, Category = "Stats|Buff")
	float GetMoveSpeedBuffMultiplier() const { return MoveSpeedBuffMultiplier; }

	UFUNCTION(BlueprintCallable, Category = "Stats|Buff")
	void AddAttackBuffMultiplier(float Multiplier);

	UFUNCTION(BlueprintCallable, Category = "Stats|Buff")
	void RemoveAttackBuffMultiplier(float Multiplier);

	UFUNCTION(BlueprintCallable, Category = "Stats|Buff")
	void AddDefenseBuffMultiplier(float Multiplier);

	UFUNCTION(BlueprintCallable, Category = "Stats|Buff")
	void RemoveDefenseBuffMultiplier(float Multiplier);

	UFUNCTION(BlueprintCallable, Category = "Stats|Buff")
	void AddJumpBuffMultiplier(float Multiplier);

	UFUNCTION(BlueprintCallable, Category = "Stats|Buff")
	void RemoveJumpBuffMultiplier(float Multiplier);

	UFUNCTION(BlueprintCallable, Category = "Stats|Buff")
	void AddMoveSpeedBuffMultiplier(float Multiplier);

	UFUNCTION(BlueprintCallable, Category = "Stats|Buff")
	void RemoveMoveSpeedBuffMultiplier(float Multiplier);
#pragma endregion Bonus Stats

public:
	UFUNCTION(BlueprintPure, Category = "Stats")
	float GetFinalAttackPower(float BaseAttack) const;

#pragma region Health Stats
protected:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Stats")
	float BaseMaxHP = 100.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Stats")
	float BaseDefense = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Stats")
	float BaseJumpZVelocity = 400.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Stats")
	float BaseMoveSpeed = 400.f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Stats")
	float MaxHP = 100.f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Stats")
	float CurrentHP = 100.f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Stats")
	bool bIsDead = false;

public:
	UPROPERTY(BlueprintAssignable, Category = "Events")
	FOnHPChangedSignature OnHPChanged;

	UPROPERTY(BlueprintAssignable, Category = "Events")
	FOnDeathSignature OnDeath;

public:
	UFUNCTION(BlueprintCallable, Category = "Stats")
	float ApplyDamage(float Amount);

	UFUNCTION(BlueprintCallable, Category = "Stats")
	void Heal(float Amount);

	UFUNCTION(BlueprintCallable, Category = "Stats")
	void RecalculateStats();

	UFUNCTION(BlueprintPure, Category = "Stats")
	float GetCurrentHP() const { return CurrentHP; }

	UFUNCTION(BlueprintPure, Category = "Stats")
	float GetMaxHP() const { return MaxHP; }

	UFUNCTION(BlueprintPure, Category = "Stats")
	float GetHPPercent() const;

	UFUNCTION(BlueprintPure, Category = "Stats")
	float GetFinalDefense() const;

	UFUNCTION(BlueprintPure, Category = "Stats")
	float GetFinalDamageAfterDefense(float DamageAmount) const;

	UFUNCTION(BlueprintPure, Category = "Stats")
	float GetFinalJumpZVelocity() const;

	UFUNCTION(BlueprintPure, Category = "Stats")
	float GetFinalMoveSpeed() const;

	UFUNCTION(BlueprintPure, Category = "Stats")
	bool IsDead() const { return bIsDead; }

protected:
	void Die();
#pragma endregion Health Stats

#pragma region Experience Stats
protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Stats")
	int32 Level = 1;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Stats")
	int32 CurrentEXP = 0;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Stats")
	int32 MaxLevel = 50;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "SFX")
	TObjectPtr<USoundBase> LevelUpSound = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "VFX|LevelUp")
	TObjectPtr<UNiagaraSystem> LevelUpVFX = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "VFX|LevelUp")
	FLinearColor LevelUpVFXColor = FLinearColor(0.25f, 0.75f, 1.0f, 1.0f);

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "VFX|LevelUp")
	float LevelUpVFXScale = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "VFX|LevelUp", meta = (ClampMin = "0.01"))
	float LevelUpVFXRevealDuration = 0.5f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "VFX|LevelUp", meta = (ClampMin = "0.0"))
	float LevelUpVFXHoldDuration = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "VFX|LevelUp", meta = (ClampMin = "0.01"))
	float LevelUpVFXLifetime = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "VFX|LevelUp")
	FName LevelUpVFXRevealParameterName = TEXT("RevealAmount");

	UPROPERTY(Transient)
	TObjectPtr<UNiagaraComponent> ActiveLevelUpVFX = nullptr;

	FTimerHandle LevelUpVFXTimerHandle;
	float LevelUpVFXElapsedTime = 0.0f;

public:
	UPROPERTY(BlueprintAssignable, Category = "Events")
	FOnEXPChangedSignature OnEXPChanged;

	UPROPERTY(BlueprintAssignable, Category = "Events")
	FOnLevelChangedSignature OnLevelChanged;

public:
	UFUNCTION(BlueprintCallable, Category = "Stats")
	void AddEXP(int32 Amount);

	UFUNCTION(BlueprintCallable, Category = "Stats")
	void LevelUp();

	UFUNCTION(BlueprintPure, Category = "Stats")
	int32 GetLevel() const { return Level; }

	UFUNCTION(BlueprintPure, Category = "Stats")
	int32 GetCurrentEXP() const { return CurrentEXP; }

	UFUNCTION(BlueprintPure, Category = "Stats")
	int32 GetMaxLevel() const { return MaxLevel; }

	UFUNCTION(BlueprintPure, Category = "Stats")
	int32 GetRequiredEXP() const;

	UFUNCTION(BlueprintPure, Category = "Stats")
	float GetEXPPercent() const;

protected:
	void PlayLevelUpVFX();
	void UpdateLevelUpVFX();
	void StopLevelUpVFX();
#pragma endregion Experience Stats
};
