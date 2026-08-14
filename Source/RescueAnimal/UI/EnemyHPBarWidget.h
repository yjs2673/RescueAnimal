#pragma once

#include "CoreMinimal.h"
#include "RAUserWidget.h"
#include "EnemyHPBarWidget.generated.h"

class UProgressBar;

UCLASS()
class RESCUEANIMAL_API UEnemyHPBarWidget : public URAUserWidget
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = "EnemyHPBar")
	void SetHPPercent(float InPercent);

protected:
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UProgressBar> PB_HP;
};
