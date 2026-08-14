#pragma once

#include "CoreMinimal.h"
#include "UI/Audio/RAUserWidget.h"
#include "BuffListWidget.generated.h"

class UPanelWidget;
class UBuffIconWidget;
class UPlayerStatComponent;

UCLASS()
class RESCUEANIMAL_API UBuffListWidget : public URAUserWidget
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = "Buff")
	void BindPlayerStatComponent(UPlayerStatComponent* InStatComponent);

	UFUNCTION(BlueprintCallable, Category = "Buff")
	void UnbindPlayerStatComponent();

	UFUNCTION()
	void RefreshBuffs();

protected:
	virtual void NativeDestruct() override;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UPanelWidget> BuffContainer = nullptr;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Buff")
	TSubclassOf<UBuffIconWidget> BuffIconWidgetClass;

private:
	UPROPERTY()
	TObjectPtr<UPlayerStatComponent> CachedStatComponent = nullptr;
};
