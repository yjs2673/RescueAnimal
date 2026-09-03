#include "PlayerUIFlowComponent.h"

#include "RAPlayerController.h"
#include "AnimalCollectionWidget.h"
#include "InventoryWidget.h"
#include "LevelTransitionComponent.h"
#include "MainHUDWidget.h"
#include "MapProgressWidget.h"
#include "SettingWidget.h"
#include "ShopActor.h"
#include "ShopWidget.h"

#include "Blueprint/UserWidget.h"

UPlayerUIFlowComponent::UPlayerUIFlowComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UPlayerUIFlowComponent::BeginPlay()
{
	Super::BeginPlay();
}

ARAPlayerController* UPlayerUIFlowComponent::GetOwnerController() const
{
	return Cast<ARAPlayerController>(GetOwner());
}

void UPlayerUIFlowComponent::InitializeHUD()
{
	ARAPlayerController* Controller = GetOwnerController();
	if (!Controller)
	{
		return;
	}

	if (Controller->MainHUDWidgetClass)
	{
		Controller->MainHUDWidget = CreateWidget<UMainHUDWidget>(Controller, Controller->MainHUDWidgetClass);

		if (Controller->MainHUDWidget)
		{
			Controller->MainHUDWidget->AddToViewport();

			Controller->MainHUDWidget->OnInventoryButtonClicked.AddDynamic(
				this,
				&UPlayerUIFlowComponent::HandleInventoryButtonClicked
			);

			Controller->MainHUDWidget->OnSettingButtonClicked.AddDynamic(
				this,
				&UPlayerUIFlowComponent::HandleSettingButtonClicked
			);
		}
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("MainHUDWidgetClass is not assigned."));
	}
}

void UPlayerUIFlowComponent::ToggleInventory()
{
	ARAPlayerController* Controller = GetOwnerController();
	if (!Controller || Controller->bIsPortalTransitionInputLocked)
	{
		return;
	}

	if (Controller->bIsInventoryOpen)
	{
		CloseInventory();
	}
	else
	{
		OpenInventory();
	}
}

void UPlayerUIFlowComponent::OpenInventory()
{
	ARAPlayerController* Controller = GetOwnerController();
	if (!Controller || Controller->bIsPortalTransitionInputLocked || Controller->bIsInventoryOpen)
	{
		return;
	}

	if (!Controller->InventoryWidget)
	{
		if (!Controller->InventoryWidgetClass)
		{
			UE_LOG(LogTemp, Warning, TEXT("InventoryWidgetClass is not assigned."));
			return;
		}

		Controller->InventoryWidget = CreateWidget<UInventoryWidget>(Controller, Controller->InventoryWidgetClass);

		if (Controller->InventoryWidget)
		{
			Controller->InventoryWidget->OnInventoryCloseRequested.AddDynamic(
				this,
				&UPlayerUIFlowComponent::HandleInventoryCloseRequested
			);
		}
	}

	if (Controller->InventoryWidget)
	{
		Controller->InventoryWidget->RefreshInventory();
		Controller->InventoryWidget->AddToViewport();

		Controller->bIsInventoryOpen = true;
		SetUIInputMode();
	}
}

void UPlayerUIFlowComponent::CloseInventory()
{
	ARAPlayerController* Controller = GetOwnerController();
	if (!Controller || !Controller->bIsInventoryOpen)
	{
		return;
	}

	if (Controller->InventoryWidget)
	{
		Controller->InventoryWidget->RemoveFromParent();
	}

	Controller->bIsInventoryOpen = false;

	if (Controller->bIsShopOpen || Controller->bIsAnimalCollectionOpen)
	{
		SetUIInputMode();
	}
	else
	{
		SetGameInputMode();
	}
}

void UPlayerUIFlowComponent::OpenShop(AShopActor* ShopActor)
{
	ARAPlayerController* Controller = GetOwnerController();
	if (!Controller || Controller->bIsPortalTransitionInputLocked || Controller->bIsShopOpen || !ShopActor)
	{
		return;
	}

	if (Controller->bIsSettingOpen)
	{
		CloseSetting();
	}

	if (!Controller->ShopWidget)
	{
		if (!Controller->ShopWidgetClass)
		{
			UE_LOG(LogTemp, Warning, TEXT("ShopWidgetClass is not assigned."));
			return;
		}

		Controller->ShopWidget = CreateWidget<UShopWidget>(Controller, Controller->ShopWidgetClass);

		if (Controller->ShopWidget)
		{
			Controller->ShopWidget->OnShopCloseRequested.AddDynamic(
				this,
				&UPlayerUIFlowComponent::HandleShopCloseRequested
			);
		}
	}

	if (Controller->ShopWidget)
	{
		Controller->ShopWidget->OpenShop(ShopActor);
		Controller->ShopWidget->AddToViewport();

		Controller->bIsShopOpen = true;
		Controller->SetIgnoreMoveInput(true);
		Controller->SetIgnoreLookInput(true);
		SetUIInputMode();

		Controller->CurrentShopActor = ShopActor;
	}
}

void UPlayerUIFlowComponent::CloseShop()
{
	ARAPlayerController* Controller = GetOwnerController();
	if (!Controller || Controller->bIsPortalTransitionInputLocked || !Controller->bIsShopOpen)
	{
		return;
	}

	if (Controller->ShopWidget)
	{
		Controller->ShopWidget->RemoveFromParent();
	}

	Controller->bIsShopOpen = false;
	Controller->SetIgnoreMoveInput(false);
	Controller->SetIgnoreLookInput(false);

	if (Controller->bIsInventoryOpen || Controller->bIsAnimalCollectionOpen || Controller->bIsSettingOpen)
	{
		SetUIInputMode();
	}
	else
	{
		SetGameInputMode();
	}

	if (Controller->CurrentShopActor)
	{
		Controller->CurrentShopActor->OnShopClosed();
		Controller->CurrentShopActor = nullptr;
	}
}

void UPlayerUIFlowComponent::ToggleAnimalCollection()
{
	ARAPlayerController* Controller = GetOwnerController();
	if (!Controller || Controller->bIsPortalTransitionInputLocked)
	{
		return;
	}

	if (Controller->bIsAnimalCollectionOpen)
	{
		CloseAnimalCollection();
	}
	else
	{
		OpenAnimalCollection();
	}
}

void UPlayerUIFlowComponent::OpenAnimalCollection()
{
	ARAPlayerController* Controller = GetOwnerController();
	if (!Controller || Controller->bIsPortalTransitionInputLocked || Controller->bIsAnimalCollectionOpen)
	{
		return;
	}

	if (Controller->bIsSettingOpen)
	{
		CloseSetting();
	}

	if (!Controller->AnimalCollectionWidget)
	{
		if (!Controller->AnimalCollectionWidgetClass)
		{
			UE_LOG(LogTemp, Warning, TEXT("AnimalCollectionWidgetClass is not assigned."));
			return;
		}

		Controller->AnimalCollectionWidget = CreateWidget<UAnimalCollectionWidget>(
			Controller,
			Controller->AnimalCollectionWidgetClass
		);

		if (Controller->AnimalCollectionWidget)
		{
			Controller->AnimalCollectionWidget->OnAnimalCollectionCloseRequested.AddDynamic(
				this,
				&UPlayerUIFlowComponent::HandleAnimalCollectionCloseRequested
			);
		}
	}

	if (Controller->AnimalCollectionWidget)
	{
		Controller->AnimalCollectionWidget->RefreshCollection();
		Controller->AnimalCollectionWidget->AddToViewport();

		Controller->bIsAnimalCollectionOpen = true;
		SetUIInputMode();
	}
}

void UPlayerUIFlowComponent::CloseAnimalCollection()
{
	ARAPlayerController* Controller = GetOwnerController();
	if (!Controller || !Controller->bIsAnimalCollectionOpen)
	{
		return;
	}

	if (Controller->AnimalCollectionWidget)
	{
		Controller->AnimalCollectionWidget->RemoveFromParent();
	}

	Controller->bIsAnimalCollectionOpen = false;

	if (Controller->bIsShopOpen || Controller->bIsInventoryOpen || Controller->bIsSettingOpen)
	{
		SetUIInputMode();
	}
	else
	{
		SetGameInputMode();
	}
}

void UPlayerUIFlowComponent::CloseUI()
{
	ARAPlayerController* Controller = GetOwnerController();
	if (!Controller || Controller->bIsPortalTransitionInputLocked)
	{
		return;
	}

	if (Controller->bIsSettingOpen)
	{
		CloseSetting();
		return;
	}

	if (Controller->bIsShopOpen)
	{
		CloseShop();
		return;
	}

	if (Controller->bIsAnimalCollectionOpen)
	{
		CloseAnimalCollection();
		return;
	}

	if (Controller->bIsInventoryOpen)
	{
		CloseInventory();
		return;
	}

	OpenSetting();
}

void UPlayerUIFlowComponent::ToggleSetting()
{
	ARAPlayerController* Controller = GetOwnerController();
	if (!Controller || Controller->bIsPortalTransitionInputLocked)
	{
		return;
	}

	if (Controller->bIsSettingOpen)
	{
		CloseSetting();
	}
	else
	{
		OpenSetting();
	}
}

void UPlayerUIFlowComponent::OpenSetting()
{
	ARAPlayerController* Controller = GetOwnerController();
	if (!Controller || Controller->bIsPortalTransitionInputLocked || Controller->bIsSettingOpen)
	{
		return;
	}

	if (Controller->bIsShopOpen)
	{
		CloseShop();
	}

	if (Controller->bIsAnimalCollectionOpen)
	{
		CloseAnimalCollection();
	}

	if (Controller->bIsInventoryOpen)
	{
		CloseInventory();
	}

	if (!Controller->SettingWidget)
	{
		if (!Controller->SettingWidgetClass)
		{
			UE_LOG(LogTemp, Warning, TEXT("SettingWidgetClass is not assigned."));
			return;
		}

		Controller->SettingWidget = CreateWidget<USettingWidget>(Controller, Controller->SettingWidgetClass);

		if (Controller->SettingWidget)
		{
			Controller->SettingWidget->OnSettingCloseRequested.AddDynamic(
				this,
				&UPlayerUIFlowComponent::HandleSettingCloseRequested
			);
		}
	}

	if (Controller->SettingWidget)
	{
		Controller->SettingWidget->AddToViewport(300);

		Controller->bIsSettingOpen = true;
		Controller->SetIgnoreMoveInput(true);
		Controller->SetIgnoreLookInput(true);
		SetSettingInputMode();
	}
}

void UPlayerUIFlowComponent::CloseSetting()
{
	ARAPlayerController* Controller = GetOwnerController();
	if (!Controller || !Controller->bIsSettingOpen)
	{
		return;
	}

	if (Controller->SettingWidget)
	{
		Controller->SettingWidget->RemoveFromParent();
	}

	Controller->bIsSettingOpen = false;

	if (Controller->bIsPortalTransitionInputLocked)
	{
		return;
	}

	if (Controller->bIsShopOpen || Controller->bIsInventoryOpen || Controller->bIsAnimalCollectionOpen)
	{
		SetUIInputMode();
	}
	else if (Controller->LevelTransitionComponent && Controller->LevelTransitionComponent->IsGameFlowMenuLevel())
	{
		Controller->SetIgnoreMoveInput(true);
		Controller->SetIgnoreLookInput(true);
		SetMenuInputMode();
	}
	else
	{
		Controller->SetIgnoreMoveInput(false);
		Controller->SetIgnoreLookInput(false);
		SetGameInputMode();
	}
}

void UPlayerUIFlowComponent::HideMainHUD()
{
	ARAPlayerController* Controller = GetOwnerController();
	if (!Controller)
	{
		return;
	}

	if (Controller->MainHUDWidget)
	{
		Controller->MainHUDWidget->SetVisibility(ESlateVisibility::Hidden);
	}

	if (Controller->MapProgressWidget)
	{
		Controller->MapProgressWidget->SetVisibility(ESlateVisibility::Hidden);
	}
}

void UPlayerUIFlowComponent::ShowMainHUD()
{
	ARAPlayerController* Controller = GetOwnerController();
	if (!Controller)
	{
		return;
	}

	if (Controller->MainHUDWidget)
	{
		Controller->MainHUDWidget->SetVisibility(ESlateVisibility::Visible);
	}

	if (Controller->MapProgressWidget)
	{
		Controller->MapProgressWidget->SetVisibility(ESlateVisibility::HitTestInvisible);
	}
}

void UPlayerUIFlowComponent::SetPortalTransitionInputLocked(bool bLocked)
{
	ARAPlayerController* Controller = GetOwnerController();
	if (!Controller)
	{
		return;
	}

	Controller->bIsPortalTransitionInputLocked = bLocked;

	if (bLocked)
	{
		CloseSetting();
		CloseInventory();
		CloseAnimalCollection();
	}

	Controller->SetIgnoreMoveInput(bLocked);
	Controller->SetIgnoreLookInput(bLocked);
}

void UPlayerUIFlowComponent::SetGameInputMode()
{
	ARAPlayerController* Controller = GetOwnerController();
	if (!Controller)
	{
		return;
	}

	Controller->bShowMouseCursor = Controller->bShowMouseCursorInGame;
	SetMouseCursorType(Controller->NormalMouseCursor.GetValue());

	FInputModeGameOnly InputMode;
	Controller->SetInputMode(InputMode);
}

void UPlayerUIFlowComponent::SetUIInputMode()
{
	ARAPlayerController* Controller = GetOwnerController();
	if (!Controller)
	{
		return;
	}

	Controller->bShowMouseCursor = true;
	SetMouseCursorType(Controller->NormalMouseCursor.GetValue());

	FInputModeGameAndUI InputMode;

	if (Controller->bIsSettingOpen && Controller->SettingWidget)
	{
		InputMode.SetWidgetToFocus(Controller->SettingWidget->TakeWidget());
	}
	else if (Controller->bIsShopOpen && Controller->ShopWidget)
	{
		InputMode.SetWidgetToFocus(Controller->ShopWidget->TakeWidget());
	}
	else if (Controller->bIsAnimalCollectionOpen && Controller->AnimalCollectionWidget)
	{
		InputMode.SetWidgetToFocus(Controller->AnimalCollectionWidget->TakeWidget());
	}
	else if (Controller->bIsInventoryOpen && Controller->InventoryWidget)
	{
		InputMode.SetWidgetToFocus(Controller->InventoryWidget->TakeWidget());
	}

	InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
	Controller->SetInputMode(InputMode);
}

void UPlayerUIFlowComponent::SetSettingInputMode()
{
	ARAPlayerController* Controller = GetOwnerController();
	if (!Controller)
	{
		return;
	}

	Controller->bShowMouseCursor = true;
	SetMouseCursorType(Controller->NormalMouseCursor.GetValue());

	FInputModeUIOnly InputMode;

	if (Controller->SettingWidget)
	{
		InputMode.SetWidgetToFocus(Controller->SettingWidget->TakeWidget());
	}

	InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
	Controller->SetInputMode(InputMode);

	if (Controller->SettingWidget)
	{
		Controller->SettingWidget->SetKeyboardFocus();
	}
}

void UPlayerUIFlowComponent::SetMenuInputMode()
{
	ARAPlayerController* Controller = GetOwnerController();
	if (!Controller)
	{
		return;
	}

	Controller->bShowMouseCursor = true;
	SetMouseCursorType(Controller->NormalMouseCursor.GetValue());

	FInputModeUIOnly InputMode;
	InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
	Controller->SetInputMode(InputMode);
}

void UPlayerUIFlowComponent::InitializeMouseCursor()
{
	ARAPlayerController* Controller = GetOwnerController();
	if (!Controller)
	{
		return;
	}

	Controller->DefaultMouseCursor = Controller->NormalMouseCursor.GetValue();
	Controller->CurrentMouseCursor = Controller->NormalMouseCursor.GetValue();
}

void UPlayerUIFlowComponent::SetMouseCursorType(EMouseCursor::Type NewMouseCursor)
{
	ARAPlayerController* Controller = GetOwnerController();
	if (!Controller)
	{
		return;
	}

	Controller->DefaultMouseCursor = Controller->NormalMouseCursor.GetValue();
	Controller->CurrentMouseCursor = NewMouseCursor;
}

void UPlayerUIFlowComponent::RemoveModalWidgets()
{
	ARAPlayerController* Controller = GetOwnerController();
	if (!Controller)
	{
		return;
	}

	if (Controller->ShopWidget)
	{
		Controller->ShopWidget->RemoveFromParent();
	}
	if (Controller->InventoryWidget)
	{
		Controller->InventoryWidget->RemoveFromParent();
	}
	if (Controller->AnimalCollectionWidget)
	{
		Controller->AnimalCollectionWidget->RemoveFromParent();
	}
	if (Controller->SettingWidget)
	{
		Controller->SettingWidget->RemoveFromParent();
	}

	Controller->bIsShopOpen = false;
	Controller->bIsInventoryOpen = false;
	Controller->bIsAnimalCollectionOpen = false;
	Controller->bIsSettingOpen = false;
	Controller->CurrentShopActor = nullptr;
}

UMainHUDWidget* UPlayerUIFlowComponent::GetMainHUDWidget() const
{
	const ARAPlayerController* Controller = GetOwnerController();
	return Controller ? Controller->MainHUDWidget : nullptr;
}

bool UPlayerUIFlowComponent::IsInventoryOpen() const
{
	const ARAPlayerController* Controller = GetOwnerController();
	return Controller && Controller->bIsInventoryOpen;
}

bool UPlayerUIFlowComponent::IsShopOpen() const
{
	const ARAPlayerController* Controller = GetOwnerController();
	return Controller && Controller->bIsShopOpen;
}

bool UPlayerUIFlowComponent::IsAnimalCollectionOpen() const
{
	const ARAPlayerController* Controller = GetOwnerController();
	return Controller && Controller->bIsAnimalCollectionOpen;
}

bool UPlayerUIFlowComponent::IsSettingOpen() const
{
	const ARAPlayerController* Controller = GetOwnerController();
	return Controller && Controller->bIsSettingOpen;
}

bool UPlayerUIFlowComponent::IsPortalTransitionInputLocked() const
{
	const ARAPlayerController* Controller = GetOwnerController();
	return Controller && (Controller->bIsPortalTransitionInputLocked || Controller->bIsSettingOpen);
}

void UPlayerUIFlowComponent::HandleInventoryButtonClicked()
{
	if (IsPortalTransitionInputLocked())
	{
		return;
	}

	ToggleInventory();
}

void UPlayerUIFlowComponent::HandleSettingButtonClicked()
{
	if (IsPortalTransitionInputLocked())
	{
		return;
	}

	ToggleSetting();
}

void UPlayerUIFlowComponent::HandleInventoryCloseRequested()
{
	if (IsPortalTransitionInputLocked())
	{
		return;
	}

	CloseInventory();
}

void UPlayerUIFlowComponent::HandleShopCloseRequested()
{
	CloseShop();
}

void UPlayerUIFlowComponent::HandleSettingCloseRequested()
{
	CloseSetting();
}

void UPlayerUIFlowComponent::HandleAnimalCollectionCloseRequested()
{
	if (IsPortalTransitionInputLocked())
	{
		return;
	}

	CloseAnimalCollection();
}
