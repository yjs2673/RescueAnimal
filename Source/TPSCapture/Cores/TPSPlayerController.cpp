#include "TPSPlayerController.h"

#include "Blueprint/UserWidget.h"
#include "MainHUDWidget.h"
#include "InventoryWidget.h"

void ATPSPlayerController::BeginPlay()
{
	Super::BeginPlay();

	SetGameInputMode();

	if (MainHUDWidgetClass)
	{
		MainHUDWidget = CreateWidget<UMainHUDWidget>(this, MainHUDWidgetClass);

		if (MainHUDWidget)
		{
			MainHUDWidget->AddToViewport();

			MainHUDWidget->OnInventoryButtonClicked.AddDynamic(
				this,
				&ATPSPlayerController::HandleInventoryButtonClicked
			);
		}
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("MainHUDWidgetClass is not assigned."));
	}
}

void ATPSPlayerController::SetupInputComponent()
{
	Super::SetupInputComponent();

	if (!InputComponent)
	{
		return;
	}

	InputComponent->BindAction(
		TEXT("ToggleInventory"),
		IE_Pressed,
		this,
		&ATPSPlayerController::ToggleInventory
	);

	InputComponent->BindAction(
		TEXT("CloseUI"),
		IE_Pressed,
		this,
		&ATPSPlayerController::CloseInventory
	);
}

void ATPSPlayerController::HandleInventoryButtonClicked()
{
	ToggleInventory();
}

void ATPSPlayerController::HandleInventoryCloseRequested()
{
	CloseInventory();
}

void ATPSPlayerController::ToggleInventory()
{
	if (bIsInventoryOpen)
	{
		CloseInventory();
	}
	else
	{
		OpenInventory();
	}
}

void ATPSPlayerController::OpenInventory()
{
	if (bIsInventoryOpen)
	{
		return;
	}

	if (!InventoryWidget)
	{
		if (!InventoryWidgetClass)
		{
			UE_LOG(LogTemp, Warning, TEXT("InventoryWidgetClass is not assigned."));
			return;
		}

		InventoryWidget = CreateWidget<UInventoryWidget>(this, InventoryWidgetClass);

		if (InventoryWidget)
		{
			InventoryWidget->OnInventoryCloseRequested.AddDynamic(
				this,
				&ATPSPlayerController::HandleInventoryCloseRequested
			);
		}
	}

	if (InventoryWidget)
	{
		InventoryWidget->AddToViewport();
		bIsInventoryOpen = true;

		SetUIInputMode();
	}
}

void ATPSPlayerController::CloseInventory()
{
	if (!bIsInventoryOpen)
	{
		return;
	}

	if (InventoryWidget)
	{
		InventoryWidget->RemoveFromParent();
	}

	bIsInventoryOpen = false;

	SetGameInputMode();
}

void ATPSPlayerController::SetGameInputMode()
{
	bShowMouseCursor = false;

	FInputModeGameOnly InputMode;
	SetInputMode(InputMode);
}

void ATPSPlayerController::SetUIInputMode()
{
	bShowMouseCursor = true;

	FInputModeGameAndUI InputMode;

	if (InventoryWidget)
	{
		InputMode.SetWidgetToFocus(InventoryWidget->TakeWidget());
	}

	InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
	SetInputMode(InputMode);
}