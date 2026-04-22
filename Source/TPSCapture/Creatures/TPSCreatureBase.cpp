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
		HandleHitReaction();

	return ActualDamage;
}

void ATPSCreatureBase::HandleHitReaction()
{
	if (bIsDead)
		return;

	if (HitMontage && GetMesh() && GetMesh()->GetAnimInstance())
		GetMesh()->GetAnimInstance()->Montage_Play(HitMontage);
}

void ATPSCreatureBase::Die()
{
	if (bIsDead)
		return;

	bIsDead = true;

	UE_LOG(LogTemp, Warning, TEXT("[%s] Dead"), *GetName());

	GetCapsuleComponent()->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	/*if (GetCharacterMovement())
	{
		GetCharacterMovement()->DisableMovement();
	}*/

	if (DeadMontage && GetMesh() && GetMesh()->GetAnimInstance())
	{
		GetMesh()->GetAnimInstance()->Montage_Play(DeadMontage);
	}

	SetLifeSpan(DestroyDelay);
}