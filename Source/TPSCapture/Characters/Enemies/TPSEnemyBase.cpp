#include "TPSEnemyBase.h"
#include "Components/SphereComponent.h"
#include "DropItemActor.h"
#include "TPSCaptureCharacter.h"
#include "ArrowProjectile.h"
#include "WeaponBase.h"
#include "EnemyHPBarWidget.h"

#include "AIController.h"
#include "Animation/AnimInstance.h"
#include "Components/SceneComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/WidgetComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/ProjectileMovementComponent.h"
#include "Kismet/GameplayStatics.h"
#include "NiagaraComponent.h"
#include "NiagaraFunctionLibrary.h"
#include "Sound/SoundBase.h"
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

	HPBarWidgetComponent = CreateDefaultSubobject<UWidgetComponent>(TEXT("HPBarWidgetComponent"));
	HPBarWidgetComponent->SetupAttachment(GetRootComponent());
	HPBarWidgetComponent->SetRelativeLocation(FVector(0.f, 0.f, 100.f));
	HPBarWidgetComponent->SetWidgetSpace(EWidgetSpace::Screen);
	HPBarWidgetComponent->SetDrawSize(FVector2D(100.f, 20.f));
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

	if (GetCharacterMovement())
	{
		GetCharacterMovement()->MaxWalkSpeed = MoveSpeed;
	}

	EquipDefaultWeapon();
	UpdateHPBar();
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
	UpdateHPBarVisibility();
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
	GetWorldTimerManager().ClearTimer(SwordHitTimerHandle);

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

	if (DistanceToTarget > AttackRange)
	{
		StopHitMontage();
		AIController->MoveToActor(TargetActor, AttackRange);
	}
	else
		AIController->StopMovement();
}

void ATPSEnemyBase::UpdateBowSpacing()
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
		AIController->MoveToActor(TargetActor, BowPreferredDistance);
		return;
	}

	if (DistanceToTarget < TooCloseDistance)
	{
		const FVector AwayDirection = (CurrentLocation - TargetLocation).GetSafeNormal();
		if (!AwayDirection.IsNearlyZero())
		{
			const FVector RetreatLocation = CurrentLocation + AwayDirection * BowRetreatStepDistance;
			AIController->MoveToLocation(RetreatLocation, BowMoveAcceptanceRadius);
			return;
		}
	}

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

	if (AttackType == EEnemyAttackType::Bow)
	{
		const float TooFarDistance = BowPreferredDistance + BowDistanceTolerance;
		if (DistanceToTarget > TooFarDistance)
			return false;
	}

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
	bSwordDamageAppliedThisAttack = false;

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

	GetWorldTimerManager().ClearTimer(SwordHitTimerHandle);
	GetWorldTimerManager().SetTimer(
		SwordHitTimerHandle,
		this,
		&ATPSEnemyBase::ApplyDamageToTarget,
		FMath::Max(0.05f, SwordHitDelay),
		false
	);

	ScheduleAttackEnd(MontageDuration);
}

void ATPSEnemyBase::PerformBowAttack()
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

void ATPSEnemyBase::SetAttackMovementLocked(bool bLocked)
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

void ATPSEnemyBase::PlayMeleeHitEffects(const FVector& HitLocation)
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

void ATPSEnemyBase::SpawnHitVFX(
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
	GetWorldTimerManager().ClearTimer(SwordHitTimerHandle);
	bIsAttacking = false;
	bIsBowCharging = false;
	if (GetCharacterMovement())
	{
		GetCharacterMovement()->MaxWalkSpeed = MoveSpeed;
	}
	SetAttackMovementLocked(false);
	bSwordDamageAppliedThisAttack = false;
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

	if (AttackType == EEnemyAttackType::Sword)
	{
		if (bSwordDamageAppliedThisAttack)
			return;

		bSwordDamageAppliedThisAttack = true;
		GetWorldTimerManager().ClearTimer(SwordHitTimerHandle);
	}

	const float DistanceToTarget = FVector::Dist(GetActorLocation(), TargetActor->GetActorLocation());
	if (DistanceToTarget > AttackRange)
		return;

	const FVector HitLocation = TargetActor->GetActorLocation();
	PlayMeleeHitEffects(HitLocation);

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

void ATPSEnemyBase::TriggerMeleeHit()
{
	ApplyDamageToTarget();
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

void ATPSEnemyBase::UpdateHPBar()
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

void ATPSEnemyBase::UpdateHPBarVisibility()
{
	if (!HPBarWidgetComponent)
	{
		return;
	}

	ATPSCaptureCharacter* PlayerCharacter = Cast<ATPSCaptureCharacter>(UGameplayStatics::GetPlayerCharacter(this, 0));
	if (!PlayerCharacter)
	{
		return;
	}

	const float Distance = FVector::Dist(GetActorLocation(), PlayerCharacter->GetActorLocation());
	const bool bShouldShow = Distance <= 1200.f;

	HPBarWidgetComponent->SetVisibility(bShouldShow);
}

void ATPSEnemyBase::Die()
{
	SpawnDropItems();
	Super::Die();
}

void ATPSEnemyBase::SpawnDropItems()
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

		FActorSpawnParameters SpawnParams;
		SpawnParams.Owner = this;
		SpawnParams.Instigator = this;
		SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;

		ADropItemActor* DropItemActor = World->SpawnActor<ADropItemActor>(
			DropItemActorClass,
			SpawnLocation,
			SpawnRotation,
			SpawnParams
		);

		if (!DropItemActor)
		{
			UE_LOG(LogTemp, Warning, TEXT("[%s] Failed to spawn drop item: %s"),
				*GetName(),
				*DropItemData.ItemID.ToString());
			continue;
		}

		DropItemActor->InitializeDropItem(DropItemData.ItemID, DropCount);

		UE_LOG(LogTemp, Warning, TEXT("[%s] Spawned drop item: %s x%d"),
			*GetName(),
			*DropItemData.ItemID.ToString(),
			DropCount);
	}
}
