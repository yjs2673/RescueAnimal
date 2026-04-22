#include "TPSCreatureBase.h"

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

void ATPSCreatureBase::ApplyDamage(float DamageAmount)
{
	if (bIsDead)
	{
		return;
	}

	if (DamageAmount <= 0.f)
	{
		return;
	}

	CurrentHP -= DamageAmount;
	UE_LOG(LogTemp, Warning, TEXT("%s took damage: %.1f / CurrentHP: %.1f"), *GetName(), DamageAmount, CurrentHP);

	if (CurrentHP <= 0.f)
	{
		Die();
	}
}

void ATPSCreatureBase::Die()
{
	if (bIsDead)
	{
		return;
	}

	bIsDead = true;
	CurrentHP = 0.f;

	UE_LOG(LogTemp, Warning, TEXT("%s died"), *GetName());

	// 일단 최소 처리만
	// 나중에 애니메이션, 드랍, 보상, 충돌 비활성화 추가
}