#include "TPSEnemyBase.h"
#include "Components/SphereComponent.h"
#include "TPSCaptureCharacter.h"
#include "ArrowProjectile.h"
#include "WeaponBase.h"

#include "AIController.h"
#include "Animation/AnimInstance.h"
#include "Components/SceneComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "GameFramework/ProjectileMovementComponent.h"
#include "Kismet/GameplayStatics.h"
#include "TimerManager.h"

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

	EquipDefaultWeapon();
}

void ATPSEnemyBase::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	UpdateChase();

	if (AttackType == EEnemyAttackType::Bow && bIsBowCharging)
	{
		FaceTargetActor();
	}

	UpdateAttack();
}

void ATPSEnemyBase::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (CurrentWeapon)
	{
		CurrentWeapon->Destroy();
		CurrentWeapon = nullptr;
	}

	GetWorldTimerManager().ClearTimer(BowFireTimerHandle);
	GetWorldTimerManager().ClearTimer(AttackEndTimerHandle);

	Super::EndPlay(EndPlayReason);
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
	{
		StopHitMontage();
		AIController->MoveToActor(TargetActor, AttackRange);
	}
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

	PerformAttack();
}

void ATPSEnemyBase::EquipDefaultWeapon()
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

void ATPSEnemyBase::EquipWeapon(AWeaponBase* NewWeapon)
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

void ATPSEnemyBase::SyncCombatDataFromWeapon()
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

void ATPSEnemyBase::PerformAttack()
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

void ATPSEnemyBase::PerformPunchAttack()
{
	if (!PlayAttackMontage(AttackMontage))
	{
		UE_LOG(LogTemp, Warning, TEXT("[%s] Punch AttackMontage is missing"), *GetName());
		EndAttack();
		return;
	}

	ScheduleAttackEnd(AttackEndFallbackDelay);
}

void ATPSEnemyBase::PerformSwordAttack()
{
	UAnimMontage* MontageToPlay = SwordAttackMontage ? SwordAttackMontage : AttackMontage;

	if (!PlayAttackMontage(MontageToPlay))
	{
		UE_LOG(LogTemp, Warning, TEXT("[%s] Sword AttackMontage is missing"), *GetName());
		EndAttack();
		return;
	}

	ScheduleAttackEnd(AttackEndFallbackDelay);
}

void ATPSEnemyBase::PerformBowAttack()
{
	bBowArrowFiredThisAttack = false;
	bIsBowCharging = true;
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

	GetWorldTimerManager().ClearTimer(BowFireTimerHandle);
	GetWorldTimerManager().SetTimer(
		BowFireTimerHandle,
		this,
		&ATPSEnemyBase::ReleaseBowChargeAtTarget,
		BowFullChargeTime,
		false
	);
}

bool ATPSEnemyBase::PlayAttackMontage(UAnimMontage* MontageToPlay)
{
	if (!MontageToPlay || !GetMesh() || !GetMesh()->GetAnimInstance())
		return false;

	StopHitMontage();

	UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance();
	return AnimInstance->Montage_Play(MontageToPlay) > 0.f;
}

void ATPSEnemyBase::ScheduleAttackEnd(float Delay)
{
	GetWorldTimerManager().ClearTimer(AttackEndTimerHandle);
	GetWorldTimerManager().SetTimer(
		AttackEndTimerHandle,
		this,
		&ATPSEnemyBase::EndAttack,
		FMath::Max(0.05f, Delay),
		false
	);
}

void ATPSEnemyBase::ReleaseBowChargeAtTarget()
{
	if (bIsDead || !HasValidTarget())
	{
		EndAttack();
		return;
	}

	bIsBowCharging = false;
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

void ATPSEnemyBase::PlayBowWeaponMontageSection(FName SectionName)
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

void ATPSEnemyBase::FaceTargetActor()
{
	if (!HasValidTarget())
		return;

	const FVector ToTarget = TargetActor->GetActorLocation() - GetActorLocation();
	const FVector FlatDirection(ToTarget.X, ToTarget.Y, 0.f);
	if (FlatDirection.IsNearlyZero())
		return;

	SetActorRotation(FlatDirection.Rotation());
}

void ATPSEnemyBase::EndAttack()
{
	if (AttackType == EEnemyAttackType::Bow &&
		!bBowArrowFiredThisAttack &&
		GetWorldTimerManager().IsTimerActive(BowFireTimerHandle))
	{
		return;
	}

	GetWorldTimerManager().ClearTimer(BowFireTimerHandle);
	GetWorldTimerManager().ClearTimer(AttackEndTimerHandle);
	bIsAttacking = false;
	bIsBowCharging = false;
}

void ATPSEnemyBase::ApplyDamageToTarget()
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

void ATPSEnemyBase::FireArrowAtTarget()
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
