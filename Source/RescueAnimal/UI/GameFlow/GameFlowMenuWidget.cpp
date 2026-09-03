#include "GameFlowMenuWidget.h"

#include "Components/Button.h"
#include "Kismet/KismetSystemLibrary.h"
#include "RAPlayerController.h"
#include "LevelTransitionComponent.h"
#include "PlayerUIFlowComponent.h"

void UGameFlowMenuWidget::NativeConstruct()
{
	Super::NativeConstruct();

	if (StartGameButton)
	{
		StartGameButton->OnClicked.AddUniqueDynamic(this, &UGameFlowMenuWidget::HandleStartGameButtonClicked);
	}

	if (TitleButton)
	{
		TitleButton->OnClicked.AddUniqueDynamic(this, &UGameFlowMenuWidget::HandleTitleButtonClicked);
	}

	if (SettingButton)
	{
		SettingButton->OnClicked.AddUniqueDynamic(this, &UGameFlowMenuWidget::HandleSettingButtonClicked);
	}

	if (QuitButton)
	{
		QuitButton->OnClicked.AddUniqueDynamic(this, &UGameFlowMenuWidget::HandleQuitButtonClicked);
	}
}

void UGameFlowMenuWidget::NativeDestruct()
{
	if (StartGameButton)
	{
		StartGameButton->OnClicked.RemoveDynamic(this, &UGameFlowMenuWidget::HandleStartGameButtonClicked);
	}

	if (TitleButton)
	{
		TitleButton->OnClicked.RemoveDynamic(this, &UGameFlowMenuWidget::HandleTitleButtonClicked);
	}

	if (SettingButton)
	{
		SettingButton->OnClicked.RemoveDynamic(this, &UGameFlowMenuWidget::HandleSettingButtonClicked);
	}

	if (QuitButton)
	{
		QuitButton->OnClicked.RemoveDynamic(this, &UGameFlowMenuWidget::HandleQuitButtonClicked);
	}

	Super::NativeDestruct();
}

void UGameFlowMenuWidget::StartGame()
{
	if (StartGameTargetLevelName.IsNone())
	{
		UE_LOG(LogTemp, Warning, TEXT("[GameFlowMenu] StartGame skipped: StartGameTargetLevelName is None."));
		return;
	}

	ARAPlayerController* RAPlayerController = Cast<ARAPlayerController>(GetOwningPlayer());
	if (!RAPlayerController)
	{
		UE_LOG(LogTemp, Warning, TEXT("[GameFlowMenu] StartGame skipped: owning RAPlayerController is unavailable."));
		return;
	}

	UE_LOG(LogTemp, Log, TEXT("[GameFlowMenu] Start game requested. Target=%s"), *StartGameTargetLevelName.ToString());
	if (ULevelTransitionComponent* LevelTransitionComponent = RAPlayerController->GetLevelTransitionComponent())
	{
		LevelTransitionComponent->TravelToLevelWithFade(StartGameTargetLevelName);
	}
}

void UGameFlowMenuWidget::ReturnToTitle()
{
	if (TitleTargetLevelName.IsNone())
	{
		UE_LOG(LogTemp, Warning, TEXT("[GameFlowMenu] ReturnToTitle skipped: TitleTargetLevelName is None."));
		return;
	}

	ARAPlayerController* RAPlayerController = Cast<ARAPlayerController>(GetOwningPlayer());
	if (!RAPlayerController)
	{
		UE_LOG(LogTemp, Warning, TEXT("[GameFlowMenu] ReturnToTitle skipped: owning RAPlayerController is unavailable."));
		return;
	}

	UE_LOG(LogTemp, Log, TEXT("[GameFlowMenu] Return to title requested. Target=%s"), *TitleTargetLevelName.ToString());
	if (ULevelTransitionComponent* LevelTransitionComponent = RAPlayerController->GetLevelTransitionComponent())
	{
		LevelTransitionComponent->TravelToLevelWithFade(TitleTargetLevelName);
	}
}

void UGameFlowMenuWidget::QuitGame()
{
	APlayerController* PlayerController = GetOwningPlayer();

	UE_LOG(LogTemp, Log, TEXT("[GameFlowMenu] Quit game requested."));
	if (ARAPlayerController* RAPlayerController = Cast<ARAPlayerController>(PlayerController))
	{
		if (ULevelTransitionComponent* LevelTransitionComponent = RAPlayerController->GetLevelTransitionComponent())
		{
			LevelTransitionComponent->QuitGame();
			return;
		}
	}

	UKismetSystemLibrary::QuitGame(this, PlayerController, EQuitPreference::Quit, false);
}

void UGameFlowMenuWidget::HandleStartGameButtonClicked()
{
	StartGame();
}

void UGameFlowMenuWidget::HandleTitleButtonClicked()
{
	ReturnToTitle();
}

void UGameFlowMenuWidget::HandleSettingButtonClicked()
{
	if (ARAPlayerController* RAPlayerController = Cast<ARAPlayerController>(GetOwningPlayer()))
	{
		if (UPlayerUIFlowComponent* PlayerUIFlowComponent = RAPlayerController->GetPlayerUIFlowComponent())
		{
			PlayerUIFlowComponent->ToggleSetting();
		}
	}
}

void UGameFlowMenuWidget::HandleQuitButtonClicked()
{
	QuitGame();
}
