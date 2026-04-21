// Fill out your copyright notice in the Description page of Project Settings.


#include "TPSCaptureEnemy.h"
#include "Animation/AnimInstance.h"
#include "Components/CapsuleComponent.h"
#include "GameFramework/CharacterMovementComponent.h"

// Sets default values
ATPSCaptureEnemy::ATPSCaptureEnemy()
{
 	// Set this character to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = false;

}

// Called when the game starts or when spawned
void ATPSCaptureEnemy::BeginPlay()
{
	Super::BeginPlay();
	CurrentHP = MaxHP;
}

// Called every frame
void ATPSCaptureEnemy::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

// Called to bind functionality to input
void ATPSCaptureEnemy::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

}

float ATPSCaptureEnemy::TakeDamage(
	float DamageAmount,
	FDamageEvent const& DamageEvent,
	AController* EventInstigator,
	AActor* DamageCauser)
{
	Super::TakeDamage(DamageAmount, DamageEvent, EventInstigator, DamageCauser);

	if (bIsDead)
	{
		return 0.0f;
	}

	const float ActualDamage = FMath::Max(0.0f, DamageAmount);
	if (ActualDamage <= 0.0f)
	{
		return 0.0f;
	}

	CurrentHP -= ActualDamage;

	UE_LOG(LogTemp, Warning, TEXT("[%s] Took Damage: %.1f / HP: %.1f"), *GetName(), ActualDamage, CurrentHP);

	if (CurrentHP <= 0.0f)
	{
		CurrentHP = 0.0f;
		HandleDeath();
	}
	else
	{
		HandleHitReaction();
	}

	return ActualDamage;
}

void ATPSCaptureEnemy::HandleHitReaction()
{
	if (bIsDead)
	{
		return;
	}

	if (HitMontage && GetMesh() && GetMesh()->GetAnimInstance())
	{
		GetMesh()->GetAnimInstance()->Montage_Play(HitMontage);
	}
}

void ATPSCaptureEnemy::HandleDeath()
{
	if (bIsDead) // 이미 죽은 상태에서 다시 데미지를 받는 중복 처리 방지
	{
		return;
	}

	bIsDead = true;

	UE_LOG(LogTemp, Warning, TEXT("[%s] Dead"), *GetName());

	GetCapsuleComponent()->SetCollisionEnabled(ECollisionEnabled::NoCollision); // 죽은 적을 계속 때리는 현상 방지

	if (GetCharacterMovement())
	{
		GetCharacterMovement()->DisableMovement(); // 죽었는데 계속 이동하는 현상 방지
	}

	if (DeadMontage && GetMesh() && GetMesh()->GetAnimInstance())
	{
		GetMesh()->GetAnimInstance()->Montage_Play(DeadMontage);
	}

	SetLifeSpan(DestroyDelay);
}
