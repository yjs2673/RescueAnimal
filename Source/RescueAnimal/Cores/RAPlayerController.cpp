#include "RAPlayerController.h"

#include "Blueprint/UserWidget.h"
#include "Camera/PlayerCameraManager.h"
#include "MainHUDWidget.h"
#include "InventoryWidget.h"
#include "ShopWidget.h"
#include "AnimalCollectionWidget.h"
#include "GameProgressMessageWidget.h"
#include "GameFlowMenuWidget.h"
#include "MapProgressWidget.h"
#include "SettingWidget.h"
#include "ShopActor.h"
#include "RAAudioSubsystem.h"
#include "RAGameInstance.h"
#include "RAWorldStateManager.h"
#include "InputCoreTypes.h"
#include "EngineUtils.h"
#include "Engine/Engine.h"
#include "Engine/GameViewportClient.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Styling/CoreStyle.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/SOverlay.h"

namespace
{
	TSharedPtr<SWidget> GViewportFadeOverlayRootWidget;
	TSharedPtr<SBorder> GViewportFadeOverlayBorderWidget;
}

ARAPlayerController::ARAPlayerController()
{
	PrimaryActorTick.bCanEverTick = true;
	InitializeMouseCursor();
}

void ARAPlayerController::BeginPlay()
{
	Super::BeginPlay();

	InitializeMouseCursor();
	SetGameInputMode();

	if (UGameInstance* GameInstance = GetGameInstance())
	{
		if (URAAudioSubsystem* AudioSubsystem = GameInstance->GetSubsystem<URAAudioSubsystem>())
		{
			AudioSubsystem->ApplyRuntimeSettings();
		}
	}

	const FString CurrentLevelName = UGameplayStatics::GetCurrentLevelName(this, true);
	const bool bIsGameFlowMenuLevel = IsTitleLevelName(CurrentLevelName) || IsEndingLevelName(CurrentLevelName);

	if (MainHUDWidgetClass)
	{
		MainHUDWidget = CreateWidget<UMainHUDWidget>(this, MainHUDWidgetClass);

		if (MainHUDWidget)
		{
			MainHUDWidget->AddToViewport();

			MainHUDWidget->OnInventoryButtonClicked.AddDynamic(
				this,
				&ARAPlayerController::HandleInventoryButtonClicked
			);

			MainHUDWidget->OnSettingButtonClicked.AddDynamic(
				this,
				&ARAPlayerController::HandleSettingButtonClicked
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

	TryCreateMapProgressWidget();
	TryCreateGameFlowMenuWidget();

	bool bShouldStartFadeIn = bIsGameFlowMenuLevel;
	if (URAGameInstance* RAGameInstance = Cast<URAGameInstance>(UGameplayStatics::GetGameInstance(this)))
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
		HideMainHUD();
		SetIgnoreMoveInput(true);
		SetIgnoreLookInput(true);
		SetMenuInputMode();
	}

	if (bShouldStartFadeIn)
	{
		SetPortalTransitionInputLocked(true);
		HideMainHUD();
		StartLevelFadeIn();
	}
}

void ARAPlayerController::TryCreateMapProgressWidget()
{
	ARAWorldStateManager* WorldStateManager = nullptr;
	for (TActorIterator<ARAWorldStateManager> It(GetWorld()); It; ++It)
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

	if (!MapProgressWidgetClass)
	{
		UE_LOG(LogTemp, Warning, TEXT("[MapProgress] MapProgressWidgetClass is not assigned. MapID=%s"),
			*MapID.ToString());
		return;
	}

	MapProgressWidget = CreateWidget<UMapProgressWidget>(this, MapProgressWidgetClass);
	if (!MapProgressWidget)
	{
		UE_LOG(LogTemp, Warning, TEXT("[MapProgress] Failed to create MapProgressWidget. MapID=%s"),
			*MapID.ToString());
		return;
	}

	MapProgressWidget->AddToViewport(10);
	MapProgressWidget->InitializeForWorldStateManager(WorldStateManager);
	MapProgressWidget->SetVisibility(ESlateVisibility::HitTestInvisible);

	UE_LOG(LogTemp, Log, TEXT("[MapProgress] Field progress widget displayed. MapID=%s"),
		*MapID.ToString());
}

#pragma region Game Progress Message
void ARAPlayerController::ShowFieldClearMessage(FName MapID)
{
	if (!GameProgressMessageWidget)
	{
		UE_LOG(LogTemp, Warning, TEXT("[GameProgress] Field clear message skipped: widget is null. MapID=%s"),
			*MapID.ToString());
		return;
	}

	GameProgressMessageWidget->ShowFieldClearMessage(MapID);
}

void ARAPlayerController::ShowGameOverMessage()
{
	if (!GameProgressMessageWidget)
	{
		UE_LOG(LogTemp, Warning, TEXT("[GameProgress] Game over message skipped: widget is null."));
		return;
	}

	if (ShopWidget)
	{
		ShopWidget->RemoveFromParent();
	}
	if (InventoryWidget)
	{
		InventoryWidget->RemoveFromParent();
	}
	if (AnimalCollectionWidget)
	{
		AnimalCollectionWidget->RemoveFromParent();
	}
	if (SettingWidget)
	{
		SettingWidget->RemoveFromParent();
	}

	bIsShopOpen = false;
	bIsInventoryOpen = false;
	bIsAnimalCollectionOpen = false;
	bIsSettingOpen = false;
	CurrentShopActor = nullptr;

	HideMainHUD();
	SetPortalTransitionInputLocked(true);
	SetMenuInputMode();

	GameProgressMessageWidget->ShowGameOverMessage();

	UE_LOG(LogTemp, Log, TEXT("[GameProgress] Game over UI displayed. Player input locked."));
}
#pragma endregion Game Progress Message

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

void ARAPlayerController::OpenInventory()
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
				&ARAPlayerController::HandleInventoryCloseRequested
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

void ARAPlayerController::CloseInventory()
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

void ARAPlayerController::SetGameInputMode()
{
	bShowMouseCursor = bShowMouseCursorInGame;
	SetMouseCursorType(NormalMouseCursor.GetValue());

	FInputModeGameOnly InputMode;
	SetInputMode(InputMode);
}

void ARAPlayerController::SetUIInputMode()
{
	bShowMouseCursor = true;
	SetMouseCursorType(NormalMouseCursor.GetValue());

	FInputModeGameAndUI InputMode;

	if (bIsSettingOpen && SettingWidget)
	{
		InputMode.SetWidgetToFocus(SettingWidget->TakeWidget());
	}
	else if (bIsShopOpen && ShopWidget)
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

void ARAPlayerController::SetSettingInputMode()
{
	bShowMouseCursor = true;
	SetMouseCursorType(NormalMouseCursor.GetValue());

	FInputModeUIOnly InputMode;

	if (SettingWidget)
	{
		InputMode.SetWidgetToFocus(SettingWidget->TakeWidget());
	}

	InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
	SetInputMode(InputMode);

	if (SettingWidget)
	{
		SettingWidget->SetKeyboardFocus();
	}
}

void ARAPlayerController::SetMenuInputMode()
{
	bShowMouseCursor = true;
	SetMouseCursorType(NormalMouseCursor.GetValue());

	FInputModeUIOnly InputMode;
	InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
	SetInputMode(InputMode);
}

void ARAPlayerController::InitializeMouseCursor()
{
	DefaultMouseCursor = NormalMouseCursor.GetValue();
	CurrentMouseCursor = NormalMouseCursor.GetValue();
}

void ARAPlayerController::SetMouseCursorType(EMouseCursor::Type NewMouseCursor)
{
	DefaultMouseCursor = NormalMouseCursor.GetValue();
	CurrentMouseCursor = NewMouseCursor;
}

void ARAPlayerController::HideMainHUD()
{
	if (MainHUDWidget)
	{
		MainHUDWidget->SetVisibility(ESlateVisibility::Hidden);
	}

	if (MapProgressWidget)
	{
		MapProgressWidget->SetVisibility(ESlateVisibility::Hidden);
	}
}

void ARAPlayerController::ShowMainHUD()
{
	if (MainHUDWidget)
	{
		MainHUDWidget->SetVisibility(ESlateVisibility::Visible);
	}

	if (MapProgressWidget)
	{
		MapProgressWidget->SetVisibility(ESlateVisibility::HitTestInvisible);
	}
}

void ARAPlayerController::SetPortalTransitionInputLocked(bool bLocked)
{
	bIsPortalTransitionInputLocked = bLocked;

	if (bLocked)
	{
		CloseSetting();
		CloseInventory();
		CloseAnimalCollection();
	}

	SetIgnoreMoveInput(bLocked);
	SetIgnoreLookInput(bLocked);
}

void ARAPlayerController::StartLevelFadeIn()
{
	const float SafeFadeInDuration = FMath::Max(0.0f, FadeInDuration);
	const float SafeFadeInStartDelay = FMath::Max(0.0f, FadeInStartDelay);
	const float FinishDelay = FMath::Max(KINDA_SMALL_NUMBER, SafeFadeInStartDelay + SafeFadeInDuration);

	EnsureViewportFadeOverlay(1.0f);
	SetViewportFadeOverlayOpacity(1.0f);

	if (PlayerCameraManager)
	{
		PlayerCameraManager->StartCameraFade(
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
		GetWorldTimerManager().SetTimer(
			FadeInStartTimerHandle,
			this,
			&ARAPlayerController::PlayLevelFadeIn,
			SafeFadeInStartDelay,
			false
		);
	}

	GetWorldTimerManager().SetTimer(
		FadeInTimerHandle,
		this,
		&ARAPlayerController::FinishLevelFadeIn,
		FinishDelay,
		false
	);

	UE_LOG(LogTemp, Log, TEXT("[GameFlow] Fade-in prepared. Duration=%.2f StartDelay=%.2f"),
		SafeFadeInDuration,
		SafeFadeInStartDelay);
}

void ARAPlayerController::PlayLevelFadeIn()
{
	GetWorldTimerManager().ClearTimer(FadeInStartTimerHandle);

	StartViewportFadeOverlay(1.0f, 0.0f, FadeInDuration, true);

	UE_LOG(LogTemp, Log, TEXT("[GameFlow] Viewport fade-in started. Duration=%.2f"), FadeInDuration);
}

void ARAPlayerController::FinishLevelFadeIn()
{
	if (IsGameFlowMenuLevel())
	{
		bIsPortalTransitionInputLocked = false;
		SetIgnoreMoveInput(true);
		SetIgnoreLookInput(true);
		HideMainHUD();
		SetMenuInputMode();
		return;
	}

	SetPortalTransitionInputLocked(false);
	ShowMainHUD();
}

void ARAPlayerController::StartViewportFadeOverlay(float FromOpacity, float ToOpacity, float Duration, bool bRemoveWhenFinished)
{
	ViewportFadeOverlayStartOpacity = FMath::Clamp(FromOpacity, 0.0f, 1.0f);
	ViewportFadeOverlayTargetOpacity = FMath::Clamp(ToOpacity, 0.0f, 1.0f);
	ViewportFadeOverlayDuration = FMath::Max(0.0f, Duration);
	ViewportFadeOverlayElapsedTime = 0.0f;
	bRemoveViewportFadeOverlayWhenFinished = bRemoveWhenFinished;

	EnsureViewportFadeOverlay(ViewportFadeOverlayStartOpacity);
	SetViewportFadeOverlayOpacity(ViewportFadeOverlayStartOpacity);

	if (!GetWorld())
	{
		if (bRemoveViewportFadeOverlayWhenFinished)
		{
			RemoveViewportFadeOverlay();
		}
		return;
	}

	GetWorldTimerManager().ClearTimer(ViewportFadeOverlayTimerHandle);

	if (ViewportFadeOverlayDuration <= KINDA_SMALL_NUMBER)
	{
		SetViewportFadeOverlayOpacity(ViewportFadeOverlayTargetOpacity);
		if (bRemoveViewportFadeOverlayWhenFinished)
		{
			RemoveViewportFadeOverlay();
		}
		return;
	}

	GetWorldTimerManager().SetTimer(
		ViewportFadeOverlayTimerHandle,
		this,
		&ARAPlayerController::TickViewportFadeOverlay,
		1.0f / 60.0f,
		true
	);
}

void ARAPlayerController::TickViewportFadeOverlay()
{
	if (!GetWorld())
	{
		return;
	}

	ViewportFadeOverlayElapsedTime += GetWorld()->GetDeltaSeconds();

	const float Alpha = ViewportFadeOverlayDuration <= KINDA_SMALL_NUMBER
		? 1.0f
		: FMath::Clamp(ViewportFadeOverlayElapsedTime / ViewportFadeOverlayDuration, 0.0f, 1.0f);
	const float NewOpacity = FMath::Lerp(
		ViewportFadeOverlayStartOpacity,
		ViewportFadeOverlayTargetOpacity,
		Alpha
	);

	SetViewportFadeOverlayOpacity(NewOpacity);

	if (Alpha >= 1.0f)
	{
		GetWorldTimerManager().ClearTimer(ViewportFadeOverlayTimerHandle);

		if (bRemoveViewportFadeOverlayWhenFinished)
		{
			RemoveViewportFadeOverlay();
		}
	}
}

void ARAPlayerController::EnsureViewportFadeOverlay(float InitialOpacity)
{
	if (GViewportFadeOverlayRootWidget.IsValid())
	{
		ViewportFadeOverlayRootWidget = GViewportFadeOverlayRootWidget;
		ViewportFadeOverlayBorderWidget = GViewportFadeOverlayBorderWidget;
		SetViewportFadeOverlayOpacity(InitialOpacity);
		return;
	}

	if (ViewportFadeOverlayRootWidget.IsValid())
	{
		SetViewportFadeOverlayOpacity(InitialOpacity);
		return;
	}

	if (!GEngine || !GEngine->GameViewport)
	{
		UE_LOG(LogTemp, Warning, TEXT("[GameFlow] Viewport fade overlay skipped: GameViewport is unavailable."));
		return;
	}

	ViewportFadeOverlayRootWidget =
		SNew(SOverlay)
		.Visibility(EVisibility::HitTestInvisible)
		.RenderOpacity(InitialOpacity)
		+ SOverlay::Slot()
		.HAlign(HAlign_Fill)
		.VAlign(VAlign_Fill)
		[
			SAssignNew(ViewportFadeOverlayBorderWidget, SBorder)
			.BorderImage(FCoreStyle::Get().GetBrush(TEXT("WhiteBrush")))
			.BorderBackgroundColor(FLinearColor::Black)
		];

	GEngine->GameViewport->AddViewportWidgetContent(
		ViewportFadeOverlayRootWidget.ToSharedRef(),
		10000
	);

	GViewportFadeOverlayRootWidget = ViewportFadeOverlayRootWidget;
	GViewportFadeOverlayBorderWidget = ViewportFadeOverlayBorderWidget;
}

void ARAPlayerController::SetViewportFadeOverlayOpacity(float Opacity)
{
	if (ViewportFadeOverlayRootWidget.IsValid())
	{
		ViewportFadeOverlayRootWidget->SetRenderOpacity(FMath::Clamp(Opacity, 0.0f, 1.0f));
	}
}

void ARAPlayerController::RemoveViewportFadeOverlay()
{
	if (GetWorld())
	{
		GetWorldTimerManager().ClearTimer(ViewportFadeOverlayTimerHandle);
	}

	if (GEngine && GEngine->GameViewport && ViewportFadeOverlayRootWidget.IsValid())
	{
		GEngine->GameViewport->RemoveViewportWidgetContent(ViewportFadeOverlayRootWidget.ToSharedRef());
	}

	ViewportFadeOverlayRootWidget.Reset();
	ViewportFadeOverlayBorderWidget.Reset();
	GViewportFadeOverlayRootWidget.Reset();
	GViewportFadeOverlayBorderWidget.Reset();
}

void ARAPlayerController::TravelToLevelWithFade(FName TargetLevelName, float FadeOutDuration)
{
	if (TargetLevelName.IsNone())
	{
		UE_LOG(LogTemp, Warning, TEXT("[GameFlow] Travel skipped: TargetLevelName is None."));
		return;
	}

	if (!GetWorld())
	{
		UE_LOG(LogTemp, Warning, TEXT("[GameFlow] Travel skipped: World is null. Target=%s"),
			*TargetLevelName.ToString());
		return;
	}

	if (PendingFadeTravelLevelName != NAME_None)
	{
		UE_LOG(LogTemp, Log, TEXT("[GameFlow] Travel skipped: another fade travel is already pending. Target=%s"),
			*PendingFadeTravelLevelName.ToString());
		return;
	}

	PendingFadeTravelLevelName = TargetLevelName;

	if (ActiveGameFlowMenuWidget)
	{
		ActiveGameFlowMenuWidget->SetIsEnabled(false);
	}

	SetPortalTransitionInputLocked(true);
	HideMainHUD();
	bShowMouseCursor = false;

	FInputModeGameOnly InputMode;
	SetInputMode(InputMode);

	const float SafeFadeOutDuration = FadeOutDuration >= 0.0f ? FadeOutDuration : DefaultFadeOutDuration;
	StartViewportFadeOverlay(0.0f, 1.0f, SafeFadeOutDuration, false);

	if (PlayerCameraManager)
	{
		PlayerCameraManager->StartCameraFade(
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

	GetWorldTimerManager().SetTimer(
		FadeOutTimerHandle,
		this,
		&ARAPlayerController::OpenPendingFadeTravelLevel,
		FMath::Max(KINDA_SMALL_NUMBER, SafeFadeOutDuration),
		false
	);
}

void ARAPlayerController::ReturnToTitleWithFade()
{
	if (TitleMapName.IsNone())
	{
		UE_LOG(LogTemp, Warning, TEXT("[GameFlow] Return to title skipped: TitleMapName is None."));
		return;
	}

	UE_LOG(LogTemp, Log, TEXT("[GameFlow] Return to title with fade. Target=%s"), *TitleMapName.ToString());
	TravelToLevelWithFade(TitleMapName);
}

void ARAPlayerController::QuitGame()
{
	UE_LOG(LogTemp, Log, TEXT("[GameFlow] Quit game requested from PlayerController."));
	UKismetSystemLibrary::QuitGame(this, this, EQuitPreference::Quit, false);
}

void ARAPlayerController::OpenPendingFadeTravelLevel()
{
	if (PendingFadeTravelLevelName.IsNone())
	{
		UE_LOG(LogTemp, Warning, TEXT("[GameFlow] Pending travel cancelled: target level is None."));
		SetPortalTransitionInputLocked(false);
		return;
	}

	if (URAGameInstance* RAGameInstance = Cast<URAGameInstance>(UGameplayStatics::GetGameInstance(this)))
	{
		RAGameInstance->bPendingPortalTransition = true;
	}

	const FName TargetLevelName = PendingFadeTravelLevelName;
	PendingFadeTravelLevelName = NAME_None;

	SetViewportFadeOverlayOpacity(1.0f);

	UE_LOG(LogTemp, Log, TEXT("[GameFlow] Opening level after fade-out: %s"), *TargetLevelName.ToString());
	UGameplayStatics::OpenLevel(this, TargetLevelName);
}

void ARAPlayerController::TryCreateGameFlowMenuWidget()
{
	const FString CurrentLevelName = UGameplayStatics::GetCurrentLevelName(this, true);
	TSubclassOf<UGameFlowMenuWidget> MenuWidgetClass = nullptr;

	if (IsTitleLevelName(CurrentLevelName))
	{
		MenuWidgetClass = TitleMenuWidgetClass;
	}
	else if (IsEndingLevelName(CurrentLevelName))
	{
		MenuWidgetClass = EndingMenuWidgetClass;
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

	ActiveGameFlowMenuWidget = CreateWidget<UGameFlowMenuWidget>(this, MenuWidgetClass);
	if (!ActiveGameFlowMenuWidget)
	{
		UE_LOG(LogTemp, Warning, TEXT("[GameFlow] Failed to create menu widget. Level=%s"), *CurrentLevelName);
		return;
	}

	ActiveGameFlowMenuWidget->AddToViewport(200);
	SetMenuInputMode();

	UE_LOG(LogTemp, Log, TEXT("[GameFlow] Menu widget displayed. Level=%s"), *CurrentLevelName);
}

bool ARAPlayerController::IsTitleLevelName(const FString& LevelName) const
{
	return LevelName == TitleMapName.ToString();
}

bool ARAPlayerController::IsEndingLevelName(const FString& LevelName) const
{
	return LevelName == EndingMapName.ToString();
}

bool ARAPlayerController::IsGameFlowMenuLevel() const
{
	const FString CurrentLevelName = UGameplayStatics::GetCurrentLevelName(this, true);
	return IsTitleLevelName(CurrentLevelName) || IsEndingLevelName(CurrentLevelName);
}

void ARAPlayerController::OpenShop(AShopActor* ShopActor)
{
	if (bIsPortalTransitionInputLocked)
	{
		return;
	}

	if (bIsShopOpen || !ShopActor)
		return;

	if (bIsSettingOpen)
	{
		CloseSetting();
	}

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
				&ARAPlayerController::CloseShop
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

void ARAPlayerController::ToggleAnimalCollection()
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

void ARAPlayerController::OpenAnimalCollection()
{
	if (bIsPortalTransitionInputLocked)
	{
		return;
	}

	if (bIsAnimalCollectionOpen)
	{
		return;
	}

	if (bIsSettingOpen)
	{
		CloseSetting();
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
				&ARAPlayerController::HandleAnimalCollectionCloseRequested
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

void ARAPlayerController::CloseAnimalCollection()
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

	if (bIsShopOpen || bIsInventoryOpen || bIsSettingOpen)
	{
		SetUIInputMode();
	}
	else
	{
		SetGameInputMode();
	}
}

void ARAPlayerController::CloseShop()
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

	if (bIsInventoryOpen || bIsAnimalCollectionOpen || bIsSettingOpen)
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

void ARAPlayerController::CloseUI()
{
	if (bIsPortalTransitionInputLocked)
	{
		return;
	}

	if (bIsSettingOpen)
	{
		CloseSetting();
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
		return;
	}

	OpenSetting();
}

void ARAPlayerController::ToggleSetting()
{
	if (bIsPortalTransitionInputLocked)
	{
		return;
	}

	if (bIsSettingOpen)
	{
		CloseSetting();
	}
	else
	{
		OpenSetting();
	}
}

void ARAPlayerController::OpenSetting()
{
	if (bIsPortalTransitionInputLocked || bIsSettingOpen)
	{
		return;
	}

	if (bIsShopOpen)
	{
		CloseShop();
	}

	if (bIsAnimalCollectionOpen)
	{
		CloseAnimalCollection();
	}

	if (bIsInventoryOpen)
	{
		CloseInventory();
	}

	if (!SettingWidget)
	{
		if (!SettingWidgetClass)
		{
			UE_LOG(LogTemp, Warning, TEXT("SettingWidgetClass is not assigned."));
			return;
		}

		SettingWidget = CreateWidget<USettingWidget>(this, SettingWidgetClass);

		if (SettingWidget)
		{
			SettingWidget->OnSettingCloseRequested.AddDynamic(
				this,
				&ARAPlayerController::HandleSettingCloseRequested
			);
		}
	}

	if (SettingWidget)
	{
		SettingWidget->AddToViewport(300);

		bIsSettingOpen = true;
		SetIgnoreMoveInput(true);
		SetIgnoreLookInput(true);
		SetSettingInputMode();
	}
}

void ARAPlayerController::CloseSetting()
{
	if (!bIsSettingOpen)
	{
		return;
	}

	if (SettingWidget)
	{
		SettingWidget->RemoveFromParent();
	}

	bIsSettingOpen = false;

	if (bIsPortalTransitionInputLocked)
	{
		return;
	}

	if (bIsShopOpen || bIsInventoryOpen || bIsAnimalCollectionOpen)
	{
		SetUIInputMode();
	}
	else if (IsGameFlowMenuLevel())
	{
		SetIgnoreMoveInput(true);
		SetIgnoreLookInput(true);
		SetMenuInputMode();
	}
	else
	{
		SetIgnoreMoveInput(false);
		SetIgnoreLookInput(false);
		SetGameInputMode();
	}
}
