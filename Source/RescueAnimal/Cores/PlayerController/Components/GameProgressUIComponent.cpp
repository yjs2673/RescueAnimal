#include "GameProgressUIComponent.h"

#include "RAPlayerController.h"
#include "GameProgressMessageWidget.h"
#include "MapProgressWidget.h"
#include "PlayerUIFlowComponent.h"
#include "RAWorldStateManager.h"

#include "Blueprint/UserWidget.h"
#include "EngineUtils.h"

UGameProgressUIComponent::UGameProgressUIComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UGameProgressUIComponent::BeginPlay()
{
	Super::BeginPlay();
}

ARAPlayerController* UGameProgressUIComponent::GetOwnerController() const
{
	return Cast<ARAPlayerController>(GetOwner());
}

void UGameProgressUIComponent::InitializeGameProgressUI()
{
	ARAPlayerController* Controller = GetOwnerController();
	if (!Controller)
	{
		return;
	}

	if (Controller->GameProgressMessageWidgetClass)
	{
		Controller->GameProgressMessageWidget = CreateWidget<UGameProgressMessageWidget>(
			Controller,
			Controller->GameProgressMessageWidgetClass
		);

		if (Controller->GameProgressMessageWidget)
		{
			Controller->GameProgressMessageWidget->AddToViewport(100);
			Controller->GameProgressMessageWidget->SetVisibility(ESlateVisibility::Collapsed);
		}
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("[GameProgress] GameProgressMessageWidgetClass is not assigned."));
	}

	TryCreateMapProgressWidget();
}

void UGameProgressUIComponent::TryCreateMapProgressWidget()
{
	ARAPlayerController* Controller = GetOwnerController();
	if (!Controller)
	{
		return;
	}

	ARAWorldStateManager* WorldStateManager = nullptr;
	for (TActorIterator<ARAWorldStateManager> It(Controller->GetWorld()); It; ++It)
	{
		WorldStateManager = *It;
		break;
	}

	if (!WorldStateManager)
	{
		return;
	}

	const FName MapID = WorldStateManager->MapID;
	const bool bIsFieldMap = MapID == TEXT("MAP_Plain") ||
		MapID == TEXT("MAP_Snow") ||
		MapID == TEXT("MAP_Desert");

	if (!bIsFieldMap)
	{
		return;
	}

	if (!Controller->MapProgressWidgetClass)
	{
		UE_LOG(LogTemp, Warning, TEXT("[MapProgress] MapProgressWidgetClass is not assigned. MapID=%s"),
			*MapID.ToString());
		return;
	}

	Controller->MapProgressWidget = CreateWidget<UMapProgressWidget>(Controller, Controller->MapProgressWidgetClass);
	if (!Controller->MapProgressWidget)
	{
		UE_LOG(LogTemp, Warning, TEXT("[MapProgress] Failed to create MapProgressWidget. MapID=%s"),
			*MapID.ToString());
		return;
	}

	Controller->MapProgressWidget->AddToViewport(10);
	Controller->MapProgressWidget->InitializeForWorldStateManager(WorldStateManager);
	Controller->MapProgressWidget->SetVisibility(ESlateVisibility::HitTestInvisible);

	UE_LOG(LogTemp, Log, TEXT("[MapProgress] Field progress widget displayed. MapID=%s"),
		*MapID.ToString());
}

void UGameProgressUIComponent::ShowFieldClearMessage(FName MapID)
{
	ARAPlayerController* Controller = GetOwnerController();
	if (!Controller || !Controller->GameProgressMessageWidget)
	{
		UE_LOG(LogTemp, Warning, TEXT("[GameProgress] Field clear message skipped: widget is null. MapID=%s"),
			*MapID.ToString());
		return;
	}

	Controller->GameProgressMessageWidget->ShowFieldClearMessage(MapID);
}

void UGameProgressUIComponent::ShowGameOverMessage()
{
	ARAPlayerController* Controller = GetOwnerController();
	if (!Controller || !Controller->GameProgressMessageWidget)
	{
		UE_LOG(LogTemp, Warning, TEXT("[GameProgress] Game over message skipped: widget is null."));
		return;
	}

	if (Controller->PlayerUIFlowComponent)
	{
		Controller->PlayerUIFlowComponent->RemoveModalWidgets();
		Controller->PlayerUIFlowComponent->HideMainHUD();
		Controller->PlayerUIFlowComponent->SetPortalTransitionInputLocked(true);
		Controller->PlayerUIFlowComponent->SetMenuInputMode();
	}

	Controller->GameProgressMessageWidget->ShowGameOverMessage();

	UE_LOG(LogTemp, Log, TEXT("[GameProgress] Game over UI displayed. Player input locked."));
}
