#pragma once

#include "CoreMinimal.h"
#include "TPSCreatureBase.h"
#include "TPSEnemyBase.generated.h"

class USphereComponent;

class UAnimMontage;

UENUM(BlueprintType)
enum class EEnemyAttackType : uint8
{
	Punch	UMETA(DisplayName = "Punch"),
	Sword	UMETA(DisplayName = "Sword"),
	Bow		UMETA(DisplayName = "Bow")
};

UCLASS()
class TPSCAPTURE_API ATPSEnemyBase : public ATPSCreatureBase
{
	GENERATED_BODY()

public:
	ATPSEnemyBase();

protected:
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaTime) override;

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Enemy|Detection")
	TObjectPtr<USphereComponent> DetectionSphere;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Enemy|Combat")
	EEnemyAttackType AttackType = EEnemyAttackType::Punch;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Enemy|Combat")
	float AttackDamage = 10.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Enemy|Combat")
	float DetectRange = 800.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Enemy|Combat")
	float AttackRange = 150.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Enemy|Combat")
	float AttackCooldown = 1.5f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Enemy|Target")
	TObjectPtr<AActor> TargetActor = nullptr;

protected:
	UFUNCTION()
	virtual void OnDetectionSphereBeginOverlap(
		UPrimitiveComponent* OverlappedComponent,
		AActor* OtherActor,
		UPrimitiveComponent* OtherComp,
		int32 OtherBodyIndex,
		bool bFromSweep,
		const FHitResult& SweepResult
	);

	UFUNCTION()
	virtual void OnDetectionSphereEndOverlap(
		UPrimitiveComponent* OverlappedComponent,
		AActor* OtherActor,
		UPrimitiveComponent* OtherComp,
		int32 OtherBodyIndex
	);

	virtual void UpdateChase();

public:
	UFUNCTION(BlueprintCallable, Category = "Enemy|Target")
	virtual void SetTargetActor(AActor* NewTarget);

	UFUNCTION(BlueprintCallable, Category = "Enemy|Target")
	virtual void ClearTargetActor();

	UFUNCTION(BlueprintPure, Category = "Enemy|Target")
	virtual bool HasValidTarget() const;

	UFUNCTION(BlueprintPure, Category = "Enemy|Combat")
	virtual bool CanAttack() const;

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Enemy|Combat")
	bool bIsAttacking = false;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Enemy|Combat")
	float LastAttackTime = -1000.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Enemy|Combat")
	UAnimMontage* AttackMontage = nullptr;

protected:
	virtual void UpdateAttack();
	virtual void PerformPunchAttack();

	UFUNCTION(BlueprintCallable, Category = "Enemy|Combat")
	virtual void EndAttack();

public:
	UFUNCTION(BlueprintCallable, Category = "Enemy|Combat")
	virtual void ApplyDamageToTarget();
};