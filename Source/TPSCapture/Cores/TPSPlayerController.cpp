#include "TPSPlayerController.h"

#include "Blueprint/UserWidget.h"
#include "Camera/PlayerCameraManager.h"
#include "MainHUDWidget.h"
#include "InventoryWidget.h"
#include "ShopWidget.h"
#include "AnimalCollectionWidget.h"
#include "GameProgressMessageWidget.h"
#include "ShopActor.h"
#include "TPSGameInstance.h"
#include "InputCoreTypes.h"
#include "Kismet/GameplayStatics.h"

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

#pragma region Game Progress Message
	if (GameProgressMessageWidgetClass)
	{
		GameProgressMessageWidget = CreateWidget<UGameProgressMessageWidget>(
			this,
			GameProgressMessageWidgetClass
		);

		if (GameProgressMessageWidget)
		{
			GameProgressMessageWidget->AddToViewport(100);
			GameProgressMessageWidget->SetVisibility(ESlateVisibility::Collapsed);
		}
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("[GameProgress] GameProgressMessageWidgetClass is not assigned."));
	}
#pragma endregion Game Progress Message

	if (UTPSGameInstance* TPSGameInstance = Cast<UTPSGameInstance>(UGameplayStatics::GetGameInstance(this)))
	{
		if (TPSGameInstance->bPendingPortalTransition)
		{
			TPSGameInstance->bPendingPortalTransition = false;
			SetPortalTransitionInputLocked(true);
			HideMainHUD();
			StartLevelFadeIn();
		}
	}
}

#pragma region Game Progress Message
void ATPSPlayerController::ShowFieldClearMessage(FName MapID)
{
	if (!GameProgressMessageWidget)
	{
		UE_LOG(LogTemp, Warning, TEXT("[GameProgress] Field clear message skipped: widget is null. MapID=%s"),
			*MapID.ToString());
		return;
	}

	GameProgressMessageWidget->ShowFieldClearMessage(MapID);
}

void ATPSPlayerController::ShowGameOverMessage()
{
	if (!GameProgressMessageWidget)
	{
		UE_LOG(LogTemp, Warning, TEXT("[GameProgress] Game over message skipped: widget is null."));
		return;
	}

	GameProgressMessageWidget->ShowGameOverMessage();
}
#pragma endregion Game Progress Message

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

	InputComponent->BindKey(
		EKeys::C,
		IE_Pressed,
		this,
		&ATPSPlayerController::ToggleAnimalCollection
	);
}

void ATPSPlayerController::HandleInventoryButtonClicked()
{
	if (bIsPortalTransitionInputLocked)
	{
		return;
	}

	ToggleInventory();
}

void ATPSPlayerController::HandleInventoryCloseRequested()
{
	if (bIsPortalTransitionInputLocked)
	{
		return;
	}

	CloseInventory();
}

void ATPSPlayerController::HandleAnimalCollectionCloseRequested()
{
	if (bIsPortalTransitionInputLocked)
	{
		return;
	}

	CloseAnimalCollection();
}

void ATPSPlayerController::ToggleInventory()
{
	if (bIsPortalTransitionInputLocked)
	{
		return;
	}

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
	if (bIsPortalTransitionInputLocked)
	{
		return;
	}

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

	if (bIsShopOpen || bIsAnimalCollectionOpen)
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
	else if (bIsAnimalCollectionOpen && AnimalCollectionWidget)
	{
		InputMode.SetWidgetToFocus(AnimalCollectionWidget->TakeWidget());
	}
	else if (bIsInventoryOpen && InventoryWidget)
	{
		InputMode.SetWidgetToFocus(InventoryWidget->TakeWidget());
	}

	InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
	SetInputMode(InputMode);
}

void ATPSPlayerController::HideMainHUD()
{
	if (MainHUDWidget)
	{
		MainHUDWidget->SetVisibility(ESlateVisibility::Hidden);
	}
}

void ATPSPlayerController::ShowMainHUD()
{
	if (MainHUDWidget)
	{
		MainHUDWidget->SetVisibility(ESlateVisibility::Visible);
	}
}

void ATPSPlayerController::SetPortalTransitionInputLocked(bool bLocked)
{
	bIsPortalTransitionInputLocked = bLocked;

	if (bLocked)
	{
		CloseInventory();
		CloseAnimalCollection();
	}

	SetIgnoreMoveInput(bLocked);
	SetIgnoreLookInput(bLocked);
}

void ATPSPlayerController::StartLevelFadeIn()
{
	const float FadeDelay = FMath::Max(KINDA_SMALL_NUMBER, FadeInDuration);

	if (PlayerCameraManager)
	{
		PlayerCameraManager->StartCameraFade(
			1.f,
			0.f,
			FMath::Max(0.f, FadeInDuration),
			FLinearColor::Black,
			false,
			false
		);
	}

	GetWorldTimerManager().SetTimer(
		FadeInTimerHandle,
		this,
		&ATPSPlayerController::FinishLevelFadeIn,
		FadeDelay,
		false
	);
}

void ATPSPlayerController::FinishLevelFadeIn()
{
	SetPortalTransitionInputLocked(false);
	ShowMainHUD();
}

void ATPSPlayerController::OpenShop(AShopActor* ShopActor)
{
	if (bIsPortalTransitionInputLocked)
	{
		return;
	}

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

void ATPSPlayerController::ToggleAnimalCollection()
{
	if (bIsPortalTransitionInputLocked)
	{
		return;
	}

	if (bIsAnimalCollectionOpen)
	{
		CloseAnimalCollection();
	}
	else
	{
		OpenAnimalCollection();
	}
}

void ATPSPlayerController::OpenAnimalCollection()
{
	if (bIsPortalTransitionInputLocked)
	{
		return;
	}

	if (bIsAnimalCollectionOpen)
	{
		return;
	}

	if (!AnimalCollectionWidget)
	{
		if (!AnimalCollectionWidgetClass)
		{
			UE_LOG(LogTemp, Warning, TEXT("AnimalCollectionWidgetClass is not assigned."));
			return;
		}

		AnimalCollectionWidget = CreateWidget<UAnimalCollectionWidget>(
			this,
			AnimalCollectionWidgetClass
		);

		if (AnimalCollectionWidget)
		{
			AnimalCollectionWidget->OnAnimalCollectionCloseRequested.AddDynamic(
				this,
				&ATPSPlayerController::HandleAnimalCollectionCloseRequested
			);
		}
	}

	if (AnimalCollectionWidget)
	{
		AnimalCollectionWidget->RefreshCollection();
		AnimalCollectionWidget->AddToViewport();

		bIsAnimalCollectionOpen = true;
		SetUIInputMode();
	}
}

void ATPSPlayerController::CloseAnimalCollection()
{
	if (!bIsAnimalCollectionOpen)
	{
		return;
	}

	if (AnimalCollectionWidget)
	{
		AnimalCollectionWidget->RemoveFromParent();
	}

	bIsAnimalCollectionOpen = false;

	if (bIsShopOpen || bIsInventoryOpen)
	{
		SetUIInputMode();
	}
	else
	{
		SetGameInputMode();
	}
}

void ATPSPlayerController::CloseShop()
{
	if (bIsPortalTransitionInputLocked)
	{
		return;
	}

	if (!bIsShopOpen)
		return;

	if (ShopWidget)
	{
		ShopWidget->RemoveFromParent();
	}

	bIsShopOpen = false;
	SetIgnoreMoveInput(false);
	SetIgnoreLookInput(false);

	if (bIsInventoryOpen || bIsAnimalCollectionOpen)
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
	if (bIsPortalTransitionInputLocked)
	{
		return;
	}

	if (bIsShopOpen)
	{
		CloseShop();
		return;
	}

	if (bIsAnimalCollectionOpen)
	{
		CloseAnimalCollection();
		return;
	}

	if (bIsInventoryOpen)
	{
		CloseInventory();
	}
}
