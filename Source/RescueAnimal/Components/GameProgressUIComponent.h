#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "GameProgressUIComponent.generated.h"

class ARAPlayerController;

UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class RESCUEANIMAL_API UGameProgressUIComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UGameProgressUIComponent();

	virtual void BeginPlay() override;

	void InitializeGameProgressUI();
	void TryCreateMapProgressWidget();
	void ShowFieldClearMessage(FName MapID);
	void ShowGameOverMessage();

private:
	ARAPlayerController* GetOwnerController() const;
};
