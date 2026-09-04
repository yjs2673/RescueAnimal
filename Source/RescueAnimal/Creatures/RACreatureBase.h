#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "RACreatureBase.generated.h"

class UAnimMontage;

UCLASS()
class RESCUEANIMAL_API ARACreatureBase : public ACharacter
{
	GENERATED_BODY()

public:
	ARACreatureBase();

protected:
	virtual void BeginPlay() override;

public:
	virtual float TakeDamage(
		float DamageAmount,
		struct FDamageEvent const& DamageEvent,
		class AController* EventInstigator,
		AActor* DamageCauser
	) override;

protected:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Creature|Stat")
	float MaxHP = 100.f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Creature|Stat")
	float CurrentHP = 100.f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Creature|State")
	bool bIsDead = false;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Creature|State")
	float DestroyDelay = 2.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Creature|Animation")
	TArray<UAnimMontage*> HitMontages;

	UPROPERTY(Transient)
	UAnimMontage* CurrentHitMontage = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Creature|Animation")
	UAnimMontage* DeadMontage;

protected:
	virtual void InitializeStats();
	virtual void PlayHitMontage();

	virtual void UpdateHPBar();

	virtual void Die();

	UFUNCTION(BlueprintCallable)
	void StopHitMontage();
};
