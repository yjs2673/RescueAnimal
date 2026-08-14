#include "RACreatureBase.h"
#include "Animation/AnimInstance.h"
#include "Components/CapsuleComponent.h"
#include "GameFramework/CharacterMovementComponent.h"

ARACreatureBase::ARACreatureBase()
{
	PrimaryActorTick.bCanEverTick = false;
}

void ARACreatureBase::BeginPlay()
{
	Super::BeginPlay();
	InitializeStats();
}

void ARACreatureBase::InitializeStats()
{
	CurrentHP = MaxHP;
	bIsDead = false;
}

float ARACreatureBase::TakeDamage(
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
	UpdateHPBar();

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

void ARACreatureBase::PlayHitMontage()
{
	if (bIsDead)
		return;

	if (!GetMesh() || !GetMesh()->GetAnimInstance())
		return;

	if (HitMontages.Num() <= 0)
		return;

	UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance();
	if (!AnimInstance)
		return;
	if (AnimInstance->IsAnyMontagePlaying()) // �ٸ� ��Ÿ�ְ� ��� ���̸� ���߰� ���� ���
		AnimInstance->Montage_Stop(0.1f);

	const int32 RandomIndex = FMath::RandRange(0, HitMontages.Num() - 1);
	CurrentHitMontage = HitMontages[RandomIndex];

	if (CurrentHitMontage)
		AnimInstance->Montage_Play(CurrentHitMontage);
}

void ARACreatureBase::Die()
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

void ARACreatureBase::StopHitMontage()
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

void ARACreatureBase::UpdateHPBar()
{
}