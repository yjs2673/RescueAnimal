#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "TimerManager.h"
#include "TPSPlayerController.generated.h"

class UMainHUDWidget;
class UInventoryWidget;
class UShopWidget;
class UAnimalCollectionWidget;
class AShopActor;

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

	UFUNCTION()
	void ToggleAnimalCollection();

	UFUNCTION()
	void OpenAnimalCollection();

	UFUNCTION()
	void CloseAnimalCollection();

	UMainHUDWidget* GetMainHUDWidget() const { return MainHUDWidget; }
	bool IsInventoryOpen() const { return bIsInventoryOpen; }

	UFUNCTION()
	void OpenShop(AShopActor* ShopActor);

	UFUNCTION()
	void CloseShop();

	UFUNCTION()
	void CloseUI();

	bool IsShopOpen() const { return bIsShopOpen; }
	bool IsAnimalCollectionOpen() const { return bIsAnimalCollectionOpen; }
	void HideMainHUD();
	void ShowMainHUD();
	void SetPortalTransitionInputLocked(bool bLocked);
	bool IsPortalTransitionInputLocked() const { return bIsPortalTransitionInputLocked; }

protected:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "UI")
	TSubclassOf<UMainHUDWidget> MainHUDWidgetClass;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "UI")
	TSubclassOf<UInventoryWidget> InventoryWidgetClass;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "UI")
	TSubclassOf<UShopWidget> ShopWidgetClass;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "UI")
	TSubclassOf<UAnimalCollectionWidget> AnimalCollectionWidgetClass;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Portal|Transition")
	float FadeInDuration = 1.0f;

	UPROPERTY()
	TObjectPtr<AShopActor> CurrentShopActor;

private:
	UPROPERTY()
	TObjectPtr<UMainHUDWidget> MainHUDWidget;

	UPROPERTY()
	TObjectPtr<UInventoryWidget> InventoryWidget;

	UPROPERTY()
	bool bIsInventoryOpen = false;

	UPROPERTY()
	TObjectPtr<UShopWidget> ShopWidget;

	UPROPERTY()
	bool bIsShopOpen = false;

	UPROPERTY()
	TObjectPtr<UAnimalCollectionWidget> AnimalCollectionWidget;

	UPROPERTY()
	bool bIsAnimalCollectionOpen = false;

	UPROPERTY()
	bool bIsPortalTransitionInputLocked = false;

	FTimerHandle FadeInTimerHandle;

private:
	UFUNCTION()
	void HandleInventoryButtonClicked();

	UFUNCTION()
	void HandleInventoryCloseRequested();

	UFUNCTION()
	void HandleAnimalCollectionCloseRequested();

	void SetGameInputMode();
	void SetUIInputMode();
	void StartLevelFadeIn();
	void FinishLevelFadeIn();
};
