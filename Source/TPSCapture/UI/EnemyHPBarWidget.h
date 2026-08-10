#pragma once

#include "CoreMinimal.h"
#include "TPSUserWidget.h"
#include "EnemyHPBarWidget.generated.h"

class UProgressBar;

UCLASS()
class TPSCAPTURE_API UEnemyHPBarWidget : public UTPSUserWidget
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = "EnemyHPBar")
	void SetHPPercent(float InPercent);

protected:
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UProgressBar> PB_HP;
};
