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
	if (bIsIntroDialogueActive || bIsChoiceMenuActive || bIsProgressDialogueActive || bIsEndingDialogueActive)
	{
		UE_LOG(LogTemp, Log, TEXT("[LobbyNPC] Interaction skipped: Dialogue is already active."));
		return;
	}

	UTPSGameInstance* TPSGameInstance = Cast<UTPSGameInstance>(
		UGameplayStatics::GetGameInstance(this)
	);

	if (!TPSGameInstance)
	{
		UE_LOG(LogTemp, Warning, TEXT("[LobbyNPC] Interaction skipped: TPSGameInstance is unavailable."));
		return;
	}

	if (!TPSGameInstance->HasPlayedIntroDialogue())
	{
		StartIntroDialogue();
		return;
	}

	ShowDialogueChoices();
}

void ALobbyNPC::ShowDialogueChoices()
{
	if (bIsChoiceMenuActive)
	{
		return;
	}

	APlayerController* PlayerController = UGameplayStatics::GetPlayerController(this, 0);
	if (!DialogueWidgetClass || !PlayerController)
	{
		UE_LOG(
			LogTemp,
			Warning,
			TEXT("[LobbyNPC] Choice menu could not open: DialogueWidgetClass or PlayerController is unavailable.")
		);
		return;
	}

	if (DialogueWidget)
	{
		DialogueWidget->OnDialogueFinished.RemoveAll(this);
		DialogueWidget->OnDialogueChoiceSelected.RemoveAll(this);
		DialogueWidget->RemoveFromParent();
		DialogueWidget = nullptr;
	}

	DialogueWidget = CreateWidget<UDialogueWidget>(PlayerController, DialogueWidgetClass);
	if (!DialogueWidget)
	{
		UE_LOG(LogTemp, Warning, TEXT("[LobbyNPC] Choice menu widget creation failed."));
		return;
	}

	bIsChoiceMenuActive = true;
	DisablePlayerControlForDialogue();

	DialogueWidget->OnDialogueChoiceSelected.AddUniqueDynamic(
		this,
		&ALobbyNPC::HandleDialogueChoiceSelected
	);
	DialogueWidget->OnDialogueFinished.AddUniqueDynamic(
		this,
		&ALobbyNPC::OnDialogueChoiceMenuClosed
	);
	DialogueWidget->AddToViewport();

	const bool bChoicesDisplayed = DialogueWidget->ShowChoices(
		FText::FromString(TEXT("무슨 일인가요?")),
		FText::FromString(TEXT("다시 설명")),
		FText::FromString(TEXT("진행 상황")),
		FText::FromString(TEXT("엔딩 확인"))
	);

	if (!bChoicesDisplayed)
	{
		UE_LOG(LogTemp, Warning, TEXT("[LobbyNPC] Choice menu closed because its buttons are not bound."));
		OnDialogueChoiceMenuClosed();
		return;
	}

	PlayerController->bShowMouseCursor = true;
	FInputModeUIOnly InputMode;
	InputMode.SetWidgetToFocus(DialogueWidget->TakeWidget());
	InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
	PlayerController->SetInputMode(InputMode);

	UE_LOG(LogTemp, Log, TEXT("[LobbyNPC] Dialogue choice menu displayed."));
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

	TArray<FText> IntroDialogueLines;
	if (bRepeatTutorialRequested)
	{
		IntroDialogueLines.Add(FText::FromString(TEXT("다시 설명드리겠습니다.")));
	}
	else
	{
		IntroDialogueLines.Add(FText::FromString(TEXT("안녕하세요. 이곳은 구조 작전의 거점입니다.")));
	}

	IntroDialogueLines.Add(FText::FromString(
		TEXT("각 섬에 갇힌 동물들을 모두 구조하는 것이 이번 작전의 목표입니다.")
	));
	IntroDialogueLines.Add(FText::FromString(
		TEXT("정면의 포탈을 통해 섬으로 이동하실 수 있습니다.")
	));
	IntroDialogueLines.Add(FText::FromString(
		TEXT("각 섬에는 납포 조직의 캠프들이 있으며, 캠프에 동물들이 붙잡혀있습니다.")
	));
	IntroDialogueLines.Add(FText::FromString(
		TEXT("캠프의 모든 조직원들을 처치하고, 구조 키트를 통해 동물들을 구조하실 수 있습니다.")
	));
	IntroDialogueLines.Add(FText::FromString(
		TEXT("구조 키트는 인벤토리에 지급해드리겠습니다.")
	));
	IntroDialogueLines.Add(FText::FromString(
		TEXT("정면을 기준으로 왼쪽이 평원 섬, 가운데가 설원 섬, 오른쪽이 사막 섬입니다.")
	));
	IntroDialogueLines.Add(FText::FromString(
		TEXT("섬마다 조직원들의 체력과 공격력이 다릅니다. 왼쪽부터 차례대로 진행하시는걸 추천드립니다.")
	));
	IntroDialogueLines.Add(FText::FromString(
		TEXT("이동은 W, A, S, D 키, 시점 조작은 마우스를 사용합니다.")
	));
	IntroDialogueLines.Add(FText::FromString(
		TEXT("Space Bar로 점프하고, Left Shift로 회피할 수 있습니다.")
	));
	IntroDialogueLines.Add(FText::FromString(
		TEXT("마우스 왼쪽 버튼으로 공격하고, E 키로 상호작용하실 수 있습니다.")
	));
	IntroDialogueLines.Add(FText::FromString(
		TEXT("I 키로 인벤토리를 열 수 있고, C 키로 구조한 동물들의 컬렉션을 확인할 수 있습니다.")
	));
	IntroDialogueLines.Add(FText::FromString(
		TEXT("인벤토리에서 드래그를 통해 아이템을 단축키에 등록 후 숫자 키를 통해 사용하실 수 있습니다.")
	));
	IntroDialogueLines.Add(FText::FromString(
		TEXT("거점의 상점 NPC를 통해 무기와 아이템을 구매하실 수 있습니다.")
	));
	IntroDialogueLines.Add(FText::FromString(
		TEXT("준비가 끝나면 포탈을 통해 구조 지역으로 이동해 주세요.")
	));

	UE_LOG(LogTemp, Log, TEXT("[LobbyNPC] Intro Dialogue Started"));
	for (const FText& DialogueLine : IntroDialogueLines)
	{
		UE_LOG(LogTemp, Log, TEXT("[LobbyNPC] %s"), *DialogueLine.ToString());
	}

	APlayerController* PlayerController = UGameplayStatics::GetPlayerController(this, 0);
	if (DialogueWidgetClass && PlayerController)
	{
		if (DialogueWidget)
		{
			DialogueWidget->OnDialogueFinished.RemoveAll(this);
			DialogueWidget->OnDialogueChoiceSelected.RemoveAll(this);
			DialogueWidget->RemoveFromParent();
		}

		DialogueWidget = CreateWidget<UDialogueWidget>(PlayerController, DialogueWidgetClass);
		if (DialogueWidget)
		{
			DialogueWidget->OnDialogueFinished.AddUniqueDynamic(this, &ALobbyNPC::OnIntroDialogueFinished);
			DialogueWidget->AddToViewport();
			DialogueWidget->BeginDialogue(IntroDialogueLines);

			PlayerController->bShowMouseCursor = true;
			FInputModeUIOnly InputMode;
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
		bRepeatTutorialRequested = false;
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

	const FString ProgressText = BuildProgressText();
	UE_LOG(LogTemp, Log, TEXT("[LobbyNPC] Progress Dialogue Started"));
	UE_LOG(LogTemp, Log, TEXT("[LobbyNPC]\n%s"), *ProgressText);

	APlayerController* PlayerController = UGameplayStatics::GetPlayerController(this, 0);
	if (DialogueWidgetClass && PlayerController)
	{
		if (DialogueWidget)
		{
			DialogueWidget->OnDialogueFinished.RemoveAll(this);
			DialogueWidget->OnDialogueChoiceSelected.RemoveAll(this);
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
			FInputModeUIOnly InputMode;
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

void ALobbyNPC::ShowEndingLockedDialogue()
{
	if (bIsProgressDialogueActive)
	{
		return;
	}

	bIsProgressDialogueActive = true;
	DisablePlayerControlForDialogue();

	const FText EndingLockedText = FText::FromString(TEXT("아직 모든 섬이 클리어되지 않았습니다. 모든 동물을 구조해주세요."));
	UE_LOG(LogTemp, Log, TEXT("[LobbyNPC] Ending condition is not met."));
	UE_LOG(LogTemp, Log, TEXT("[LobbyNPC] %s"), *EndingLockedText.ToString());

	APlayerController* PlayerController = UGameplayStatics::GetPlayerController(this, 0);
	if (DialogueWidgetClass && PlayerController)
	{
		if (DialogueWidget)
		{
			DialogueWidget->OnDialogueFinished.RemoveAll(this);
			DialogueWidget->OnDialogueChoiceSelected.RemoveAll(this);
			DialogueWidget->RemoveFromParent();
		}

		DialogueWidget = CreateWidget<UDialogueWidget>(PlayerController, DialogueWidgetClass);
		if (DialogueWidget)
		{
			DialogueWidget->OnDialogueFinished.AddUniqueDynamic(this, &ALobbyNPC::OnProgressDialogueFinished);
			DialogueWidget->AddToViewport();

			TArray<FText> EndingLockedDialogueLines;
			EndingLockedDialogueLines.Add(EndingLockedText);
			DialogueWidget->BeginDialogue(EndingLockedDialogueLines);

			PlayerController->bShowMouseCursor = true;
			FInputModeUIOnly InputMode;
			InputMode.SetWidgetToFocus(DialogueWidget->TakeWidget());
			InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
			PlayerController->SetInputMode(InputMode);
			return;
		}
	}

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
		ShowEndingLockedDialogue();
		return;
	}

	bIsEndingDialogueActive = true;
	DisablePlayerControlForDialogue();

	TArray<FText> EndingDialogueLines;
	EndingDialogueLines.Add(FText::FromString(TEXT("3개의 섬을 모두 클리어하셨군요!")));
	EndingDialogueLines.Add(FText::FromString(
		TEXT("모든 동물들이 자유를 되찾았습니다.")
	));
	EndingDialogueLines.Add(FText::FromString(TEXT("구조 작전이 끝났으니 이곳에서 철수하겠습니다. 돌아가시죠!")));

	UE_LOG(LogTemp, Log, TEXT("[LobbyNPC] Ending Dialogue Started"));
	for (const FText& DialogueLine : EndingDialogueLines)
	{
		UE_LOG(LogTemp, Log, TEXT("[LobbyNPC] %s"), *DialogueLine.ToString());
	}

	APlayerController* PlayerController = UGameplayStatics::GetPlayerController(this, 0);
	if (DialogueWidgetClass && PlayerController)
	{
		if (DialogueWidget)
		{
			DialogueWidget->OnDialogueFinished.RemoveAll(this);
			DialogueWidget->OnDialogueChoiceSelected.RemoveAll(this);
			DialogueWidget->RemoveFromParent();
		}

		DialogueWidget = CreateWidget<UDialogueWidget>(PlayerController, DialogueWidgetClass);
		if (DialogueWidget)
		{
			DialogueWidget->OnDialogueFinished.AddUniqueDynamic(this, &ALobbyNPC::OnEndingDialogueFinished);
			DialogueWidget->AddToViewport();
			DialogueWidget->BeginDialogue(EndingDialogueLines);

			PlayerController->bShowMouseCursor = true;
			FInputModeUIOnly InputMode;
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
		DialogueWidget->OnDialogueChoiceSelected.RemoveAll(this);
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

	if (!bIsIntroDialogueActive)
	{
		return;
	}

	bIsIntroDialogueActive = false;
	bRepeatTutorialRequested = false;

	if (DialogueWidget)
	{
		DialogueWidget->OnDialogueFinished.RemoveAll(this);
		DialogueWidget->OnDialogueChoiceSelected.RemoveAll(this);
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
	if (!bIsProgressDialogueActive)
	{
		return;
	}

	bIsProgressDialogueActive = false;

	if (DialogueWidget)
	{
		DialogueWidget->OnDialogueFinished.RemoveAll(this);
		DialogueWidget->OnDialogueChoiceSelected.RemoveAll(this);
		DialogueWidget->RemoveFromParent();
		DialogueWidget = nullptr;
	}

	EnablePlayerControlAfterDialogue();
	UE_LOG(LogTemp, Log, TEXT("[LobbyNPC] Progress Dialogue Finished"));
}

void ALobbyNPC::HandleDialogueChoiceSelected(EDialogueChoice SelectedChoice)
{
	if (!bIsChoiceMenuActive)
	{
		return;
	}

	if (DialogueWidget)
	{
		DialogueWidget->OnDialogueFinished.RemoveAll(this);
		DialogueWidget->OnDialogueChoiceSelected.RemoveAll(this);
		DialogueWidget->RemoveFromParent();
		DialogueWidget = nullptr;
	}

	bIsChoiceMenuActive = false;
	EnablePlayerControlAfterDialogue();

	switch (SelectedChoice)
	{
	case EDialogueChoice::Tutorial:
		UE_LOG(LogTemp, Log, TEXT("[LobbyNPC] Tutorial choice selected."));
		bRepeatTutorialRequested = true;
		StartIntroDialogue();
		break;

	case EDialogueChoice::Progress:
		UE_LOG(LogTemp, Log, TEXT("[LobbyNPC] Progress choice selected."));
		ShowProgressDialogue();
		break;

	case EDialogueChoice::Ending:
	default:
		UE_LOG(LogTemp, Log, TEXT("[LobbyNPC] Ending choice selected."));

		if (const UTPSGameInstance* TPSGameInstance = Cast<UTPSGameInstance>(
			UGameplayStatics::GetGameInstance(this)))
		{
			if (TPSGameInstance->AreAllFieldMapsCleared() && !TPSGameInstance->IsEndingTriggered())
			{
				StartEndingDialogue();
			}
			else if (!TPSGameInstance->AreAllFieldMapsCleared())
			{
				ShowEndingLockedDialogue();
			}
			else
			{
				UE_LOG(LogTemp, Log, TEXT("[LobbyNPC] Ending choice ignored: Ending was already triggered."));
			}
		}
		else
		{
			UE_LOG(LogTemp, Warning, TEXT("[LobbyNPC] Ending choice failed: TPSGameInstance is unavailable."));
		}
		break;
	}
}

void ALobbyNPC::OnDialogueChoiceMenuClosed()
{
	if (!bIsChoiceMenuActive)
	{
		return;
	}

	bIsChoiceMenuActive = false;

	if (DialogueWidget)
	{
		DialogueWidget->OnDialogueFinished.RemoveAll(this);
		DialogueWidget->OnDialogueChoiceSelected.RemoveAll(this);
		DialogueWidget->RemoveFromParent();
		DialogueWidget = nullptr;
	}

	EnablePlayerControlAfterDialogue();
	UE_LOG(LogTemp, Log, TEXT("[LobbyNPC] Dialogue choice menu closed."));
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
