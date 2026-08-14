#include "RAEnemyBase.h"
#include "Components/SphereComponent.h"
#include "DropItemActor.h"
#include "RACharacter.h"
#include "ArrowProjectile.h"
#include "WeaponBase.h"
#include "EnemyHPBarWidget.h"
#include "PlayerStatComponent.h"
#include "AnimalBase.h"
#include "RAGameInstance.h"
#include "RAWorldStateManager.h"

#include "AIController.h"
#include "Animation/AnimInstance.h"
#include "Components/SceneComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/WidgetComponent.h"
#include "Engine/OverlapResult.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/ProjectileMovementComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Navigation/PathFollowingComponent.h"
#include "NavigationSystem.h"
#include "NiagaraComponent.h"
#include "NiagaraFunctionLibrary.h"
#include "Sound/SoundBase.h"
#include "TimerManager.h"
#include "EngineUtils.h"

ARAEnemyBase::ARAEnemyBase()
{
	PrimaryActorTick.bCanEverTick = true;

	DetectionSphere = CreateDefaultSubobject<USphereComponent>(TEXT("DetectionSphere"));
	DetectionSphere->SetupAttachment(RootComponent);
	DetectionSphere->SetSphereRadius(800.f);
	DetectionSphere->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	DetectionSphere->SetCollisionObjectType(ECC_WorldDynamic);
	DetectionSphere->SetCollisionResponseToAllChannels(ECR_Ignore);
	DetectionSphere->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);

	HPBarWidgetComponent = CreateDefaultSubobject<UWidgetComponent>(TEXT("HPBarWidgetComponent"));
	HPBarWidgetComponent->SetupAttachment(GetRootComponent());
	HPBarWidgetComponent->SetRelativeLocation(FVector(0.f, 0.f, 100.f));
	HPBarWidgetComponent->SetWidgetSpace(EWidgetSpace::Screen);
	HPBarWidgetComponent->SetDrawSize(FVector2D(100.f, 20.f));

	if (GetCharacterMovement())
	{
		GetCharacterMovement()->bUseRVOAvoidance = true;
		GetCharacterMovement()->AvoidanceConsiderationRadius = EnemySeparationRadius;
	}
}

float ARAEnemyBase::TakeDamage(
	float DamageAmount,
	FDamageEvent const& DamageEvent,
	AController* EventInstigator,
	AActor* DamageCauser)
{
	LastDamageInstigator = EventInstigator;
	LastDamageCauser = DamageCauser;

	const float AppliedDamage = Super::TakeDamage(DamageAmount, DamageEvent, EventInstigator, DamageCauser);
	if (AppliedDamage > 0.f && !bIsDead)
	{
		if (ARACharacter* PlayerCharacter = ResolvePlayerFromDamage(EventInstigator, DamageCauser))
		{
			SetTargetActor(PlayerCharacter);
			UE_LOG(LogTemp, Warning, TEXT("[%s] Aggroed by damage from %s"), *GetName(), *PlayerCharacter->GetName());
		}
	}

	return AppliedDamage;
}

void ARAEnemyBase::BeginPlay()
{
	Super::BeginPlay();

	if (DetectionSphere)
	{
		DetectionSphere->SetSphereRadius(DetectRange);
		DetectionSphere->OnComponentBeginOverlap.AddDynamic(this, &ARAEnemyBase::OnDetectionSphereBeginOverlap);
		DetectionSphere->OnComponentEndOverlap.AddDynamic(this, &ARAEnemyBase::OnDetectionSphereEndOverlap);
	}

	if (GetCharacterMovement())
	{
		GetCharacterMovement()->MaxWalkSpeed = MoveSpeed;
		GetCharacterMovement()->bUseRVOAvoidance = bUseEnemySeparation;
		GetCharacterMovement()->AvoidanceConsiderationRadius = EnemySeparationRadius;
	}

	EquipDefaultWeapon();
	UpdateHPBar();
}

void ARAEnemyBase::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	bWantsMovementThisTick = false;
	UpdateChase();
	UpdateMovementStuckCheck(DeltaTime);
	ApplySeparationFromNearbyEnemies(DeltaTime);

	if (AttackType == EEnemyAttackType::Bow && bIsBowCharging)
	{
		FaceTargetActor();
	}

	UpdateAttack();
	UpdateHPBarVisibility();
}

void ARAEnemyBase::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (CurrentWeapon)
	{
		CurrentWeapon->Destroy();
		CurrentWeapon = nullptr;
	}

	GetWorldTimerManager().ClearTimer(BowFireTimerHandle);
	GetWorldTimerManager().ClearTimer(AttackEndTimerHandle);
	GetWorldTimerManager().ClearTimer(MeleeHitTimerHandle);

	Super::EndPlay(EndPlayReason);
}

void ARAEnemyBase::OnDetectionSphereBeginOverlap(
	UPrimitiveComponent* OverlappedComponent,
	AActor* OtherActor,
	UPrimitiveComponent* OtherComp,
	int32 OtherBodyIndex,
	bool bFromSweep,
	const FHitResult& SweepResult)
{
	if (bIsDead)
		return;

	ARACharacter* PlayerCharacter = Cast<ARACharacter>(OtherActor);
	if (PlayerCharacter)
	{
		SetTargetActor(PlayerCharacter);
		UE_LOG(LogTemp, Warning, TEXT("[%s] Detected Player: %s"), *GetName(), *OtherActor->GetName());
	}
}

void ARAEnemyBase::OnDetectionSphereEndOverlap(
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

void ARAEnemyBase::UpdateChase()
{
	if (bIsDead)
		return;

	if (!HasValidTarget())
	{
		UpdateCampWander();
		return;
	}

	if (bIsAttackMovementLocked)
	{
		if (AAIController* AIController = Cast<AAIController>(GetController()))
		{
			AIController->StopMovement();
		}
		return;
	}

	if (AttackType == EEnemyAttackType::Bow)
	{
		UpdateBowSpacing();
		return;
	}

	AAIController* AIController = Cast<AAIController>(GetController());
	if (!AIController)
		return;

	const float DistanceToTarget = FVector::Dist(GetActorLocation(), TargetActor->GetActorLocation());

	if (DistanceToTarget > GetAttackStartRange())
	{
		StopHitMontage();
		const EPathFollowingRequestResult::Type MoveResult =
			AIController->MoveToActor(TargetActor, GetChaseAcceptanceRadius(), false);
		bWantsMovementThisTick = MoveResult != EPathFollowingRequestResult::Failed;
	}
	else
		AIController->StopMovement();
}

void ARAEnemyBase::UpdateBowSpacing()
{
	AAIController* AIController = Cast<AAIController>(GetController());
	if (!AIController)
		return;

	const FVector CurrentLocation = GetActorLocation();
	const FVector TargetLocation = TargetActor->GetActorLocation();
	const float DistanceToTarget = FVector::Dist(CurrentLocation, TargetLocation);

	const float TooFarDistance = BowPreferredDistance + BowDistanceTolerance;
	const float TooCloseDistance = FMath::Max(0.f, BowPreferredDistance - BowDistanceTolerance);

	if (DistanceToTarget > TooFarDistance)
	{
		const EPathFollowingRequestResult::Type MoveResult =
			AIController->MoveToActor(TargetActor, BowPreferredDistance, false);
		bWantsMovementThisTick = MoveResult != EPathFollowingRequestResult::Failed;
		return;
	}

	if (DistanceToTarget < TooCloseDistance)
	{
		const FVector AwayDirection = (CurrentLocation - TargetLocation).GetSafeNormal();
		if (!AwayDirection.IsNearlyZero())
		{
			const FVector RetreatLocation = CurrentLocation + AwayDirection * BowRetreatStepDistance;
			const EPathFollowingRequestResult::Type MoveResult =
				AIController->MoveToLocation(RetreatLocation, BowMoveAcceptanceRadius);
			bWantsMovementThisTick = MoveResult != EPathFollowingRequestResult::Failed;
			return;
		}
	}

	AIController->StopMovement();
}

void ARAEnemyBase::SetTargetActor(AActor* NewTarget)
{
	if (!IsValidCombatTarget(NewTarget))
	{
		return;
	}

	TargetActor = NewTarget;
}

void ARAEnemyBase::ClearTargetActor()
{
	TargetActor = nullptr;

	if (AAIController* AIController = Cast<AAIController>(GetController()))
		AIController->StopMovement();
}

bool ARAEnemyBase::HasValidTarget() const
{
	return IsValidCombatTarget(TargetActor);
}

bool ARAEnemyBase::CanAttack() const
{
	if (bIsDead)
		return false;

	if (bIsAttacking)
		return false;

	if (!HasValidTarget())
		return false;

	const float DistanceToTarget = FVector::Dist(GetActorLocation(), TargetActor->GetActorLocation());
	if (DistanceToTarget > GetAttackStartRange())
		return false;

	if (AttackType == EEnemyAttackType::Bow)
	{
		const float TooFarDistance = BowPreferredDistance + BowDistanceTolerance;
		if (DistanceToTarget > TooFarDistance)
			return false;
	}

	return true;
}

void ARAEnemyBase::SetCampPatrolArea(const FVector& InCenter, float InRadius)
{
	CampPatrolCenter = InCenter;
	CampPatrolRadius = FMath::Max(0.0f, InRadius);
	bUseCampPatrolArea = CampPatrolRadius > 0.0f;
	LastCampWanderTime = -1000.0f;
}

void ARAEnemyBase::ClearCampPatrolArea()
{
	bUseCampPatrolArea = false;
	CampPatrolCenter = FVector::ZeroVector;
	CampPatrolRadius = 0.0f;
	LastCampWanderTime = -1000.0f;
}

void ARAEnemyBase::UpdateCampWander()
{
	if (bIsDead || bIsAttacking || bIsAttackMovementLocked)
	{
		return;
	}

	if (!bUseCampPatrolArea || CampPatrolRadius <= 0.0f || !GetWorld())
	{
		return;
	}

	const float CurrentTime = GetWorld()->GetTimeSeconds();
	if (CurrentTime - LastCampWanderTime < CampWanderInterval)
	{
		return;
	}

	LastCampWanderTime = CurrentTime;
	MoveToRandomCampLocation();
}

void ARAEnemyBase::MoveToRandomCampLocation()
{
	if (!GetWorld())
	{
		return;
	}

	AAIController* AIController = Cast<AAIController>(GetController());
	if (!AIController)
	{
		return;
	}

	UNavigationSystemV1* NavSystem = UNavigationSystemV1::GetCurrent(GetWorld());
	if (!NavSystem)
	{
		return;
	}

	FNavLocation RandomLocation;
	const bool bFoundLocation = NavSystem->GetRandomReachablePointInRadius(
		CampPatrolCenter,
		CampPatrolRadius,
		RandomLocation
	);

	if (!bFoundLocation)
	{
		return;
	}

	const EPathFollowingRequestResult::Type MoveResult =
		AIController->MoveToLocation(RandomLocation.Location, CampWanderAcceptanceRadius);
	bWantsMovementThisTick = MoveResult != EPathFollowingRequestResult::Failed;
}

bool ARAEnemyBase::IsValidCombatTarget(const AActor* InTargetActor) const
{
	return IsValid(InTargetActor) && InTargetActor->IsA<ARACharacter>();
}

ARACharacter* ARAEnemyBase::ResolvePlayerFromDamage(AController* EventInstigator, AActor* DamageCauser) const
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

float ARAEnemyBase::GetAttackStartRange() const
{
	return AttackRange + FMath::Max(0.f, AttackStartRangePadding);
}

float ARAEnemyBase::GetAttackHitRange() const
{
	return AttackRange + FMath::Max(0.f, AttackHitRangePadding);
}

float ARAEnemyBase::GetChaseAcceptanceRadius() const
{
	return FMath::Max(10.f, AttackRange * 0.25f);
}

void ARAEnemyBase::UpdateMovementStuckCheck(float DeltaTime)
{
	if (!bUseStuckRecovery || bIsDead || bIsAttacking || bIsAttackMovementLocked)
	{
		StuckTime = 0.f;
		return;
	}

	AAIController* AIController = Cast<AAIController>(GetController());
	if (!AIController)
	{
		StuckTime = 0.f;
		return;
	}

	const bool bHasActiveMove =
		bWantsMovementThisTick ||
		AIController->GetMoveStatus() == EPathFollowingStatus::Moving;

	if (!bHasActiveMove)
	{
		StuckTime = 0.f;
		return;
	}

	if (GetVelocity().Size2D() > StuckVelocityThreshold)
	{
		StuckTime = 0.f;
		return;
	}

	StuckTime += DeltaTime;
	if (StuckTime < StuckTimeThreshold)
	{
		return;
	}

	const float CurrentTime = GetWorld() ? GetWorld()->GetTimeSeconds() : 0.f;
	if (CurrentTime - LastStuckRecoveryTime < StuckRecoveryCooldown)
	{
		return;
	}

	LastStuckRecoveryTime = CurrentTime;
	StuckTime = 0.f;
	HandleMovementStuck();
}

void ARAEnemyBase::HandleMovementStuck()
{
	if (AAIController* AIController = Cast<AAIController>(GetController()))
	{
		AIController->StopMovement();
	}

	if (HasValidTarget())
	{
		if (TryMoveToStrafeLocationAroundTarget())
		{
			return;
		}
	}

	MoveToRandomCampLocation();
}

bool ARAEnemyBase::TryMoveToStrafeLocationAroundTarget()
{
	if (!HasValidTarget() || !GetWorld())
	{
		return false;
	}

	AAIController* AIController = Cast<AAIController>(GetController());
	UNavigationSystemV1* NavSystem = UNavigationSystemV1::GetCurrent(GetWorld());
	if (!AIController || !NavSystem)
	{
		return false;
	}

	const FVector CurrentLocation = GetActorLocation();
	FVector ToTarget = (TargetActor->GetActorLocation() - CurrentLocation).GetSafeNormal2D();
	if (ToTarget.IsNearlyZero())
	{
		ToTarget = GetActorForwardVector();
	}

	const FVector RightVector(-ToTarget.Y, ToTarget.X, 0.f);
	const int32 FirstSign = FMath::RandBool() ? 1 : -1;
	const int32 Signs[2] = { FirstSign, -FirstSign };

	for (const int32 Sign : Signs)
	{
		const FVector CandidateLocation =
			CurrentLocation +
			RightVector * static_cast<float>(Sign) * StuckSideStepDistance +
			ToTarget * 80.f;

		FNavLocation ProjectedLocation;
		if (!NavSystem->ProjectPointToNavigation(CandidateLocation, ProjectedLocation, FVector(150.f, 150.f, 200.f)))
		{
			continue;
		}

		const EPathFollowingRequestResult::Type MoveResult =
			AIController->MoveToLocation(ProjectedLocation.Location, GetChaseAcceptanceRadius());
		bWantsMovementThisTick = MoveResult != EPathFollowingRequestResult::Failed;
		if (bWantsMovementThisTick)
		{
			return true;
		}
	}

	return false;
}

void ARAEnemyBase::ApplySeparationFromNearbyEnemies(float DeltaTime)
{
	if (!bUseEnemySeparation || bIsDead || bIsAttacking || bIsAttackMovementLocked || EnemySeparationRadius <= 0.f)
	{
		return;
	}

	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	TArray<FOverlapResult> OverlapResults;
	FCollisionQueryParams QueryParams;
	QueryParams.AddIgnoredActor(this);

	FCollisionObjectQueryParams ObjectQueryParams;
	ObjectQueryParams.AddObjectTypesToQuery(ECC_Pawn);

	const FCollisionShape SeparationShape = FCollisionShape::MakeSphere(EnemySeparationRadius);
	if (!World->OverlapMultiByObjectType(
		OverlapResults,
		GetActorLocation(),
		FQuat::Identity,
		ObjectQueryParams,
		SeparationShape,
		QueryParams))
	{
		return;
	}

	FVector SeparationDirection = FVector::ZeroVector;
	const FVector CurrentLocation = GetActorLocation();

	for (const FOverlapResult& OverlapResult : OverlapResults)
	{
		const ARAEnemyBase* OtherEnemy = Cast<ARAEnemyBase>(OverlapResult.GetActor());
		if (!OtherEnemy || OtherEnemy == this || OtherEnemy->bIsDead)
		{
			continue;
		}

		FVector AwayDirection = CurrentLocation - OtherEnemy->GetActorLocation();
		AwayDirection.Z = 0.f;

		const float Distance = AwayDirection.Size();
		if (Distance <= KINDA_SMALL_NUMBER)
		{
			AwayDirection = GetActorRightVector();
		}
		else
		{
			AwayDirection /= Distance;
		}

		const float Weight = FMath::Clamp((EnemySeparationRadius - Distance) / EnemySeparationRadius, 0.f, 1.f);
		SeparationDirection += AwayDirection * Weight;
	}

	if (SeparationDirection.IsNearlyZero())
	{
		return;
	}

	const float ScaledStrength = EnemySeparationStrength * FMath::Clamp(DeltaTime * 60.f, 0.25f, 1.5f);
	AddMovementInput(SeparationDirection.GetSafeNormal(), ScaledStrength);
	bWantsMovementThisTick = true;
}

void ARAEnemyBase::UpdateAttack()
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

	FaceTargetActor();
	PerformAttack();
}

void ARAEnemyBase::EquipDefaultWeapon()
{
	if (!EnemyWeaponClass || CurrentWeapon)
		return;

	FActorSpawnParameters SpawnParams;
	SpawnParams.Owner = this;
	SpawnParams.Instigator = this;

	AWeaponBase* SpawnedWeapon = GetWorld()->SpawnActor<AWeaponBase>(
		EnemyWeaponClass,
		GetActorTransform(),
		SpawnParams
	);
	if (!SpawnedWeapon)
	{
		UE_LOG(LogTemp, Warning, TEXT("[%s] Failed to spawn enemy weapon"), *GetName());
		return;
	}

	EquipWeapon(SpawnedWeapon);
}

void ARAEnemyBase::EquipWeapon(AWeaponBase* NewWeapon)
{
	if (!NewWeapon || !GetMesh())
		return;

	if (CurrentWeapon && CurrentWeapon != NewWeapon)
	{
		CurrentWeapon->Destroy();
	}

	CurrentWeapon = NewWeapon;
	CurrentWeapon->SetOwner(this);
	CurrentWeapon->SetInstigator(this);
	CurrentWeapon->SetPickupEnabled(false);
	CurrentWeapon->UpdateWeaponVisualState();

	if (CurrentWeapon->WeaponMesh)
	{
		CurrentWeapon->WeaponMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		CurrentWeapon->WeaponMesh->SetGenerateOverlapEvents(false);
		CurrentWeapon->WeaponMesh->SetSimulatePhysics(false);
	}

	if (CurrentWeapon->WeaponSkeletalMesh)
	{
		CurrentWeapon->WeaponSkeletalMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		CurrentWeapon->WeaponSkeletalMesh->SetGenerateOverlapEvents(false);
		CurrentWeapon->WeaponSkeletalMesh->SetSimulatePhysics(false);
	}

	const FName AttachSocketName =
		(CurrentWeapon->WeaponType == EWeaponType::Bow)
		? LeftWeaponSocketName : RightWeaponSocketName;

	CurrentWeapon->AttachToComponent(
		GetMesh(),
		FAttachmentTransformRules::SnapToTargetNotIncludingScale,
		AttachSocketName
	);

	if (USceneComponent* ActiveVisual = CurrentWeapon->GetActiveVisualComponent())
	{
		ActiveVisual->SetRelativeLocation(CurrentWeapon->EquipRelativeLocation);
		ActiveVisual->SetRelativeRotation(CurrentWeapon->EquipRelativeRotation);
		ActiveVisual->SetRelativeScale3D(CurrentWeapon->EquipRelativeScale);
	}

	SyncCombatDataFromWeapon();

	UE_LOG(LogTemp, Warning, TEXT("[%s] Equipped Enemy Weapon: %s | Socket: %s"),
		*GetName(),
		*CurrentWeapon->GetName(),
		*AttachSocketName.ToString());
}

void ARAEnemyBase::SyncCombatDataFromWeapon()
{
	if (!CurrentWeapon || !bUseEquippedWeaponCombatData)
		return;

	AttackDamage = CurrentWeapon->AttackDamage;

	if (CurrentWeapon->AttackMontage)
	{
		AttackMontage = CurrentWeapon->AttackMontage;
	}

	switch (CurrentWeapon->WeaponType)
	{
	case EWeaponType::Sword:
		AttackType = EEnemyAttackType::Sword;
		if (CurrentWeapon->AttackMontage)
		{
			SwordAttackMontage = CurrentWeapon->AttackMontage;
		}
		break;
	case EWeaponType::Bow:
		AttackType = EEnemyAttackType::Bow;
		AttackRange = BowAttackRange;
		if (CurrentWeapon->AttackMontage)
		{
			BowAttackMontage = CurrentWeapon->AttackMontage;
		}
		if (CurrentWeapon->ProjectileClass)
		{
			BowProjectileClass = CurrentWeapon->ProjectileClass;
		}
		BowProjectileSpeed = CurrentWeapon->ProjectileSpeed;
		break;
	default:
		break;
	}
}

void ARAEnemyBase::PerformAttack()
{
	switch (AttackType)
	{
	case EEnemyAttackType::Sword:
		PerformSwordAttack();
		break;
	case EEnemyAttackType::Bow:
		PerformBowAttack();
		break;
	case EEnemyAttackType::Punch:
	default:
		PerformPunchAttack();
		break;
	}
}

void ARAEnemyBase::PerformPunchAttack()
{
	bMeleeDamageAppliedThisAttack = false;

	if (!PlayAttackMontage(AttackMontage))
	{
		UE_LOG(LogTemp, Warning, TEXT("[%s] Punch AttackMontage is missing"), *GetName());
		EndAttack();
		return;
	}

	GetWorldTimerManager().ClearTimer(MeleeHitTimerHandle);
	GetWorldTimerManager().SetTimer(
		MeleeHitTimerHandle,
		this,
		&ARAEnemyBase::ApplyDamageToTarget,
		FMath::Max(0.05f, PunchHitDelay),
		false
	);

	ScheduleAttackEnd(FMath::Max(AttackEndFallbackDelay, PunchHitDelay + 0.05f));
}

void ARAEnemyBase::PerformSwordAttack()
{
	bMeleeDamageAppliedThisAttack = false;

	UAnimMontage* MontageToPlay = SwordAttackMontage ? SwordAttackMontage : AttackMontage;

	if (!MontageToPlay || !GetMesh() || !GetMesh()->GetAnimInstance())
	{
		UE_LOG(LogTemp, Warning, TEXT("[%s] Sword AttackMontage is missing"), *GetName());
		EndAttack();
		return;
	}

	StopHitMontage();

	UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance();
	const float MontageDuration = AnimInstance->Montage_Play(MontageToPlay);
	if (MontageDuration <= 0.f)
	{
		EndAttack();
		return;
	}

	SetAttackMovementLocked(true);

	GetWorldTimerManager().ClearTimer(MeleeHitTimerHandle);
	GetWorldTimerManager().SetTimer(
		MeleeHitTimerHandle,
		this,
		&ARAEnemyBase::ApplyDamageToTarget,
		FMath::Max(0.05f, SwordHitDelay),
		false
	);

	ScheduleAttackEnd(FMath::Max(MontageDuration, SwordHitDelay + 0.05f));
}

void ARAEnemyBase::PerformBowAttack()
{
	bBowArrowFiredThisAttack = false;
	bIsBowCharging = true;
	if (GetCharacterMovement())
	{
		GetCharacterMovement()->MaxWalkSpeed = BowChargingMoveSpeed;
	}
	FaceTargetActor();

	UAnimMontage* MontageToPlay = BowAttackMontage ? BowAttackMontage : AttackMontage;

	if (!MontageToPlay || !GetMesh() || !GetMesh()->GetAnimInstance())
	{
		UE_LOG(LogTemp, Warning, TEXT("[%s] Bow AttackMontage is missing"), *GetName());
		EndAttack();
		return;
	}

	StopHitMontage();

	UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance();
	if (AnimInstance->Montage_Play(MontageToPlay) <= 0.f)
	{
		EndAttack();
		return;
	}

	AnimInstance->Montage_JumpToSection(FName("Drawing"), MontageToPlay);
	PlayBowWeaponMontageSection(FName("Default"));

	if (BowDrawSound)
	{
		UGameplayStatics::PlaySoundAtLocation(
			this,
			BowDrawSound,
			GetActorLocation()
		);
	}

	GetWorldTimerManager().ClearTimer(BowFireTimerHandle);
	GetWorldTimerManager().SetTimer(
		BowFireTimerHandle,
		this,
		&ARAEnemyBase::ReleaseBowChargeAtTarget,
		BowFullChargeTime,
		false
	);
}

bool ARAEnemyBase::PlayAttackMontage(UAnimMontage* MontageToPlay)
{
	if (!MontageToPlay || !GetMesh() || !GetMesh()->GetAnimInstance())
		return false;

	StopHitMontage();

	UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance();
	return AnimInstance->Montage_Play(MontageToPlay) > 0.f;
}

void ARAEnemyBase::ScheduleAttackEnd(float Delay)
{
	GetWorldTimerManager().ClearTimer(AttackEndTimerHandle);
	GetWorldTimerManager().SetTimer(
		AttackEndTimerHandle,
		this,
		&ARAEnemyBase::EndAttack,
		FMath::Max(0.05f, Delay),
		false
	);
}

void ARAEnemyBase::ReleaseBowChargeAtTarget()
{
	if (bIsDead || !HasValidTarget())
	{
		EndAttack();
		return;
	}

	bIsBowCharging = false;
	if (GetCharacterMovement())
	{
		GetCharacterMovement()->MaxWalkSpeed = MoveSpeed;
	}
	SetAttackMovementLocked(true);
	FaceTargetActor();

	UAnimMontage* MontageToPlay = BowAttackMontage ? BowAttackMontage : AttackMontage;
	if (MontageToPlay && GetMesh() && GetMesh()->GetAnimInstance())
	{
		UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance();
		AnimInstance->Montage_Play(MontageToPlay);
		AnimInstance->Montage_JumpToSection(FName("Releasing"), MontageToPlay);
	}

	PlayBowWeaponMontageSection(FName("Release"));
	FireArrowAtTarget();
	ScheduleAttackEnd(BowReleaseEndDelay);
}

void ARAEnemyBase::PlayBowWeaponMontageSection(FName SectionName)
{
	if (!CurrentWeapon || !CurrentWeapon->UsesSkeletalMesh())
		return;

	if (!CurrentWeapon->WeaponSkeletalMesh || !CurrentWeapon->WeaponAnimMontage)
		return;

	UAnimInstance* WeaponAnimInstance = CurrentWeapon->WeaponSkeletalMesh->GetAnimInstance();
	if (!WeaponAnimInstance)
	{
		UE_LOG(LogTemp, Warning, TEXT("[%s] Bow weapon anim instance is missing"), *GetName());
		return;
	}

	WeaponAnimInstance->Montage_Play(CurrentWeapon->WeaponAnimMontage);
	WeaponAnimInstance->Montage_JumpToSection(SectionName, CurrentWeapon->WeaponAnimMontage);
}

void ARAEnemyBase::FaceTargetActor()
{
	if (!HasValidTarget())
		return;

	const FVector ToTarget = TargetActor->GetActorLocation() - GetActorLocation();
	const FVector FlatDirection(ToTarget.X, ToTarget.Y, 0.f);
	if (FlatDirection.IsNearlyZero())
		return;

	SetActorRotation(FlatDirection.Rotation());
}

void ARAEnemyBase::SetAttackMovementLocked(bool bLocked)
{
	bIsAttackMovementLocked = bLocked;

	if (bLocked)
	{
		if (AAIController* AIController = Cast<AAIController>(GetController()))
		{
			AIController->StopMovement();
		}
	}
}

void ARAEnemyBase::PlayMeleeHitEffects(const FVector& HitLocation)
{
	switch (AttackType)
	{
	case EEnemyAttackType::Sword:
		SpawnHitVFX(
			SwordHitVFX,
			HitLocation,
			GetActorRotation(),
			SwordHitColor,
			SwordHitScale,
			SwordHitLifetime
		);

		if (SwordHitSound)
		{
			UGameplayStatics::PlaySoundAtLocation(
				this,
				SwordHitSound,
				HitLocation
			);
		}
		break;
	case EEnemyAttackType::Punch:
	default:
		SpawnHitVFX(
			PunchHitVFX,
			HitLocation,
			GetActorRotation(),
			PunchHitColor,
			PunchHitScale,
			PunchHitLifetime
		);

		if (PunchHitSounds.Num() > 0)
		{
			const int32 SoundIndex = FMath::RandRange(0, PunchHitSounds.Num() - 1);
			USoundBase* SelectedHitSound = PunchHitSounds[SoundIndex];
			if (SelectedHitSound)
			{
				UGameplayStatics::PlaySoundAtLocation(
					this,
					SelectedHitSound,
					HitLocation
				);
			}
		}
		break;
	}
}

void ARAEnemyBase::SpawnHitVFX(
	UNiagaraSystem* NiagaraSystem,
	const FVector& SpawnLocation,
	const FRotator& SpawnRotation,
	const FLinearColor& Color,
	float Scale,
	float Lifetime)
{
	if (!NiagaraSystem)
		return;

	UNiagaraComponent* NiagaraComp = UNiagaraFunctionLibrary::SpawnSystemAtLocation(
		GetWorld(),
		NiagaraSystem,
		SpawnLocation,
		SpawnRotation,
		FVector(1.0f),
		true,
		true,
		ENCPoolMethod::None,
		true
	);

	if (!NiagaraComp)
		return;

	NiagaraComp->SetVariableLinearColor(TEXT("Color"), Color);
	NiagaraComp->SetVariableFloat(TEXT("Scale"), Scale);
	NiagaraComp->SetVariableFloat(TEXT("Lifetime"), Lifetime);
}

void ARAEnemyBase::EndAttack()
{
	if (AttackType == EEnemyAttackType::Bow &&
		!bBowArrowFiredThisAttack &&
		GetWorldTimerManager().IsTimerActive(BowFireTimerHandle))
	{
		return;
	}

	GetWorldTimerManager().ClearTimer(BowFireTimerHandle);
	GetWorldTimerManager().ClearTimer(AttackEndTimerHandle);
	GetWorldTimerManager().ClearTimer(MeleeHitTimerHandle);
	bIsAttacking = false;
	bIsBowCharging = false;
	if (GetCharacterMovement())
	{
		GetCharacterMovement()->MaxWalkSpeed = MoveSpeed;
	}
	SetAttackMovementLocked(false);
	bMeleeDamageAppliedThisAttack = false;
}

void ARAEnemyBase::ApplyDamageToTarget()
{
	if (bIsDead || !HasValidTarget())
		return;

	if (AttackType == EEnemyAttackType::Bow)
	{
		const float CurrentTime = GetWorld()->GetTimeSeconds();
		if (CurrentTime - LastAttackTime >= BowFullChargeTime)
		{
			FireArrowAtTarget();
		}
		return;
	}

	if (AttackType != EEnemyAttackType::Bow)
	{
		if (bMeleeDamageAppliedThisAttack)
			return;
	}

	const float DistanceToTarget = FVector::Dist(GetActorLocation(), TargetActor->GetActorLocation());
	if (DistanceToTarget > GetAttackHitRange())
		return;

	if (const ARACharacter* PlayerCharacter = Cast<ARACharacter>(TargetActor))
	{
		if (PlayerCharacter->IsDodging())
		{
			UE_LOG(LogTemp, Warning, TEXT("[%s] Attack missed because target is dodging."), *GetName());
			return;
		}
	}

	if (const AAnimalBase* Animal = Cast<AAnimalBase>(TargetActor))
	{
		if (Animal->IsTrapped())
		{
			UE_LOG(LogTemp, Warning, TEXT("[%s] Attack ignored because target animal is trapped."), *GetName());
			return;
		}
	}

	bMeleeDamageAppliedThisAttack = true;
	GetWorldTimerManager().ClearTimer(MeleeHitTimerHandle);

	const FVector HitLocation = TargetActor->GetActorLocation();
	PlayMeleeHitEffects(HitLocation);

	const float AppliedDamage = UGameplayStatics::ApplyDamage(
		TargetActor,
		AttackDamage,
		GetController(),
		this,
		UDamageType::StaticClass()
	);

	UE_LOG(LogTemp, Warning, TEXT("[%s] Requested %.1f damage to %s / Applied=%.1f"),
		*GetName(),
		AttackDamage,
		*TargetActor->GetName(),
		AppliedDamage);
}

void ARAEnemyBase::TriggerMeleeHit()
{
	ApplyDamageToTarget();
}

void ARAEnemyBase::FireArrowAtTarget()
{
	if (bIsDead || !HasValidTarget())
		return;

	if (bBowArrowFiredThisAttack)
		return;

	GetWorldTimerManager().ClearTimer(BowFireTimerHandle);

	if (!BowProjectileClass)
	{
		UE_LOG(LogTemp, Warning, TEXT("[%s] BowProjectileClass is missing"), *GetName());
		return;
	}

	bBowArrowFiredThisAttack = true;
	FaceTargetActor();

	FVector SpawnLocation = GetActorLocation() + GetActorForwardVector() * 50.f + FVector(0.f, 0.f, 50.f);
	if (CurrentWeapon &&
		CurrentWeapon->UsesSkeletalMesh() &&
		CurrentWeapon->WeaponSkeletalMesh &&
		CurrentWeapon->WeaponSkeletalMesh->DoesSocketExist(ArrowSpawnSocketName))
	{
		SpawnLocation = CurrentWeapon->WeaponSkeletalMesh->GetSocketLocation(ArrowSpawnSocketName);
	}
	else if (CurrentWeapon &&
		CurrentWeapon->WeaponMesh &&
		CurrentWeapon->WeaponMesh->DoesSocketExist(ArrowSpawnSocketName))
	{
		SpawnLocation = CurrentWeapon->WeaponMesh->GetSocketLocation(ArrowSpawnSocketName);
	}
	else if (GetMesh() && GetMesh()->DoesSocketExist(ArrowSpawnSocketName))
	{
		SpawnLocation = GetMesh()->GetSocketLocation(ArrowSpawnSocketName);
	}

	const FVector TargetLocation = TargetActor->GetActorLocation() + FVector(0.f, 0.f, 50.f);
	const FVector ShootDirection = (TargetLocation - SpawnLocation).GetSafeNormal();
	if (ShootDirection.IsNearlyZero())
		return;

	FActorSpawnParameters SpawnParams;
	SpawnParams.Owner = this;
	SpawnParams.Instigator = this;

	AArrowProjectile* Arrow = GetWorld()->SpawnActor<AArrowProjectile>(
		BowProjectileClass,
		SpawnLocation,
		ShootDirection.Rotation(),
		SpawnParams
	);

	if (!Arrow)
		return;

	Arrow->Damage = AttackDamage;

	if (Arrow->ProjectileMovement)
	{
		Arrow->ProjectileMovement->InitialSpeed = BowProjectileSpeed;
		Arrow->ProjectileMovement->MaxSpeed = BowProjectileSpeed;
		Arrow->ProjectileMovement->Velocity = ShootDirection * BowProjectileSpeed;
	}

	UE_LOG(LogTemp, Warning, TEXT("[%s] Fired arrow at %s"),
		*GetName(),
		*TargetActor->GetName());
}

void ARAEnemyBase::UpdateHPBar()
{
	if (!HPBarWidgetComponent)
	{
		return;
	}

	UUserWidget* UserWidget = HPBarWidgetComponent->GetUserWidgetObject();
	if (!UserWidget)
	{
		return;
	}

	UEnemyHPBarWidget* HPBarWidget = Cast<UEnemyHPBarWidget>(UserWidget);
	if (!HPBarWidget)
	{
		return;
	}

	const float HPPercent = (MaxHP > 0.f) ? (CurrentHP / MaxHP) : 0.f;
	HPBarWidget->SetHPPercent(HPPercent);
}

void ARAEnemyBase::UpdateHPBarVisibility()
{
	if (!HPBarWidgetComponent)
	{
		return;
	}

	ARACharacter* PlayerCharacter = Cast<ARACharacter>(UGameplayStatics::GetPlayerCharacter(this, 0));
	if (!PlayerCharacter)
	{
		return;
	}

	const float Distance = FVector::Dist(GetActorLocation(), PlayerCharacter->GetActorLocation());
	const bool bShouldShow = Distance <= 1200.f;

	HPBarWidgetComponent->SetVisibility(bShouldShow);
}

void ARAEnemyBase::Die()
{
	GrantEXPToKiller();
	SpawnDropItems();

#pragma region Runtime World State
	for (TActorIterator<ARAWorldStateManager> It(GetWorld()); It; ++It)
	{
		It->NotifyEnemyDefeated(ActorSaveID);
		break;
	}
#pragma endregion Runtime World State

	Super::Die();
}

void ARAEnemyBase::GrantEXPToKiller()
{
	if (bHasGrantedEXP || EXPReward <= 0)
	{
		return;
	}

	ARACharacter* PlayerCharacter = nullptr;

	if (LastDamageInstigator)
	{
		PlayerCharacter = Cast<ARACharacter>(LastDamageInstigator->GetPawn());
	}

	if (!PlayerCharacter && LastDamageCauser)
	{
		PlayerCharacter = Cast<ARACharacter>(LastDamageCauser);
	}

	if (!PlayerCharacter && LastDamageCauser)
	{
		PlayerCharacter = Cast<ARACharacter>(LastDamageCauser->GetOwner());
	}

	if (!PlayerCharacter && LastDamageCauser)
	{
		PlayerCharacter = Cast<ARACharacter>(LastDamageCauser->GetInstigator());
	}

	if (!PlayerCharacter)
	{
		return;
	}

	UPlayerStatComponent* PlayerStatComponent = PlayerCharacter->FindComponentByClass<UPlayerStatComponent>();
	if (!PlayerStatComponent)
	{
		UE_LOG(LogTemp, Warning, TEXT("[%s] Failed to grant EXP: PlayerStatComponent is missing"), *GetName());
		return;
	}

	bHasGrantedEXP = true;
	PlayerStatComponent->AddEXP(EXPReward);

	UE_LOG(LogTemp, Warning, TEXT("[%s] Granted %d EXP to %s"),
		*GetName(),
		EXPReward,
		*PlayerCharacter->GetName());
}

void ARAEnemyBase::SpawnDropItems()
{
	if (bHasDroppedItems)
	{
		return;
	}

	bHasDroppedItems = true;

	if (DropItems.Num() <= 0)
	{
		return;
	}

	if (!DropItemActorClass)
	{
		UE_LOG(LogTemp, Warning, TEXT("[%s] DropItemActorClass is missing"), *GetName());
		return;
	}

	UWorld* World = GetWorld();
	if (!World)
	{
		UE_LOG(LogTemp, Warning, TEXT("[%s] Failed to spawn drop items: World is null"), *GetName());
		return;
	}

#pragma region Runtime Spawned Drop Items
	ARAWorldStateManager* WorldStateManager = nullptr;
	for (TActorIterator<ARAWorldStateManager> It(World); It; ++It)
	{
		WorldStateManager = *It;
		break;
	}

	int32 SpawnedDropNumber = 0;
#pragma endregion Runtime Spawned Drop Items

	for (const FDropItemData& DropItemData : DropItems)
	{
		if (DropItemData.ItemID.IsNone())
		{
			UE_LOG(LogTemp, Warning, TEXT("[%s] Skipped drop item: ItemID is None"), *GetName());
			continue;
		}

		const float SafeDropRate = FMath::Clamp(DropItemData.DropRate, 0.0f, 1.0f);
		if (FMath::FRand() > SafeDropRate)
		{
			continue;
		}

		const int32 SafeMinCount = FMath::Max(1, DropItemData.MinCount);
		const int32 SafeMaxCount = FMath::Max(SafeMinCount, DropItemData.MaxCount);
		const int32 DropCount = FMath::RandRange(SafeMinCount, SafeMaxCount);

		const FVector RandomOffset(
			FMath::FRandRange(-80.f, 80.f),
			FMath::FRandRange(-80.f, 80.f),
			30.f
		);
		const FVector SpawnLocation = GetActorLocation() + RandomOffset;
		const FRotator SpawnRotation = FRotator::ZeroRotator;
		const FTransform SpawnTransform(SpawnRotation, SpawnLocation);

		FActorSpawnParameters SpawnParams;
		SpawnParams.Owner = this;
		SpawnParams.Instigator = this;
		SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;

		ADropItemActor* DropItemActor = World->SpawnActorDeferred<ADropItemActor>(
			DropItemActorClass,
			SpawnTransform,
			SpawnParams.Owner,
			SpawnParams.Instigator,
			SpawnParams.SpawnCollisionHandlingOverride
		);

		if (!DropItemActor)
		{
			UE_LOG(LogTemp, Warning, TEXT("[%s] Failed to spawn drop item: %s"),
				*GetName(),
				*DropItemData.ItemID.ToString());
			continue;
		}

#pragma region Runtime Spawned Drop Items
		++SpawnedDropNumber;
		const FName RuntimeItemSaveID(*FString::Printf(
			TEXT("%s_%s_%d"),
			*GetName(),
			*DropItemData.ItemID.ToString(),
			SpawnedDropNumber
		));

		DropItemActor->InitializeRuntimeDropItem(
			DropItemData.ItemID,
			DropCount,
			RuntimeItemSaveID
		);

		if (WorldStateManager && !WorldStateManager->MapID.IsNone())
		{
			if (URAGameInstance* RAGameInstance = World->GetGameInstance<URAGameInstance>())
			{
				FSpawnedDropItemRuntimeData RuntimeDropData;
				RuntimeDropData.ItemSaveID = RuntimeItemSaveID;
				RuntimeDropData.ItemID = DropItemData.ItemID;
				RuntimeDropData.Count = DropCount;
				RuntimeDropData.SpawnTransform = SpawnTransform;
				RuntimeDropData.DropItemActorClass = DropItemActor->GetClass();

				RAGameInstance->RegisterSpawnedDropItem(WorldStateManager->MapID, RuntimeDropData);
			}
		}
		else
		{
			UE_LOG(LogTemp, Warning, TEXT("[%s] Runtime drop state was not registered: WorldStateManager or MapID is missing. ItemSaveID=%s"),
				*GetName(),
				*RuntimeItemSaveID.ToString());
		}
#pragma endregion Runtime Spawned Drop Items

		UGameplayStatics::FinishSpawningActor(DropItemActor, SpawnTransform);

		UE_LOG(LogTemp, Warning, TEXT("[%s] Spawned drop item: %s x%d / ItemSaveID=%s"),
			*GetName(),
			*DropItemData.ItemID.ToString(),
			DropCount,
			*RuntimeItemSaveID.ToString());
	}
}
