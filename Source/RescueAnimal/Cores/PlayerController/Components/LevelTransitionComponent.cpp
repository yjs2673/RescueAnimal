#include "LevelTransitionComponent.h"

#include "RAPlayerController.h"
#include "GameFlowMenuWidget.h"
#include "PlayerUIFlowComponent.h"
#include "RAAudioSubsystem.h"
#include "RAGameInstance.h"

#include "Blueprint/UserWidget.h"
#include "Camera/PlayerCameraManager.h"
#include "Engine/Engine.h"
#include "Engine/GameViewportClient.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Styling/CoreStyle.h"
#include "TimerManager.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/SOverlay.h"

namespace
{
	TSharedPtr<SWidget> GViewportFadeOverlayRootWidget;
	TSharedPtr<SBorder> GViewportFadeOverlayBorderWidget;
}

ULevelTransitionComponent::ULevelTransitionComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void ULevelTransitionComponent::BeginPlay()
{
	Super::BeginPlay();
}

void ULevelTransitionComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (ARAPlayerController* Controller = GetOwnerController())
	{
		Controller->GetWorldTimerManager().ClearTimer(Controller->FadeInTimerHandle);
		Controller->GetWorldTimerManager().ClearTimer(Controller->FadeInStartTimerHandle);
		Controller->GetWorldTimerManager().ClearTimer(Controller->FadeOutTimerHandle);
		Controller->GetWorldTimerManager().ClearTimer(Controller->ViewportFadeOverlayTimerHandle);
	}

	Super::EndPlay(EndPlayReason);
}

ARAPlayerController* ULevelTransitionComponent::GetOwnerController() const
{
	return Cast<ARAPlayerController>(GetOwner());
}

void ULevelTransitionComponent::HandleControllerBeginPlay()
{
	ARAPlayerController* Controller = GetOwnerController();
	if (!Controller)
	{
		return;
	}

	if (UGameInstance* GameInstance = Controller->GetGameInstance())
	{
		if (URAAudioSubsystem* AudioSubsystem = GameInstance->GetSubsystem<URAAudioSubsystem>())
		{
			AudioSubsystem->ApplyRuntimeSettings();
		}
	}

	const FString CurrentLevelName = UGameplayStatics::GetCurrentLevelName(Controller, true);
	const bool bIsGameFlowMenuLevel = IsTitleLevelName(CurrentLevelName) || IsEndingLevelName(CurrentLevelName);

	TryCreateGameFlowMenuWidget();

	bool bShouldStartFadeIn = bIsGameFlowMenuLevel;
	if (URAGameInstance* RAGameInstance = Cast<URAGameInstance>(UGameplayStatics::GetGameInstance(Controller)))
	{
		UE_LOG(LogTemp, Log, TEXT("[GameFlow] BeginPlay transition check. Level=%s PendingTransition=%s MenuLevel=%s"),
			*CurrentLevelName,
			RAGameInstance->bPendingPortalTransition ? TEXT("true") : TEXT("false"),
			bIsGameFlowMenuLevel ? TEXT("true") : TEXT("false"));

		if (RAGameInstance->bPendingPortalTransition)
		{
			RAGameInstance->bPendingPortalTransition = false;
			bShouldStartFadeIn = true;
		}
	}

	if (bIsGameFlowMenuLevel)
	{
		Controller->HideMainHUD();
		Controller->SetIgnoreMoveInput(true);
		Controller->SetIgnoreLookInput(true);
		Controller->SetMenuInputMode();
	}

	if (bShouldStartFadeIn)
	{
		Controller->SetPortalTransitionInputLocked(true);
		Controller->HideMainHUD();
		StartLevelFadeIn();
	}
}

void ULevelTransitionComponent::StartLevelFadeIn()
{
	ARAPlayerController* Controller = GetOwnerController();
	if (!Controller)
	{
		return;
	}

	const float SafeFadeInDuration = FMath::Max(0.0f, Controller->FadeInDuration);
	const float SafeFadeInStartDelay = FMath::Max(0.0f, Controller->FadeInStartDelay);
	const float FinishDelay = FMath::Max(KINDA_SMALL_NUMBER, SafeFadeInStartDelay + SafeFadeInDuration);

	EnsureViewportFadeOverlay(1.0f);
	SetViewportFadeOverlayOpacity(1.0f);

	if (Controller->PlayerCameraManager)
	{
		Controller->PlayerCameraManager->StartCameraFade(
			1.f,
			0.f,
			SafeFadeInDuration,
			FLinearColor::Black,
			false,
			false
		);
	}

	if (SafeFadeInStartDelay <= KINDA_SMALL_NUMBER)
	{
		PlayLevelFadeIn();
	}
	else
	{
		Controller->GetWorldTimerManager().SetTimer(
			Controller->FadeInStartTimerHandle,
			this,
			&ULevelTransitionComponent::PlayLevelFadeIn,
			SafeFadeInStartDelay,
			false
		);
	}

	Controller->GetWorldTimerManager().SetTimer(
		Controller->FadeInTimerHandle,
		this,
		&ULevelTransitionComponent::FinishLevelFadeIn,
		FinishDelay,
		false
	);

	UE_LOG(LogTemp, Log, TEXT("[GameFlow] Fade-in prepared. Duration=%.2f StartDelay=%.2f"),
		SafeFadeInDuration,
		SafeFadeInStartDelay);
}

void ULevelTransitionComponent::PlayLevelFadeIn()
{
	ARAPlayerController* Controller = GetOwnerController();
	if (!Controller)
	{
		return;
	}

	Controller->GetWorldTimerManager().ClearTimer(Controller->FadeInStartTimerHandle);

	StartViewportFadeOverlay(1.0f, 0.0f, Controller->FadeInDuration, true);

	UE_LOG(LogTemp, Log, TEXT("[GameFlow] Viewport fade-in started. Duration=%.2f"), Controller->FadeInDuration);
}

void ULevelTransitionComponent::FinishLevelFadeIn()
{
	ARAPlayerController* Controller = GetOwnerController();
	if (!Controller)
	{
		return;
	}

	if (IsGameFlowMenuLevel())
	{
		Controller->bIsPortalTransitionInputLocked = false;
		Controller->SetIgnoreMoveInput(true);
		Controller->SetIgnoreLookInput(true);
		Controller->HideMainHUD();
		Controller->SetMenuInputMode();
		return;
	}

	Controller->SetPortalTransitionInputLocked(false);
	Controller->ShowMainHUD();
}

void ULevelTransitionComponent::StartViewportFadeOverlay(float FromOpacity, float ToOpacity, float Duration, bool bRemoveWhenFinished)
{
	ARAPlayerController* Controller = GetOwnerController();
	if (!Controller)
	{
		return;
	}

	Controller->ViewportFadeOverlayStartOpacity = FMath::Clamp(FromOpacity, 0.0f, 1.0f);
	Controller->ViewportFadeOverlayTargetOpacity = FMath::Clamp(ToOpacity, 0.0f, 1.0f);
	Controller->ViewportFadeOverlayDuration = FMath::Max(0.0f, Duration);
	Controller->ViewportFadeOverlayElapsedTime = 0.0f;
	Controller->bRemoveViewportFadeOverlayWhenFinished = bRemoveWhenFinished;

	EnsureViewportFadeOverlay(Controller->ViewportFadeOverlayStartOpacity);
	SetViewportFadeOverlayOpacity(Controller->ViewportFadeOverlayStartOpacity);

	if (!Controller->GetWorld())
	{
		if (Controller->bRemoveViewportFadeOverlayWhenFinished)
		{
			RemoveViewportFadeOverlay();
		}
		return;
	}

	Controller->GetWorldTimerManager().ClearTimer(Controller->ViewportFadeOverlayTimerHandle);

	if (Controller->ViewportFadeOverlayDuration <= KINDA_SMALL_NUMBER)
	{
		SetViewportFadeOverlayOpacity(Controller->ViewportFadeOverlayTargetOpacity);
		if (Controller->bRemoveViewportFadeOverlayWhenFinished)
		{
			RemoveViewportFadeOverlay();
		}
		return;
	}

	Controller->GetWorldTimerManager().SetTimer(
		Controller->ViewportFadeOverlayTimerHandle,
		this,
		&ULevelTransitionComponent::TickViewportFadeOverlay,
		1.0f / 60.0f,
		true
	);
}

void ULevelTransitionComponent::TickViewportFadeOverlay()
{
	ARAPlayerController* Controller = GetOwnerController();
	if (!Controller || !Controller->GetWorld())
	{
		return;
	}

	Controller->ViewportFadeOverlayElapsedTime += Controller->GetWorld()->GetDeltaSeconds();

	const float Alpha = Controller->ViewportFadeOverlayDuration <= KINDA_SMALL_NUMBER
		? 1.0f
		: FMath::Clamp(Controller->ViewportFadeOverlayElapsedTime / Controller->ViewportFadeOverlayDuration, 0.0f, 1.0f);
	const float NewOpacity = FMath::Lerp(
		Controller->ViewportFadeOverlayStartOpacity,
		Controller->ViewportFadeOverlayTargetOpacity,
		Alpha
	);

	SetViewportFadeOverlayOpacity(NewOpacity);

	if (Alpha >= 1.0f)
	{
		Controller->GetWorldTimerManager().ClearTimer(Controller->ViewportFadeOverlayTimerHandle);

		if (Controller->bRemoveViewportFadeOverlayWhenFinished)
		{
			RemoveViewportFadeOverlay();
		}
	}
}

void ULevelTransitionComponent::EnsureViewportFadeOverlay(float InitialOpacity)
{
	ARAPlayerController* Controller = GetOwnerController();
	if (!Controller)
	{
		return;
	}

	if (GViewportFadeOverlayRootWidget.IsValid())
	{
		Controller->ViewportFadeOverlayRootWidget = GViewportFadeOverlayRootWidget;
		Controller->ViewportFadeOverlayBorderWidget = GViewportFadeOverlayBorderWidget;
		SetViewportFadeOverlayOpacity(InitialOpacity);
		return;
	}

	if (Controller->ViewportFadeOverlayRootWidget.IsValid())
	{
		SetViewportFadeOverlayOpacity(InitialOpacity);
		return;
	}

	if (!GEngine || !GEngine->GameViewport)
	{
		UE_LOG(LogTemp, Warning, TEXT("[GameFlow] Viewport fade overlay skipped: GameViewport is unavailable."));
		return;
	}

	Controller->ViewportFadeOverlayRootWidget =
		SNew(SOverlay)
		.Visibility(EVisibility::HitTestInvisible)
		.RenderOpacity(InitialOpacity)
		+ SOverlay::Slot()
		.HAlign(HAlign_Fill)
		.VAlign(VAlign_Fill)
		[
			SAssignNew(Controller->ViewportFadeOverlayBorderWidget, SBorder)
			.BorderImage(FCoreStyle::Get().GetBrush(TEXT("WhiteBrush")))
			.BorderBackgroundColor(FLinearColor::Black)
		];

	GEngine->GameViewport->AddViewportWidgetContent(
		Controller->ViewportFadeOverlayRootWidget.ToSharedRef(),
		10000
	);

	GViewportFadeOverlayRootWidget = Controller->ViewportFadeOverlayRootWidget;
	GViewportFadeOverlayBorderWidget = Controller->ViewportFadeOverlayBorderWidget;
}

void ULevelTransitionComponent::SetViewportFadeOverlayOpacity(float Opacity)
{
	ARAPlayerController* Controller = GetOwnerController();
	if (Controller && Controller->ViewportFadeOverlayRootWidget.IsValid())
	{
		Controller->ViewportFadeOverlayRootWidget->SetRenderOpacity(FMath::Clamp(Opacity, 0.0f, 1.0f));
	}
}

void ULevelTransitionComponent::RemoveViewportFadeOverlay()
{
	ARAPlayerController* Controller = GetOwnerController();
	if (!Controller)
	{
		return;
	}

	if (Controller->GetWorld())
	{
		Controller->GetWorldTimerManager().ClearTimer(Controller->ViewportFadeOverlayTimerHandle);
	}

	if (GEngine && GEngine->GameViewport && Controller->ViewportFadeOverlayRootWidget.IsValid())
	{
		GEngine->GameViewport->RemoveViewportWidgetContent(Controller->ViewportFadeOverlayRootWidget.ToSharedRef());
	}

	Controller->ViewportFadeOverlayRootWidget.Reset();
	Controller->ViewportFadeOverlayBorderWidget.Reset();
	GViewportFadeOverlayRootWidget.Reset();
	GViewportFadeOverlayBorderWidget.Reset();
}

void ULevelTransitionComponent::TravelToLevelWithFade(FName TargetLevelName, float FadeOutDuration)
{
	ARAPlayerController* Controller = GetOwnerController();
	if (!Controller)
	{
		return;
	}

	if (TargetLevelName.IsNone())
	{
		UE_LOG(LogTemp, Warning, TEXT("[GameFlow] Travel skipped: TargetLevelName is None."));
		return;
	}

	if (!Controller->GetWorld())
	{
		UE_LOG(LogTemp, Warning, TEXT("[GameFlow] Travel skipped: World is null. Target=%s"),
			*TargetLevelName.ToString());
		return;
	}

	if (Controller->PendingFadeTravelLevelName != NAME_None)
	{
		UE_LOG(LogTemp, Log, TEXT("[GameFlow] Travel skipped: another fade travel is already pending. Target=%s"),
			*Controller->PendingFadeTravelLevelName.ToString());
		return;
	}

	Controller->PendingFadeTravelLevelName = TargetLevelName;

	if (Controller->ActiveGameFlowMenuWidget)
	{
		Controller->ActiveGameFlowMenuWidget->SetIsEnabled(false);
	}

	Controller->SetPortalTransitionInputLocked(true);
	Controller->HideMainHUD();
	Controller->bShowMouseCursor = false;

	FInputModeGameOnly InputMode;
	Controller->SetInputMode(InputMode);

	const float SafeFadeOutDuration = FadeOutDuration >= 0.0f ? FadeOutDuration : Controller->DefaultFadeOutDuration;
	StartViewportFadeOverlay(0.0f, 1.0f, SafeFadeOutDuration, false);

	if (Controller->PlayerCameraManager)
	{
		Controller->PlayerCameraManager->StartCameraFade(
			0.f,
			1.f,
			FMath::Max(0.f, SafeFadeOutDuration),
			FLinearColor::Black,
			false,
			true
		);
	}

	UE_LOG(LogTemp, Log, TEXT("[GameFlow] Fade-out travel started. Target=%s Duration=%.2f"),
		*TargetLevelName.ToString(),
		SafeFadeOutDuration);

	Controller->GetWorldTimerManager().SetTimer(
		Controller->FadeOutTimerHandle,
		this,
		&ULevelTransitionComponent::OpenPendingFadeTravelLevel,
		FMath::Max(KINDA_SMALL_NUMBER, SafeFadeOutDuration),
		false
	);
}

void ULevelTransitionComponent::ReturnToTitleWithFade()
{
	ARAPlayerController* Controller = GetOwnerController();
	if (!Controller)
	{
		return;
	}

	if (Controller->TitleMapName.IsNone())
	{
		UE_LOG(LogTemp, Warning, TEXT("[GameFlow] Return to title skipped: TitleMapName is None."));
		return;
	}

	UE_LOG(LogTemp, Log, TEXT("[GameFlow] Return to title with fade. Target=%s"), *Controller->TitleMapName.ToString());
	TravelToLevelWithFade(Controller->TitleMapName);
}

void ULevelTransitionComponent::QuitGame()
{
	ARAPlayerController* Controller = GetOwnerController();
	if (!Controller)
	{
		return;
	}

	UE_LOG(LogTemp, Log, TEXT("[GameFlow] Quit game requested from PlayerController."));
	UKismetSystemLibrary::QuitGame(Controller, Controller, EQuitPreference::Quit, false);
}

void ULevelTransitionComponent::OpenPendingFadeTravelLevel()
{
	ARAPlayerController* Controller = GetOwnerController();
	if (!Controller)
	{
		return;
	}

	if (Controller->PendingFadeTravelLevelName.IsNone())
	{
		UE_LOG(LogTemp, Warning, TEXT("[GameFlow] Pending travel cancelled: target level is None."));
		Controller->SetPortalTransitionInputLocked(false);
		return;
	}

	if (URAGameInstance* RAGameInstance = Cast<URAGameInstance>(UGameplayStatics::GetGameInstance(Controller)))
	{
		RAGameInstance->bPendingPortalTransition = true;
	}

	const FName TargetLevelName = Controller->PendingFadeTravelLevelName;
	Controller->PendingFadeTravelLevelName = NAME_None;

	SetViewportFadeOverlayOpacity(1.0f);

	UE_LOG(LogTemp, Log, TEXT("[GameFlow] Opening level after fade-out: %s"), *TargetLevelName.ToString());
	UGameplayStatics::OpenLevel(Controller, TargetLevelName);
}

void ULevelTransitionComponent::TryCreateGameFlowMenuWidget()
{
	ARAPlayerController* Controller = GetOwnerController();
	if (!Controller)
	{
		return;
	}

	const FString CurrentLevelName = UGameplayStatics::GetCurrentLevelName(Controller, true);
	TSubclassOf<UGameFlowMenuWidget> MenuWidgetClass = nullptr;

	if (IsTitleLevelName(CurrentLevelName))
	{
		MenuWidgetClass = Controller->TitleMenuWidgetClass;
	}
	else if (IsEndingLevelName(CurrentLevelName))
	{
		MenuWidgetClass = Controller->EndingMenuWidgetClass;
	}
	else
	{
		return;
	}

	if (!MenuWidgetClass)
	{
		UE_LOG(LogTemp, Warning, TEXT("[GameFlow] Menu widget class is not assigned. Level=%s"), *CurrentLevelName);
		return;
	}

	Controller->ActiveGameFlowMenuWidget = CreateWidget<UGameFlowMenuWidget>(Controller, MenuWidgetClass);
	if (!Controller->ActiveGameFlowMenuWidget)
	{
		UE_LOG(LogTemp, Warning, TEXT("[GameFlow] Failed to create menu widget. Level=%s"), *CurrentLevelName);
		return;
	}

	Controller->ActiveGameFlowMenuWidget->AddToViewport(200);
	Controller->SetMenuInputMode();

	UE_LOG(LogTemp, Log, TEXT("[GameFlow] Menu widget displayed. Level=%s"), *CurrentLevelName);
}

bool ULevelTransitionComponent::IsTitleLevelName(const FString& LevelName) const
{
	const ARAPlayerController* Controller = GetOwnerController();
	return Controller && LevelName == Controller->TitleMapName.ToString();
}

bool ULevelTransitionComponent::IsEndingLevelName(const FString& LevelName) const
{
	const ARAPlayerController* Controller = GetOwnerController();
	return Controller && LevelName == Controller->EndingMapName.ToString();
}

bool ULevelTransitionComponent::IsGameFlowMenuLevel() const
{
	const ARAPlayerController* Controller = GetOwnerController();
	if (!Controller)
	{
		return false;
	}

	const FString CurrentLevelName = UGameplayStatics::GetCurrentLevelName(Controller, true);
	return IsTitleLevelName(CurrentLevelName) || IsEndingLevelName(CurrentLevelName);
}
