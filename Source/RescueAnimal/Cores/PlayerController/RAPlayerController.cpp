#include "RAPlayerController.h"

#include "GameProgressUIComponent.h"
#include "LevelTransitionComponent.h"
#include "PlayerUIFlowComponent.h"

#include "Components/InputComponent.h"

ARAPlayerController::ARAPlayerController()
{
	PrimaryActorTick.bCanEverTick = true;

	PlayerUIFlowComponent = CreateDefaultSubobject<UPlayerUIFlowComponent>(TEXT("PlayerUIFlowComponent"));
	GameProgressUIComponent = CreateDefaultSubobject<UGameProgressUIComponent>(TEXT("GameProgressUIComponent"));
	LevelTransitionComponent = CreateDefaultSubobject<ULevelTransitionComponent>(TEXT("LevelTransitionComponent"));

	InitializeMouseCursor();
}

void ARAPlayerController::BeginPlay()
{
	Super::BeginPlay();

	InitializeMouseCursor();
	SetGameInputMode();

	if (PlayerUIFlowComponent)
	{
		PlayerUIFlowComponent->InitializeHUD();
	}

	if (GameProgressUIComponent)
	{
		GameProgressUIComponent->InitializeGameProgressUI();
	}

	if (LevelTransitionComponent)
	{
		LevelTransitionComponent->HandleControllerBeginPlay();
	}
}

void ARAPlayerController::SetupInputComponent()
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
		&ARAPlayerController::ToggleInventory
	);

	InputComponent->BindAction(
		TEXT("CloseUI"),
		IE_Pressed,
		this,
		&ARAPlayerController::CloseUI
	);

	InputComponent->BindKey(
		EKeys::C,
		IE_Pressed,
		this,
		&ARAPlayerController::ToggleAnimalCollection
	);
}

void ARAPlayerController::HandleInventoryButtonClicked()
{
	if (bIsPortalTransitionInputLocked)
	{
		return;
	}

	ToggleInventory();
}

void ARAPlayerController::HandleSettingButtonClicked()
{
	if (bIsPortalTransitionInputLocked)
	{
		return;
	}

	ToggleSetting();
}

void ARAPlayerController::HandleInventoryCloseRequested()
{
	if (bIsPortalTransitionInputLocked)
	{
		return;
	}

	CloseInventory();
}

void ARAPlayerController::HandleSettingCloseRequested()
{
	CloseSetting();
}

void ARAPlayerController::HandleAnimalCollectionCloseRequested()
{
	if (bIsPortalTransitionInputLocked)
	{
		return;
	}

	CloseAnimalCollection();
}

void ARAPlayerController::ToggleInventory()
{
	if (PlayerUIFlowComponent)
	{
		PlayerUIFlowComponent->ToggleInventory();
	}
}

void ARAPlayerController::OpenInventory()
{
	if (PlayerUIFlowComponent)
	{
		PlayerUIFlowComponent->OpenInventory();
	}
}

void ARAPlayerController::CloseInventory()
{
	if (PlayerUIFlowComponent)
	{
		PlayerUIFlowComponent->CloseInventory();
	}
}

void ARAPlayerController::OpenShop(AShopActor* ShopActor)
{
	if (PlayerUIFlowComponent)
	{
		PlayerUIFlowComponent->OpenShop(ShopActor);
	}
}

void ARAPlayerController::CloseShop()
{
	if (PlayerUIFlowComponent)
	{
		PlayerUIFlowComponent->CloseShop();
	}
}

void ARAPlayerController::ToggleAnimalCollection()
{
	if (PlayerUIFlowComponent)
	{
		PlayerUIFlowComponent->ToggleAnimalCollection();
	}
}

void ARAPlayerController::OpenAnimalCollection()
{
	if (PlayerUIFlowComponent)
	{
		PlayerUIFlowComponent->OpenAnimalCollection();
	}
}

void ARAPlayerController::CloseAnimalCollection()
{
	if (PlayerUIFlowComponent)
	{
		PlayerUIFlowComponent->CloseAnimalCollection();
	}
}

void ARAPlayerController::CloseUI()
{
	if (PlayerUIFlowComponent)
	{
		PlayerUIFlowComponent->CloseUI();
	}
}

void ARAPlayerController::ToggleSetting()
{
	if (PlayerUIFlowComponent)
	{
		PlayerUIFlowComponent->ToggleSetting();
	}
}

void ARAPlayerController::OpenSetting()
{
	if (PlayerUIFlowComponent)
	{
		PlayerUIFlowComponent->OpenSetting();
	}
}

void ARAPlayerController::CloseSetting()
{
	if (PlayerUIFlowComponent)
	{
		PlayerUIFlowComponent->CloseSetting();
	}
}

void ARAPlayerController::SetGameInputMode()
{
	if (PlayerUIFlowComponent)
	{
		PlayerUIFlowComponent->SetGameInputMode();
	}
}

void ARAPlayerController::SetUIInputMode()
{
	if (PlayerUIFlowComponent)
	{
		PlayerUIFlowComponent->SetUIInputMode();
	}
}

void ARAPlayerController::SetSettingInputMode()
{
	if (PlayerUIFlowComponent)
	{
		PlayerUIFlowComponent->SetSettingInputMode();
	}
}

void ARAPlayerController::SetMenuInputMode()
{
	if (PlayerUIFlowComponent)
	{
		PlayerUIFlowComponent->SetMenuInputMode();
	}
}

void ARAPlayerController::InitializeMouseCursor()
{
	if (PlayerUIFlowComponent)
	{
		PlayerUIFlowComponent->InitializeMouseCursor();
		return;
	}

	DefaultMouseCursor = NormalMouseCursor.GetValue();
	CurrentMouseCursor = NormalMouseCursor.GetValue();
}

void ARAPlayerController::SetMouseCursorType(EMouseCursor::Type NewMouseCursor)
{
	if (PlayerUIFlowComponent)
	{
		PlayerUIFlowComponent->SetMouseCursorType(NewMouseCursor);
		return;
	}

	DefaultMouseCursor = NormalMouseCursor.GetValue();
	CurrentMouseCursor = NewMouseCursor;
}

void ARAPlayerController::HideMainHUD()
{
	if (PlayerUIFlowComponent)
	{
		PlayerUIFlowComponent->HideMainHUD();
	}
}

void ARAPlayerController::ShowMainHUD()
{
	if (PlayerUIFlowComponent)
	{
		PlayerUIFlowComponent->ShowMainHUD();
	}
}

void ARAPlayerController::SetPortalTransitionInputLocked(bool bLocked)
{
	if (PlayerUIFlowComponent)
	{
		PlayerUIFlowComponent->SetPortalTransitionInputLocked(bLocked);
	}
}

void ARAPlayerController::TryCreateMapProgressWidget()
{
	if (GameProgressUIComponent)
	{
		GameProgressUIComponent->TryCreateMapProgressWidget();
	}
}

void ARAPlayerController::ShowFieldClearMessage(FName MapID)
{
	if (GameProgressUIComponent)
	{
		GameProgressUIComponent->ShowFieldClearMessage(MapID);
	}
}

void ARAPlayerController::ShowGameOverMessage()
{
	if (GameProgressUIComponent)
	{
		GameProgressUIComponent->ShowGameOverMessage();
	}
}

void ARAPlayerController::StartLevelFadeIn()
{
	if (LevelTransitionComponent)
	{
		LevelTransitionComponent->StartLevelFadeIn();
	}
}

void ARAPlayerController::PlayLevelFadeIn()
{
	if (LevelTransitionComponent)
	{
		LevelTransitionComponent->PlayLevelFadeIn();
	}
}

void ARAPlayerController::FinishLevelFadeIn()
{
	if (LevelTransitionComponent)
	{
		LevelTransitionComponent->FinishLevelFadeIn();
	}
}

void ARAPlayerController::StartViewportFadeOverlay(float FromOpacity, float ToOpacity, float Duration, bool bRemoveWhenFinished)
{
	if (LevelTransitionComponent)
	{
		LevelTransitionComponent->StartViewportFadeOverlay(FromOpacity, ToOpacity, Duration, bRemoveWhenFinished);
	}
}

void ARAPlayerController::TickViewportFadeOverlay()
{
	if (LevelTransitionComponent)
	{
		LevelTransitionComponent->TickViewportFadeOverlay();
	}
}

void ARAPlayerController::EnsureViewportFadeOverlay(float InitialOpacity)
{
	if (LevelTransitionComponent)
	{
		LevelTransitionComponent->EnsureViewportFadeOverlay(InitialOpacity);
	}
}

void ARAPlayerController::SetViewportFadeOverlayOpacity(float Opacity)
{
	if (LevelTransitionComponent)
	{
		LevelTransitionComponent->SetViewportFadeOverlayOpacity(Opacity);
	}
}

void ARAPlayerController::RemoveViewportFadeOverlay()
{
	if (LevelTransitionComponent)
	{
		LevelTransitionComponent->RemoveViewportFadeOverlay();
	}
}

void ARAPlayerController::TravelToLevelWithFade(FName TargetLevelName, float FadeOutDuration)
{
	if (LevelTransitionComponent)
	{
		LevelTransitionComponent->TravelToLevelWithFade(TargetLevelName, FadeOutDuration);
	}
}

void ARAPlayerController::ReturnToTitleWithFade()
{
	if (LevelTransitionComponent)
	{
		LevelTransitionComponent->ReturnToTitleWithFade();
	}
}

void ARAPlayerController::QuitGame()
{
	if (LevelTransitionComponent)
	{
		LevelTransitionComponent->QuitGame();
	}
}

void ARAPlayerController::OpenPendingFadeTravelLevel()
{
	if (LevelTransitionComponent)
	{
		LevelTransitionComponent->OpenPendingFadeTravelLevel();
	}
}

void ARAPlayerController::TryCreateGameFlowMenuWidget()
{
	if (LevelTransitionComponent)
	{
		LevelTransitionComponent->TryCreateGameFlowMenuWidget();
	}
}

bool ARAPlayerController::IsTitleLevelName(const FString& LevelName) const
{
	return LevelTransitionComponent && LevelTransitionComponent->IsTitleLevelName(LevelName);
}

bool ARAPlayerController::IsEndingLevelName(const FString& LevelName) const
{
	return LevelTransitionComponent && LevelTransitionComponent->IsEndingLevelName(LevelName);
}

bool ARAPlayerController::IsGameFlowMenuLevel() const
{
	return LevelTransitionComponent && LevelTransitionComponent->IsGameFlowMenuLevel();
}
