#pragma once

#include "CoreMinimal.h"
#include "TPSCreatureBase.h"
#include "TPSEnemyBase.generated.h"

UCLASS()
class TPSCAPTURE_API ATPSEnemyBase : public ATPSCreatureBase
{
	GENERATED_BODY()

public:
	ATPSEnemyBase();

protected:
	virtual void BeginPlay() override;

protected:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Enemy|Combat")
	float AttackDamage = 10.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Enemy|Combat")
	float DetectRange = 600.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Enemy|Combat")
	float AttackRange = 150.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Enemy|Combat")
	float AttackCooldown = 1.5f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Enemy|Target")
	AActor* TargetActor = nullptr;

public:
	UFUNCTION(BlueprintCallable, Category = "Enemy|Target")
	virtual void SetTargetActor(AActor* NewTarget);

	UFUNCTION(BlueprintCallable, Category = "Enemy|Target")
	virtual void ClearTargetActor();

	UFUNCTION(BlueprintPure, Category = "Enemy|Combat")
	virtual bool CanAttack() const;
};