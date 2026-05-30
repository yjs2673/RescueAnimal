#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "PlayerStatusWidget.generated.h"

class UTextBlock;
class UProgressBar;

UCLASS()
class TPSCAPTURE_API UPlayerStatusWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = "Player Status")
	void UpdateHP(float CurrentHP, float MaxHP);

	UFUNCTION(BlueprintCallable, Category = "Player Status")
	void UpdateEXP(int32 CurrentEXP, int32 RequiredEXP);

	UFUNCTION(BlueprintCallable, Category = "Player Status")
	void UpdateLevel(int32 NewLevel);

protected:
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> LevelText;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UProgressBar> HPProgressBar;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> HPText;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UProgressBar> EXPProgressBar;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> EXPText;
};