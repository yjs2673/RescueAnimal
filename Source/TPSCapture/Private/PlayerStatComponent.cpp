#include "PlayerStatComponent.h"

UPlayerStatComponent::UPlayerStatComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UPlayerStatComponent::BeginPlay()
{
	Super::BeginPlay();

	RecalculateStats();
	CurrentHP = MaxHP;

	OnHPChanged.Broadcast(CurrentHP, MaxHP);
}

void UPlayerStatComponent::RecalculateStats()
{
	MaxHP = BaseMaxHP;
	CurrentHP = FMath::Clamp(CurrentHP, 0.f, MaxHP);
}

float UPlayerStatComponent::ApplyDamage(float Amount)
{
	if (bIsDead || Amount <= 0.f)
		return CurrentHP;

	CurrentHP = FMath::Clamp(CurrentHP - Amount, 0.f, MaxHP);
	OnHPChanged.Broadcast(CurrentHP, MaxHP);

	if (CurrentHP <= 0.f)
		Die();

	return CurrentHP;
}

void UPlayerStatComponent::Heal(float Amount)
{
	if (bIsDead || Amount <= 0.f)
		return;

	CurrentHP = FMath::Clamp(CurrentHP + Amount, 0.f, MaxHP);
	OnHPChanged.Broadcast(CurrentHP, MaxHP);
}

float UPlayerStatComponent::GetHPPercent() const
{
	if (MaxHP <= 0.f)
		return 0.f;

	return CurrentHP / MaxHP;
}

void UPlayerStatComponent::Die()
{
	if (bIsDead)
		return;

	bIsDead = true;
	OnDeath.Broadcast();
}