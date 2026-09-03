#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "InputCoreTypes.h"
#include "PlayerUIFlowComponent.generated.h"

class ARAPlayerController;
class AShopActor;
class UMainHUDWidget;

UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class RESCUEANIMAL_API UPlayerUIFlowComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UPlayerUIFlowComponent();

	virtual void BeginPlay() override;

	void InitializeHUD();
	void ToggleInventory();
	void OpenInventory();
	void CloseInventory();
	void OpenShop(AShopActor* ShopActor);
	void CloseShop();
	void ToggleAnimalCollection();
	void OpenAnimalCollection();
	void CloseAnimalCollection();
	void CloseUI();
	void ToggleSetting();
	void OpenSetting();
	void CloseSetting();
	void HideMainHUD();
	void ShowMainHUD();
	void SetPortalTransitionInputLocked(bool bLocked);
	void SetGameInputMode();
	void SetUIInputMode();
	void SetSettingInputMode();
	void SetMenuInputMode();
	void InitializeMouseCursor();
	void SetMouseCursorType(EMouseCursor::Type NewMouseCursor);
	void RemoveModalWidgets();

	UMainHUDWidget* GetMainHUDWidget() const;
	bool IsInventoryOpen() const;
	bool IsShopOpen() const;
	bool IsAnimalCollectionOpen() const;
	bool IsSettingOpen() const;
	bool IsPortalTransitionInputLocked() const;

private:
	ARAPlayerController* GetOwnerController() const;

	UFUNCTION()
	void HandleInventoryButtonClicked();

	UFUNCTION()
	void HandleSettingButtonClicked();

	UFUNCTION()
	void HandleInventoryCloseRequested();

	UFUNCTION()
	void HandleShopCloseRequested();

	UFUNCTION()
	void HandleSettingCloseRequested();

	UFUNCTION()
	void HandleAnimalCollectionCloseRequested();
};
