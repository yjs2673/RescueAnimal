#pragma once

#include "CoreMinimal.h"
#include "TPSCreatureBase.h"
#include "TimerManager.h"
#include "TPSEnemyBase.generated.h"

class USphereComponent;

class UAnimMontage;
class AArrowProjectile;
class AWeaponBase;

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
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

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
	float LastAttackTime = -1000.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Enemy|Combat")
	UAnimMontage* AttackMontage = nullptr;

protected:
	virtual void UpdateAttack();
	virtual void EquipDefaultWeapon();
	virtual void EquipWeapon(AWeaponBase* NewWeapon);
	virtual void SyncCombatDataFromWeapon();
	virtual void PerformAttack();
	virtual void PerformPunchAttack();
	virtual void PerformSwordAttack();
	virtual void PerformBowAttack();
	virtual bool PlayAttackMontage(UAnimMontage* MontageToPlay);

	UFUNCTION(BlueprintCallable, Category = "Enemy|Combat")
	virtual void EndAttack();

public:
	UFUNCTION(BlueprintCallable, Category = "Enemy|Combat")
	virtual void ApplyDamageToTarget();

	UFUNCTION(BlueprintCallable, Category = "Enemy|Combat")
	virtual void FireArrowAtTarget();

protected:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Enemy|Combat|Animation")
	UAnimMontage* SwordAttackMontage = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Enemy|Combat|Animation")
	UAnimMontage* BowAttackMontage = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Enemy|Combat|Bow")
	TSubclassOf<AArrowProjectile> BowProjectileClass;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Enemy|Combat|Bow")
	float BowProjectileSpeed = 2000.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Enemy|Combat|Bow")
	float BowFireDelay = 0.35f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Enemy|Combat|Bow")
	FName ArrowSpawnSocketName = TEXT("ArrowSpawnSocket");

	UPROPERTY(Transient)
	bool bBowArrowFiredThisAttack = false;

	FTimerHandle BowFireTimerHandle;
};
