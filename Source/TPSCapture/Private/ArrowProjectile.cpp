#include "ArrowProjectile.h"
#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "GameFramework/ProjectileMovementComponent.h"
#include "Kismet/GameplayStatics.h"

#include "Sound/SoundBase.h"

AArrowProjectile::AArrowProjectile()
{
	PrimaryActorTick.bCanEverTick = false;

	CollisionComp = CreateDefaultSubobject<USphereComponent>(TEXT("CollisionComp"));
	RootComponent = CollisionComp;

	CollisionComp->InitSphereRadius(10.0f);
	CollisionComp->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	CollisionComp->SetCollisionObjectType(ECC_WorldDynamic);
	CollisionComp->SetCollisionResponseToAllChannels(ECR_Ignore);
	CollisionComp->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
	CollisionComp->SetCollisionResponseToChannel(ECC_WorldStatic, ECR_Block);
	CollisionComp->SetCollisionResponseToChannel(ECC_WorldDynamic, ECR_Block);

	ArrowMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("ArrowMesh"));
	ArrowMesh->SetupAttachment(RootComponent);
	ArrowMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	ProjectileMovement = CreateDefaultSubobject<UProjectileMovementComponent>(TEXT("ProjectileMovement"));
	ProjectileMovement->InitialSpeed = 2000.0f;
	ProjectileMovement->MaxSpeed = 2000.0f;
	ProjectileMovement->bRotationFollowsVelocity = true;
	ProjectileMovement->bShouldBounce = false;

	InitialLifeSpan = 5.0f;
}

void AArrowProjectile::BeginPlay()
{
	Super::BeginPlay();

	CollisionComp->OnComponentBeginOverlap.AddDynamic(this, &AArrowProjectile::OnArrowOverlap);
	SetLifeSpan(LifeSeconds);
}

void AArrowProjectile::OnArrowOverlap(
	UPrimitiveComponent* OverlappedComponent,
	AActor* OtherActor,
	UPrimitiveComponent* OtherComp,
	int32 OtherBodyIndex,
	bool bFromSweep,
	const FHitResult& SweepResult)
{
	UE_LOG(LogTemp, Warning, TEXT("Arrow Overlap Triggered"));

	AActor* OwnerActor = GetOwner();

	if (OtherActor && OtherActor != this && OtherActor != OwnerActor)
	{
		UE_LOG(LogTemp, Warning, TEXT("Arrow hit actor: %s"), *OtherActor->GetName());

		UGameplayStatics::ApplyDamage(
			OtherActor,
			Damage,
			OwnerActor ? OwnerActor->GetInstigatorController() : nullptr,
			this,
			UDamageType::StaticClass()
		);

		if (ArrowHitSound)
		{
			FVector SoundLocation = OtherActor->GetActorLocation();

			if (bFromSweep)
				SoundLocation = SweepResult.ImpactPoint;

			UGameplayStatics::PlaySoundAtLocation(
				this,
				ArrowHitSound,
				SoundLocation
			);
		}

		Destroy();
	}
}