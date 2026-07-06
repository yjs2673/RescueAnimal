#include "LobbyNPC.h"

#include "DialogueWidget.h"
#include "Components/BoxComponent.h"
#include "Components/SceneComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "TPSCaptureCharacter.h"
#include "TPSGameInstance.h"

ALobbyNPC::ALobbyNPC()
{
	PrimaryActorTick.bCanEverTick = false;

	SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
	SetRootComponent(SceneRoot);

	NPCMesh = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("NPCMesh"));
	NPCMesh->SetupAttachment(SceneRoot);

	InteractionBox = CreateDefaultSubobject<UBoxComponent>(TEXT("InteractionBox"));
	InteractionBox->SetupAttachment(SceneRoot);
	InteractionBox->SetBoxExtent(FVector(150.0f, 150.0f, 120.0f));
	InteractionBox->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	InteractionBox->SetCollisionResponseToAllChannels(ECR_Ignore);
	InteractionBox->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
}

ENPCDialogueState ALobbyNPC::GetCurrentDialogueState() const
{
	const UWorld* World = GetWorld();
	const UTPSGameInstance* TPSGameInstance = World
		? Cast<UTPSGameInstance>(World->GetGameInstance())
		: nullptr;

	if (!TPSGameInstance)
	{
		return ENPCDialogueState::Progress;
	}

	if (!TPSGameInstance->HasPlayedIntroDialogue())
	{
		return ENPCDialogueState::Intro;
	}

	if (TPSGameInstance->AreAllFieldMapsCleared() && !TPSGameInstance->IsEndingTriggered())
	{
		return ENPCDialogueState::Ending;
	}

	return ENPCDialogueState::Progress;
}

void ALobbyNPC::Interact()
{
	if (bIsIntroDialogueActive || bIsProgressDialogueActive || bIsEndingDialogueActive)
	{
		UE_LOG(LogTemp, Log, TEXT("[LobbyNPC] Interaction skipped: Dialogue is already active."));
		return;
	}

	switch (GetCurrentDialogueState())
	{
	case ENPCDialogueState::Intro:
		StartIntroDialogue();
		break;

	case ENPCDialogueState::Ending:
		StartEndingDialogue();
		break;

	case ENPCDialogueState::Progress:
	default:
		ShowProgressDialogue();
		break;
	}
}

void ALobbyNPC::StartIntroDialogue()
{
	if (bIsIntroDialogueActive)
	{
		UE_LOG(LogTemp, Log, TEXT("[LobbyNPC] Intro dialogue start skipped: Dialogue is already active."));
		return;
	}

	bIsIntroDialogueActive = true;
	DisablePlayerControlForDialogue();

	UE_LOG(LogTemp, Log, TEXT("[LobbyNPC] Intro Dialogue Started"));
	UE_LOG(LogTemp, Log, TEXT("[LobbyNPC] 안녕하세요. 이곳은 구조 작전의 거점입니다."));
	UE_LOG(LogTemp, Log, TEXT("[LobbyNPC] 각 섬에 갇힌 동물들을 구조해 주세요."));

	APlayerController* PlayerController = UGameplayStatics::GetPlayerController(this, 0);
	if (DialogueWidgetClass && PlayerController)
	{
		if (DialogueWidget)
		{
			DialogueWidget->RemoveFromParent();
		}

		DialogueWidget = CreateWidget<UDialogueWidget>(PlayerController, DialogueWidgetClass);
		if (DialogueWidget)
		{
			DialogueWidget->OnDialogueFinished.AddUniqueDynamic(this, &ALobbyNPC::OnIntroDialogueFinished);
			DialogueWidget->AddToViewport();

			TArray<FText> IntroDialogueLines;
			IntroDialogueLines.Add(FText::FromString(TEXT("안녕하세요. 이곳은 구조 작전의 거점입니다.")));
			IntroDialogueLines.Add(FText::FromString(TEXT("각 섬에 갇힌 동물들을 구조해 주세요.")));
			DialogueWidget->BeginDialogue(IntroDialogueLines);

			PlayerController->bShowMouseCursor = true;
			FInputModeGameAndUI InputMode;
			InputMode.SetWidgetToFocus(DialogueWidget->TakeWidget());
			InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
			PlayerController->SetInputMode(InputMode);

			UE_LOG(LogTemp, Log, TEXT("[LobbyNPC] Intro dialogue widget displayed."));
			return;
		}
	}

	UWorld* World = GetWorld();
	if (!World)
	{
		UE_LOG(LogTemp, Warning, TEXT("[LobbyNPC] Intro finish timer was not started: World is null."));
		bIsIntroDialogueActive = false;
		EnablePlayerControlAfterDialogue();
		return;
	}

	UE_LOG(LogTemp, Log, TEXT("[LobbyNPC] Dialogue widget is unavailable. Using intro fallback timer."));
	World->GetTimerManager().SetTimer(
		IntroDialogueFinishTimerHandle,
		this,
		&ALobbyNPC::OnIntroDialogueFinished,
		1.0f,
		false
	);
}

void ALobbyNPC::ShowProgressDialogue()
{
	if (bIsProgressDialogueActive)
	{
		return;
	}

	bIsProgressDialogueActive = true;
	DisablePlayerControlForDialogue();

	UE_LOG(LogTemp, Log, TEXT("[LobbyNPC] Progress Dialogue Started"));

	const FString ProgressText = BuildProgressText();
	UE_LOG(LogTemp, Log, TEXT("[LobbyNPC]\n%s"), *ProgressText);

	APlayerController* PlayerController = UGameplayStatics::GetPlayerController(this, 0);
	if (DialogueWidgetClass && PlayerController)
	{
		if (DialogueWidget)
		{
			DialogueWidget->OnDialogueFinished.RemoveAll(this);
			DialogueWidget->RemoveFromParent();
		}

		DialogueWidget = CreateWidget<UDialogueWidget>(PlayerController, DialogueWidgetClass);
		if (DialogueWidget)
		{
			DialogueWidget->OnDialogueFinished.AddUniqueDynamic(this, &ALobbyNPC::OnProgressDialogueFinished);
			DialogueWidget->AddToViewport();

			TArray<FString> ProgressStringLines;
			ProgressText.ParseIntoArrayLines(ProgressStringLines, true);

			TArray<FText> ProgressDialogueLines;
			ProgressDialogueLines.Reserve(ProgressStringLines.Num());
			for (const FString& ProgressLine : ProgressStringLines)
			{
				ProgressDialogueLines.Add(FText::FromString(ProgressLine));
			}

			DialogueWidget->BeginDialogue(ProgressDialogueLines);

			PlayerController->bShowMouseCursor = true;
			FInputModeGameAndUI InputMode;
			InputMode.SetWidgetToFocus(DialogueWidget->TakeWidget());
			InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
			PlayerController->SetInputMode(InputMode);

			UE_LOG(LogTemp, Log, TEXT("[LobbyNPC] Progress dialogue widget displayed."));
			return;
		}
	}

	UE_LOG(LogTemp, Log, TEXT("[LobbyNPC] Dialogue widget is unavailable. Progress was shown by log only."));
	OnProgressDialogueFinished();
}

void ALobbyNPC::StartEndingDialogue()
{
	if (bIsEndingDialogueActive)
	{
		UE_LOG(LogTemp, Log, TEXT("[LobbyNPC] Ending dialogue start skipped: Dialogue is already active."));
		return;
	}

	UTPSGameInstance* TPSGameInstance = Cast<UTPSGameInstance>(
		UGameplayStatics::GetGameInstance(this)
	);

	if (!TPSGameInstance)
	{
		UE_LOG(LogTemp, Warning, TEXT("[LobbyNPC] Ending dialogue skipped: TPSGameInstance is unavailable."));
		return;
	}

	if (TPSGameInstance->IsEndingTriggered())
	{
		UE_LOG(LogTemp, Log, TEXT("[LobbyNPC] Ending dialogue skipped: Ending was already triggered."));
		return;
	}

	if (!TPSGameInstance->AreAllFieldMapsCleared())
	{
		UE_LOG(LogTemp, Warning, TEXT("[LobbyNPC] Ending dialogue skipped: Not all field maps are cleared."));
		return;
	}

	bIsEndingDialogueActive = true;
	DisablePlayerControlForDialogue();

	UE_LOG(LogTemp, Log, TEXT("[LobbyNPC] Ending Dialogue Started"));
	UE_LOG(LogTemp, Log, TEXT("[LobbyNPC] 모든 지역의 구조가 완료되었습니다."));
	UE_LOG(LogTemp, Log, TEXT("[LobbyNPC] 정말 해냈군요."));
	UE_LOG(LogTemp, Log, TEXT("[LobbyNPC] 덕분에 모든 동물들이 자유를 되찾았습니다."));
	UE_LOG(LogTemp, Log, TEXT("[LobbyNPC] 이제 구조 작전은 끝났습니다."));

	APlayerController* PlayerController = UGameplayStatics::GetPlayerController(this, 0);
	if (DialogueWidgetClass && PlayerController)
	{
		if (DialogueWidget)
		{
			DialogueWidget->OnDialogueFinished.RemoveAll(this);
			DialogueWidget->RemoveFromParent();
		}

		DialogueWidget = CreateWidget<UDialogueWidget>(PlayerController, DialogueWidgetClass);
		if (DialogueWidget)
		{
			DialogueWidget->OnDialogueFinished.AddUniqueDynamic(this, &ALobbyNPC::OnEndingDialogueFinished);
			DialogueWidget->AddToViewport();

			TArray<FText> EndingDialogueLines;
			EndingDialogueLines.Add(FText::FromString(TEXT("모든 지역의 구조가 완료되었습니다.")));
			EndingDialogueLines.Add(FText::FromString(TEXT("정말 해냈군요.")));
			EndingDialogueLines.Add(FText::FromString(TEXT("덕분에 모든 동물들이 자유를 되찾았습니다.")));
			EndingDialogueLines.Add(FText::FromString(TEXT("이제 구조 작전은 끝났습니다.")));
			DialogueWidget->BeginDialogue(EndingDialogueLines);

			PlayerController->bShowMouseCursor = true;
			FInputModeGameAndUI InputMode;
			InputMode.SetWidgetToFocus(DialogueWidget->TakeWidget());
			InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
			PlayerController->SetInputMode(InputMode);

			UE_LOG(LogTemp, Log, TEXT("[LobbyNPC] Ending dialogue widget displayed."));
			return;
		}
	}
	else if (!DialogueWidgetClass)
	{
		UE_LOG(LogTemp, Log, TEXT("[LobbyNPC] DialogueWidgetClass is not assigned. Ending dialogue was shown by log only."));
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("[LobbyNPC] Ending dialogue widget was not created: PlayerController is unavailable."));
	}

	UWorld* World = GetWorld();
	if (!World)
	{
		UE_LOG(LogTemp, Warning, TEXT("[LobbyNPC] Ending finish timer was not started: World is null."));
		bIsEndingDialogueActive = false;
		if (DialogueWidget)
		{
			DialogueWidget->RemoveFromParent();
			DialogueWidget = nullptr;
		}
		EnablePlayerControlAfterDialogue();
		return;
	}

	World->GetTimerManager().SetTimer(
		EndingDialogueFinishTimerHandle,
		this,
		&ALobbyNPC::OnEndingDialogueFinished,
		1.5f,
		false
	);
}

void ALobbyNPC::OnEndingDialogueFinished()
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(EndingDialogueFinishTimerHandle);
	}

	if (!bIsEndingDialogueActive)
	{
		UE_LOG(LogTemp, Log, TEXT("[LobbyNPC] Ending finish ignored: Ending dialogue is not active."));
		return;
	}

	bIsEndingDialogueActive = false;

	if (DialogueWidget)
	{
		DialogueWidget->OnDialogueFinished.RemoveAll(this);
		DialogueWidget->RemoveFromParent();
		DialogueWidget = nullptr;
	}

	UTPSGameInstance* TPSGameInstance = Cast<UTPSGameInstance>(
		UGameplayStatics::GetGameInstance(this)
	);

	if (!TPSGameInstance)
	{
		UE_LOG(LogTemp, Warning, TEXT("[LobbyNPC] Ending was not completed: TPSGameInstance is unavailable."));
		EnablePlayerControlAfterDialogue();
		return;
	}

	if (TPSGameInstance->IsEndingTriggered())
	{
		UE_LOG(LogTemp, Log, TEXT("[LobbyNPC] Ending finish ignored: Ending was already triggered."));
		EnablePlayerControlAfterDialogue();
		return;
	}

	if (EndingMapName.IsNone())
	{
		UE_LOG(LogTemp, Warning, TEXT("[LobbyNPC] Ending level travel skipped: EndingMapName is None."));
		EnablePlayerControlAfterDialogue();
		return;
	}

	TPSGameInstance->SetEndingTriggered(true);
	UE_LOG(LogTemp, Log, TEXT("[LobbyNPC] Ending Dialogue Finished. Opening level: %s"), *EndingMapName.ToString());
	UGameplayStatics::OpenLevel(this, EndingMapName);
}

FString ALobbyNPC::BuildProgressText() const
{
	const UTPSGameInstance* TPSGameInstance = Cast<UTPSGameInstance>(
		UGameplayStatics::GetGameInstance(this)
	);

	if (!TPSGameInstance)
	{
		return TEXT("현재 구조 진행 상황을 불러올 수 없습니다.");
	}

	const FString PlainStatus = TPSGameInstance->IsMapCleared(TEXT("MAP_Plain"))
		? TEXT("클리어 완료")
		: TEXT("진행 중");
	const FString SnowStatus = TPSGameInstance->IsMapCleared(TEXT("MAP_Snow"))
		? TEXT("클리어 완료")
		: TEXT("진행 중");
	const FString DesertStatus = TPSGameInstance->IsMapCleared(TEXT("MAP_Desert"))
		? TEXT("클리어 완료")
		: TEXT("진행 중");

	return FString::Printf(
		TEXT("현재 구조 진행 상황입니다.\n초원 섬: %s\n설원 섬: %s\n사막 섬: %s"),
		*PlainStatus,
		*SnowStatus,
		*DesertStatus
	);
}

void ALobbyNPC::BeginPlay()
{
	Super::BeginPlay();

	if (InteractionBox)
	{
		InteractionBox->OnComponentBeginOverlap.AddDynamic(this, &ALobbyNPC::OnInteractionBeginOverlap);
		InteractionBox->OnComponentEndOverlap.AddDynamic(this, &ALobbyNPC::OnInteractionEndOverlap);
	}

	UWorld* World = GetWorld();
	if (!World)
	{
		UE_LOG(LogTemp, Warning, TEXT("[LobbyNPC] Auto intro timer was not started: World is null."));
		return;
	}

	World->GetTimerManager().SetTimer(
		AutoIntroTimerHandle,
		this,
		&ALobbyNPC::TryAutoStartIntroDialogue,
		FMath::Max(KINDA_SMALL_NUMBER, AutoIntroDelay),
		false
	);
}

void ALobbyNPC::TryAutoStartIntroDialogue()
{
	if (bIsIntroDialogueActive)
	{
		UE_LOG(LogTemp, Log, TEXT("[LobbyNPC] Auto intro skipped: Intro dialogue is already active."));
		return;
	}

	UTPSGameInstance* TPSGameInstance = Cast<UTPSGameInstance>(
		UGameplayStatics::GetGameInstance(this)
	);

	if (!TPSGameInstance)
	{
		UE_LOG(LogTemp, Warning, TEXT("[LobbyNPC] Auto intro skipped: TPSGameInstance is unavailable."));
		return;
	}

	if (TPSGameInstance->HasPlayedIntroDialogue())
	{
		UE_LOG(LogTemp, Log, TEXT("[LobbyNPC] Auto intro skipped: Intro dialogue was already completed."));
		return;
	}

	UE_LOG(LogTemp, Log, TEXT("[LobbyNPC] Auto intro condition met. Starting intro dialogue."));
	StartIntroDialogue();
}

void ALobbyNPC::OnIntroDialogueFinished()
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(IntroDialogueFinishTimerHandle);
	}

	bIsIntroDialogueActive = false;

	if (DialogueWidget)
	{
		DialogueWidget->OnDialogueFinished.RemoveAll(this);
		DialogueWidget->RemoveFromParent();
		DialogueWidget = nullptr;
	}

	UTPSGameInstance* TPSGameInstance = Cast<UTPSGameInstance>(
		UGameplayStatics::GetGameInstance(this)
	);

	if (TPSGameInstance)
	{
		TPSGameInstance->SetHasPlayedIntroDialogue(true);
		TPSGameInstance->SetGameStarted(true);
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("[LobbyNPC] Intro state was not saved: TPSGameInstance is unavailable."));
	}

	EnablePlayerControlAfterDialogue();
	UE_LOG(LogTemp, Log, TEXT("[LobbyNPC] Intro Dialogue Finished"));
}

void ALobbyNPC::OnProgressDialogueFinished()
{
	bIsProgressDialogueActive = false;

	if (DialogueWidget)
	{
		DialogueWidget->OnDialogueFinished.RemoveAll(this);
		DialogueWidget->RemoveFromParent();
		DialogueWidget = nullptr;
	}

	EnablePlayerControlAfterDialogue();
	UE_LOG(LogTemp, Log, TEXT("[LobbyNPC] Progress Dialogue Finished"));
}

void ALobbyNPC::DisablePlayerControlForDialogue()
{
	APlayerController* PlayerController = UGameplayStatics::GetPlayerController(this, 0);
	if (!PlayerController)
	{
		UE_LOG(LogTemp, Warning, TEXT("[LobbyNPC] Dialogue input lock skipped: PlayerController is unavailable."));
		return;
	}

	PlayerController->SetIgnoreMoveInput(true);
	PlayerController->SetIgnoreLookInput(true);

	UE_LOG(LogTemp, Log, TEXT("[LobbyNPC] Player movement and look input disabled for dialogue."));
}

void ALobbyNPC::EnablePlayerControlAfterDialogue()
{
	APlayerController* PlayerController = UGameplayStatics::GetPlayerController(this, 0);
	if (!PlayerController)
	{
		UE_LOG(LogTemp, Warning, TEXT("[LobbyNPC] Dialogue input restore skipped: PlayerController is unavailable."));
		return;
	}

	PlayerController->SetIgnoreMoveInput(false);
	PlayerController->SetIgnoreLookInput(false);
	PlayerController->bShowMouseCursor = false;

	FInputModeGameOnly InputMode;
	PlayerController->SetInputMode(InputMode);

	UE_LOG(LogTemp, Log, TEXT("[LobbyNPC] Player input restored after dialogue."));
}

void ALobbyNPC::OnInteractionBeginOverlap(
	UPrimitiveComponent* OverlappedComponent,
	AActor* OtherActor,
	UPrimitiveComponent* OtherComp,
	int32 OtherBodyIndex,
	bool bFromSweep,
	const FHitResult& SweepResult)
{
	ATPSCaptureCharacter* PlayerCharacter = Cast<ATPSCaptureCharacter>(OtherActor);
	if (!PlayerCharacter)
	{
		return;
	}

	PlayerCharacter->SetCurrentLobbyNPC(this);
}

void ALobbyNPC::OnInteractionEndOverlap(
	UPrimitiveComponent* OverlappedComponent,
	AActor* OtherActor,
	UPrimitiveComponent* OtherComp,
	int32 OtherBodyIndex)
{
	ATPSCaptureCharacter* PlayerCharacter = Cast<ATPSCaptureCharacter>(OtherActor);
	if (!PlayerCharacter)
	{
		return;
	}

	PlayerCharacter->ClearCurrentLobbyNPC(this);
}

