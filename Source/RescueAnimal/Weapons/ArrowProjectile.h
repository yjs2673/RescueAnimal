#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ArrowProjectile.generated.h"

class USphereComponent;
class UStaticMeshComponent;
class UProjectileMovementComponent;

class USoundBase;

class UNiagaraSystem;

UCLASS()
class RESCUEANIMAL_API AArrowProjectile : public AActor
{
	GENERATED_BODY()

public:
	AArrowProjectile();

protected:
	virtual void BeginPlay() override;

	UFUNCTION()
	void OnArrowOverlap(
		UPrimitiveComponent* OverlappedComponent,
		AActor* OtherActor,
		UPrimitiveComponent* OtherComp,
		int32 OtherBodyIndex,
		bool bFromSweep,
		const FHitResult& SweepResult
	);

public:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Arrow")
	USphereComponent* CollisionComp;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Arrow")
	UStaticMeshComponent* ArrowMesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Arrow")
	UProjectileMovementComponent* ProjectileMovement;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Arrow")
	float Damage = 20.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Arrow")
	float LifeSeconds = 5.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Arrow")
	bool bPiercing = false;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "SFX|Arrow")
	USoundBase* ArrowHitSound;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "VFX")
	UNiagaraSystem* ArrowHitVFX = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "VFX")
	FLinearColor ArrowHitColor = FLinearColor(1.f, 0.8f, 0.2f, 1.f);

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "VFX")
	float ArrowHitScale = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "VFX")
	float ArrowHitLifetime = 0.4f;

protected:
	TSet<AActor*> HitActors;
};
