#include "WeaponBase.h"
#include "RACharacter.h"
#include "PlayerEquipmentComponent.h"

#include "Components/StaticMeshComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/SphereComponent.h"
#include "Components/SceneComponent.h"
#include "TimerManager.h"

AWeaponBase::AWeaponBase()
{
	PrimaryActorTick.bCanEverTick = true;

	DefaultRoot = CreateDefaultSubobject<USceneComponent>(TEXT("DefaultRoot"));
	RootComponent = DefaultRoot;

	WeaponMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("WeaponMesh"));
	WeaponMesh->SetupAttachment(DefaultRoot);
	WeaponMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	WeaponMesh->SetGenerateOverlapEvents(false);
	WeaponMesh->SetSimulatePhysics(false);

	WeaponSkeletalMesh = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("WeaponSkeletalMesh"));
	WeaponSkeletalMesh->SetupAttachment(DefaultRoot);
	WeaponSkeletalMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	WeaponSkeletalMesh->SetGenerateOverlapEvents(false);
	WeaponSkeletalMesh->SetSimulatePhysics(false);

	PickupSphere = CreateDefaultSubobject<USphereComponent>(TEXT("PickupSphere"));
	PickupSphere->SetupAttachment(DefaultRoot);
	PickupSphere->SetSphereRadius(100.0f);
	PickupSphere->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	PickupSphere->SetCollisionResponseToAllChannels(ECR_Ignore);
	PickupSphere->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
	PickupSphere->SetGenerateOverlapEvents(true);
}

void AWeaponBase::BeginPlay()
{
	Super::BeginPlay();

	InitialPickupActorRotation = GetActorRotation();

	if (WeaponMesh)
	{
		InitialWeaponMeshRelativeLocation = WeaponMesh->GetRelativeLocation();
		InitialWeaponMeshRelativeRotation = WeaponMesh->GetRelativeRotation();
		InitialWeaponMeshRelativeScale = WeaponMesh->GetRelativeScale3D();
	}

	if (WeaponSkeletalMesh)
	{
		InitialWeaponSkeletalMeshRelativeLocation = WeaponSkeletalMesh->GetRelativeLocation();
		InitialWeaponSkeletalMeshRelativeRotation = WeaponSkeletalMesh->GetRelativeRotation();
		InitialWeaponSkeletalMeshRelativeScale = WeaponSkeletalMesh->GetRelativeScale3D();
	}

	UpdateWeaponVisualState();

	if (PickupSphere)
	{
		PickupSphere->OnComponentBeginOverlap.AddDynamic(this, &AWeaponBase::OnPickupSphereBeginOverlap);
		PickupSphere->OnComponentEndOverlap.AddDynamic(this, &AWeaponBase::OnPickupSphereEndOverlap);
	}
}

void AWeaponBase::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	ApplyPickupMotion(DeltaTime);
}

bool AWeaponBase::UsesSkeletalMesh() const
{
	return WeaponSkeletalMesh && WeaponSkeletalMesh->GetSkeletalMeshAsset() != nullptr;
}

void AWeaponBase::UpdateWeaponVisualState()
{
	const bool bUseSkeletal = UsesSkeletalMesh();

	if (WeaponMesh)
	{
		WeaponMesh->SetVisibility(!bUseSkeletal);
		WeaponMesh->SetHiddenInGame(bUseSkeletal);
		WeaponMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		WeaponMesh->SetSimulatePhysics(false);
	}

	if (WeaponSkeletalMesh)
	{
		WeaponSkeletalMesh->SetVisibility(bUseSkeletal);
		WeaponSkeletalMesh->SetHiddenInGame(!bUseSkeletal);
		WeaponSkeletalMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		WeaponSkeletalMesh->SetSimulatePhysics(false);
	}
}

USceneComponent* AWeaponBase::GetActiveVisualComponent() const
{
	if (UsesSkeletalMesh())
	{
		return WeaponSkeletalMesh;
	}

	return WeaponMesh;
}

void AWeaponBase::OnPickupSphereBeginOverlap(
	UPrimitiveComponent* OverlappedComponent,
	AActor* OtherActor,
	UPrimitiveComponent* OtherComp,
	int32 OtherBodyIndex,
	bool bFromSweep,
	const FHitResult& SweepResult
)
{
	if (!bCanBePickedUp)
		return;

	ARACharacter* Character = Cast<ARACharacter>(OtherActor);
	UPlayerEquipmentComponent* PlayerEquipmentComponent = Character
		? Character->GetPlayerEquipmentComponent()
		: nullptr;
	if (PlayerEquipmentComponent)
	{
		PlayerEquipmentComponent->SetNearbyWeapon(this);
	}
}

void AWeaponBase::OnPickupSphereEndOverlap(
	UPrimitiveComponent* OverlappedComponent,
	AActor* OtherActor,
	UPrimitiveComponent* OtherComp,
	int32 OtherBodyIndex
)
{
	ARACharacter* Character = Cast<ARACharacter>(OtherActor);
	UPlayerEquipmentComponent* PlayerEquipmentComponent = Character
		? Character->GetPlayerEquipmentComponent()
		: nullptr;
	if (PlayerEquipmentComponent)
	{
		PlayerEquipmentComponent->ClearNearbyWeapon(this);
	}
}

void AWeaponBase::SetPickupEnabled(bool bEnabled)
{
	bCanBePickedUp = bEnabled;

	if (bEnabled)
	{
		InitialPickupActorRotation = GetActorRotation();
		ResetPickupMotionVisuals();
	}

	if (PickupSphere)
	{
		PickupSphere->SetGenerateOverlapEvents(bEnabled);
		PickupSphere->SetCollisionEnabled(bEnabled ? ECollisionEnabled::QueryOnly : ECollisionEnabled::NoCollision);
	}
}

void AWeaponBase::EnablePickupAfterDrop()
{
	SetPickupEnabled(false);

	FTimerHandle TimerHandle;
	GetWorldTimerManager().SetTimer(
		TimerHandle,
		[this]()
		{
			SetPickupEnabled(true);
		},
		PickupEnableDelay,
		false
	);
}

bool AWeaponBase::ShouldPlayPickupMotion() const
{
	return bEnablePickupMotion && bCanBePickedUp && GetAttachParentActor() == nullptr;
}

void AWeaponBase::ApplyPickupMotion(float DeltaTime)
{
	if (!ShouldPlayPickupMotion())
	{
		return;
	}

	USceneComponent* ActiveVisualComponent = GetActiveVisualComponent();
	if (!ActiveVisualComponent)
	{
		return;
	}

	PickupMotionElapsedTime += DeltaTime;
	PickupMotionRotationYaw = FMath::Fmod(PickupMotionRotationYaw + PickupRotationSpeed * DeltaTime, 360.0f);

	const float BobOffsetZ = FMath::Sin(PickupMotionElapsedTime * PickupBobSpeed) * PickupBobAmplitude;
	SetActorRotation(InitialPickupActorRotation + FRotator(0.0f, PickupMotionRotationYaw, 0.0f));

	if (ActiveVisualComponent == WeaponSkeletalMesh)
	{
		WeaponSkeletalMesh->SetRelativeLocation(InitialWeaponSkeletalMeshRelativeLocation + FVector(0.0f, 0.0f, BobOffsetZ));
		WeaponSkeletalMesh->SetRelativeRotation(InitialWeaponSkeletalMeshRelativeRotation);
		return;
	}

	WeaponMesh->SetRelativeLocation(InitialWeaponMeshRelativeLocation + FVector(0.0f, 0.0f, BobOffsetZ));
	WeaponMesh->SetRelativeRotation(InitialWeaponMeshRelativeRotation);
}

void AWeaponBase::ResetPickupMotionVisuals()
{
	PickupMotionElapsedTime = 0.0f;
	PickupMotionRotationYaw = 0.0f;
	SetActorRotation(InitialPickupActorRotation);

	if (WeaponMesh)
	{
		WeaponMesh->SetRelativeLocation(InitialWeaponMeshRelativeLocation);
		WeaponMesh->SetRelativeRotation(InitialWeaponMeshRelativeRotation);
		WeaponMesh->SetRelativeScale3D(InitialWeaponMeshRelativeScale);
	}

	if (WeaponSkeletalMesh)
	{
		WeaponSkeletalMesh->SetRelativeLocation(InitialWeaponSkeletalMeshRelativeLocation);
		WeaponSkeletalMesh->SetRelativeRotation(InitialWeaponSkeletalMeshRelativeRotation);
		WeaponSkeletalMesh->SetRelativeScale3D(InitialWeaponSkeletalMeshRelativeScale);
	}
}
