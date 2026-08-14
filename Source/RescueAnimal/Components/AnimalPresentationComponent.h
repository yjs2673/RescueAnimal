#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "AnimalPresentationComponent.generated.h"

class AAnimalBase;
class UNiagaraSystem;

UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class RESCUEANIMAL_API UAnimalPresentationComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UAnimalPresentationComponent();

	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	void ShowSaveWidget();
	void HideSaveWidget();
	void UpdateHPBar();
	void ShowHPBar();
	void HideHPBar();
	void PlayAnimalDeathVisual();

	UFUNCTION()
	void UpdateDeathFallRotation();

private:
	AAnimalBase* GetOwnerAnimal() const;
};
