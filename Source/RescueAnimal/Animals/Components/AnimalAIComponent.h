#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "AnimalAIComponent.generated.h"

class AAnimalBase;
class AActor;

UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class RESCUEANIMAL_API UAnimalAIComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UAnimalAIComponent();

	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	void StartWander();
	void MoveToRandomLocation();
	void StartFlee(AActor* ThreatActor);
	void StopFlee();
	void StopMovement();

private:
	AAnimalBase* GetOwnerAnimal() const;
};
