#include "WeaponBase.h"
#include "TPSCaptureCharacter.h"

#include "Components/StaticMeshComponent.h"
#include "Components/SphereComponent.h"
#include "Components/SceneComponent.h"
#include "TimerManager.h"

AWeaponBase::AWeaponBase()
{
	PrimaryActorTick.bCanEverTick = false;

	DefaultRoot = CreateDefaultSubobject<USceneComponent>(TEXT("DefaultRoot"));
	RootComponent = DefaultRoot;

	WeaponMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("WeaponMesh"));
	WeaponMesh->SetupAttachment(DefaultRoot);
	WeaponMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	WeaponMesh->SetGenerateOverlapEvents(false);
	WeaponMesh->SetSimulatePhysics(false);

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

	if (PickupSphere)
	{
		PickupSphere->OnComponentBeginOverlap.AddDynamic(this, &AWeaponBase::OnPickupSphereBeginOverlap);
		PickupSphere->OnComponentEndOverlap.AddDynamic(this, &AWeaponBase::OnPickupSphereEndOverlap);
	}
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

	ATPSCaptureCharacter* Character = Cast<ATPSCaptureCharacter>(OtherActor);
	if (Character)
	{
		Character->SetNearbyWeapon(this);
	}
}

void AWeaponBase::OnPickupSphereEndOverlap(
	UPrimitiveComponent* OverlappedComponent,
	AActor* OtherActor,
	UPrimitiveComponent* OtherComp,
	int32 OtherBodyIndex
)
{
	ATPSCaptureCharacter* Character = Cast<ATPSCaptureCharacter>(OtherActor);
	if (Character)
	{
		Character->ClearNearbyWeapon(this);
	}
}

void AWeaponBase::SetPickupEnabled(bool bEnabled)
{
	bCanBePickedUp = bEnabled;

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