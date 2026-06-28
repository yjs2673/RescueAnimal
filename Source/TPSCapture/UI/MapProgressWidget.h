#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "MapProgressWidget.generated.h"

class UButton;
class UTextBlock;

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnMapProgressCloseRequested);

UCLASS()
class TPSCAPTURE_API UMapProgressWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;

	UFUNCTION(BlueprintCallable, Category = "Map Progress")
	void SetProgressText(const FText& InProgressText);

	UFUNCTION(BlueprintCallable, Category = "Map Progress")
	void RequestClose();

	UPROPERTY(BlueprintAssignable, Category = "Map Progress")
	FOnMapProgressCloseRequested OnCloseRequested;

protected:
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> ProgressText;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UButton> CloseButton;

private:
	UFUNCTION()
	void HandleCloseButtonClicked();

	UPROPERTY(Transient)
	FText CachedProgressText;
};
