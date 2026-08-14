#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "EnemyCombatComponent.generated.h"

class ARAEnemyBase;
class AArrowProjectile;
class UAnimMontage;
class UNiagaraSystem;

UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class RESCUEANIMAL_API UEnemyCombatComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UEnemyCombatComponent();

	virtual void BeginPlay() override;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	bool CanAttack() const;
	void UpdateAttack();
	void PerformAttack();
	void PerformPunchAttack();
	void PerformSwordAttack();
	void PerformBowAttack();
	bool PlayAttackMontage(UAnimMontage* MontageToPlay);
	void ScheduleAttackEnd(float Delay);
	void ReleaseBowChargeAtTarget();
	void PlayBowWeaponMontageSection(FName SectionName);
	void FaceTargetActor();
	void SetAttackMovementLocked(bool bLocked);
	void PlayMeleeHitEffects(const FVector& HitLocation);
	void SpawnHitVFX(UNiagaraSystem* NiagaraSystem, const FVector& SpawnLocation, const FRotator& SpawnRotation, const FLinearColor& Color, float Scale, float Lifetime);
	void EndAttack();
	void ApplyDamageToTarget();
	void TriggerMeleeHit();
	void FireArrowAtTarget();
	bool IsBowCharging() const;

private:
	ARAEnemyBase* GetOwnerEnemy() const;
};
