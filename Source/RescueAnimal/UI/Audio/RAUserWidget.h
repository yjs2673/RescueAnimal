#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "RAUserWidget.generated.h"

class UButton;
class USoundBase;
class UWidget;

UCLASS()
class RESCUEANIMAL_API URAUserWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	URAUserWidget(const FObjectInitializer& ObjectInitializer);

	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;

protected:
	UFUNCTION(BlueprintCallable, Category = "UI|Sound")
	void PlayUISound(USoundBase* Sound) const;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "UI|Sound")
	bool bPlayOpenCloseSounds = false;

private:
	UFUNCTION()
	void HandleAnyButtonClicked();

	void BindButtonClickSounds();
	void UnbindButtonClickSounds();
	USoundBase* ResolveButtonClickSound() const;
	USoundBase* ResolvePopupOpenSound() const;
	USoundBase* ResolvePopupCloseSound() const;
};
