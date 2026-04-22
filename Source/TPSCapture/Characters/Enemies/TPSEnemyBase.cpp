#include "TPSEnemyBase.h"
#include "AIController.h"
#include "Components/SphereComponent.h"
#include "TPSCaptureCharacter.h"

ATPSEnemyBase::ATPSEnemyBase()
{
	PrimaryActorTick.bCanEverTick = true;

	DetectionSphere = CreateDefaultSubobject<USphereComponent>(TEXT("DetectionSphere"));
	DetectionSphere->SetupAttachment(RootComponent);
	DetectionSphere->SetSphereRadius(800.f);
	DetectionSphere->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	DetectionSphere->SetCollisionObjectType(ECC_WorldDynamic);
	DetectionSphere->SetCollisionResponseToAllChannels(ECR_Ignore);
	DetectionSphere->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
}

void ATPSEnemyBase::BeginPlay()
{
	Super::BeginPlay();

	if (DetectionSphere)
	{
		DetectionSphere->SetSphereRadius(DetectRange);
		DetectionSphere->OnComponentBeginOverlap.AddDynamic(this, &ATPSEnemyBase::OnDetectionSphereBeginOverlap);
		DetectionSphere->OnComponentEndOverlap.AddDynamic(this, &ATPSEnemyBase::OnDetectionSphereEndOverlap);
	}
}

void ATPSEnemyBase::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	UpdateChase();
}

void ATPSEnemyBase::OnDetectionSphereBeginOverlap(
	UPrimitiveComponent* OverlappedComponent,
	AActor* OtherActor,
	UPrimitiveComponent* OtherComp,
	int32 OtherBodyIndex,
	bool bFromSweep,
	const FHitResult& SweepResult)
{
	if (bIsDead)
		return;

	ATPSCaptureCharacter* PlayerCharacter = Cast<ATPSCaptureCharacter>(OtherActor);
	if (PlayerCharacter)
	{
		SetTargetActor(PlayerCharacter);
		UE_LOG(LogTemp, Warning, TEXT("[%s] Detected Player: %s"), *GetName(), *OtherActor->GetName());
	}
}

void ATPSEnemyBase::OnDetectionSphereEndOverlap(
	UPrimitiveComponent* OverlappedComponent,
	AActor* OtherActor,
	UPrimitiveComponent* OtherComp,
	int32 OtherBodyIndex)
{
	if (OtherActor && OtherActor == TargetActor)
	{
		UE_LOG(LogTemp, Warning, TEXT("[%s] Lost Target: %s"), *GetName(), *OtherActor->GetName());
		ClearTargetActor();
	}
}

void ATPSEnemyBase::UpdateChase()
{
	if (bIsDead)
		return;

	if (!HasValidTarget())
		return;

	AAIController* AIController = Cast<AAIController>(GetController());
	if (!AIController)
		return;

	const float DistanceToTarget = FVector::Dist(GetActorLocation(), TargetActor->GetActorLocation());

	if (DistanceToTarget > AttackRange)
		AIController->MoveToActor(TargetActor, AttackRange);
	else
		AIController->StopMovement();
}

void ATPSEnemyBase::SetTargetActor(AActor* NewTarget)
{
	TargetActor = NewTarget;
}

void ATPSEnemyBase::ClearTargetActor()
{
	TargetActor = nullptr;

	if (AAIController* AIController = Cast<AAIController>(GetController()))
		AIController->StopMovement();
}

bool ATPSEnemyBase::HasValidTarget() const
{
	return TargetActor != nullptr;
}

bool ATPSEnemyBase::CanAttack() const
{
	if (bIsDead)
		return false;

	if (!HasValidTarget())
		return false;

	const float DistanceToTarget = FVector::Dist(GetActorLocation(), TargetActor->GetActorLocation());
	return DistanceToTarget <= AttackRange;
}