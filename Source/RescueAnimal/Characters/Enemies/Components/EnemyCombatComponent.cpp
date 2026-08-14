#include "EnemyCombatComponent.h"

#include "RAEnemyBase.h"
#include "EnemyAIComponent.h"
#include "AnimalBase.h"
#include "ArrowProjectile.h"
#include "RACharacter.h"
#include "WeaponBase.h"

#include "AIController.h"
#include "Animation/AnimInstance.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/ProjectileMovementComponent.h"
#include "Kismet/GameplayStatics.h"
#include "NiagaraComponent.h"
#include "NiagaraFunctionLibrary.h"
#include "Sound/SoundBase.h"
#include "TimerManager.h"

UEnemyCombatComponent::UEnemyCombatComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
}

void UEnemyCombatComponent::BeginPlay()
{
	Super::BeginPlay();
}

void UEnemyCombatComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	ARAEnemyBase* Enemy = GetOwnerEnemy();
	if (!Enemy)
		return;

	if (Enemy->AttackType == EEnemyAttackType::Bow && Enemy->bIsBowCharging)
	{
		FaceTargetActor();
	}

	UpdateAttack();
}

void UEnemyCombatComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (ARAEnemyBase* Enemy = GetOwnerEnemy())
	{
		Enemy->GetWorldTimerManager().ClearTimer(Enemy->BowFireTimerHandle);
		Enemy->GetWorldTimerManager().ClearTimer(Enemy->AttackEndTimerHandle);
		Enemy->GetWorldTimerManager().ClearTimer(Enemy->MeleeHitTimerHandle);
	}

	Super::EndPlay(EndPlayReason);
}

ARAEnemyBase* UEnemyCombatComponent::GetOwnerEnemy() const
{
	return Cast<ARAEnemyBase>(GetOwner());
}

bool UEnemyCombatComponent::IsBowCharging() const
{
	const ARAEnemyBase* Enemy = GetOwnerEnemy();
	return Enemy && Enemy->bIsBowCharging;
}

bool UEnemyCombatComponent::CanAttack() const
{
	const ARAEnemyBase* Enemy = GetOwnerEnemy();
	if (!Enemy || Enemy->bIsDead || Enemy->bIsAttacking)
		return false;

	if (!Enemy->EnemyAIComponent || !Enemy->EnemyAIComponent->HasValidTarget())
		return false;

	const float DistanceToTarget = FVector::Dist(Enemy->GetActorLocation(), Enemy->TargetActor->GetActorLocation());
	if (DistanceToTarget > Enemy->EnemyAIComponent->GetAttackStartRange())
		return false;

	if (Enemy->AttackType == EEnemyAttackType::Bow)
	{
		const float TooFarDistance = Enemy->BowPreferredDistance + Enemy->BowDistanceTolerance;
		if (DistanceToTarget > TooFarDistance)
			return false;
	}

	return true;
}

void UEnemyCombatComponent::UpdateAttack()
{
	ARAEnemyBase* Enemy = GetOwnerEnemy();
	if (!Enemy || !CanAttack())
		return;

	const float CurrentTime = Enemy->GetWorld()->GetTimeSeconds();
	if (CurrentTime - Enemy->LastAttackTime < Enemy->AttackCooldown)
		return;

	Enemy->bIsAttacking = true;
	Enemy->LastAttackTime = CurrentTime;

	if (AAIController* AIController = Cast<AAIController>(Enemy->GetController()))
	{
		AIController->StopMovement();
	}

	FaceTargetActor();
	PerformAttack();
}

void UEnemyCombatComponent::PerformAttack()
{
	ARAEnemyBase* Enemy = GetOwnerEnemy();
	if (!Enemy)
		return;

	switch (Enemy->AttackType)
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

void UEnemyCombatComponent::PerformPunchAttack()
{
	ARAEnemyBase* Enemy = GetOwnerEnemy();
	if (!Enemy)
		return;

	Enemy->bMeleeDamageAppliedThisAttack = false;

	if (!PlayAttackMontage(Enemy->AttackMontage))
	{
		UE_LOG(LogTemp, Warning, TEXT("[%s] Punch AttackMontage is missing"), *Enemy->GetName());
		EndAttack();
		return;
	}

	Enemy->GetWorldTimerManager().ClearTimer(Enemy->MeleeHitTimerHandle);
	Enemy->GetWorldTimerManager().SetTimer(
		Enemy->MeleeHitTimerHandle,
		this,
		&UEnemyCombatComponent::ApplyDamageToTarget,
		FMath::Max(0.05f, Enemy->PunchHitDelay),
		false
	);

	ScheduleAttackEnd(FMath::Max(Enemy->AttackEndFallbackDelay, Enemy->PunchHitDelay + 0.05f));
}

void UEnemyCombatComponent::PerformSwordAttack()
{
	ARAEnemyBase* Enemy = GetOwnerEnemy();
	if (!Enemy)
		return;

	Enemy->bMeleeDamageAppliedThisAttack = false;

	UAnimMontage* MontageToPlay = Enemy->SwordAttackMontage ? Enemy->SwordAttackMontage : Enemy->AttackMontage;
	if (!MontageToPlay || !Enemy->GetMesh() || !Enemy->GetMesh()->GetAnimInstance())
	{
		UE_LOG(LogTemp, Warning, TEXT("[%s] Sword AttackMontage is missing"), *Enemy->GetName());
		EndAttack();
		return;
	}

	Enemy->StopHitMontage();

	UAnimInstance* AnimInstance = Enemy->GetMesh()->GetAnimInstance();
	const float MontageDuration = AnimInstance->Montage_Play(MontageToPlay);
	if (MontageDuration <= 0.f)
	{
		EndAttack();
		return;
	}

	SetAttackMovementLocked(true);

	Enemy->GetWorldTimerManager().ClearTimer(Enemy->MeleeHitTimerHandle);
	Enemy->GetWorldTimerManager().SetTimer(
		Enemy->MeleeHitTimerHandle,
		this,
		&UEnemyCombatComponent::ApplyDamageToTarget,
		FMath::Max(0.05f, Enemy->SwordHitDelay),
		false
	);

	ScheduleAttackEnd(FMath::Max(MontageDuration, Enemy->SwordHitDelay + 0.05f));
}

void UEnemyCombatComponent::PerformBowAttack()
{
	ARAEnemyBase* Enemy = GetOwnerEnemy();
	if (!Enemy)
		return;

	Enemy->bBowArrowFiredThisAttack = false;
	Enemy->bIsBowCharging = true;
	if (Enemy->GetCharacterMovement())
	{
		Enemy->GetCharacterMovement()->MaxWalkSpeed = Enemy->BowChargingMoveSpeed;
	}
	FaceTargetActor();

	UAnimMontage* MontageToPlay = Enemy->BowAttackMontage ? Enemy->BowAttackMontage : Enemy->AttackMontage;
	if (!MontageToPlay || !Enemy->GetMesh() || !Enemy->GetMesh()->GetAnimInstance())
	{
		UE_LOG(LogTemp, Warning, TEXT("[%s] Bow AttackMontage is missing"), *Enemy->GetName());
		EndAttack();
		return;
	}

	Enemy->StopHitMontage();

	UAnimInstance* AnimInstance = Enemy->GetMesh()->GetAnimInstance();
	if (AnimInstance->Montage_Play(MontageToPlay) <= 0.f)
	{
		EndAttack();
		return;
	}

	AnimInstance->Montage_JumpToSection(FName("Drawing"), MontageToPlay);
	PlayBowWeaponMontageSection(FName("Default"));

	if (Enemy->BowDrawSound)
	{
		UGameplayStatics::PlaySoundAtLocation(Enemy, Enemy->BowDrawSound, Enemy->GetActorLocation());
	}

	Enemy->GetWorldTimerManager().ClearTimer(Enemy->BowFireTimerHandle);
	Enemy->GetWorldTimerManager().SetTimer(
		Enemy->BowFireTimerHandle,
		this,
		&UEnemyCombatComponent::ReleaseBowChargeAtTarget,
		Enemy->BowFullChargeTime,
		false
	);
}

bool UEnemyCombatComponent::PlayAttackMontage(UAnimMontage* MontageToPlay)
{
	ARAEnemyBase* Enemy = GetOwnerEnemy();
	if (!Enemy || !MontageToPlay || !Enemy->GetMesh() || !Enemy->GetMesh()->GetAnimInstance())
		return false;

	Enemy->StopHitMontage();

	UAnimInstance* AnimInstance = Enemy->GetMesh()->GetAnimInstance();
	return AnimInstance->Montage_Play(MontageToPlay) > 0.f;
}

void UEnemyCombatComponent::ScheduleAttackEnd(float Delay)
{
	ARAEnemyBase* Enemy = GetOwnerEnemy();
	if (!Enemy)
		return;

	Enemy->GetWorldTimerManager().ClearTimer(Enemy->AttackEndTimerHandle);
	Enemy->GetWorldTimerManager().SetTimer(
		Enemy->AttackEndTimerHandle,
		this,
		&UEnemyCombatComponent::EndAttack,
		FMath::Max(0.05f, Delay),
		false
	);
}

void UEnemyCombatComponent::ReleaseBowChargeAtTarget()
{
	ARAEnemyBase* Enemy = GetOwnerEnemy();
	if (!Enemy || Enemy->bIsDead || !Enemy->EnemyAIComponent || !Enemy->EnemyAIComponent->HasValidTarget())
	{
		EndAttack();
		return;
	}

	Enemy->bIsBowCharging = false;
	if (Enemy->GetCharacterMovement())
	{
		Enemy->GetCharacterMovement()->MaxWalkSpeed = Enemy->MoveSpeed;
	}
	SetAttackMovementLocked(true);
	FaceTargetActor();

	UAnimMontage* MontageToPlay = Enemy->BowAttackMontage ? Enemy->BowAttackMontage : Enemy->AttackMontage;
	if (MontageToPlay && Enemy->GetMesh() && Enemy->GetMesh()->GetAnimInstance())
	{
		UAnimInstance* AnimInstance = Enemy->GetMesh()->GetAnimInstance();
		AnimInstance->Montage_Play(MontageToPlay);
		AnimInstance->Montage_JumpToSection(FName("Releasing"), MontageToPlay);
	}

	PlayBowWeaponMontageSection(FName("Release"));
	FireArrowAtTarget();
	ScheduleAttackEnd(Enemy->BowReleaseEndDelay);
}

void UEnemyCombatComponent::PlayBowWeaponMontageSection(FName SectionName)
{
	ARAEnemyBase* Enemy = GetOwnerEnemy();
	if (!Enemy || !Enemy->CurrentWeapon || !Enemy->CurrentWeapon->UsesSkeletalMesh())
		return;

	if (!Enemy->CurrentWeapon->WeaponSkeletalMesh || !Enemy->CurrentWeapon->WeaponAnimMontage)
		return;

	UAnimInstance* WeaponAnimInstance = Enemy->CurrentWeapon->WeaponSkeletalMesh->GetAnimInstance();
	if (!WeaponAnimInstance)
	{
		UE_LOG(LogTemp, Warning, TEXT("[%s] Bow weapon anim instance is missing"), *Enemy->GetName());
		return;
	}

	WeaponAnimInstance->Montage_Play(Enemy->CurrentWeapon->WeaponAnimMontage);
	WeaponAnimInstance->Montage_JumpToSection(SectionName, Enemy->CurrentWeapon->WeaponAnimMontage);
}

void UEnemyCombatComponent::FaceTargetActor()
{
	ARAEnemyBase* Enemy = GetOwnerEnemy();
	if (!Enemy || !Enemy->EnemyAIComponent || !Enemy->EnemyAIComponent->HasValidTarget())
		return;

	const FVector ToTarget = Enemy->TargetActor->GetActorLocation() - Enemy->GetActorLocation();
	const FVector FlatDirection(ToTarget.X, ToTarget.Y, 0.f);
	if (FlatDirection.IsNearlyZero())
		return;

	Enemy->SetActorRotation(FlatDirection.Rotation());
}

void UEnemyCombatComponent::SetAttackMovementLocked(bool bLocked)
{
	ARAEnemyBase* Enemy = GetOwnerEnemy();
	if (!Enemy)
		return;

	Enemy->bIsAttackMovementLocked = bLocked;

	if (bLocked)
	{
		if (AAIController* AIController = Cast<AAIController>(Enemy->GetController()))
		{
			AIController->StopMovement();
		}
	}
}

void UEnemyCombatComponent::PlayMeleeHitEffects(const FVector& HitLocation)
{
	ARAEnemyBase* Enemy = GetOwnerEnemy();
	if (!Enemy)
		return;

	switch (Enemy->AttackType)
	{
	case EEnemyAttackType::Sword:
		SpawnHitVFX(Enemy->SwordHitVFX, HitLocation, Enemy->GetActorRotation(), Enemy->SwordHitColor, Enemy->SwordHitScale, Enemy->SwordHitLifetime);

		if (Enemy->SwordHitSound)
		{
			UGameplayStatics::PlaySoundAtLocation(Enemy, Enemy->SwordHitSound, HitLocation);
		}
		break;
	case EEnemyAttackType::Punch:
	default:
		SpawnHitVFX(Enemy->PunchHitVFX, HitLocation, Enemy->GetActorRotation(), Enemy->PunchHitColor, Enemy->PunchHitScale, Enemy->PunchHitLifetime);

		if (Enemy->PunchHitSounds.Num() > 0)
		{
			const int32 SoundIndex = FMath::RandRange(0, Enemy->PunchHitSounds.Num() - 1);
			USoundBase* SelectedHitSound = Enemy->PunchHitSounds[SoundIndex];
			if (SelectedHitSound)
			{
				UGameplayStatics::PlaySoundAtLocation(Enemy, SelectedHitSound, HitLocation);
			}
		}
		break;
	}
}

void UEnemyCombatComponent::SpawnHitVFX(UNiagaraSystem* NiagaraSystem, const FVector& SpawnLocation, const FRotator& SpawnRotation, const FLinearColor& Color, float Scale, float Lifetime)
{
	ARAEnemyBase* Enemy = GetOwnerEnemy();
	if (!Enemy || !NiagaraSystem)
		return;

	UNiagaraComponent* NiagaraComp = UNiagaraFunctionLibrary::SpawnSystemAtLocation(
		Enemy->GetWorld(),
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

void UEnemyCombatComponent::EndAttack()
{
	ARAEnemyBase* Enemy = GetOwnerEnemy();
	if (!Enemy)
		return;

	if (Enemy->AttackType == EEnemyAttackType::Bow &&
		!Enemy->bBowArrowFiredThisAttack &&
		Enemy->GetWorldTimerManager().IsTimerActive(Enemy->BowFireTimerHandle))
	{
		return;
	}

	Enemy->GetWorldTimerManager().ClearTimer(Enemy->BowFireTimerHandle);
	Enemy->GetWorldTimerManager().ClearTimer(Enemy->AttackEndTimerHandle);
	Enemy->GetWorldTimerManager().ClearTimer(Enemy->MeleeHitTimerHandle);
	Enemy->bIsAttacking = false;
	Enemy->bIsBowCharging = false;
	if (Enemy->GetCharacterMovement())
	{
		Enemy->GetCharacterMovement()->MaxWalkSpeed = Enemy->MoveSpeed;
	}
	SetAttackMovementLocked(false);
	Enemy->bMeleeDamageAppliedThisAttack = false;
}

void UEnemyCombatComponent::ApplyDamageToTarget()
{
	ARAEnemyBase* Enemy = GetOwnerEnemy();
	if (!Enemy || Enemy->bIsDead || !Enemy->EnemyAIComponent || !Enemy->EnemyAIComponent->HasValidTarget())
		return;

	if (Enemy->AttackType == EEnemyAttackType::Bow)
	{
		const float CurrentTime = Enemy->GetWorld()->GetTimeSeconds();
		if (CurrentTime - Enemy->LastAttackTime >= Enemy->BowFullChargeTime)
		{
			FireArrowAtTarget();
		}
		return;
	}

	if (Enemy->bMeleeDamageAppliedThisAttack)
		return;

	const float DistanceToTarget = FVector::Dist(Enemy->GetActorLocation(), Enemy->TargetActor->GetActorLocation());
	if (DistanceToTarget > Enemy->EnemyAIComponent->GetAttackHitRange())
		return;

	if (const ARACharacter* PlayerCharacter = Cast<ARACharacter>(Enemy->TargetActor))
	{
		if (PlayerCharacter->IsDodging())
		{
			UE_LOG(LogTemp, Warning, TEXT("[%s] Attack missed because target is dodging."), *Enemy->GetName());
			return;
		}
	}

	if (const AAnimalBase* Animal = Cast<AAnimalBase>(Enemy->TargetActor))
	{
		if (Animal->IsTrapped())
		{
			UE_LOG(LogTemp, Warning, TEXT("[%s] Attack ignored because target animal is trapped."), *Enemy->GetName());
			return;
		}
	}

	Enemy->bMeleeDamageAppliedThisAttack = true;
	Enemy->GetWorldTimerManager().ClearTimer(Enemy->MeleeHitTimerHandle);

	const FVector HitLocation = Enemy->TargetActor->GetActorLocation();
	PlayMeleeHitEffects(HitLocation);

	const float AppliedDamage = UGameplayStatics::ApplyDamage(
		Enemy->TargetActor,
		Enemy->AttackDamage,
		Enemy->GetController(),
		Enemy,
		UDamageType::StaticClass()
	);

	UE_LOG(LogTemp, Warning, TEXT("[%s] Requested %.1f damage to %s / Applied=%.1f"),
		*Enemy->GetName(),
		Enemy->AttackDamage,
		*Enemy->TargetActor->GetName(),
		AppliedDamage);
}

void UEnemyCombatComponent::TriggerMeleeHit()
{
	ApplyDamageToTarget();
}

void UEnemyCombatComponent::FireArrowAtTarget()
{
	ARAEnemyBase* Enemy = GetOwnerEnemy();
	if (!Enemy || Enemy->bIsDead || !Enemy->EnemyAIComponent || !Enemy->EnemyAIComponent->HasValidTarget())
		return;

	if (Enemy->bBowArrowFiredThisAttack)
		return;

	Enemy->GetWorldTimerManager().ClearTimer(Enemy->BowFireTimerHandle);

	if (!Enemy->BowProjectileClass)
	{
		UE_LOG(LogTemp, Warning, TEXT("[%s] BowProjectileClass is missing"), *Enemy->GetName());
		return;
	}

	Enemy->bBowArrowFiredThisAttack = true;
	FaceTargetActor();

	FVector SpawnLocation = Enemy->GetActorLocation() + Enemy->GetActorForwardVector() * 50.f + FVector(0.f, 0.f, 50.f);
	if (Enemy->CurrentWeapon &&
		Enemy->CurrentWeapon->UsesSkeletalMesh() &&
		Enemy->CurrentWeapon->WeaponSkeletalMesh &&
		Enemy->CurrentWeapon->WeaponSkeletalMesh->DoesSocketExist(Enemy->ArrowSpawnSocketName))
	{
		SpawnLocation = Enemy->CurrentWeapon->WeaponSkeletalMesh->GetSocketLocation(Enemy->ArrowSpawnSocketName);
	}
	else if (Enemy->CurrentWeapon &&
		Enemy->CurrentWeapon->WeaponMesh &&
		Enemy->CurrentWeapon->WeaponMesh->DoesSocketExist(Enemy->ArrowSpawnSocketName))
	{
		SpawnLocation = Enemy->CurrentWeapon->WeaponMesh->GetSocketLocation(Enemy->ArrowSpawnSocketName);
	}
	else if (Enemy->GetMesh() && Enemy->GetMesh()->DoesSocketExist(Enemy->ArrowSpawnSocketName))
	{
		SpawnLocation = Enemy->GetMesh()->GetSocketLocation(Enemy->ArrowSpawnSocketName);
	}

	const FVector TargetLocation = Enemy->TargetActor->GetActorLocation() + FVector(0.f, 0.f, 50.f);
	const FVector ShootDirection = (TargetLocation - SpawnLocation).GetSafeNormal();
	if (ShootDirection.IsNearlyZero())
		return;

	FActorSpawnParameters SpawnParams;
	SpawnParams.Owner = Enemy;
	SpawnParams.Instigator = Enemy;

	AArrowProjectile* Arrow = Enemy->GetWorld()->SpawnActor<AArrowProjectile>(
		Enemy->BowProjectileClass,
		SpawnLocation,
		ShootDirection.Rotation(),
		SpawnParams
	);

	if (!Arrow)
		return;

	Arrow->Damage = Enemy->AttackDamage;

	if (Arrow->ProjectileMovement)
	{
		Arrow->ProjectileMovement->InitialSpeed = Enemy->BowProjectileSpeed;
		Arrow->ProjectileMovement->MaxSpeed = Enemy->BowProjectileSpeed;
		Arrow->ProjectileMovement->Velocity = ShootDirection * Enemy->BowProjectileSpeed;
	}

	UE_LOG(LogTemp, Warning, TEXT("[%s] Fired arrow at %s"),
		*Enemy->GetName(),
		*Enemy->TargetActor->GetName());
}
