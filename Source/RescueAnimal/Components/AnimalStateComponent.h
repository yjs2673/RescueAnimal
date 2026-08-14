#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "RAGameEnums.h"
#include "AnimalStateComponent.generated.h"

class AAnimalBase;
class AActor;

UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class RESCUEANIMAL_API UAnimalStateComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UAnimalStateComponent();

	virtual void BeginPlay() override;

	void InitAnimalData();
	void SetAnimalState(EAnimalState NewState);
	bool CanTakeDamage() const;
	void HandleDamageTaken(float ActualDamage, AActor* DamageCauser);

private:
	AAnimalBase* GetOwnerAnimal() const;
};
