#include "ArrowProjectile.h"
#include "TPSCaptureCharacter.h"
#include "TPSAnimalBase.h"

#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "GameFramework/ProjectileMovementComponent.h"
#include "Kismet/GameplayStatics.h"

#include "Sound/SoundBase.h"

#include "NiagaraFunctionLibrary.h"
#include "NiagaraComponent.h"

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
		if (const ATPSCaptureCharacter* PlayerCharacter = Cast<ATPSCaptureCharacter>(OtherActor))
		{
			if (PlayerCharacter->IsDodging())
			{
				UE_LOG(LogTemp, Warning, TEXT("Arrow passed through dodging player: %s"), *OtherActor->GetName());
				return;
			}
		}

		if (const AAnimalBase* Animal = Cast<AAnimalBase>(OtherActor))
		{
			if (Animal->IsTrapped())
			{
				UE_LOG(LogTemp, Warning, TEXT("Arrow passed through trapped animal: %s"), *OtherActor->GetName());
				return;
			}
		}

		UE_LOG(LogTemp, Warning, TEXT("Arrow hit actor: %s"), *OtherActor->GetName());

		UGameplayStatics::ApplyDamage(
			OtherActor,
			Damage,
			OwnerActor ? OwnerActor->GetInstigatorController() : nullptr,
			this,
			UDamageType::StaticClass()
		);

		if (ArrowHitVFX)
		{
			FVector VFXLocation = OtherActor->GetActorLocation();

			if (bFromSweep && !SweepResult.ImpactPoint.IsNearlyZero())
				VFXLocation = SweepResult.ImpactPoint;
			else if (OtherComp)
				VFXLocation = OtherComp->GetComponentLocation();

			UNiagaraComponent* NiagaraComp = UNiagaraFunctionLibrary::SpawnSystemAtLocation(
				GetWorld(),
				ArrowHitVFX,
				VFXLocation,
				GetActorRotation(),
				FVector(1.0f),
				true,
				true,
				ENCPoolMethod::None,
				true
			);

			if (NiagaraComp)
			{
				NiagaraComp->SetVariableLinearColor(TEXT("Color"), ArrowHitColor);
				NiagaraComp->SetVariableFloat(TEXT("Scale"), ArrowHitScale);
				NiagaraComp->SetVariableFloat(TEXT("Lifetime"), ArrowHitLifetime);
			}
		}

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
