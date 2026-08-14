#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "AnimalRescueComponent.generated.h"

class AAnimalBase;
class AEnemyCampActor;

UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class RESCUEANIMAL_API UAnimalRescueComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UAnimalRescueComponent();

	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	bool Rescue();
	void SetOwningCamp(AEnemyCampActor* InCamp);
	bool CanBeRescued() const;
	void ApplyTrappedState();
	void ApplyRescuedState();
	void ApplyRuntimeRescuedState();

private:
	AAnimalBase* GetOwnerAnimal() const;
};
