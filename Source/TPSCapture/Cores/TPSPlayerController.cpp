#include "TPSPlayerController.h"

#include "Blueprint/UserWidget.h"
#include "MainHUDWidget.h"
#include "InventoryWidget.h"
#include "ShopWidget.h"
#include "ShopActor.h"

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
		&ATPSPlayerController::CloseUI
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
		InventoryWidget->RefreshInventory();
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

	if (bIsShopOpen)
	{
		SetUIInputMode();
	}
	else
	{
		SetGameInputMode();
	}
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

	if (bIsShopOpen && ShopWidget)
	{
		InputMode.SetWidgetToFocus(ShopWidget->TakeWidget());
	}
	else if (bIsInventoryOpen && InventoryWidget)
	{
		InputMode.SetWidgetToFocus(InventoryWidget->TakeWidget());
	}

	InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
	SetInputMode(InputMode);
}

void ATPSPlayerController::OpenShop(AShopActor* ShopActor)
{
	if (bIsShopOpen || !ShopActor)
		return;

	if (!ShopWidget)
	{
		if (!ShopWidgetClass)
		{
			UE_LOG(LogTemp, Warning, TEXT("ShopWidgetClass is not assigned."));
			return;
		}

		ShopWidget = CreateWidget<UShopWidget>(this, ShopWidgetClass);

		if (ShopWidget)
		{
			ShopWidget->OnShopCloseRequested.AddDynamic(
				this,
				&ATPSPlayerController::CloseShop
			);
		}
	}

	if (ShopWidget)
	{
		ShopWidget->OpenShop(ShopActor);
		ShopWidget->AddToViewport();

		bIsShopOpen = true;
		SetIgnoreMoveInput(true);
		SetIgnoreLookInput(true);
		SetUIInputMode();

		CurrentShopActor = ShopActor;
	}
}

void ATPSPlayerController::CloseShop()
{
	if (!bIsShopOpen)
		return;

	if (ShopWidget)
	{
		ShopWidget->RemoveFromParent();
	}

	bIsShopOpen = false;
	SetIgnoreMoveInput(false);
	SetIgnoreLookInput(false);

	if (bIsInventoryOpen)
	{
		SetUIInputMode();
	}
	else
	{
		SetGameInputMode();
	}

	if (CurrentShopActor)
	{
		CurrentShopActor->OnShopClosed();
		CurrentShopActor = nullptr;
	}
}

void ATPSPlayerController::CloseUI()
{
	if (bIsShopOpen)
	{
		CloseShop();
		return;
	}

	if (bIsInventoryOpen)
	{
		CloseInventory();
	}
}
