#include "PortalActor.h"
#include "Components/BoxComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Kismet/GameplayStatics.h"
#include "TPSCaptureCharacter.h"

APortalActor::APortalActor()
{
	PrimaryActorTick.bCanEverTick = false;

	Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	SetRootComponent(Root);

	PortalMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("PortalMesh"));
	PortalMesh->SetupAttachment(Root);
	PortalMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	TriggerBox = CreateDefaultSubobject<UBoxComponent>(TEXT("TriggerBox"));
	TriggerBox->SetupAttachment(Root);
	TriggerBox->SetBoxExtent(FVector(100.f, 100.f, 150.f));
	TriggerBox->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	TriggerBox->SetCollisionResponseToAllChannels(ECR_Ignore);
	TriggerBox->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
}

void APortalActor::BeginPlay()
{
	Super::BeginPlay();

	if (TriggerBox)
	{
		TriggerBox->OnComponentBeginOverlap.AddDynamic(this, &APortalActor::OnOverlapBegin);
		TriggerBox->OnComponentEndOverlap.AddDynamic(this, &APortalActor::OnOverlapEnd);
	}
}

void APortalActor::OnOverlapBegin(
	UPrimitiveComponent* OverlappedComponent,
	AActor* OtherActor,
	UPrimitiveComponent* OtherComp,
	int32 OtherBodyIndex,
	bool bFromSweep,
	const FHitResult& SweepResult)
{
	if (!OtherActor)
	{
		return;
	}

	ATPSCaptureCharacter* PlayerCharacter = Cast<ATPSCaptureCharacter>(OtherActor);
	if (!PlayerCharacter)
	{
		return;
	}

	PlayerCharacter->SetCurrentPortal(this);
	UE_LOG(LogTemp, Warning, TEXT("Entered Portal Range"));
}

void APortalActor::OnOverlapEnd(
	UPrimitiveComponent* OverlappedComponent,
	AActor* OtherActor,
	UPrimitiveComponent* OtherComp,
	int32 OtherBodyIndex)
{
	if (!OtherActor)
	{
		return;
	}

	ATPSCaptureCharacter* PlayerCharacter = Cast<ATPSCaptureCharacter>(OtherActor);
	if (!PlayerCharacter)
	{
		return;
	}

	PlayerCharacter->ClearCurrentPortal(this);
	UE_LOG(LogTemp, Warning, TEXT("Exited Portal Range"));
}

void APortalActor::Interact(AActor* InteractingActor)
{
	if (!InteractingActor || bIsTeleporting)
	{
		return;
	}

	TeleportPlayer(InteractingActor);
}

void APortalActor::TeleportPlayer(AActor* OverlappingActor)
{
	if (DestinationLevelName.IsNone())
	{
		UE_LOG(LogTemp, Warning, TEXT("PortalActor: DestinationLevelName is None."));
		return;
	}

	if (bOneShotTeleport)
	{
		bIsTeleporting = true;
	}

	UE_LOG(LogTemp, Warning, TEXT("Teleporting to level: %s"), *DestinationLevelName.ToString());
	UGameplayStatics::OpenLevel(this, DestinationLevelName);
}