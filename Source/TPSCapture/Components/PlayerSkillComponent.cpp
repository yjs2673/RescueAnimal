#include "PlayerSkillComponent.h"

#include "TPSCaptureCharacter.h"
#include "TPSAnimalBase.h"
#include "TPSCreatureBase.h"
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

	OwnerCharacter = Cast<ATPSCaptureCharacter>(GetOwner());
	if (!OwnerCharacter)
	{
		UE_LOG(LogTemp, Warning, TEXT("PlayerSkillComponent owner is not ATPSCaptureCharacter."));
	}
}

void UPlayerSkillComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
}

bool UPlayerSkillComponent::TryActivateSkill()
{
	if (!OwnerCharacter)
	{
		return false;
	}

	switch (OwnerCharacter->GetCurrentWeaponType())
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

	if (!OwnerCharacter->CanStartSkillAction(false))
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

	OwnerCharacter->FaceSkillDirection();
	OwnerCharacter->BeginSkillAction();

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

	if (!OwnerCharacter->CanStartSkillAction(false))
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

	OwnerCharacter->FaceSkillDirection();
	OwnerCharacter->BeginSkillAction();

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

	if (!OwnerCharacter->CanPrepareBowSkill())
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
		OwnerCharacter->SetBowPreviewArrowStaticMesh(BowSkill.FirePreviewArrowStaticMesh);
	}

	if (BowSkill.FirePreviewArrowVFX)
	{
		OwnerCharacter->SetBowPreviewArrowVFX(
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

	float Cooldown = 0.0f;
	switch (SkillWeaponType)
	{
	case EWeaponType::None:
		Cooldown = UnarmedSkill.Common.Cooldown;
		break;

	case EWeaponType::Sword:
		Cooldown = SwordSkill.Common.Cooldown;
		break;

	case EWeaponType::Bow:
		Cooldown = BowSkill.Common.Cooldown;
		break;

	default:
		break;
	}

	return FMath::Max(0.0f, Cooldown - (World->GetTimeSeconds() - *LastUseTime));
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
		OwnerCharacter->ResetBowPreviewArrowStaticMesh();
		OwnerCharacter->ClearBowPreviewArrowVFX();
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

	TArray<FHitResult> HitResults;
	const bool bHit = GetWorld()->SweepMultiByChannel(
		HitResults,
		Start,
		End,
		FQuat::Identity,
		ECC_Pawn,
		Sphere,
		QueryParams
	);

	DrawDebugCapsule(
		GetWorld(),
		(Start + End) * 0.5f,
		UnarmedSkill.Range * 0.5f,
		UnarmedSkill.Radius,
		FRotationMatrix::MakeFromX(End - Start).ToQuat(),
		bHit ? FColor::Red : FColor::Green,
		false,
		1.5f
	);

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

	TArray<FHitResult> HitResults;
	const bool bHit = GetWorld()->SweepMultiByChannel(
		HitResults,
		Start,
		End,
		FQuat::Identity,
		ECC_Pawn,
		Sphere,
		QueryParams
	);

	DrawDebugCapsule(
		GetWorld(),
		(Start + End) * 0.5f,
		SwordSkill.Range * 0.5f,
		SwordSkill.Radius,
		FRotationMatrix::MakeFromX(End - Start).ToQuat(),
		bHit ? FColor::Red : FColor::Green,
		false,
		1.5f
	);

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

	if (!TargetActor->IsA<ATPSCreatureBase>())
	{
		return true;
	}

	if (const AAnimalBase* Animal = Cast<AAnimalBase>(TargetActor))
	{
		return Animal->IsTrapped();
	}

	return false;
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
		OwnerCharacter->EndSkillAction();
	}
}

