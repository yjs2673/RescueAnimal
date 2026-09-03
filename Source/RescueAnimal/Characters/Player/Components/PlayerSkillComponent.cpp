#include "PlayerSkillComponent.h"

#include "RACharacter.h"
#include "RAEnemyBase.h"
#include "PlayerCombatComponent.h"
#include "PlayerEquipmentComponent.h"
#include "PlayerStatComponent.h"
#include "ArrowProjectile.h"

#include "Animation/AnimInstance.h"
#include "Components/SkeletalMeshComponent.h"
#include "DrawDebugHelpers.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Kismet/GameplayStatics.h"
#include "NiagaraComponent.h"
#include "NiagaraFunctionLibrary.h"
#include "Sound/SoundBase.h"
#include "TimerManager.h"

UPlayerSkillComponent::UPlayerSkillComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UPlayerSkillComponent::BeginPlay()
{
	Super::BeginPlay();

	OwnerCharacter = Cast<ARACharacter>(GetOwner());
	if (!OwnerCharacter)
	{
		UE_LOG(LogTemp, Warning, TEXT("PlayerSkillComponent owner is not ARACharacter."));
	}
}

bool UPlayerSkillComponent::TryActivateSkill()
{
	if (!OwnerCharacter)
	{
		return false;
	}

	const UPlayerEquipmentComponent* PlayerEquipmentComponent = OwnerCharacter->GetPlayerEquipmentComponent();
	const EWeaponType CurrentWeaponType = PlayerEquipmentComponent
		? PlayerEquipmentComponent->GetCurrentWeaponType()
		: EWeaponType::None;

	switch (CurrentWeaponType)
	{
	case EWeaponType::None:
		return ActivateUnarmedSkill();

	case EWeaponType::Sword:
		return ActivateSwordSkill();

	case EWeaponType::Bow:
		return ToggleBowSkillPreparation();

	default:
		UE_LOG(LogTemp, Warning, TEXT("No skill is assigned for this weapon type."));
		return false;
	}
}

void UPlayerSkillComponent::HandleSkillInput()
{
	TryActivateSkill();
}

bool UPlayerSkillComponent::ActivateUnarmedSkill()
{
	if (!OwnerCharacter)
	{
		return false;
	}

	UPlayerCombatComponent* PlayerCombatComponent = OwnerCharacter->GetPlayerCombatComponent();
	if (!PlayerCombatComponent || !PlayerCombatComponent->CanStartSkillAction(false))
	{
		return false;
	}

	if (!CanActivateSkill(EWeaponType::None, UnarmedSkill.Common))
	{
		UE_LOG(LogTemp, Warning, TEXT("Unarmed skill is on cooldown: %.2f seconds remaining."), GetCooldownRemaining(EWeaponType::None));
		return false;
	}

	StartSkillCooldown(EWeaponType::None);

	bIsSkillActive = true;
	bUnarmedSkillHitApplied = false;
	bSwordSkillHitApplied = false;
	ActiveSkillWeaponType = EWeaponType::None;

	PlayerCombatComponent->FaceSkillDirection();
	PlayerCombatComponent->BeginSkillAction();

	const FVector LaunchVelocity =
		OwnerCharacter->GetActorForwardVector() * UnarmedSkill.ForwardLaunchStrength +
		FVector::UpVector * UnarmedSkill.UpwardLaunchStrength;
	OwnerCharacter->LaunchCharacter(LaunchVelocity, true, true);

	if (UnarmedSkill.HitDelay > 0.0f)
	{
		GetWorld()->GetTimerManager().SetTimer(
			SkillHitTimerHandle,
			this,
			&UPlayerSkillComponent::TriggerUnarmedSkillHit,
			UnarmedSkill.HitDelay,
			false
		);
	}
	else
	{
		TriggerUnarmedSkillHit();
	}

	UAnimInstance* AnimInstance = OwnerCharacter->GetMesh() ? OwnerCharacter->GetMesh()->GetAnimInstance() : nullptr;
	if (UnarmedSkill.SkillMontage && AnimInstance)
	{
		const float Duration = AnimInstance->Montage_Play(UnarmedSkill.SkillMontage);
		AnimInstance->OnMontageEnded.RemoveDynamic(this, &UPlayerSkillComponent::OnSkillMontageEnded);
		AnimInstance->OnMontageEnded.AddDynamic(this, &UPlayerSkillComponent::OnSkillMontageEnded);

		if (Duration <= 0.0f)
		{
			GetWorld()->GetTimerManager().SetTimer(
				SkillEndTimerHandle,
				this,
				&UPlayerSkillComponent::EndActiveSkill,
				UnarmedSkill.EndDelayWhenNoMontage,
				false
			);
		}
	}
	else
	{
		GetWorld()->GetTimerManager().SetTimer(
			SkillEndTimerHandle,
			this,
			&UPlayerSkillComponent::EndActiveSkill,
			UnarmedSkill.EndDelayWhenNoMontage,
			false
		);
	}

	return true;
}

bool UPlayerSkillComponent::ActivateSwordSkill()
{
	if (!OwnerCharacter)
	{
		return false;
	}

	UPlayerCombatComponent* PlayerCombatComponent = OwnerCharacter->GetPlayerCombatComponent();
	if (!PlayerCombatComponent || !PlayerCombatComponent->CanStartSkillAction(false))
	{
		return false;
	}

	if (!CanActivateSkill(EWeaponType::Sword, SwordSkill.Common))
	{
		UE_LOG(LogTemp, Warning, TEXT("Sword skill is on cooldown: %.2f seconds remaining."), GetCooldownRemaining(EWeaponType::Sword));
		return false;
	}

	StartSkillCooldown(EWeaponType::Sword);

	bIsSkillActive = true;
	bUnarmedSkillHitApplied = false;
	bSwordSkillHitApplied = false;
	ActiveSkillWeaponType = EWeaponType::Sword;

	PlayerCombatComponent->FaceSkillDirection();
	PlayerCombatComponent->BeginSkillAction();

	if (!FMath::IsNearlyZero(SwordSkill.ForwardLaunchStrength))
	{
		const FVector LaunchVelocity = OwnerCharacter->GetActorForwardVector() * SwordSkill.ForwardLaunchStrength;
		OwnerCharacter->LaunchCharacter(LaunchVelocity, true, false);
	}

	if (SwordSkill.HitDelay > 0.0f)
	{
		GetWorld()->GetTimerManager().SetTimer(
			SkillHitTimerHandle,
			this,
			&UPlayerSkillComponent::TriggerSwordSkillHit,
			SwordSkill.HitDelay,
			false
		);
	}
	else
	{
		TriggerSwordSkillHit();
	}

	UAnimInstance* AnimInstance = OwnerCharacter->GetMesh() ? OwnerCharacter->GetMesh()->GetAnimInstance() : nullptr;
	if (SwordSkill.SkillMontage && AnimInstance)
	{
		const float Duration = AnimInstance->Montage_Play(SwordSkill.SkillMontage);
		AnimInstance->OnMontageEnded.RemoveDynamic(this, &UPlayerSkillComponent::OnSkillMontageEnded);
		AnimInstance->OnMontageEnded.AddDynamic(this, &UPlayerSkillComponent::OnSkillMontageEnded);

		if (Duration <= 0.0f)
		{
			GetWorld()->GetTimerManager().SetTimer(
				SkillEndTimerHandle,
				this,
				&UPlayerSkillComponent::EndActiveSkill,
				SwordSkill.EndDelayWhenNoMontage,
				false
			);
		}
	}
	else
	{
		GetWorld()->GetTimerManager().SetTimer(
			SkillEndTimerHandle,
			this,
			&UPlayerSkillComponent::EndActiveSkill,
			SwordSkill.EndDelayWhenNoMontage,
			false
		);
	}

	return true;
}

bool UPlayerSkillComponent::ToggleBowSkillPreparation()
{
	if (!OwnerCharacter)
	{
		return false;
	}

	if (bBowSkillPrepared)
	{
		CancelBowSkillPreparation();
		UE_LOG(LogTemp, Warning, TEXT("Bow skill preparation canceled."));
		return true;
	}

	UPlayerCombatComponent* PlayerCombatComponent = OwnerCharacter->GetPlayerCombatComponent();
	if (!PlayerCombatComponent || !PlayerCombatComponent->CanPrepareBowSkill())
	{
		UE_LOG(LogTemp, Warning, TEXT("Bow skill can only be prepared while aiming."));
		return false;
	}

	if (!CanActivateSkill(EWeaponType::Bow, BowSkill.Common))
	{
		UE_LOG(LogTemp, Warning, TEXT("Bow skill is on cooldown: %.2f seconds remaining."), GetCooldownRemaining(EWeaponType::Bow));
		return false;
	}

	if (!BowSkill.FireArrowProjectileClass)
	{
		UE_LOG(LogTemp, Warning, TEXT("Bow skill failed: FireArrowProjectileClass is null."));
		return false;
	}

	bBowSkillPrepared = true;

	if (BowSkill.FirePreviewArrowStaticMesh)
	{
		PlayerCombatComponent->SetBowPreviewArrowStaticMesh(BowSkill.FirePreviewArrowStaticMesh);
	}

	if (BowSkill.FirePreviewArrowVFX)
	{
		PlayerCombatComponent->SetBowPreviewArrowVFX(
			BowSkill.FirePreviewArrowVFX,
			BowSkill.FirePreviewVFXRelativeLocation,
			BowSkill.FirePreviewVFXRelativeRotation,
			BowSkill.FirePreviewVFXRelativeScale
		);
	}

	UE_LOG(LogTemp, Warning, TEXT("Bow skill prepared."));
	return true;
}

bool UPlayerSkillComponent::CanActivateSkill(EWeaponType SkillWeaponType, const FPlayerSkillCommonInfo& SkillInfo) const
{
	if (!OwnerCharacter || bIsSkillActive)
	{
		return false;
	}

	const UWorld* World = GetWorld();
	if (!World)
	{
		return false;
	}

	const float* LastUseTime = LastSkillUseTimes.Find(SkillWeaponType);
	if (!LastUseTime)
	{
		return true;
	}

	return World->GetTimeSeconds() - *LastUseTime >= SkillInfo.Cooldown;
}

void UPlayerSkillComponent::StartSkillCooldown(EWeaponType SkillWeaponType)
{
	if (const UWorld* World = GetWorld())
	{
		LastSkillUseTimes.FindOrAdd(SkillWeaponType) = World->GetTimeSeconds();
		OnSkillCooldownStarted.Broadcast(SkillWeaponType);
	}
}

float UPlayerSkillComponent::GetCooldownRemaining(EWeaponType SkillWeaponType) const
{
	const UWorld* World = GetWorld();
	const float* LastUseTime = LastSkillUseTimes.Find(SkillWeaponType);
	if (!World || !LastUseTime)
	{
		return 0.0f;
	}

	const float Cooldown = GetSkillCooldown(SkillWeaponType);
	return FMath::Max(0.0f, Cooldown - (World->GetTimeSeconds() - *LastUseTime));
}

float UPlayerSkillComponent::GetSkillCooldown(EWeaponType SkillWeaponType) const
{
	switch (SkillWeaponType)
	{
	case EWeaponType::None:
		return UnarmedSkill.Common.Cooldown;

	case EWeaponType::Sword:
		return SwordSkill.Common.Cooldown;

	case EWeaponType::Bow:
		return BowSkill.Common.Cooldown;

	default:
		return 0.0f;
	}
}

float UPlayerSkillComponent::GetCooldownPercent(EWeaponType SkillWeaponType) const
{
	const float Cooldown = GetSkillCooldown(SkillWeaponType);
	if (Cooldown <= 0.0f)
	{
		return 0.0f;
	}

	return FMath::Clamp(GetCooldownRemaining(SkillWeaponType) / Cooldown, 0.0f, 1.0f);
}

void UPlayerSkillComponent::TriggerUnarmedSkillHit()
{
	if (!bIsSkillActive || bUnarmedSkillHitApplied || ActiveSkillWeaponType != EWeaponType::None)
	{
		return;
	}

	bUnarmedSkillHitApplied = true;
	PerformUnarmedSkillHit();
}

void UPlayerSkillComponent::TriggerSwordSkillHit()
{
	if (!bIsSkillActive || bSwordSkillHitApplied || ActiveSkillWeaponType != EWeaponType::Sword)
	{
		return;
	}

	bSwordSkillHitApplied = true;
	PerformSwordSkillHit();
}

void UPlayerSkillComponent::TriggerSkillHit()
{
	switch (ActiveSkillWeaponType)
	{
	case EWeaponType::None:
		TriggerUnarmedSkillHit();
		break;

	case EWeaponType::Sword:
		TriggerSwordSkillHit();
		break;

	default:
		break;
	}
}

TSubclassOf<AArrowProjectile> UPlayerSkillComponent::GetPreparedBowProjectileClass() const
{
	return bBowSkillPrepared ? BowSkill.FireArrowProjectileClass : nullptr;
}

bool UPlayerSkillComponent::CommitBowSkillRelease()
{
	if (!bBowSkillPrepared)
	{
		return false;
	}

	StartSkillCooldown(EWeaponType::Bow);
	CancelBowSkillPreparation();
	return true;
}

void UPlayerSkillComponent::CancelBowSkillPreparation()
{
	if (!bBowSkillPrepared)
	{
		return;
	}

	bBowSkillPrepared = false;

	if (OwnerCharacter)
	{
		if (UPlayerCombatComponent* PlayerCombatComponent = OwnerCharacter->GetPlayerCombatComponent())
		{
			PlayerCombatComponent->ResetBowPreviewArrowStaticMesh();
			PlayerCombatComponent->ClearBowPreviewArrowVFX();
		}
	}
}

void UPlayerSkillComponent::PlayBowSkillReleaseSound() const
{
	if (!bBowSkillPrepared || !BowSkill.SkillReleaseSound || !OwnerCharacter)
	{
		return;
	}

	UGameplayStatics::PlaySoundAtLocation(
		this,
		BowSkill.SkillReleaseSound,
		OwnerCharacter->GetActorLocation()
	);
}

void UPlayerSkillComponent::ApplyBowSkillHitEffects(AArrowProjectile* Arrow) const
{
	if (!Arrow)
	{
		return;
	}

	Arrow->ArrowHitSound = BowSkill.HitSound;
	Arrow->ArrowHitVFX = BowSkill.HitVFX;
	Arrow->ArrowHitColor = BowSkill.HitVFXColor;
	Arrow->ArrowHitScale = BowSkill.HitVFXScale;
	Arrow->ArrowHitLifetime = BowSkill.HitVFXLifetime;
}

void UPlayerSkillComponent::PerformUnarmedSkillHit()
{
	if (!OwnerCharacter || !GetWorld())
	{
		return;
	}

	const FVector Start = OwnerCharacter->GetActorLocation() + FVector(0.0f, 0.0f, 50.0f);
	const FVector End = Start + OwnerCharacter->GetActorForwardVector() * UnarmedSkill.Range;
	const FCollisionShape Sphere = FCollisionShape::MakeSphere(UnarmedSkill.Radius);

	FCollisionQueryParams QueryParams;
	QueryParams.AddIgnoredActor(OwnerCharacter);

	FCollisionObjectQueryParams ObjectQueryParams;
	ObjectQueryParams.AddObjectTypesToQuery(ECC_Pawn);
	TArray<FHitResult> HitResults;
	GetWorld()->SweepMultiByObjectType(
		HitResults,
		Start,
		End,
		FQuat::Identity,
		ObjectQueryParams,
		Sphere,
		QueryParams
	);

	if (bDrawSkillDebug)
	{
		const bool bHitEnemy = HitResults.ContainsByPredicate([this](const FHitResult& HitResult)
			{
				return !ShouldIgnoreSkillTarget(HitResult.GetActor());
			});

		DrawDebugCapsule(
			GetWorld(),
			(Start + End) * 0.5f,
			UnarmedSkill.Range * 0.5f,
			UnarmedSkill.Radius,
			FRotationMatrix::MakeFromX(End - Start).ToQuat(),
			bHitEnemy ? FColor::Red : FColor::Green,
			false,
			1.5f
		);
	}

	TSet<AActor*> DamagedActors;
	for (const FHitResult& HitResult : HitResults)
	{
		AActor* HitActor = HitResult.GetActor();
		if (!HitActor || DamagedActors.Contains(HitActor) || ShouldIgnoreSkillTarget(HitActor))
		{
			continue;
		}

		DamagedActors.Add(HitActor);

		FVector HitLocation = HitResult.ImpactPoint;
		if (HitLocation.IsNearlyZero())
		{
			HitLocation = HitActor->GetActorLocation();
		}

		SpawnSkillVFX(
			UnarmedSkill.HitVFX,
			HitLocation,
			OwnerCharacter->GetActorRotation(),
			UnarmedSkill.HitVFXColor,
			UnarmedSkill.HitVFXScale,
			UnarmedSkill.HitVFXLifetime
		);

		if (UnarmedSkill.HitSound)
		{
			UGameplayStatics::PlaySoundAtLocation(this, UnarmedSkill.HitSound, HitLocation);
		}

		float FinalDamage = UnarmedSkill.Damage;
		if (UPlayerStatComponent* StatComponent = OwnerCharacter->GetPlayerStatComponent())
		{
			FinalDamage = StatComponent->GetFinalAttackPower(FinalDamage);
		}

		UGameplayStatics::ApplyDamage(
			HitActor,
			FinalDamage,
			OwnerCharacter->GetController(),
			OwnerCharacter,
			UDamageType::StaticClass()
		);

		if (ACharacter* HitCharacter = Cast<ACharacter>(HitActor))
		{
			FVector KnockbackDirection = (HitActor->GetActorLocation() - OwnerCharacter->GetActorLocation()).GetSafeNormal2D();
			if (KnockbackDirection.IsNearlyZero())
			{
				KnockbackDirection = OwnerCharacter->GetActorForwardVector();
			}

			const FVector KnockbackVelocity =
				KnockbackDirection * UnarmedSkill.KnockbackStrength +
				FVector::UpVector * UnarmedSkill.KnockbackUpwardStrength;
			HitCharacter->LaunchCharacter(KnockbackVelocity, true, true);
		}
	}

	if (DamagedActors.Num() > 0)
	{
		SpawnSkillVFX(
			UnarmedSkill.ShockwaveVFX,
			GetFootSocketLocation(),
			OwnerCharacter->GetActorRotation(),
			UnarmedSkill.ShockwaveVFXColor,
			UnarmedSkill.ShockwaveVFXScale,
			UnarmedSkill.ShockwaveVFXLifetime
		);
	}
}

void UPlayerSkillComponent::PerformSwordSkillHit()
{
	if (!OwnerCharacter || !GetWorld())
	{
		return;
	}

	const FVector Start = OwnerCharacter->GetActorLocation() + FVector(0.0f, 0.0f, 50.0f);
	const FVector End = Start + OwnerCharacter->GetActorForwardVector() * SwordSkill.Range;
	const FCollisionShape Sphere = FCollisionShape::MakeSphere(SwordSkill.Radius);

	FCollisionQueryParams QueryParams;
	QueryParams.AddIgnoredActor(OwnerCharacter);

	FCollisionObjectQueryParams ObjectQueryParams;
	ObjectQueryParams.AddObjectTypesToQuery(ECC_Pawn);
	TArray<FHitResult> HitResults;
	GetWorld()->SweepMultiByObjectType(
		HitResults,
		Start,
		End,
		FQuat::Identity,
		ObjectQueryParams,
		Sphere,
		QueryParams
	);

	if (bDrawSkillDebug)
	{
		const bool bHitEnemy = HitResults.ContainsByPredicate([this](const FHitResult& HitResult)
			{
				return !ShouldIgnoreSkillTarget(HitResult.GetActor());
			});

		DrawDebugCapsule(
			GetWorld(),
			(Start + End) * 0.5f,
			SwordSkill.Range * 0.5f,
			SwordSkill.Radius,
			FRotationMatrix::MakeFromX(End - Start).ToQuat(),
			bHitEnemy ? FColor::Red : FColor::Green,
			false,
			1.5f
		);
	}

	TSet<AActor*> DamagedActors;
	for (const FHitResult& HitResult : HitResults)
	{
		AActor* HitActor = HitResult.GetActor();
		if (!HitActor || DamagedActors.Contains(HitActor) || ShouldIgnoreSkillTarget(HitActor))
		{
			continue;
		}

		DamagedActors.Add(HitActor);

		FVector HitLocation = HitResult.ImpactPoint;
		if (HitLocation.IsNearlyZero())
		{
			HitLocation = HitActor->GetActorLocation();
		}

		SpawnSkillVFX(
			SwordSkill.HitVFX,
			HitLocation,
			OwnerCharacter->GetActorRotation(),
			SwordSkill.HitVFXColor,
			SwordSkill.HitVFXScale,
			SwordSkill.HitVFXLifetime
		);

		if (SwordSkill.HitSound)
		{
			UGameplayStatics::PlaySoundAtLocation(this, SwordSkill.HitSound, HitLocation);
		}

		float FinalDamage = SwordSkill.Damage;
		if (UPlayerStatComponent* StatComponent = OwnerCharacter->GetPlayerStatComponent())
		{
			FinalDamage = StatComponent->GetFinalAttackPower(FinalDamage);
		}

		UGameplayStatics::ApplyDamage(
			HitActor,
			FinalDamage,
			OwnerCharacter->GetController(),
			OwnerCharacter,
			UDamageType::StaticClass()
		);
	}
}

bool UPlayerSkillComponent::ShouldIgnoreSkillTarget(AActor* TargetActor) const
{
	if (!TargetActor || TargetActor == OwnerCharacter)
	{
		return true;
	}

	return !TargetActor->IsA<ARAEnemyBase>();
}

void UPlayerSkillComponent::SpawnSkillVFX(
	UNiagaraSystem* NiagaraSystem,
	const FVector& SpawnLocation,
	const FRotator& SpawnRotation,
	const FLinearColor& Color,
	float Scale,
	float Lifetime) const
{
	if (!NiagaraSystem || !GetWorld())
	{
		return;
	}

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
	{
		return;
	}

	NiagaraComp->SetVariableLinearColor(TEXT("Color"), Color);
	NiagaraComp->SetVariableFloat(TEXT("Scale"), Scale);
	NiagaraComp->SetVariableFloat(TEXT("Lifetime"), Lifetime);
}

FVector UPlayerSkillComponent::GetFootSocketLocation() const
{
	if (!OwnerCharacter || !OwnerCharacter->GetMesh())
	{
		return FVector::ZeroVector;
	}

	if (OwnerCharacter->GetMesh()->DoesSocketExist(UnarmedSkill.FootSocketName))
	{
		return OwnerCharacter->GetMesh()->GetSocketLocation(UnarmedSkill.FootSocketName);
	}

	return OwnerCharacter->GetActorLocation() + OwnerCharacter->GetActorForwardVector() * UnarmedSkill.Range;
}

void UPlayerSkillComponent::OnSkillMontageEnded(UAnimMontage* Montage, bool bInterrupted)
{
	const bool bIsActiveSkillMontage =
		(ActiveSkillWeaponType == EWeaponType::None && Montage == UnarmedSkill.SkillMontage) ||
		(ActiveSkillWeaponType == EWeaponType::Sword && Montage == SwordSkill.SkillMontage);

	if (!bIsActiveSkillMontage)
	{
		return;
	}

	if (OwnerCharacter && OwnerCharacter->GetMesh())
	{
		if (UAnimInstance* AnimInstance = OwnerCharacter->GetMesh()->GetAnimInstance())
		{
			AnimInstance->OnMontageEnded.RemoveDynamic(this, &UPlayerSkillComponent::OnSkillMontageEnded);
		}
	}

	EndActiveSkill();
}

void UPlayerSkillComponent::EndActiveSkill()
{
	if (!bIsSkillActive)
	{
		return;
	}

	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(SkillHitTimerHandle);
		World->GetTimerManager().ClearTimer(SkillEndTimerHandle);
	}

	bIsSkillActive = false;
	bUnarmedSkillHitApplied = false;
	bSwordSkillHitApplied = false;
	ActiveSkillWeaponType = EWeaponType::None;

	if (OwnerCharacter)
	{
		if (UPlayerCombatComponent* PlayerCombatComponent = OwnerCharacter->GetPlayerCombatComponent())
		{
			PlayerCombatComponent->EndSkillAction();
		}
	}
}
