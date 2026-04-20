#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "PlayerStatComponent.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnHPChangedSignature, float, CurrentHP, float, MaxHP);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnDeathSignature);

UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class TPSCAPTURE_API UPlayerStatComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UPlayerStatComponent();

protected:
	virtual void BeginPlay() override;

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
};