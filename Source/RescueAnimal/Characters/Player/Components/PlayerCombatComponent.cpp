#include "PlayerCombatComponent.h"

#include "RACharacter.h"
#include "ArrowProjectile.h"
#include "CrosshairBowWidget.h"
#include "InventoryComponent.h"
#include "PlayerMovementComponent.h"
#include "PlayerSkillComponent.h"
#include "PlayerStatComponent.h"
#include "PlayerUIFlowComponent.h"
#include "RAEnemyBase.h"
#include "RAPlayerController.h"
#include "WeaponBase.h"

#include "Animation/AnimInstance.h"
#include "Camera/CameraComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "DrawDebugHelpers.h"
#include "GameFramework/Controller.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/ProjectileMovementComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "Kismet/GameplayStatics.h"
#include "NiagaraComponent.h"
#include "NiagaraFunctionLibrary.h"
#include "Sound/SoundBase.h"

UPlayerCombatComponent::UPlayerCombatComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
}

void UPlayerCombatComponent::BeginPlay()
{
	Super::BeginPlay();
}

void UPlayerCombatComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	ARACharacter* Character = GetOwnerCharacter();
	if (!Character)
		return;

	if (Character->bIsBowCharging)
	{
		UpdateBowFacing(DeltaTime);

		const float CurrentTime = Character->GetWorld()->GetTimeSeconds();
		const float ChargeDuration = CurrentTime - Character->BowChargeStartTime;
		const float ClampedCharge = FMath::Clamp(ChargeDuration, Character->MinBowChargeTime, Character->MaxBowChargeTime);

		const float ChargeAlpha =
			(Character->MaxBowChargeTime > Character->MinBowChargeTime)
			? (ClampedCharge - Character->MinBowChargeTime) / (Character->MaxBowChargeTime - Character->MinBowChargeTime)
			: 1.0f;

		Character->CachedBowChargeAlpha = FMath::Clamp(ChargeAlpha, 0.0f, 1.0f);

		if (Character->CrosshairWidgetInstance)
		{
			Character->CrosshairWidgetInstance->SetChargeAlpha(Character->CachedBowChargeAlpha);

			if (Character->CachedBowChargeAlpha >= 1.0f)
			{
				Character->CrosshairWidgetInstance->PlayFullChargeEffect();
			}
		}
	}

	UpdateBowZoom(DeltaTime);
	UpdateBowCameraArm(DeltaTime);
}

ARACharacter* UPlayerCombatComponent::GetOwnerCharacter() const
{
	return Cast<ARACharacter>(GetOwner());
}

bool UPlayerCombatComponent::IsAttacking() const
{
	const ARACharacter* Character = GetOwnerCharacter();
	return Character && Character->bIsAttacking;
}

bool UPlayerCombatComponent::IsBowAiming() const
{
	const ARACharacter* Character = GetOwnerCharacter();
	return Character && Character->bIsBowAiming;
}

bool UPlayerCombatComponent::IsBowCharging() const
{
	const ARACharacter* Character = GetOwnerCharacter();
	return Character && Character->bIsBowCharging;
}

void UPlayerCombatComponent::Attack()
{
	ARACharacter* Character = GetOwnerCharacter();
	if (!Character || Character->IsPortalTransitionInputLocked())
		return;

	if (Character->StatComponent && Character->StatComponent->IsDead())
		return;

	if (Character->PlayerMovementComponent && Character->PlayerMovementComponent->IsDodging())
		return;

	if (Character->bIsAttacking && !Character->bIsPunching)
	{
		UE_LOG(LogTemp, Warning, TEXT("Already Attacking"));
		return;
	}

	Character->StopHitMontage();

	if (Character->CurrentWeapon)
		AttackWithWeapon();
	else
		AttackUnarmed();
}

void UPlayerCombatComponent::AttackUnarmed()
{
	ARACharacter* Character = GetOwnerCharacter();
	if (!Character)
		return;

	if (!Character->bIsPunching)
	{
		FaceAttackDirection();
		Character->bIsAttacking = true;
		StartComboAttack();
		return;
	}

	QueueComboInput();
}

void UPlayerCombatComponent::AttackWithWeapon()
{
	ARACharacter* Character = GetOwnerCharacter();
	if (!Character || !Character->CurrentWeapon)
	{
		UE_LOG(LogTemp, Warning, TEXT("AttackWithWeapon: No CurrentWeapon"));
		return;
	}

	FaceAttackDirection();
	Character->bIsAttacking = true;

	UE_LOG(LogTemp, Warning, TEXT("Weapon Attack: %s"), *Character->CurrentWeapon->GetName());

	if (Character->CurrentWeapon->AttackMontage && Character->GetMesh() && Character->GetMesh()->GetAnimInstance())
	{
		const float Duration = Character->GetMesh()->GetAnimInstance()->Montage_Play(Character->CurrentWeapon->AttackMontage);
		if (Duration > 0.0f)
		{
			FTimerHandle AttackEndTimerHandle;
			Character->GetWorldTimerManager().SetTimer(AttackEndTimerHandle, this, &UPlayerCombatComponent::EndAttack, Duration, false);
		}
		else
		{
			EndAttack();
		}
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("Weapon AttackMontage is missing"));
		EndAttack();
	}
}

void UPlayerCombatComponent::FaceAttackDirection()
{
	ARACharacter* Character = GetOwnerCharacter();
	if (!Character)
		return;

	AController* Controller = Character->GetController();
	if (!Controller)
		return;

	const FRotator ControlRot = Controller->GetControlRotation();
	const FRotator TargetRot(0.0f, ControlRot.Yaw, 0.0f);
	Character->SetActorRotation(TargetRot);
}

bool UPlayerCombatComponent::IsValidPlayerAttackTarget(const AActor* TargetActor) const
{
	const ARACharacter* Character = GetOwnerCharacter();
	return Character && TargetActor && TargetActor != Character && TargetActor->IsA<ARAEnemyBase>();
}

void UPlayerCombatComponent::EndAttack()
{
	ARACharacter* Character = GetOwnerCharacter();
	if (!Character)
		return;

	Character->bIsAttacking = false;
	Character->bIsBowCharging = false;
	UE_LOG(LogTemp, Warning, TEXT("Attack End"));
}

void UPlayerCombatComponent::FireArrow()
{
	ARACharacter* Character = GetOwnerCharacter();
	if (!Character || !Character->CurrentWeapon)
	{
		UE_LOG(LogTemp, Warning, TEXT("FireArrow: No CurrentWeapon"));
		return;
	}

	if (Character->CurrentWeapon->AttackType != EAttackType::Ranged || !Character->CurrentWeapon->ProjectileClass || !Character->GetMesh())
		return;

	if (!ConsumeArrowAmmo())
	{
		UE_LOG(LogTemp, Warning, TEXT("FireArrow failed: no Arrow ammo"));
		return;
	}

	const FVector SpawnLocation = Character->GetMesh()->GetSocketLocation(TEXT("ArrowSpawnSocket"));
	const FRotator SpawnRotation = Character->GetControlRotation();

	FActorSpawnParameters SpawnParams;
	SpawnParams.Owner = Character;
	SpawnParams.Instigator = Character;

	AArrowProjectile* Arrow = Character->GetWorld()->SpawnActor<AArrowProjectile>(
		Character->CurrentWeapon->ProjectileClass,
		SpawnLocation,
		SpawnRotation,
		SpawnParams
	);

	if (!Arrow)
	{
		RefundArrowAmmo();
		return;
	}

	Arrow->Damage = Character->CurrentWeapon->AttackDamage;

	if (Arrow->ProjectileMovement)
	{
		Arrow->ProjectileMovement->InitialSpeed = Character->CurrentWeapon->ProjectileSpeed;
		Arrow->ProjectileMovement->MaxSpeed = Character->CurrentWeapon->ProjectileSpeed;
	}

	UE_LOG(LogTemp, Warning, TEXT("Arrow Fired"));
}

void UPlayerCombatComponent::PerformPunchHit(float Damage, float Range, float Radius)
{
	ARACharacter* Character = GetOwnerCharacter();
	if (!Character || !Character->GetWorld())
		return;

	const FVector Start = Character->GetActorLocation() + FVector(0.f, 0.f, 50.f);
	const FVector End = Start + (Character->GetActorForwardVector() * Range);
	const FCollisionShape Sphere = FCollisionShape::MakeSphere(Radius);

	FCollisionQueryParams QueryParams;
	QueryParams.AddIgnoredActor(Character);

	FCollisionObjectQueryParams ObjectQueryParams;
	ObjectQueryParams.AddObjectTypesToQuery(ECC_Pawn);

	TArray<FHitResult> HitResults;
	Character->GetWorld()->SweepMultiByObjectType(HitResults, Start, End, FQuat::Identity, ObjectQueryParams, Sphere, QueryParams);

	if (Character->bDrawAttackDebug)
	{
		const bool bHitEnemy = HitResults.ContainsByPredicate([this](const FHitResult& HitResult)
		{
			return IsValidPlayerAttackTarget(HitResult.GetActor());
		});
		DrawDebugCapsule(
			Character->GetWorld(),
			(Start + End) * 0.5f,
			Range * 0.5f,
			Radius,
			FRotationMatrix::MakeFromX(End - Start).ToQuat(),
			bHitEnemy ? FColor::Red : FColor::Green,
			false,
			1.5f
		);
	}

	for (const FHitResult& HitResult : HitResults)
	{
		AActor* HitActor = HitResult.GetActor();
		if (!IsValidPlayerAttackTarget(HitActor))
		{
			UE_LOG(LogTemplateCharacter, Warning, TEXT("Punch ignored non-enemy hit: %s"), *GetNameSafe(HitActor));
			continue;
		}

		SpawnHitVFX(Character->PunchHitVFX, HitResult.ImpactPoint, Character->GetActorRotation(), Character->PunchHitColor, Character->PunchHitScale, Character->PunchHitLifetime);

		USoundBase* SelectedHitSound = nullptr;
		const int32 SoundIndex = Character->CurrentComboIndex - 1;
		if (Character->PunchHitSounds.IsValidIndex(SoundIndex))
		{
			SelectedHitSound = Character->PunchHitSounds[SoundIndex];
		}

		if (SelectedHitSound)
		{
			UGameplayStatics::PlaySoundAtLocation(Character, SelectedHitSound, HitResult.ImpactPoint);
		}

		UGameplayStatics::ApplyDamage(HitActor, Damage, Character->GetController(), Character, UDamageType::StaticClass());
		break;
	}
}

void UPlayerCombatComponent::StartComboAttack()
{
	ARACharacter* Character = GetOwnerCharacter();
	if (!Character || !Character->PunchMontage || !Character->GetMesh() || !Character->GetMesh()->GetAnimInstance())
		return;

	UAnimInstance* AnimInstance = Character->GetMesh()->GetAnimInstance();
	Character->bIsPunching = true;
	Character->bIsAttacking = true;
	Character->bComboInputBuffered = false;
	Character->bCanAcceptComboInput = false;
	Character->CurrentComboIndex = 1;

	AnimInstance->Montage_Play(Character->PunchMontage);
	AnimInstance->Montage_JumpToSection(FName("Combo1"), Character->PunchMontage);

	AnimInstance->OnMontageEnded.RemoveDynamic(this, &UPlayerCombatComponent::OnPunchMontageEnded);
	AnimInstance->OnMontageEnded.AddDynamic(this, &UPlayerCombatComponent::OnPunchMontageEnded);

	UE_LOG(LogTemplateCharacter, Warning, TEXT("Combo 1 Start"));
}

void UPlayerCombatComponent::QueueComboInput()
{
	ARACharacter* Character = GetOwnerCharacter();
	if (!Character || !Character->bIsPunching)
		return;

	if (!Character->bCanAcceptComboInput)
	{
		UE_LOG(LogTemplateCharacter, Warning, TEXT("Combo input ignored: not in combo window"));
		return;
	}

	if (Character->CurrentComboIndex >= Character->MaxComboCount)
	{
		UE_LOG(LogTemplateCharacter, Warning, TEXT("Already at max combo"));
		return;
	}

	Character->bComboInputBuffered = true;
	UE_LOG(LogTemplateCharacter, Warning, TEXT("Combo input buffered"));
}

void UPlayerCombatComponent::PerformSwordHit(float Damage, float Range, float Radius)
{
	ARACharacter* Character = GetOwnerCharacter();
	if (!Character || !Character->GetWorld())
		return;

	const FVector Start = Character->GetActorLocation() + FVector(0.f, 0.f, 50.f);
	const FVector End = Start + (Character->GetActorForwardVector() * Range);
	const FCollisionShape Sphere = FCollisionShape::MakeSphere(Radius);

	FCollisionQueryParams QueryParams;
	QueryParams.AddIgnoredActor(Character);

	FCollisionObjectQueryParams ObjectQueryParams;
	ObjectQueryParams.AddObjectTypesToQuery(ECC_Pawn);

	TArray<FHitResult> HitResults;
	Character->GetWorld()->SweepMultiByObjectType(HitResults, Start, End, FQuat::Identity, ObjectQueryParams, Sphere, QueryParams);

	if (Character->bDrawAttackDebug)
	{
		const bool bHitEnemy = HitResults.ContainsByPredicate([this](const FHitResult& HitResult)
		{
			return IsValidPlayerAttackTarget(HitResult.GetActor());
		});
		DrawDebugCapsule(
			Character->GetWorld(),
			(Start + End) * 0.5f,
			Range * 0.5f,
			Radius,
			FRotationMatrix::MakeFromX(End - Start).ToQuat(),
			bHitEnemy ? FColor::Red : FColor::Green,
			false,
			1.5f
		);
	}

	for (const FHitResult& HitResult : HitResults)
	{
		AActor* HitActor = HitResult.GetActor();
		if (!IsValidPlayerAttackTarget(HitActor))
		{
			UE_LOG(LogTemplateCharacter, Warning, TEXT("Sword ignored non-enemy hit: %s"), *GetNameSafe(HitActor));
			continue;
		}

		SpawnHitVFX(Character->SwordHitVFX, HitResult.ImpactPoint, Character->GetActorRotation(), Character->SwordHitColor, Character->SwordHitScale, Character->SwordHitLifetime);

		if (Character->SwordHitSound)
		{
			UGameplayStatics::PlaySoundAtLocation(Character, Character->SwordHitSound, HitResult.ImpactPoint);
		}

		UGameplayStatics::ApplyDamage(HitActor, Damage, Character->GetController(), Character, UDamageType::StaticClass());
		break;
	}
}

void UPlayerCombatComponent::OnAttackPressed()
{
	ARACharacter* Character = GetOwnerCharacter();
	if (!Character || Character->IsPortalTransitionInputLocked())
		return;

	if (Character->StatComponent && Character->StatComponent->IsDead())
		return;

	if (const ARAPlayerController* RAPlayerController = Cast<ARAPlayerController>(Character->GetController()))
	{
		const UPlayerUIFlowComponent* PlayerUIFlowComponent = RAPlayerController->GetPlayerUIFlowComponent();
		if (PlayerUIFlowComponent &&
			(PlayerUIFlowComponent->IsInventoryOpen() ||
				PlayerUIFlowComponent->IsShopOpen() ||
				PlayerUIFlowComponent->IsAnimalCollectionOpen()))
		{
			return;
		}
	}

	if (Character->PlayerMovementComponent && Character->PlayerMovementComponent->IsDodging())
		return;

	if (Character->CurrentWeapon && Character->CurrentWeapon->AttackType == EAttackType::Ranged)
	{
		if (!HasArrowAmmo())
		{
			UE_LOG(LogTemp, Warning, TEXT("Bow attack blocked: no Arrow ammo"));
			return;
		}

		StartBowCharge();
		return;
	}

	Attack();
}

void UPlayerCombatComponent::OnAttackReleased()
{
	ARACharacter* Character = GetOwnerCharacter();
	if (!Character || Character->IsPortalTransitionInputLocked())
		return;

	if (Character->StatComponent && Character->StatComponent->IsDead())
		return;

	if (const ARAPlayerController* RAPlayerController = Cast<ARAPlayerController>(Character->GetController()))
	{
		const UPlayerUIFlowComponent* PlayerUIFlowComponent = RAPlayerController->GetPlayerUIFlowComponent();
		if (PlayerUIFlowComponent && PlayerUIFlowComponent->IsShopOpen())
		{
			return;
		}
	}

	if (Character->bIsBowCharging)
	{
		ReleaseBowCharge();
	}
}

void UPlayerCombatComponent::StartBowCharge()
{
	ARACharacter* Character = GetOwnerCharacter();
	if (!Character || !Character->CurrentWeapon || Character->CurrentWeapon->AttackType != EAttackType::Ranged)
		return;

	if (!HasArrowAmmo())
	{
		UE_LOG(LogTemp, Warning, TEXT("StartBowCharge failed: no Arrow ammo"));
		return;
	}

	if (Character->bIsAttacking || Character->bIsBowCharging || Character->bIsBowAiming)
		return;

	if (!Character->CurrentWeapon->AttackMontage || !Character->GetMesh() || !Character->GetMesh()->GetAnimInstance())
		return;

	Character->bIsAttacking = true;
	Character->bIsBowCharging = true;
	Character->bIsBowAiming = true;
	Character->CachedBowChargeAlpha = 0.0f;
	Character->BowChargeStartTime = Character->GetWorld()->GetTimeSeconds();

	if (Character->PlayerSkillComponent)
	{
		Character->PlayerSkillComponent->CancelBowSkillPreparation();
	}

	Character->GetCharacterMovement()->MaxWalkSpeed = 100.f;

	if (Character->CrosshairWidgetInstance)
	{
		Character->CrosshairWidgetInstance->SetCrosshairVisible(true);
		Character->CrosshairWidgetInstance->SetChargeAlpha(0.0f);
	}

	UAnimInstance* AnimInstance = Character->GetMesh()->GetAnimInstance();
	AnimInstance->Montage_Play(Character->CurrentWeapon->AttackMontage);
	AnimInstance->Montage_JumpToSection(FName("Drawing"), Character->CurrentWeapon->AttackMontage);

	PlayBowWeaponMontageSection(FName("Default"));

	if (Character->BowDrawSound)
	{
		UGameplayStatics::PlaySoundAtLocation(Character, Character->BowDrawSound, Character->GetActorLocation());
	}

	ShowPreviewArrow();

	UE_LOG(LogTemp, Warning, TEXT("Bow Charge Start"));
}

void UPlayerCombatComponent::ReleaseBowCharge()
{
	ARACharacter* Character = GetOwnerCharacter();
	if (!Character || !Character->bIsBowCharging)
		return;

	if (!Character->CurrentWeapon || Character->CurrentWeapon->AttackType != EAttackType::Ranged || !Character->CurrentWeapon->AttackMontage || !Character->GetMesh() || !Character->GetMesh()->GetAnimInstance())
	{
		EndAttack();
		Character->bIsBowCharging = false;
		return;
	}

	Character->bIsBowCharging = false;
	if (Character->PlayerMovementComponent)
	{
		Character->PlayerMovementComponent->ApplyMovementStats();
	}

	const float CurrentTime = Character->GetWorld()->GetTimeSeconds();
	const float ChargeDuration = CurrentTime - Character->BowChargeStartTime;
	const float ClampedCharge = FMath::Clamp(ChargeDuration, Character->MinBowChargeTime, Character->MaxBowChargeTime);

	Character->CachedBowChargeAlpha =
		(Character->MaxBowChargeTime > Character->MinBowChargeTime)
		? (ClampedCharge - Character->MinBowChargeTime) / (Character->MaxBowChargeTime - Character->MinBowChargeTime)
		: 1.0f;

	Character->CachedBowChargeAlpha = FMath::Clamp(Character->CachedBowChargeAlpha, 0.0f, 1.0f);

	UAnimInstance* AnimInstance = Character->GetMesh()->GetAnimInstance();
	AnimInstance->Montage_Play(Character->CurrentWeapon->AttackMontage);
	AnimInstance->Montage_JumpToSection(FName("Releasing"), Character->CurrentWeapon->AttackMontage);

	PlayBowWeaponMontageSection(FName("Release"));

	UE_LOG(LogTemp, Warning, TEXT("Bow Charge Released | Alpha=%.2f"), Character->CachedBowChargeAlpha);
}

void UPlayerCombatComponent::UpdateBowFacing(float DeltaTime)
{
	FaceAttackDirection();
}

bool UPlayerCombatComponent::HasArrowAmmo() const
{
	const ARACharacter* Character = GetOwnerCharacter();
	return Character && Character->InventoryComponent && Character->InventoryComponent->HasItem(TEXT("Arrow"), 1);
}

bool UPlayerCombatComponent::ConsumeArrowAmmo()
{
	ARACharacter* Character = GetOwnerCharacter();
	return Character && Character->InventoryComponent && Character->InventoryComponent->RemoveItem(TEXT("Arrow"), 1);
}

void UPlayerCombatComponent::RefundArrowAmmo()
{
	ARACharacter* Character = GetOwnerCharacter();
	if (Character && Character->InventoryComponent)
	{
		Character->InventoryComponent->AddItem(TEXT("Arrow"), 1);
	}
}

void UPlayerCombatComponent::FireChargedArrow()
{
	ARACharacter* Character = GetOwnerCharacter();
	if (!Character || !Character->CurrentWeapon || Character->CurrentWeapon->AttackType != EAttackType::Ranged)
		return;

	TSubclassOf<AArrowProjectile> ProjectileClassToSpawn = Character->CurrentWeapon->ProjectileClass;
	const bool bUseBowSkillProjectile = Character->PlayerSkillComponent && Character->PlayerSkillComponent->IsBowSkillPrepared();
	if (bUseBowSkillProjectile)
	{
		ProjectileClassToSpawn = Character->PlayerSkillComponent->GetPreparedBowProjectileClass();
	}

	if (!ProjectileClassToSpawn)
	{
		if (Character->PlayerSkillComponent)
		{
			Character->PlayerSkillComponent->CancelBowSkillPreparation();
		}
		UE_LOG(LogTemp, Warning, TEXT("FireChargedArrow: ProjectileClassToSpawn is null"));
		return;
	}

	APlayerController* PC = Cast<APlayerController>(Character->GetController());
	if (!PC || !Character->GetMesh())
		return;

	HidePreviewArrow();

	int32 ViewportSizeX = 0;
	int32 ViewportSizeY = 0;
	PC->GetViewportSize(ViewportSizeX, ViewportSizeY);

	const FVector2D ScreenCenter(ViewportSizeX * 0.5f, ViewportSizeY * 0.5f);

	FVector WorldLocation;
	FVector WorldDirection;
	if (!PC->DeprojectScreenPositionToWorld(ScreenCenter.X, ScreenCenter.Y, WorldLocation, WorldDirection))
	{
		UE_LOG(LogTemp, Warning, TEXT("FireChargedArrow: Deproject failed"));
		return;
	}

	const FVector TraceStart = WorldLocation;
	const FVector TraceEnd = TraceStart + (WorldDirection * 10000.0f);

	FHitResult HitResult;
	FCollisionQueryParams QueryParams;
	QueryParams.AddIgnoredActor(Character);
	QueryParams.AddIgnoredActor(Character->CurrentWeapon);

	const bool bHit = Character->GetWorld()->LineTraceSingleByChannel(HitResult, TraceStart, TraceEnd, ECC_Visibility, QueryParams);
	const FVector AimTargetLocation = bHit ? HitResult.ImpactPoint : TraceEnd;

	FVector SpawnLocation = FVector::ZeroVector;
	if (Character->CurrentWeapon->UsesSkeletalMesh() &&
		Character->CurrentWeapon->WeaponSkeletalMesh &&
		Character->CurrentWeapon->WeaponSkeletalMesh->DoesSocketExist(TEXT("ArrowSpawnSocket")))
	{
		SpawnLocation = Character->CurrentWeapon->WeaponSkeletalMesh->GetSocketLocation(TEXT("ArrowSpawnSocket"));
	}
	else if (Character->CurrentWeapon->WeaponMesh && Character->CurrentWeapon->WeaponMesh->DoesSocketExist(TEXT("ArrowSpawnSocket")))
	{
		SpawnLocation = Character->CurrentWeapon->WeaponMesh->GetSocketLocation(TEXT("ArrowSpawnSocket"));
	}
	else if (Character->GetMesh()->DoesSocketExist(TEXT("LeftHandSocket")))
	{
		SpawnLocation = Character->GetMesh()->GetSocketLocation(TEXT("LeftHandSocket"));
	}
	else
	{
		SpawnLocation = Character->GetActorLocation() + Character->GetActorForwardVector() * 50.0f + FVector(0.0f, 0.0f, 50.0f);
		UE_LOG(LogTemp, Warning, TEXT("FireChargedArrow: LeftHandSocket not found, using fallback location"));
	}

	const FVector ShootDirection = (AimTargetLocation - SpawnLocation).GetSafeNormal();
	const FRotator SpawnRotation = ShootDirection.Rotation();

	if (!ConsumeArrowAmmo())
	{
		HidePreviewArrow();
		if (Character->PlayerSkillComponent)
		{
			Character->PlayerSkillComponent->CancelBowSkillPreparation();
		}
		UE_LOG(LogTemp, Warning, TEXT("FireChargedArrow failed: no Arrow ammo"));
		return;
	}

	FActorSpawnParameters SpawnParams;
	SpawnParams.Owner = Character;
	SpawnParams.Instigator = Character;

	AArrowProjectile* Arrow = Character->GetWorld()->SpawnActor<AArrowProjectile>(ProjectileClassToSpawn, SpawnLocation, SpawnRotation, SpawnParams);
	if (!Arrow)
	{
		RefundArrowAmmo();
		if (Character->PlayerSkillComponent)
		{
			Character->PlayerSkillComponent->CancelBowSkillPreparation();
		}
		return;
	}

	if (bUseBowSkillProjectile && Character->PlayerSkillComponent)
	{
		Character->PlayerSkillComponent->ApplyBowSkillHitEffects(Arrow);
		Character->PlayerSkillComponent->CommitBowSkillRelease();
	}

	const float DamageMultiplier = FMath::Lerp(Character->MinBowDamageMultiplier, Character->MaxBowDamageMultiplier, Character->CachedBowChargeAlpha);
	const float SpeedMultiplier = FMath::Lerp(Character->MinBowSpeedMultiplier, Character->MaxBowSpeedMultiplier, Character->CachedBowChargeAlpha);

	float FinalBaseDamage = Character->CurrentWeapon->AttackDamage;
	if (Character->StatComponent)
	{
		FinalBaseDamage = Character->StatComponent->GetFinalAttackPower(FinalBaseDamage);
	}

	Arrow->Damage = FinalBaseDamage * DamageMultiplier;

	if (Arrow->ProjectileMovement)
	{
		const float FinalSpeed = Character->CurrentWeapon->ProjectileSpeed * SpeedMultiplier;
		Arrow->ProjectileMovement->InitialSpeed = FinalSpeed;
		Arrow->ProjectileMovement->MaxSpeed = FinalSpeed;
		Arrow->ProjectileMovement->Velocity = ShootDirection * FinalSpeed;
	}

#if WITH_EDITOR
	if (Character->bDrawAttackDebug)
	{
		DrawDebugSphere(Character->GetWorld(), AimTargetLocation, 12.0f, 12, FColor::Red, false, 1.5f);
		DrawDebugLine(Character->GetWorld(), SpawnLocation, AimTargetLocation, FColor::Yellow, false, 1.5f, 0, 1.5f);
	}
#endif

	UE_LOG(LogTemp, Warning, TEXT("Charged Arrow Fired | Alpha=%.2f Damage=%.1f Spawn=%s"),
		Character->CachedBowChargeAlpha,
		Arrow->Damage,
		*SpawnLocation.ToString());
}

void UPlayerCombatComponent::UpdateBowZoom(float DeltaTime)
{
	ARACharacter* Character = GetOwnerCharacter();
	if (!Character || !Character->FollowCamera)
		return;

	const float TargetFOV = Character->bIsBowAiming ? Character->BowZoomFOV : Character->DefaultFOV;
	const float NewFOV = FMath::FInterpTo(Character->FollowCamera->FieldOfView, TargetFOV, DeltaTime, Character->BowZoomInterpSpeed);
	Character->FollowCamera->SetFieldOfView(NewFOV);
}

void UPlayerCombatComponent::UpdateBowCameraArm(float DeltaTime)
{
	ARACharacter* Character = GetOwnerCharacter();
	if (!Character || !Character->CameraBoom)
		return;

	const float TargetArm = Character->bIsBowAiming ? Character->BowZoomArmLength : Character->DefaultArmLength;
	Character->CameraBoom->TargetArmLength = FMath::FInterpTo(Character->CameraBoom->TargetArmLength, TargetArm, DeltaTime, Character->BowArmInterpSpeed);
}

void UPlayerCombatComponent::EndBowAim()
{
	ARACharacter* Character = GetOwnerCharacter();
	if (!Character)
		return;

	if (Character->PlayerSkillComponent)
	{
		Character->PlayerSkillComponent->CancelBowSkillPreparation();
	}

	Character->bIsBowCharging = false;
	Character->bIsBowAiming = false;
	Character->bIsAttacking = false;
	Character->CachedBowChargeAlpha = 0.0f;

	ResetBowCrosshairUI();
	HidePreviewArrow();

	UE_LOG(LogTemp, Warning, TEXT("Bow Aim End"));
}

bool UPlayerCombatComponent::CanPrepareBowSkill() const
{
	const ARACharacter* Character = GetOwnerCharacter();
	return Character &&
		Character->bIsBowCharging &&
		Character->bIsBowAiming &&
		Character->CurrentWeapon &&
		Character->CurrentWeapon->WeaponType == EWeaponType::Bow &&
		Character->CurrentWeapon->AttackType == EAttackType::Ranged;
}

void UPlayerCombatComponent::SetBowPreviewArrowStaticMesh(UStaticMesh* NewPreviewArrowStaticMesh)
{
	ARACharacter* Character = GetOwnerCharacter();
	if (!Character || !Character->PreviewArrowMesh || !NewPreviewArrowStaticMesh)
		return;

	Character->PreviewArrowMesh->SetStaticMesh(NewPreviewArrowStaticMesh);
	Character->PreviewArrowMesh->SetHiddenInGame(false);
}

void UPlayerCombatComponent::ResetBowPreviewArrowStaticMesh()
{
	ARACharacter* Character = GetOwnerCharacter();
	if (!Character || !Character->PreviewArrowMesh || !Character->PreviewArrowStaticMesh)
		return;

	Character->PreviewArrowMesh->SetStaticMesh(Character->PreviewArrowStaticMesh);
}

void UPlayerCombatComponent::SetBowPreviewArrowVFX(UNiagaraSystem* NewPreviewArrowVFX, FVector RelativeLocation, FRotator RelativeRotation, FVector RelativeScale)
{
	ARACharacter* Character = GetOwnerCharacter();
	if (!Character || !Character->PreviewArrowVFXComponent || !NewPreviewArrowVFX)
		return;

	Character->PreviewArrowVFXComponent->SetAsset(NewPreviewArrowVFX);
	Character->PreviewArrowVFXComponent->SetRelativeLocation(RelativeLocation);
	Character->PreviewArrowVFXComponent->SetRelativeRotation(RelativeRotation);
	Character->PreviewArrowVFXComponent->SetRelativeScale3D(RelativeScale);
	Character->PreviewArrowVFXComponent->SetHiddenInGame(false);
	Character->PreviewArrowVFXComponent->Activate(true);
}

void UPlayerCombatComponent::ClearBowPreviewArrowVFX()
{
	ARACharacter* Character = GetOwnerCharacter();
	if (!Character || !Character->PreviewArrowVFXComponent)
		return;

	Character->PreviewArrowVFXComponent->Deactivate();
	Character->PreviewArrowVFXComponent->SetAsset(nullptr);
	Character->PreviewArrowVFXComponent->SetHiddenInGame(true);
}

void UPlayerCombatComponent::TriggerMeleeHit()
{
	ARACharacter* Character = GetOwnerCharacter();
	if (!Character)
		return;

	UE_LOG(LogTemplateCharacter, Warning, TEXT("TriggerMeleeHit"));

	float Damage = Character->PunchDamage;
	float Range = Character->PunchRange;
	float Radius = Character->PunchRadius;

	if (Character->CurrentWeapon && Character->CurrentWeapon->AttackType == EAttackType::Melee)
	{
		Damage = Character->CurrentWeapon->AttackDamage;
		Range = Character->CurrentWeapon->AttackRange;
		Radius = Character->CurrentWeapon->AttackRadius;
	}

	if (Character->StatComponent)
	{
		Damage = Character->StatComponent->GetFinalAttackPower(Damage);
	}

	(Character->CurrentWeapon && Character->CurrentWeapon->WeaponType == EWeaponType::Sword)
		? PerformSwordHit(Damage, Range, Radius)
		: PerformPunchHit(Damage, Range, Radius);
}

void UPlayerCombatComponent::TriggerSkillHit()
{
	ARACharacter* Character = GetOwnerCharacter();
	if (Character && Character->PlayerSkillComponent)
	{
		Character->PlayerSkillComponent->TriggerSkillHit();
	}
}

void UPlayerCombatComponent::NormalRelease()
{
	ARACharacter* Character = GetOwnerCharacter();
	if (!Character)
		return;

	if (Character->PlayerSkillComponent && Character->PlayerSkillComponent->IsBowSkillPrepared())
	{
		return;
	}

	if (Character->BowReleaseSound)
	{
		UGameplayStatics::PlaySoundAtLocation(Character, Character->BowReleaseSound, Character->GetActorLocation());
	}
}

void UPlayerCombatComponent::SkillRelease()
{
	ARACharacter* Character = GetOwnerCharacter();
	if (Character && Character->PlayerSkillComponent)
	{
		Character->PlayerSkillComponent->PlayBowSkillReleaseSound();
	}
}

void UPlayerCombatComponent::ProceedCombo()
{
	ARACharacter* Character = GetOwnerCharacter();
	if (!Character || !Character->PunchMontage || !Character->GetMesh() || !Character->GetMesh()->GetAnimInstance())
		return;
	if (!Character->bComboInputBuffered)
		return;
	if (Character->CurrentComboIndex >= Character->MaxComboCount)
		return;

	Character->CurrentComboIndex++;
	Character->bComboInputBuffered = false;
	Character->bCanAcceptComboInput = false;

	FaceAttackDirection();

	UAnimInstance* AnimInstance = Character->GetMesh()->GetAnimInstance();
	const FName NextSectionName = FName(*FString::Printf(TEXT("Combo%d"), Character->CurrentComboIndex));
	AnimInstance->Montage_JumpToSection(NextSectionName, Character->PunchMontage);

	UE_LOG(LogTemplateCharacter, Warning, TEXT("Proceed to %s"), *NextSectionName.ToString());
}

void UPlayerCombatComponent::EnableComboWindow()
{
	ARACharacter* Character = GetOwnerCharacter();
	if (Character)
	{
		Character->bCanAcceptComboInput = true;
		UE_LOG(LogTemplateCharacter, Warning, TEXT("Combo Window Open"));
	}
}

void UPlayerCombatComponent::DisableComboWindow()
{
	ARACharacter* Character = GetOwnerCharacter();
	if (Character)
	{
		Character->bCanAcceptComboInput = false;
		UE_LOG(LogTemplateCharacter, Warning, TEXT("Combo Window Closed"));
	}
}

void UPlayerCombatComponent::OnPunchMontageEnded(UAnimMontage* Montage, bool bInterrupted)
{
	ARACharacter* Character = GetOwnerCharacter();
	if (!Character || Montage != Character->PunchMontage)
		return;

	Character->bIsPunching = false;
	Character->bIsAttacking = false;
	Character->bComboInputBuffered = false;
	Character->bCanAcceptComboInput = false;
	Character->CurrentComboIndex = 0;

	UE_LOG(LogTemplateCharacter, Warning, TEXT("Punch Montage Ended"));
}

void UPlayerCombatComponent::PlayBowWeaponMontageSection(FName SectionName)
{
	ARACharacter* Character = GetOwnerCharacter();
	if (!Character || !Character->CurrentWeapon || !Character->CurrentWeapon->UsesSkeletalMesh() ||
		!Character->CurrentWeapon->WeaponSkeletalMesh || !Character->CurrentWeapon->WeaponAnimMontage)
		return;

	UAnimInstance* WeaponAnimInstance = Character->CurrentWeapon->WeaponSkeletalMesh->GetAnimInstance();
	if (!WeaponAnimInstance)
	{
		UE_LOG(LogTemp, Warning, TEXT("PlayBowWeaponMontageSection: WeaponAnimInstance is null"));
		return;
	}

	WeaponAnimInstance->Montage_Play(Character->CurrentWeapon->WeaponAnimMontage);
	WeaponAnimInstance->Montage_JumpToSection(SectionName, Character->CurrentWeapon->WeaponAnimMontage);

	UE_LOG(LogTemp, Warning, TEXT("Bow Weapon Montage Section: %s"), *SectionName.ToString());
}

void UPlayerCombatComponent::ResetBowCrosshairUI()
{
	ARACharacter* Character = GetOwnerCharacter();
	if (Character && Character->CrosshairWidgetInstance)
	{
		Character->CrosshairWidgetInstance->ResetCrosshair();
		Character->CrosshairWidgetInstance->SetCrosshairVisible(false);
	}
}

void UPlayerCombatComponent::ShowPreviewArrow()
{
	ARACharacter* Character = GetOwnerCharacter();
	if (!Character || !Character->PreviewArrowMesh || !Character->PreviewArrowStaticMesh || !Character->CurrentWeapon)
		return;

	Character->PreviewArrowMesh->SetStaticMesh(Character->PreviewArrowStaticMesh);

	if (Character->CurrentWeapon->UsesSkeletalMesh() &&
		Character->CurrentWeapon->WeaponSkeletalMesh &&
		Character->CurrentWeapon->WeaponSkeletalMesh->DoesSocketExist(TEXT("ArrowSocket")))
	{
		Character->PreviewArrowMesh->AttachToComponent(
			Character->CurrentWeapon->WeaponSkeletalMesh,
			FAttachmentTransformRules::SnapToTargetNotIncludingScale,
			TEXT("ArrowSocket")
		);
	}
	else if (Character->GetMesh()->DoesSocketExist(TEXT("LeftHandSocket")))
	{
		Character->PreviewArrowMesh->AttachToComponent(
			Character->GetMesh(),
			FAttachmentTransformRules::SnapToTargetNotIncludingScale,
			TEXT("LeftHandSocket")
		);
	}
	else
	{
		Character->PreviewArrowMesh->AttachToComponent(
			Character->GetMesh(),
			FAttachmentTransformRules::SnapToTargetNotIncludingScale
		);
	}

	Character->PreviewArrowMesh->SetRelativeLocation(FVector::ZeroVector);
	Character->PreviewArrowMesh->SetRelativeRotation(FRotator::ZeroRotator);
	Character->PreviewArrowMesh->SetRelativeScale3D(FVector(1.0f));
	Character->PreviewArrowMesh->SetHiddenInGame(false);
}

void UPlayerCombatComponent::HidePreviewArrow()
{
	ARACharacter* Character = GetOwnerCharacter();
	if (!Character || !Character->PreviewArrowMesh)
		return;

	ClearBowPreviewArrowVFX();

	Character->PreviewArrowMesh->SetHiddenInGame(true);
	Character->PreviewArrowMesh->DetachFromComponent(FDetachmentTransformRules::KeepWorldTransform);
}

bool UPlayerCombatComponent::CanStartSkillAction(bool bAllowBowAiming) const
{
	const ARACharacter* Character = GetOwnerCharacter();
	if (!Character || Character->IsPortalTransitionInputLocked())
		return false;

	if (Character->StatComponent && Character->StatComponent->IsDead())
		return false;

	if (const ARAPlayerController* RAPlayerController = Cast<ARAPlayerController>(Character->GetController()))
	{
		const UPlayerUIFlowComponent* PlayerUIFlowComponent = RAPlayerController->GetPlayerUIFlowComponent();
		if (PlayerUIFlowComponent &&
			(PlayerUIFlowComponent->IsInventoryOpen() ||
				PlayerUIFlowComponent->IsShopOpen() ||
				PlayerUIFlowComponent->IsAnimalCollectionOpen()))
		{
			return false;
		}
	}

	if (Character->PlayerMovementComponent && Character->PlayerMovementComponent->IsDodging())
		return false;

	const bool bBowAimException = bAllowBowAiming && Character->bIsBowAiming;
	if (Character->bIsAttacking && !bBowAimException)
		return false;

	return true;
}

void UPlayerCombatComponent::BeginSkillAction()
{
	ARACharacter* Character = GetOwnerCharacter();
	if (!Character)
		return;

	Character->StopHitMontage();
	Character->bIsAttacking = true;
}

void UPlayerCombatComponent::EndSkillAction()
{
	ARACharacter* Character = GetOwnerCharacter();
	if (Character)
	{
		Character->bIsAttacking = false;
	}
}

void UPlayerCombatComponent::FaceSkillDirection()
{
	FaceAttackDirection();
}

void UPlayerCombatComponent::SpawnHitVFX(UNiagaraSystem* NiagaraSystem, const FVector& SpawnLocation, const FRotator& SpawnRotation, const FLinearColor& Color, float Scale, float Lifetime)
{
	ARACharacter* Character = GetOwnerCharacter();
	if (!Character || !NiagaraSystem)
		return;

	UNiagaraComponent* NiagaraComp = UNiagaraFunctionLibrary::SpawnSystemAtLocation(
		Character->GetWorld(),
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
