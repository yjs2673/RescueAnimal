#include "TPSCreatureBase.h"
#include "Animation/AnimInstance.h"
#include "Components/CapsuleComponent.h"
#include "GameFramework/CharacterMovementComponent.h"

ATPSCreatureBase::ATPSCreatureBase()
{
	PrimaryActorTick.bCanEverTick = false;
}

void ATPSCreatureBase::BeginPlay()
{
	Super::BeginPlay();
	InitializeStats();
}

void ATPSCreatureBase::InitializeStats()
{
	CurrentHP = MaxHP;
	bIsDead = false;
}

float ATPSCreatureBase::TakeDamage(
	float DamageAmount,
	FDamageEvent const& DamageEvent,
	AController* EventInstigator,
	AActor* DamageCauser)
{
	Super::TakeDamage(DamageAmount, DamageEvent, EventInstigator, DamageCauser);

	if (bIsDead)
		return 0.f;

	const float ActualDamage = FMath::Max(0.f, DamageAmount);
	if (ActualDamage <= 0.f)
		return 0.f;

	CurrentHP -= ActualDamage;

	UE_LOG(LogTemp, Warning, TEXT("[%s] Took Damage: %.1f / HP: %.1f"), *GetName(), ActualDamage, CurrentHP);

	if (CurrentHP <= 0.f)
	{
		CurrentHP = 0.f;
		Die();
	}
	else
		PlayHitMontage();

	return ActualDamage;
}

void ATPSCreatureBase::PlayHitMontage()
{
	if (bIsDead || bIsAttacking)
		return;

	if (!GetMesh() || !GetMesh()->GetAnimInstance())
		return;

	if (HitMontages.Num() <= 0)
		return;

	UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance();
	if (!AnimInstance)
		return;
	if (AnimInstance->IsAnyMontagePlaying()) // 다른 몽타주가 재생 중이면 멈추고 새로 재생
		AnimInstance->Montage_Stop(0.1f);

	const int32 RandomIndex = FMath::RandRange(0, HitMontages.Num() - 1);
	UAnimMontage* SelectedMontage = HitMontages[RandomIndex];

	if (SelectedMontage)
		AnimInstance->Montage_Play(SelectedMontage);
}

void ATPSCreatureBase::Die()
{
	if (bIsDead)
		return;

	bIsDead = true;

	UE_LOG(LogTemp, Warning, TEXT("[%s] Dead"), *GetName());

	GetCapsuleComponent()->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	if (GetCharacterMovement())
		GetCharacterMovement()->DisableMovement();

	if (DeadMontage && GetMesh() && GetMesh()->GetAnimInstance())
		GetMesh()->GetAnimInstance()->Montage_Play(DeadMontage);

	SetLifeSpan(DestroyDelay);
}

void ATPSCreatureBase::StopHitMontage()
{
	if (!CurrentHitMontage)
		return;

	if (!GetMesh())
		return;

	UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance();
	if (!AnimInstance)
		return;

	if (AnimInstance->Montage_IsPlaying(CurrentHitMontage))
		AnimInstance->Montage_Stop(0.1f, CurrentHitMontage);

	CurrentHitMontage = nullptr;
}