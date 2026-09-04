#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "EnemyAIComponent.generated.h"

class ARACharacter;
class ARAEnemyBase;
class AController;
class UPrimitiveComponent;

enum class EEnemyAIState : uint8;

UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class RESCUEANIMAL_API UEnemyAIComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UEnemyAIComponent();

	virtual void BeginPlay() override;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	UFUNCTION()
	void OnDetectionSphereBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);

	UFUNCTION()
	void OnDetectionSphereEndOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex);

	void UpdateChase();
	void UpdateBowSpacing();
	EEnemyAIState GetCurrentAIState() const;
	bool IsInAIState(EEnemyAIState State) const;
	bool CanChangeAIState(EEnemyAIState NewState) const;
	bool ChangeAIState(EEnemyAIState NewState);
	bool RefreshAIStateFromTarget();
	void NotifyAttackStarted();
	void NotifyAttackFinished();
	void NotifyOwnerDied();
	bool SetTargetActor(AActor* NewTarget);
	void ClearTargetActor();
	bool HasValidTarget() const;
	void SetCampPatrolArea(const FVector& InCenter, float InRadius);
	void ClearCampPatrolArea();
	void UpdateCampWander();
	void MoveToRandomCampLocation();
	bool IsValidCombatTarget(const AActor* InTargetActor) const;
	ARACharacter* ResolvePlayerFromDamage(AController* EventInstigator, AActor* DamageCauser) const;
	float GetAttackStartRange() const;
	float GetAttackHitRange() const;
	float GetChaseAcceptanceRadius() const;
	void UpdateMovementStuckCheck(float DeltaTime);
	void HandleMovementStuck();
	bool TryMoveToStrafeLocationAroundTarget();
	void ApplySeparationFromNearbyEnemies(float DeltaTime);

private:
	ARAEnemyBase* GetOwnerEnemy() const;
	bool ChangeAIStateInternal(EEnemyAIState NewState, bool bForce);
	void HandleAIStateEnter(EEnemyAIState PreviousState, EEnemyAIState NewState);
	void HandleAIStateExit(EEnemyAIState PreviousState, EEnemyAIState NewState);
	void StopOwnerMovement();
};
