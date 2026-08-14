#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "PlayerCombatComponent.generated.h"

class AActor;
class ARACharacter;
class UAnimMontage;
class UNiagaraSystem;
class UStaticMesh;

UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class RESCUEANIMAL_API UPlayerCombatComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UPlayerCombatComponent();

	virtual void BeginPlay() override;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	void Attack();
	void OnAttackPressed();
	void OnAttackReleased();
	void EndAttack();
	void TriggerMeleeHit();
	void TriggerSkillHit();
	void NormalRelease();
	void SkillRelease();
	void ProceedCombo();
	void EnableComboWindow();
	void DisableComboWindow();
	void FireArrow();
	void FireChargedArrow();
	void EndBowAim();
	bool CanPrepareBowSkill() const;
	void SetBowPreviewArrowStaticMesh(UStaticMesh* NewPreviewArrowStaticMesh);
	void ResetBowPreviewArrowStaticMesh();
	void SetBowPreviewArrowVFX(UNiagaraSystem* NewPreviewArrowVFX, FVector RelativeLocation, FRotator RelativeRotation, FVector RelativeScale);
	void ClearBowPreviewArrowVFX();
	void ResetBowCrosshairUI();
	void ShowPreviewArrow();
	void HidePreviewArrow();
	bool CanStartSkillAction(bool bAllowBowAiming = false) const;
	void BeginSkillAction();
	void EndSkillAction();
	void FaceSkillDirection();
	bool IsAttacking() const;
	bool IsBowAiming() const;
	bool IsBowCharging() const;
	void AttackUnarmed();
	void AttackWithWeapon();
	void FaceAttackDirection();
	bool IsValidPlayerAttackTarget(const AActor* TargetActor) const;
	void PerformPunchHit(float Damage, float Range, float Radius);
	void StartComboAttack();
	void QueueComboInput();
	void PerformSwordHit(float Damage, float Range, float Radius);
	void StartBowCharge();
	void ReleaseBowCharge();
	void UpdateBowFacing(float DeltaTime);
	bool HasArrowAmmo() const;
	bool ConsumeArrowAmmo();
	void RefundArrowAmmo();
	void UpdateBowZoom(float DeltaTime);
	void UpdateBowCameraArm(float DeltaTime);
	void PlayBowWeaponMontageSection(FName SectionName);
	void SpawnHitVFX(UNiagaraSystem* NiagaraSystem, const FVector& SpawnLocation, const FRotator& SpawnRotation, const FLinearColor& Color, float Scale, float Lifetime);

	UFUNCTION()
	void OnPunchMontageEnded(UAnimMontage* Montage, bool bInterrupted);

private:
	ARACharacter* GetOwnerCharacter() const;
};
