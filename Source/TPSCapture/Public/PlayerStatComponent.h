#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "PlayerStatComponent.generated.h"

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

#pragma region Bonus Stats
protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Stats")
	float BonusAttack = 0.f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Stats")
	float BonusMaxHP = 0.f;

public:
	UFUNCTION(BlueprintPure, Category = "Stats")
	float GetBonusAttack() const { return BonusAttack; }

	UFUNCTION(BlueprintPure, Category = "Stats")
	float GetBonusMaxHP() const { return BonusMaxHP; }
#pragma endregion Bonus Stats

#pragma region Health Stats
protected:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Stats")
	float BaseMaxHP = 100.f;

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
#pragma endregion Experience Stats
};