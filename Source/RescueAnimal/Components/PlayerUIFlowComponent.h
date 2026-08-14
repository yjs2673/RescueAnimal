#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "InputCoreTypes.h"
#include "PlayerUIFlowComponent.generated.h"

class ARAPlayerController;
class AShopActor;

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

private:
	ARAPlayerController* GetOwnerController() const;
};
