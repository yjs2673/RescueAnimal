#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "TPSCreatureBase.generated.h"

UCLASS()
class TPSCAPTURE_API ATPSCreatureBase : public ACharacter
{
	GENERATED_BODY()

public:
	ATPSCreatureBase();

protected:
	virtual void BeginPlay() override;

protected:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stats")
	float MaxHP = 100.f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Stats")
	float CurrentHP = 100.f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Stats")
	bool bIsDead = false;

public:
	UFUNCTION(BlueprintCallable, Category = "Stats")
	virtual void InitializeStats();

	UFUNCTION(BlueprintCallable, Category = "Stats")
	virtual void ApplyDamage(float DamageAmount);

	UFUNCTION(BlueprintCallable, Category = "Stats")
	virtual void Die();

	UFUNCTION(BlueprintPure, Category = "Stats")
	bool IsDead() const { return bIsDead; }
};