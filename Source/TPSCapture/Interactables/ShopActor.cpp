#include "ShopActor.h"
#include "Components/BoxComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "TPSCaptureCharacter.h"
#include "TPSPlayerController.h"

AShopActor::AShopActor()
{
	PrimaryActorTick.bCanEverTick = false;

	SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
	SetRootComponent(SceneRoot);

	NPCMesh = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("NPCMesh"));
	NPCMesh->SetupAttachment(SceneRoot);

	DisplayMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("DisplayMesh"));
	DisplayMesh->SetupAttachment(SceneRoot);

	InteractionBox = CreateDefaultSubobject<UBoxComponent>(TEXT("InteractionBox"));
	InteractionBox->SetupAttachment(SceneRoot);
	InteractionBox->SetBoxExtent(FVector(150.f, 150.f, 120.f));
	InteractionBox->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	InteractionBox->SetCollisionResponseToAllChannels(ECR_Ignore);
	InteractionBox->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
}

void AShopActor::BeginPlay()
{
	Super::BeginPlay();

	if (InteractionBox)
	{
		InteractionBox->OnComponentBeginOverlap.AddDynamic(this, &AShopActor::OnInteractionBeginOverlap);
		InteractionBox->OnComponentEndOverlap.AddDynamic(this, &AShopActor::OnInteractionEndOverlap);
	}
}

void AShopActor::OnInteractionBeginOverlap(
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

	PlayerCharacter->SetCurrentShop(this);
}

void AShopActor::OnInteractionEndOverlap(
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

	PlayerCharacter->ClearCurrentShop(this);
}

void AShopActor::Interact(AActor* InteractingActor)
{
	ATPSCaptureCharacter* PlayerCharacter = Cast<ATPSCaptureCharacter>(InteractingActor);
	if (!PlayerCharacter)
	{
		return;
	}

	ATPSPlayerController* PlayerController = Cast<ATPSPlayerController>(PlayerCharacter->GetController());
	if (!PlayerController)
	{
		return;
	}

	PlayerController->OpenShop(this);
}