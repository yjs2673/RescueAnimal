#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "TPSCreatureBase.generated.h"

class UAnimMontage;

UCLASS()
class TPSCAPTURE_API ATPSCreatureBase : public ACharacter
{
	GENERATED_BODY()

public:
	ATPSCreatureBase();

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
	virtual void Hit();
	virtual void Die();

	UFUNCTION(BlueprintCallable)
	void StopHitMontage();
};