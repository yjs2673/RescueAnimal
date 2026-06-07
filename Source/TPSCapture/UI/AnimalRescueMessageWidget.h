#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "AnimalRescueMessageWidget.generated.h"

class UImage;
class UTexture2D;

UCLASS()
class TPSCAPTURE_API UAnimalRescueMessageWidget : public UUserWidget
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