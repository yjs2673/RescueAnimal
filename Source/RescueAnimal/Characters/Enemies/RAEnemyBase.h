#pragma once

#include "CoreMinimal.h"
#include "RAStructTypes.h"
#include "RACreatureBase.h"
#include "TimerManager.h"
#include "RAEnemyBase.generated.h"

class USphereComponent;

class ADropItemActor;
class AController;
class UAnimMontage;
class AArrowProjectile;
class AWeaponBase;
class UNiagaraSystem;
class USoundBase;
class UWidgetComponent;

class ARACharacter;

class UEnemyAIComponent;
class UEnemyCombatComponent;
class UEnemyEquipmentComponent;
class UEnemyRewardComponent;

UENUM(BlueprintType)
enum class EEnemyAttackType : uint8
{
	Punch	UMETA(DisplayName = "Punch"),
	Sword	UMETA(DisplayName = "Sword"),
	Bow		UMETA(DisplayName = "Bow")
};

UENUM(BlueprintType)
enum class EEnemyAIState : uint8
{
	Idle	UMETA(DisplayName = "Idle"),
	Patrol	UMETA(DisplayName = "Patrol"),
	Chase	UMETA(DisplayName = "Chase"),
	Attack	UMETA(DisplayName = "Attack"),
	Dead	UMETA(DisplayName = "Dead")
};

UCLASS()
class RESCUEANIMAL_API ARAEnemyBase : public ARACreatureBase
{
	GENERATED_BODY()

	friend class UEnemyAIComponent;
	friend class UEnemyCombatComponent;
	friend class UEnemyEquipmentComponent;
	friend class UEnemyRewardComponent;

public:
	ARAEnemyBase();

	virtual float TakeDamage(
		float DamageAmount,
		struct FDamageEvent const& DamageEvent,
		class AController* EventInstigator,
		AActor* DamageCauser
	) override;

	UFUNCTION(BlueprintPure, Category = "Components")
	FORCEINLINE UEnemyAIComponent* GetEnemyAIComponent() const { return EnemyAIComponent; }

	UFUNCTION(BlueprintPure, Category = "Components")
	FORCEINLINE UEnemyCombatComponent* GetEnemyCombatComponent() const { return EnemyCombatComponent; }

	UFUNCTION(BlueprintPure, Category = "Components")
	FORCEINLINE UEnemyEquipmentComponent* GetEnemyEquipmentComponent() const { return EnemyEquipmentComponent; }

	UFUNCTION(BlueprintPure, Category = "Components")
	FORCEINLINE UEnemyRewardComponent* GetEnemyRewardComponent() const { return EnemyRewardComponent; }

	UFUNCTION(BlueprintPure, Category = "Enemy|AI")
	FORCEINLINE EEnemyAIState GetEnemyAIState() const { return CurrentAIState; }

	UFUNCTION(BlueprintCallable, Category = "Enemy|AI")
	void SetEnemyAIState(EEnemyAIState NewState);

protected:
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaTime) override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Enemy|Detection")
	TObjectPtr<USphereComponent> DetectionSphere;

#pragma region Runtime World State
	UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Category = "World State")
	FName ActorSaveID = NAME_None;
#pragma endregion Runtime World State

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Enemy|Combat")
	EEnemyAttackType AttackType = EEnemyAttackType::Punch;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Enemy|AI")
	EEnemyAIState CurrentAIState = EEnemyAIState::Idle;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Enemy|Combat")
	float AttackDamage = 10.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Enemy|Combat")
	float DetectRange = 800.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Enemy|Combat")
	float AttackRange = 150.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Enemy|Combat")
	float AttackStartRangePadding = 35.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Enemy|Combat")
	float AttackHitRangePadding = 60.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Enemy|Combat")
	float AttackCooldown = 1.5f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Enemy|Reward")
	int32 EXPReward = 10;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Enemy|Movement")
	float MoveSpeed = 400.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Enemy|Movement")
	float BowChargingMoveSpeed = 150.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Enemy|Movement")
	bool bUseStuckRecovery = true;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Enemy|Movement", meta = (ClampMin = "0.0"))
	float StuckVelocityThreshold = 12.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Enemy|Movement", meta = (ClampMin = "0.1"))
	float StuckTimeThreshold = 0.8f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Enemy|Movement", meta = (ClampMin = "0.0"))
	float StuckRecoveryCooldown = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Enemy|Movement", meta = (ClampMin = "0.0"))
	float StuckSideStepDistance = 250.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Enemy|Movement")
	bool bUseEnemySeparation = true;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Enemy|Movement", meta = (ClampMin = "0.0"))
	float EnemySeparationRadius = 120.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Enemy|Movement", meta = (ClampMin = "0.0"))
	float EnemySeparationStrength = 0.6f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Enemy|Target")
	TObjectPtr<AActor> TargetActor = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Enemy|Weapon")
	TSubclassOf<AWeaponBase> EnemyWeaponClass;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Enemy|Weapon")
	TObjectPtr<AWeaponBase> CurrentWeapon = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Enemy|Weapon")
	FName RightWeaponSocketName = TEXT("RightHandSocket");

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Enemy|Weapon")
	FName LeftWeaponSocketName = TEXT("LeftHandSocket");

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Enemy|Weapon")
	bool bUseEquippedWeaponCombatData = true;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Enemy|UI")
	TObjectPtr<UWidgetComponent> HPBarWidgetComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UEnemyAIComponent> EnemyAIComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UEnemyCombatComponent> EnemyCombatComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UEnemyEquipmentComponent> EnemyEquipmentComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UEnemyRewardComponent> EnemyRewardComponent;

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

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Enemy|Combat")
	float LastAttackTime = -1000.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Enemy|Combat")
	UAnimMontage* AttackMontage = nullptr;

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Enemy|Camp")
	bool bUseCampPatrolArea = false;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Enemy|Camp")
	FVector CampPatrolCenter = FVector::ZeroVector;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Enemy|Camp")
	float CampPatrolRadius = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Enemy|Camp")
	float CampWanderInterval = 3.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Enemy|Camp")
	float CampWanderAcceptanceRadius = 80.0f;

	float LastCampWanderTime = -1000.0f;

	UFUNCTION(BlueprintCallable, Category = "Enemy|Combat")
	virtual void EndAttack();

	virtual void UpdateHPBar() override;
	virtual void UpdateHPBarVisibility();

	virtual void Die() override;

public:
	UFUNCTION(BlueprintCallable, Category = "Enemy|Combat")
	virtual void ApplyDamageToTarget();

	UFUNCTION(BlueprintCallable, Category = "Enemy|Combat")
	virtual void TriggerMeleeHit();

protected:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Enemy|Combat|Animation")
	UAnimMontage* SwordAttackMontage = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Enemy|Combat|Animation")
	UAnimMontage* BowAttackMontage = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Enemy|SFX|Punch")
	TArray<USoundBase*> PunchHitSounds;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Enemy|SFX|Sword")
	USoundBase* SwordHitSound = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Enemy|SFX|Bow")
	USoundBase* BowDrawSound = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Enemy|VFX|Punch")
	UNiagaraSystem* PunchHitVFX = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Enemy|VFX|Punch")
	FLinearColor PunchHitColor = FLinearColor(1.f, 1.f, 1.f, 1.f);

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Enemy|VFX|Punch")
	float PunchHitScale = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Enemy|VFX|Punch")
	float PunchHitLifetime = 0.35f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Enemy|VFX|Sword")
	UNiagaraSystem* SwordHitVFX = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Enemy|VFX|Sword")
	FLinearColor SwordHitColor = FLinearColor(1.f, 0.2f, 0.2f, 1.f);

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Enemy|VFX|Sword")
	float SwordHitScale = 1.2f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Enemy|VFX|Sword")
	float SwordHitLifetime = 0.45f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Enemy|Combat|Bow")
	TSubclassOf<AArrowProjectile> BowProjectileClass;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Enemy|Combat|Bow")
	float BowProjectileSpeed = 2000.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Enemy|Combat|Bow")
	float BowAttackRange = 800.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Enemy|Combat|Bow")
	float BowPreferredDistance = 600.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Enemy|Combat|Bow")
	float BowDistanceTolerance = 100.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Enemy|Combat|Bow")
	float BowRetreatStepDistance = 250.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Enemy|Combat|Bow")
	float BowMoveAcceptanceRadius = 50.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Enemy|Combat|Bow")
	float BowFullChargeTime = 1.5f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Enemy|Combat|Bow")
	float BowReleaseEndDelay = 0.8f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Enemy|Combat")
	float AttackEndFallbackDelay = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Enemy|Combat")
	float PunchHitDelay = 0.35f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Enemy|Combat")
	float SwordHitDelay = 0.35f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Drop")
	TArray<FDropItemData> DropItems;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Drop")
	TSubclassOf<ADropItemActor> DropItemActorClass;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Drop")
	bool bHasDroppedItems = false;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Enemy|Reward")
	bool bHasGrantedEXP = false;

	UPROPERTY(Transient)
	TObjectPtr<AController> LastDamageInstigator = nullptr;

	UPROPERTY(Transient)
	TObjectPtr<AActor> LastDamageCauser = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Enemy|Combat|Bow")
	FName ArrowSpawnSocketName = TEXT("ArrowSpawnSocket");

	UPROPERTY(Transient)
	bool bBowArrowFiredThisAttack = false;

	UPROPERTY(Transient)
	bool bMeleeDamageAppliedThisAttack = false;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Enemy|Combat|Bow")
	bool bIsBowCharging = false;

	UPROPERTY(Transient)
	bool bWantsMovementThisTick = false;

	UPROPERTY(Transient)
	float StuckTime = 0.f;

	UPROPERTY(Transient)
	float LastStuckRecoveryTime = -1000.f;

	FTimerHandle BowFireTimerHandle;
	FTimerHandle AttackEndTimerHandle;
	FTimerHandle MeleeHitTimerHandle;
};
