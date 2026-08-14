#pragma once

#include "CoreMinimal.h"
#include "RAUserWidget.h"
#include "ShopResultWidget.generated.h"

class UButton;
class UTextBlock;

UCLASS()
class RESCUEANIMAL_API UShopResultWidget : public URAUserWidget
{
	GENERATED_BODY()

public:
	UShopResultWidget(const FObjectInitializer& ObjectInitializer);

	virtual void NativeConstruct() override;

	UFUNCTION(BlueprintCallable, Category = "Shop")
	void SetupResult(const FText& InResultMessage);

protected:
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> ResultText;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UButton> ConfirmButton;

private:
	UFUNCTION()
	void HandleConfirmClicked();
};
