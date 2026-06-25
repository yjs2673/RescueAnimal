#include "LobbyNPC.h"

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

	UWorld* World = GetWorld();
	if (!World)
	{
		UE_LOG(LogTemp, Warning, TEXT("[LobbyNPC] Intro finish timer was not started: World is null."));
		bIsIntroDialogueActive = false;
		EnablePlayerControlAfterDialogue();
		return;
	}

	// TODO: WBP_Dialogue가 연결되면 임시 타이머 대신 대화 UI 종료 콜백에서
	// OnIntroDialogueFinished()를 호출하도록 교체한다.
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
	UE_LOG(LogTemp, Log, TEXT("[LobbyNPC] Progress Dialogue Started"));

	const UWorld* World = GetWorld();
	const UTPSGameInstance* TPSGameInstance = World
		? Cast<UTPSGameInstance>(World->GetGameInstance())
		: nullptr;

	if (!TPSGameInstance)
	{
		UE_LOG(LogTemp, Warning, TEXT("[LobbyNPC] GameInstance is unavailable. Map clear states cannot be read."));
	}
	else
	{
		UE_LOG(LogTemp, Log, TEXT("[LobbyNPC] Plain Cleared: %s"),
			TPSGameInstance->IsMapCleared(TEXT("Plain")) ? TEXT("true") : TEXT("false"));
		UE_LOG(LogTemp, Log, TEXT("[LobbyNPC] Snow Cleared: %s"),
			TPSGameInstance->IsMapCleared(TEXT("Snow")) ? TEXT("true") : TEXT("false"));
		UE_LOG(LogTemp, Log, TEXT("[LobbyNPC] Desert Cleared: %s"),
			TPSGameInstance->IsMapCleared(TEXT("Desert")) ? TEXT("true") : TEXT("false"));
	}

	UE_LOG(LogTemp, Log, TEXT("[LobbyNPC] TODO: Remaining animal count function is not implemented."));
}

void ALobbyNPC::StartEndingDialogue()
{
	UE_LOG(LogTemp, Log, TEXT("[LobbyNPC] Ending Dialogue Started"));
	UE_LOG(LogTemp, Log, TEXT("[LobbyNPC] 정말 해냈군요. 모든 동물들이 자유를 되찾았습니다."));
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

