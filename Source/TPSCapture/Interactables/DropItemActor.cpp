#include "DropItemActor.h"

#include "Components/SceneComponent.h"
#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "InventoryComponent.h"

ADropItemActor::ADropItemActor()
{
	PrimaryActorTick.bCanEverTick = false;

	SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
	SetRootComponent(SceneRoot);

	ItemMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("ItemMesh"));
	ItemMesh->SetupAttachment(SceneRoot);
	ItemMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	ItemMesh->SetGenerateOverlapEvents(false);

	PickupCollision = CreateDefaultSubobject<USphereComponent>(TEXT("PickupCollision"));
	PickupCollision->SetupAttachment(SceneRoot);
	PickupCollision->SetSphereRadius(80.f);
	PickupCollision->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	PickupCollision->SetCollisionObjectType(ECC_WorldDynamic);
	PickupCollision->SetCollisionResponseToAllChannels(ECR_Ignore);
	PickupCollision->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
	PickupCollision->SetGenerateOverlapEvents(true);
}

void ADropItemActor::BeginPlay()
{
	Super::BeginPlay();

	if (PickupCollision)
	{
		PickupCollision->OnComponentBeginOverlap.AddDynamic(this, &ADropItemActor::OnPickupCollisionBeginOverlap);
	}
}

void ADropItemActor::InitializeDropItem(FName InItemID, int32 InCount)
{
	ItemID = InItemID;
	Count = FMath::Max(1, InCount);
}

void ADropItemActor::OnPickupCollisionBeginOverlap(
	UPrimitiveComponent* OverlappedComponent,
	AActor* OtherActor,
	UPrimitiveComponent* OtherComp,
	int32 OtherBodyIndex,
	bool bFromSweep,
	const FHitResult& SweepResult)
{
	if (!OtherActor || OtherActor == this)
	{
		return;
	}

	if (ItemID.IsNone())
	{
		UE_LOG(LogTemp, Warning, TEXT("[DropItemActor] Pickup failed: ItemID is None on %s"), *GetName());
		return;
	}

	UInventoryComponent* InventoryComponent = OtherActor->FindComponentByClass<UInventoryComponent>();
	if (!InventoryComponent)
	{
		UE_LOG(LogTemp, Warning, TEXT("[DropItemActor] Pickup failed: %s has no InventoryComponent"), *OtherActor->GetName());
		return;
	}

	const int32 SafeCount = FMath::Max(1, Count);
	InventoryComponent->AddItem(ItemID, SafeCount);

	UE_LOG(LogTemp, Warning, TEXT("[DropItemActor] Pickup success: %s picked up %s x%d"),
		*OtherActor->GetName(),
		*ItemID.ToString(),
		SafeCount);

	Destroy();
}
