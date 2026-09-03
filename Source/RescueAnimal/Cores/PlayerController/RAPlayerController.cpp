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

	if (PlayerUIFlowComponent)
	{
		PlayerUIFlowComponent->InitializeMouseCursor();
	}
}

void ARAPlayerController::BeginPlay()
{
	Super::BeginPlay();

	if (PlayerUIFlowComponent)
	{
		PlayerUIFlowComponent->InitializeMouseCursor();
		PlayerUIFlowComponent->SetGameInputMode();
	}

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

	if (!InputComponent || !PlayerUIFlowComponent)
	{
		return;
	}

	InputComponent->BindAction(
		TEXT("ToggleInventory"),
		IE_Pressed,
		PlayerUIFlowComponent.Get(),
		&UPlayerUIFlowComponent::ToggleInventory
	);

	InputComponent->BindAction(
		TEXT("CloseUI"),
		IE_Pressed,
		PlayerUIFlowComponent.Get(),
		&UPlayerUIFlowComponent::CloseUI
	);

	InputComponent->BindKey(
		EKeys::C,
		IE_Pressed,
		PlayerUIFlowComponent.Get(),
		&UPlayerUIFlowComponent::ToggleAnimalCollection
	);
}
