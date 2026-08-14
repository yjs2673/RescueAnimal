#pragma once

#include "CoreMinimal.h"
#include "UI/Audio/RAUserWidget.h"
#include "AnimalRescueMessageWidget.generated.h"

class UImage;
class UTexture2D;

UCLASS()
class RESCUEANIMAL_API UAnimalRescueMessageWidget : public URAUserWidget
{
	GENERATED_BODY()

public:
	virtual void NativeConstruct() override;

protected:
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UImage> RescueImage;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Rescue")
	TObjectPtr<UTexture2D> DefaultRescueTexture;
};
