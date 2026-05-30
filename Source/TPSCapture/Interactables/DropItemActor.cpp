#include "DropItemActor.h"

#include "Components/SceneComponent.h"
#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "TPSGameInstance.h"
#include "Kismet/GameplayStatics.h"
#include "InventoryComponent.h"
#include "Sound/SoundBase.h"

ADropItemActor::ADropItemActor()
{
	PrimaryActorTick.bCanEverTick = true;

	SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
	SetRootComponent(SceneRoot);

	ItemMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("ItemMesh"));
	ItemMesh->SetupAttachment(SceneRoot);
	ItemMesh->SetMobility(EComponentMobility::Movable);
	ItemMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	ItemMesh->SetGenerateOverlapEvents(false);

	PickupCollision = CreateDefaultSubobject<USphereComponent>(TEXT("PickupCollision"));
	PickupCollision->SetupAttachment(SceneRoot);
	PickupCollision->SetMobility(EComponentMobility::Movable);
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

	InitialActorRotation = GetActorRotation();
	ApplyItemDataToDropVisual();
	CacheInitialPickupMotionTransform();

	if (PickupCollision)
	{
		PickupCollision->OnComponentBeginOverlap.AddDynamic(this, &ADropItemActor::OnPickupCollisionBeginOverlap);
	}
}

void ADropItemActor::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	ApplyPickupMotion(DeltaTime);
}

void ADropItemActor::InitializeDropItem(FName InItemID, int32 InCount)
{
	ItemID = InItemID;
	Count = FMath::Max(1, InCount);

	ApplyItemDataToDropVisual();
	CacheInitialPickupMotionTransform();
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

	if (PickupSound)
	{
		UGameplayStatics::PlaySoundAtLocation(
			this,
			PickupSound,
			GetActorLocation(),
			PickupSoundVolume,
			PickupSoundPitch
		);
	}

	UE_LOG(LogTemp, Warning, TEXT("[DropItemActor] Pickup success: %s picked up %s x%d"),
		*OtherActor->GetName(),
		*ItemID.ToString(),
		SafeCount);

	Destroy();
}

void ADropItemActor::ApplyItemDataToDropVisual()
{
	if (ItemID.IsNone())
	{
		return;
	}

	const UTPSGameInstance* TPSGameInstance = GetGameInstance<UTPSGameInstance>();
	if (!TPSGameInstance)
	{
		UE_LOG(LogTemp, Warning, TEXT("[DropItemActor] Failed to apply drop visual for %s: TPSGameInstance is null"), *ItemID.ToString());
		return;
	}

	FItemData ItemData;
	if (!TPSGameInstance->GetItemDataByID(ItemID, ItemData))
	{
		UE_LOG(LogTemp, Warning, TEXT("[DropItemActor] ItemData not found for %s"), *ItemID.ToString());
		return;
	}

	if (ItemMesh)
	{
		if (ItemData.Mesh)
		{
			ItemMesh->SetStaticMesh(ItemData.Mesh);
		}

		ItemMesh->SetRelativeLocation(ItemData.ItemMeshPosition);
		ItemMesh->SetRelativeRotation(ItemData.ItemMeshRotation);
		ItemMesh->SetRelativeScale3D(ItemData.ItemMeshScale);
	}

	if (PickupCollision)
	{
		PickupCollision->SetRelativeLocation(FVector::ZeroVector);
		PickupCollision->SetRelativeRotation(FRotator::ZeroRotator);
		PickupCollision->SetRelativeScale3D(ItemData.PickupCollisionScale);
	}
}

void ADropItemActor::CacheInitialPickupMotionTransform()
{
	if (ItemMesh)
	{
		InitialMeshRelativeLocation = ItemMesh->GetRelativeLocation();
		InitialMeshRelativeRotation = ItemMesh->GetRelativeRotation();
	}
}

void ADropItemActor::ApplyPickupMotion(float DeltaTime)
{
	if (!bEnablePickupMotion || !ItemMesh)
	{
		return;
	}

	PickupMotionElapsedTime += DeltaTime;
	PickupMotionRotationYaw = FMath::Fmod(PickupMotionRotationYaw + PickupRotationSpeed * DeltaTime, 360.0f);

	const float BobOffsetZ = FMath::Sin(PickupMotionElapsedTime * PickupBobSpeed) * PickupBobAmplitude;
	SetActorRotation(InitialActorRotation + FRotator(0.0f, PickupMotionRotationYaw, 0.0f));
	ItemMesh->SetRelativeLocation(InitialMeshRelativeLocation + FVector(0.0f, 0.0f, BobOffsetZ));
	ItemMesh->SetRelativeRotation(InitialMeshRelativeRotation);
}
