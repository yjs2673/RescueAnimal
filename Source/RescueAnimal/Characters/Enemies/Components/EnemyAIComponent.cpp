#include "EnemyAIComponent.h"

#include "RAEnemyBase.h"
#include "AnimalBase.h"
#include "RACharacter.h"

#include "AIController.h"
#include "Components/SphereComponent.h"
#include "Engine/OverlapResult.h"
#include "GameFramework/Controller.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Navigation/PathFollowingComponent.h"
#include "NavigationSystem.h"

UEnemyAIComponent::UEnemyAIComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
}

void UEnemyAIComponent::BeginPlay()
{
	Super::BeginPlay();

	ARAEnemyBase* Enemy = GetOwnerEnemy();
	if (!Enemy)
		return;

	if (Enemy->DetectionSphere)
	{
		Enemy->DetectionSphere->SetSphereRadius(Enemy->DetectRange);
		Enemy->DetectionSphere->OnComponentBeginOverlap.AddDynamic(this, &UEnemyAIComponent::OnDetectionSphereBeginOverlap);
		Enemy->DetectionSphere->OnComponentEndOverlap.AddDynamic(this, &UEnemyAIComponent::OnDetectionSphereEndOverlap);
	}

	if (Enemy->GetCharacterMovement())
	{
		Enemy->GetCharacterMovement()->MaxWalkSpeed = Enemy->MoveSpeed;
		Enemy->GetCharacterMovement()->bUseRVOAvoidance = Enemy->bUseEnemySeparation;
		Enemy->GetCharacterMovement()->AvoidanceConsiderationRadius = Enemy->EnemySeparationRadius;
	}

	ChangeAIState(Enemy->bUseCampPatrolArea ? EEnemyAIState::Patrol : EEnemyAIState::Idle);
}

void UEnemyAIComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	ARAEnemyBase* Enemy = GetOwnerEnemy();
	if (!Enemy)
		return;

	Enemy->bWantsMovementThisTick = false;
	UpdateChase();
	UpdateMovementStuckCheck(DeltaTime);
	ApplySeparationFromNearbyEnemies(DeltaTime);
}

void UEnemyAIComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (ARAEnemyBase* Enemy = GetOwnerEnemy())
	{
		if (Enemy->DetectionSphere)
		{
			Enemy->DetectionSphere->OnComponentBeginOverlap.RemoveDynamic(this, &UEnemyAIComponent::OnDetectionSphereBeginOverlap);
			Enemy->DetectionSphere->OnComponentEndOverlap.RemoveDynamic(this, &UEnemyAIComponent::OnDetectionSphereEndOverlap);
		}
	}

	Super::EndPlay(EndPlayReason);
}

ARAEnemyBase* UEnemyAIComponent::GetOwnerEnemy() const
{
	return Cast<ARAEnemyBase>(GetOwner());
}

EEnemyAIState UEnemyAIComponent::GetCurrentAIState() const
{
	const ARAEnemyBase* Enemy = GetOwnerEnemy();
	return Enemy ? Enemy->GetEnemyAIState() : EEnemyAIState::Idle;
}

bool UEnemyAIComponent::IsInAIState(EEnemyAIState State) const
{
	return GetCurrentAIState() == State;
}

bool UEnemyAIComponent::CanChangeAIState(EEnemyAIState NewState) const
{
	const ARAEnemyBase* Enemy = GetOwnerEnemy();
	if (!Enemy)
	{
		return false;
	}

	const EEnemyAIState CurrentState = Enemy->GetEnemyAIState();
	if (CurrentState == NewState)
	{
		return false;
	}

	if (CurrentState == EEnemyAIState::Dead)
	{
		return false;
	}

	if (Enemy->bIsDead && NewState != EEnemyAIState::Dead)
	{
		return false;
	}

	if (IsInAIState(EEnemyAIState::Attack) && NewState != EEnemyAIState::Dead)
	{
		return false;
	}

	if (NewState == EEnemyAIState::Chase || NewState == EEnemyAIState::Attack)
	{
		return HasValidTarget();
	}

	if (NewState == EEnemyAIState::Patrol)
	{
		return Enemy->bUseCampPatrolArea && Enemy->CampPatrolRadius > 0.0f;
	}

	return true;
}

bool UEnemyAIComponent::ChangeAIState(EEnemyAIState NewState)
{
	return ChangeAIStateInternal(NewState, false);
}

bool UEnemyAIComponent::ChangeAIStateInternal(EEnemyAIState NewState, bool bForce)
{
	ARAEnemyBase* Enemy = GetOwnerEnemy();
	if (!Enemy)
	{
		return false;
	}

	if (!bForce && !CanChangeAIState(NewState))
	{
		return false;
	}

	const EEnemyAIState PreviousState = Enemy->GetEnemyAIState();
	if (PreviousState == NewState)
	{
		return false;
	}

	HandleAIStateExit(PreviousState, NewState);
	Enemy->SetEnemyAIState(NewState);
	HandleAIStateEnter(PreviousState, NewState);

	return true;
}

bool UEnemyAIComponent::RefreshAIStateFromTarget()
{
	ARAEnemyBase* Enemy = GetOwnerEnemy();
	if (!Enemy)
	{
		return false;
	}

	if (Enemy->bIsDead)
	{
		return ChangeAIState(EEnemyAIState::Dead);
	}

	if (HasValidTarget())
	{
		return ChangeAIState(EEnemyAIState::Chase);
	}

	return ChangeAIState(Enemy->bUseCampPatrolArea ? EEnemyAIState::Patrol : EEnemyAIState::Idle);
}

void UEnemyAIComponent::NotifyAttackStarted()
{
	ChangeAIState(EEnemyAIState::Attack);
}

void UEnemyAIComponent::NotifyAttackFinished()
{
	ARAEnemyBase* Enemy = GetOwnerEnemy();
	if (!Enemy)
	{
		return;
	}

	if (Enemy->bIsDead)
	{
		ChangeAIStateInternal(EEnemyAIState::Dead, true);
		return;
	}

	if (HasValidTarget())
	{
		ChangeAIStateInternal(EEnemyAIState::Chase, true);
		return;
	}

	ChangeAIStateInternal(Enemy->bUseCampPatrolArea ? EEnemyAIState::Patrol : EEnemyAIState::Idle, true);
}

void UEnemyAIComponent::NotifyOwnerDied()
{
	ARAEnemyBase* Enemy = GetOwnerEnemy();
	if (!Enemy)
	{
		return;
	}

	Enemy->TargetActor = nullptr;
	StopOwnerMovement();
	ChangeAIState(EEnemyAIState::Dead);
}

void UEnemyAIComponent::HandleAIStateEnter(EEnemyAIState PreviousState, EEnemyAIState NewState)
{
	(void)PreviousState;

	ARAEnemyBase* Enemy = GetOwnerEnemy();
	if (!Enemy)
	{
		return;
	}

	switch (NewState)
	{
	case EEnemyAIState::Idle:
		StopOwnerMovement();
		Enemy->bWantsMovementThisTick = false;
		break;
	case EEnemyAIState::Patrol:
		Enemy->StuckTime = 0.f;
		break;
	case EEnemyAIState::Chase:
		Enemy->StuckTime = 0.f;
		break;
	case EEnemyAIState::Attack:
		StopOwnerMovement();
		Enemy->bWantsMovementThisTick = false;
		break;
	case EEnemyAIState::Dead:
		StopOwnerMovement();
		Enemy->TargetActor = nullptr;
		Enemy->bWantsMovementThisTick = false;
		break;
	default:
		break;
	}
}

void UEnemyAIComponent::HandleAIStateExit(EEnemyAIState PreviousState, EEnemyAIState NewState)
{
	(void)PreviousState;
	(void)NewState;
}

void UEnemyAIComponent::StopOwnerMovement()
{
	ARAEnemyBase* Enemy = GetOwnerEnemy();
	if (!Enemy)
	{
		return;
	}

	if (AAIController* AIController = Cast<AAIController>(Enemy->GetController()))
	{
		AIController->StopMovement();
	}
}

void UEnemyAIComponent::OnDetectionSphereBeginOverlap(
	UPrimitiveComponent* OverlappedComponent,
	AActor* OtherActor,
	UPrimitiveComponent* OtherComp,
	int32 OtherBodyIndex,
	bool bFromSweep,
	const FHitResult& SweepResult)
{
	ARAEnemyBase* Enemy = GetOwnerEnemy();
	if (!Enemy || Enemy->bIsDead)
		return;

	if (ARACharacter* PlayerCharacter = Cast<ARACharacter>(OtherActor))
	{
		if (SetTargetActor(PlayerCharacter))
		{
			UE_LOG(LogTemp, Warning, TEXT("[%s] Detected Player: %s"), *Enemy->GetName(), *OtherActor->GetName());
		}
	}
}

void UEnemyAIComponent::OnDetectionSphereEndOverlap(
	UPrimitiveComponent* OverlappedComponent,
	AActor* OtherActor,
	UPrimitiveComponent* OtherComp,
	int32 OtherBodyIndex)
{
	ARAEnemyBase* Enemy = GetOwnerEnemy();
	if (!Enemy)
		return;

	if (OtherActor && OtherActor == Enemy->TargetActor)
	{
		UE_LOG(LogTemp, Warning, TEXT("[%s] Lost Target: %s"), *Enemy->GetName(), *OtherActor->GetName());
		ClearTargetActor();
	}
}

void UEnemyAIComponent::UpdateChase()
{
	ARAEnemyBase* Enemy = GetOwnerEnemy();
	if (!Enemy || Enemy->bIsDead)
		return;

	if (IsInAIState(EEnemyAIState::Attack))
	{
		StopOwnerMovement();
		return;
	}

	if (!HasValidTarget())
	{
		RefreshAIStateFromTarget();
		UpdateCampWander();
		return;
	}

	if (Enemy->AttackType == EEnemyAttackType::Bow)
	{
		UpdateBowSpacing();
		return;
	}

	AAIController* AIController = Cast<AAIController>(Enemy->GetController());
	if (!AIController)
		return;

	const float DistanceToTarget = FVector::Dist(Enemy->GetActorLocation(), Enemy->TargetActor->GetActorLocation());

	if (DistanceToTarget > GetAttackStartRange())
	{
		ChangeAIState(EEnemyAIState::Chase);
		Enemy->StopHitMontage();
		const EPathFollowingRequestResult::Type MoveResult =
			AIController->MoveToActor(Enemy->TargetActor, GetChaseAcceptanceRadius(), false);
		Enemy->bWantsMovementThisTick = MoveResult != EPathFollowingRequestResult::Failed;
	}
	else
	{
		AIController->StopMovement();
	}
}

void UEnemyAIComponent::UpdateBowSpacing()
{
	ARAEnemyBase* Enemy = GetOwnerEnemy();
	if (!Enemy || !Enemy->TargetActor)
		return;

	AAIController* AIController = Cast<AAIController>(Enemy->GetController());
	if (!AIController)
		return;

	const FVector CurrentLocation = Enemy->GetActorLocation();
	const FVector TargetLocation = Enemy->TargetActor->GetActorLocation();
	const float DistanceToTarget = FVector::Dist(CurrentLocation, TargetLocation);

	const float TooFarDistance = Enemy->BowPreferredDistance + Enemy->BowDistanceTolerance;
	const float TooCloseDistance = FMath::Max(0.f, Enemy->BowPreferredDistance - Enemy->BowDistanceTolerance);

	if (DistanceToTarget > TooFarDistance)
	{
		ChangeAIState(EEnemyAIState::Chase);
		const EPathFollowingRequestResult::Type MoveResult =
			AIController->MoveToActor(Enemy->TargetActor, Enemy->BowPreferredDistance, false);
		Enemy->bWantsMovementThisTick = MoveResult != EPathFollowingRequestResult::Failed;
		return;
	}

	if (DistanceToTarget < TooCloseDistance)
	{
		ChangeAIState(EEnemyAIState::Chase);
		const FVector AwayDirection = (CurrentLocation - TargetLocation).GetSafeNormal();
		if (!AwayDirection.IsNearlyZero())
		{
			const FVector RetreatLocation = CurrentLocation + AwayDirection * Enemy->BowRetreatStepDistance;
			const EPathFollowingRequestResult::Type MoveResult =
				AIController->MoveToLocation(RetreatLocation, Enemy->BowMoveAcceptanceRadius);
			Enemy->bWantsMovementThisTick = MoveResult != EPathFollowingRequestResult::Failed;
			return;
		}
	}

	AIController->StopMovement();
}

bool UEnemyAIComponent::SetTargetActor(AActor* NewTarget)
{
	ARAEnemyBase* Enemy = GetOwnerEnemy();
	if (!Enemy || !IsValidCombatTarget(NewTarget))
	{
		return false;
	}

	Enemy->TargetActor = NewTarget;
	RefreshAIStateFromTarget();
	return true;
}

void UEnemyAIComponent::ClearTargetActor()
{
	ARAEnemyBase* Enemy = GetOwnerEnemy();
	if (!Enemy)
		return;

	Enemy->TargetActor = nullptr;

	StopOwnerMovement();
	RefreshAIStateFromTarget();
}

bool UEnemyAIComponent::HasValidTarget() const
{
	const ARAEnemyBase* Enemy = GetOwnerEnemy();
	return Enemy && IsValidCombatTarget(Enemy->TargetActor);
}

void UEnemyAIComponent::SetCampPatrolArea(const FVector& InCenter, float InRadius)
{
	ARAEnemyBase* Enemy = GetOwnerEnemy();
	if (!Enemy)
		return;

	Enemy->CampPatrolCenter = InCenter;
	Enemy->CampPatrolRadius = FMath::Max(0.0f, InRadius);
	Enemy->bUseCampPatrolArea = Enemy->CampPatrolRadius > 0.0f;
	Enemy->LastCampWanderTime = -1000.0f;

	if (!HasValidTarget())
	{
		RefreshAIStateFromTarget();
	}
}

void UEnemyAIComponent::ClearCampPatrolArea()
{
	ARAEnemyBase* Enemy = GetOwnerEnemy();
	if (!Enemy)
		return;

	Enemy->bUseCampPatrolArea = false;
	Enemy->CampPatrolCenter = FVector::ZeroVector;
	Enemy->CampPatrolRadius = 0.0f;
	Enemy->LastCampWanderTime = -1000.0f;

	if (!HasValidTarget())
	{
		RefreshAIStateFromTarget();
	}
}

void UEnemyAIComponent::UpdateCampWander()
{
	ARAEnemyBase* Enemy = GetOwnerEnemy();
	if (!Enemy || Enemy->bIsDead || IsInAIState(EEnemyAIState::Attack))
	{
		return;
	}

	if (!Enemy->bUseCampPatrolArea || Enemy->CampPatrolRadius <= 0.0f || !Enemy->GetWorld())
	{
		RefreshAIStateFromTarget();
		return;
	}

	RefreshAIStateFromTarget();

	const float CurrentTime = Enemy->GetWorld()->GetTimeSeconds();
	if (CurrentTime - Enemy->LastCampWanderTime < Enemy->CampWanderInterval)
	{
		return;
	}

	Enemy->LastCampWanderTime = CurrentTime;
	MoveToRandomCampLocation();
}

void UEnemyAIComponent::MoveToRandomCampLocation()
{
	ARAEnemyBase* Enemy = GetOwnerEnemy();
	if (!Enemy || !Enemy->GetWorld())
	{
		return;
	}

	AAIController* AIController = Cast<AAIController>(Enemy->GetController());
	if (!AIController)
	{
		return;
	}

	UNavigationSystemV1* NavSystem = UNavigationSystemV1::GetCurrent(Enemy->GetWorld());
	if (!NavSystem)
	{
		return;
	}

	FNavLocation RandomLocation;
	const bool bFoundLocation = NavSystem->GetRandomReachablePointInRadius(
		Enemy->CampPatrolCenter,
		Enemy->CampPatrolRadius,
		RandomLocation
	);

	if (!bFoundLocation)
	{
		return;
	}

	const EPathFollowingRequestResult::Type MoveResult =
		AIController->MoveToLocation(RandomLocation.Location, Enemy->CampWanderAcceptanceRadius);
	Enemy->bWantsMovementThisTick = MoveResult != EPathFollowingRequestResult::Failed;
}

bool UEnemyAIComponent::IsValidCombatTarget(const AActor* InTargetActor) const
{
	return IsValid(InTargetActor) && InTargetActor->IsA<ARACharacter>();
}

ARACharacter* UEnemyAIComponent::ResolvePlayerFromDamage(AController* EventInstigator, AActor* DamageCauser) const
{
	if (EventInstigator)
	{
		if (ARACharacter* PlayerCharacter = Cast<ARACharacter>(EventInstigator->GetPawn()))
		{
			return PlayerCharacter;
		}
	}

	if (ARACharacter* PlayerCharacter = Cast<ARACharacter>(DamageCauser))
	{
		return PlayerCharacter;
	}

	if (DamageCauser)
	{
		if (ARACharacter* PlayerCharacter = Cast<ARACharacter>(DamageCauser->GetOwner()))
		{
			return PlayerCharacter;
		}

		if (ARACharacter* PlayerCharacter = Cast<ARACharacter>(DamageCauser->GetInstigator()))
		{
			return PlayerCharacter;
		}
	}

	return nullptr;
}

float UEnemyAIComponent::GetAttackStartRange() const
{
	const ARAEnemyBase* Enemy = GetOwnerEnemy();
	return Enemy ? Enemy->AttackRange + FMath::Max(0.f, Enemy->AttackStartRangePadding) : 0.f;
}

float UEnemyAIComponent::GetAttackHitRange() const
{
	const ARAEnemyBase* Enemy = GetOwnerEnemy();
	return Enemy ? Enemy->AttackRange + FMath::Max(0.f, Enemy->AttackHitRangePadding) : 0.f;
}

float UEnemyAIComponent::GetChaseAcceptanceRadius() const
{
	const ARAEnemyBase* Enemy = GetOwnerEnemy();
	return Enemy ? FMath::Max(10.f, Enemy->AttackRange * 0.25f) : 10.f;
}

void UEnemyAIComponent::UpdateMovementStuckCheck(float DeltaTime)
{
	ARAEnemyBase* Enemy = GetOwnerEnemy();
	if (!Enemy || !Enemy->bUseStuckRecovery || Enemy->bIsDead || IsInAIState(EEnemyAIState::Attack))
	{
		if (Enemy)
		{
			Enemy->StuckTime = 0.f;
		}
		return;
	}

	AAIController* AIController = Cast<AAIController>(Enemy->GetController());
	if (!AIController)
	{
		Enemy->StuckTime = 0.f;
		return;
	}

	const bool bHasActiveMove =
		Enemy->bWantsMovementThisTick ||
		AIController->GetMoveStatus() == EPathFollowingStatus::Moving;

	if (!bHasActiveMove)
	{
		Enemy->StuckTime = 0.f;
		return;
	}

	if (Enemy->GetVelocity().Size2D() > Enemy->StuckVelocityThreshold)
	{
		Enemy->StuckTime = 0.f;
		return;
	}

	Enemy->StuckTime += DeltaTime;
	if (Enemy->StuckTime < Enemy->StuckTimeThreshold)
	{
		return;
	}

	const float CurrentTime = Enemy->GetWorld() ? Enemy->GetWorld()->GetTimeSeconds() : 0.f;
	if (CurrentTime - Enemy->LastStuckRecoveryTime < Enemy->StuckRecoveryCooldown)
	{
		return;
	}

	Enemy->LastStuckRecoveryTime = CurrentTime;
	Enemy->StuckTime = 0.f;
	HandleMovementStuck();
}

void UEnemyAIComponent::HandleMovementStuck()
{
	ARAEnemyBase* Enemy = GetOwnerEnemy();
	if (!Enemy)
		return;

	if (AAIController* AIController = Cast<AAIController>(Enemy->GetController()))
	{
		AIController->StopMovement();
	}

	if (HasValidTarget() && TryMoveToStrafeLocationAroundTarget())
	{
		return;
	}

	MoveToRandomCampLocation();
}

bool UEnemyAIComponent::TryMoveToStrafeLocationAroundTarget()
{
	ARAEnemyBase* Enemy = GetOwnerEnemy();
	if (!Enemy || !HasValidTarget() || !Enemy->GetWorld())
	{
		return false;
	}

	AAIController* AIController = Cast<AAIController>(Enemy->GetController());
	UNavigationSystemV1* NavSystem = UNavigationSystemV1::GetCurrent(Enemy->GetWorld());
	if (!AIController || !NavSystem)
	{
		return false;
	}

	const FVector CurrentLocation = Enemy->GetActorLocation();
	FVector ToTarget = (Enemy->TargetActor->GetActorLocation() - CurrentLocation).GetSafeNormal2D();
	if (ToTarget.IsNearlyZero())
	{
		ToTarget = Enemy->GetActorForwardVector();
	}

	const FVector RightVector(-ToTarget.Y, ToTarget.X, 0.f);
	const int32 FirstSign = FMath::RandBool() ? 1 : -1;
	const int32 Signs[2] = { FirstSign, -FirstSign };

	for (const int32 Sign : Signs)
	{
		const FVector CandidateLocation =
			CurrentLocation +
			RightVector * static_cast<float>(Sign) * Enemy->StuckSideStepDistance +
			ToTarget * 80.f;

		FNavLocation ProjectedLocation;
		if (!NavSystem->ProjectPointToNavigation(CandidateLocation, ProjectedLocation, FVector(150.f, 150.f, 200.f)))
		{
			continue;
		}

		const EPathFollowingRequestResult::Type MoveResult =
			AIController->MoveToLocation(ProjectedLocation.Location, GetChaseAcceptanceRadius());
		Enemy->bWantsMovementThisTick = MoveResult != EPathFollowingRequestResult::Failed;
		if (Enemy->bWantsMovementThisTick)
		{
			return true;
		}
	}

	return false;
}

void UEnemyAIComponent::ApplySeparationFromNearbyEnemies(float DeltaTime)
{
	ARAEnemyBase* Enemy = GetOwnerEnemy();
	if (!Enemy || !Enemy->bUseEnemySeparation || Enemy->bIsDead || IsInAIState(EEnemyAIState::Attack) || Enemy->EnemySeparationRadius <= 0.f)
	{
		return;
	}

	UWorld* World = Enemy->GetWorld();
	if (!World)
	{
		return;
	}

	TArray<FOverlapResult> OverlapResults;
	FCollisionQueryParams QueryParams;
	QueryParams.AddIgnoredActor(Enemy);

	FCollisionObjectQueryParams ObjectQueryParams;
	ObjectQueryParams.AddObjectTypesToQuery(ECC_Pawn);

	const FCollisionShape SeparationShape = FCollisionShape::MakeSphere(Enemy->EnemySeparationRadius);
	if (!World->OverlapMultiByObjectType(
		OverlapResults,
		Enemy->GetActorLocation(),
		FQuat::Identity,
		ObjectQueryParams,
		SeparationShape,
		QueryParams))
	{
		return;
	}

	FVector SeparationDirection = FVector::ZeroVector;
	const FVector CurrentLocation = Enemy->GetActorLocation();

	for (const FOverlapResult& OverlapResult : OverlapResults)
	{
		const ARAEnemyBase* OtherEnemy = Cast<ARAEnemyBase>(OverlapResult.GetActor());
		if (!OtherEnemy || OtherEnemy == Enemy || OtherEnemy->bIsDead)
		{
			continue;
		}

		FVector AwayDirection = CurrentLocation - OtherEnemy->GetActorLocation();
		AwayDirection.Z = 0.f;

		const float Distance = AwayDirection.Size();
		if (Distance <= KINDA_SMALL_NUMBER)
		{
			AwayDirection = Enemy->GetActorRightVector();
		}
		else
		{
			AwayDirection /= Distance;
		}

		const float Weight = FMath::Clamp((Enemy->EnemySeparationRadius - Distance) / Enemy->EnemySeparationRadius, 0.f, 1.f);
		SeparationDirection += AwayDirection * Weight;
	}

	if (SeparationDirection.IsNearlyZero())
	{
		return;
	}

	const float ScaledStrength = Enemy->EnemySeparationStrength * FMath::Clamp(DeltaTime * 60.f, 0.25f, 1.5f);
	Enemy->AddMovementInput(SeparationDirection.GetSafeNormal(), ScaledStrength);
	Enemy->bWantsMovementThisTick = true;
}
