#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "TPSPlayerController.generated.h"

class UMainHUDWidget;
class UInventoryWidget;

UCLASS()
class TPSCAPTURE_API ATPSPlayerController : public APlayerController
{
	GENERATED_BODY()

public:
	virtual void BeginPlay() override;
	virtual void SetupInputComponent() override;

public:
	UFUNCTION()
	void ToggleInventory();

	UFUNCTION()
	void OpenInventory();

	UFUNCTION()
	void CloseInventory();

	UMainHUDWidget* GetMainHUDWidget() const { return MainHUDWidget; }

protected:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "UI")
	TSubclassOf<UMainHUDWidget> MainHUDWidgetClass;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "UI")
	TSubclassOf<UInventoryWidget> InventoryWidgetClass;

private:
	UPROPERTY()
	TObjectPtr<UMainHUDWidget> MainHUDWidget;

	UPROPERTY()
	TObjectPtr<UInventoryWidget> InventoryWidget;

	UPROPERTY()
	bool bIsInventoryOpen = false;

private:
	UFUNCTION()
	void HandleInventoryButtonClicked();

	UFUNCTION()
	void HandleInventoryCloseRequested();

	void SetGameInputMode();
	void SetUIInputMode();
};
