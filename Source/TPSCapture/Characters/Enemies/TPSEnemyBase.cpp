#include "TPSEnemyBase.h"
#include "Components/SphereComponent.h"
#include "TPSCaptureCharacter.h"

#include "AIController.h"
#include "Animation/AnimInstance.h"
#include "Kismet/GameplayStatics.h"

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
	UpdateAttack();
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

	if (bIsAttacking)
		return false;

	if (!HasValidTarget())
		return false;

	const float DistanceToTarget = FVector::Dist(GetActorLocation(), TargetActor->GetActorLocation());
	if (DistanceToTarget > AttackRange)
		return false;

	return true;
}

void ATPSEnemyBase::UpdateAttack()
{
	if (!CanAttack())
		return;

	const float CurrentTime = GetWorld()->GetTimeSeconds();
	if (CurrentTime - LastAttackTime < AttackCooldown)
		return;

	bIsAttacking = true;
	LastAttackTime = CurrentTime;

	if (AAIController* AIController = Cast<AAIController>(GetController()))
		AIController->StopMovement();

	PerformPunchAttack();
}

void ATPSEnemyBase::PerformPunchAttack()
{
	if (!AttackMontage || !GetMesh() || !GetMesh()->GetAnimInstance())
	{
		UE_LOG(LogTemp, Warning, TEXT("[%s] AttackMontage is missing"), *GetName());
		EndAttack();
		return;
	}

	UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance();
	AnimInstance->Montage_Play(AttackMontage);
}

void ATPSEnemyBase::EndAttack()
{
	bIsAttacking = false;
}

void ATPSEnemyBase::ApplyDamageToTarget()
{
	if (bIsDead || !HasValidTarget())
		return;

	const float DistanceToTarget = FVector::Dist(GetActorLocation(), TargetActor->GetActorLocation());
	if (DistanceToTarget > AttackRange)
		return;

	UGameplayStatics::ApplyDamage(
		TargetActor,
		AttackDamage,
		GetController(),
		this,
		UDamageType::StaticClass()
	);

	UE_LOG(LogTemp, Warning, TEXT("[%s] Applied %.1f damage to %s"),
		*GetName(),
		AttackDamage,
		*TargetActor->GetName());
}