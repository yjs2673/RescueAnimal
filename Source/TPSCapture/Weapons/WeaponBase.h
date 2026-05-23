#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "TPSGameEnums.h"
#include "WeaponBase.generated.h"

class UStaticMeshComponent;
class USkeletalMeshComponent;
class UAnimMontage;
class AArrowProjectile;
class USphereComponent;
class USceneComponent;

UCLASS()
class TPSCAPTURE_API AWeaponBase : public AActor
{
	GENERATED_BODY()

public:
	AWeaponBase();

protected:
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaTime) override;

	UFUNCTION()
	void OnPickupSphereBeginOverlap(
		UPrimitiveComponent* OverlappedComponent,
		AActor* OtherActor,
		UPrimitiveComponent* OtherComp,
		int32 OtherBodyIndex,
		bool bFromSweep,
		const FHitResult& SweepResult
	);

	UFUNCTION()
	void OnPickupSphereEndOverlap(
		UPrimitiveComponent* OverlappedComponent,
		AActor* OtherActor,
		UPrimitiveComponent* OtherComp,
		int32 OtherBodyIndex
	);

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon")
	FName WeaponID = NAME_None;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Weapon")
	USceneComponent* DefaultRoot;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Weapon")
	UStaticMeshComponent* WeaponMesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Weapon")
	USkeletalMeshComponent* WeaponSkeletalMesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Weapon")
	USphereComponent* PickupSphere;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon")
	EWeaponType WeaponType = EWeaponType::None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon")
	EAttackType AttackType = EAttackType::Melee;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon")
	float AttackDamage = 10.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon")
	float AttackRange = 150.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon")
	float AttackRadius = 150.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon")
	float AttackRate = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon")
	bool bUseDurability = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon")
	int32 Durability = 100;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon|Animation")
	UAnimMontage* AttackMontage;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon|Animation")
	UAnimMontage* WeaponAnimMontage = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon|Ranged")
	TSubclassOf<AArrowProjectile> ProjectileClass;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon|Ranged")
	float ProjectileSpeed = 2000.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon|Pickup")
	bool bCanBePickedUp = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon|Pickup")
	float PickupEnableDelay = 0.3f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon|Pickup|Motion")
	bool bEnablePickupMotion = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon|Pickup|Motion")
	float PickupBobAmplitude = 15.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon|Pickup|Motion")
	float PickupBobSpeed = 2.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon|Pickup|Motion")
	float PickupRotationSpeed = 90.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon|Attach")
	FVector EquipRelativeLocation = FVector::ZeroVector;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon|Attach")
	FRotator EquipRelativeRotation = FRotator::ZeroRotator;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon|Attach")
	FVector EquipRelativeScale = FVector(1.0f);

public:
	void SetPickupEnabled(bool bEnabled);
	void EnablePickupAfterDrop();

	UFUNCTION(BlueprintPure, Category = "Weapon")
	bool UsesSkeletalMesh() const;

	UFUNCTION(BlueprintCallable, Category = "Weapon")
	void UpdateWeaponVisualState();

	UFUNCTION(BlueprintPure, Category = "Weapon")
	USceneComponent* GetActiveVisualComponent() const;

protected:
	bool ShouldPlayPickupMotion() const;
	void ApplyPickupMotion(float DeltaTime);
	void ResetPickupMotionVisuals();

	FVector InitialWeaponMeshRelativeLocation = FVector::ZeroVector;
	FRotator InitialWeaponMeshRelativeRotation = FRotator::ZeroRotator;
	FVector InitialWeaponSkeletalMeshRelativeLocation = FVector::ZeroVector;
	FRotator InitialWeaponSkeletalMeshRelativeRotation = FRotator::ZeroRotator;
	float PickupMotionElapsedTime = 0.0f;
	float PickupMotionRotationYaw = 0.0f;
};
