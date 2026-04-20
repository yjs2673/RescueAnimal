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

	OnEXPChanged.Broadcast(CurrentEXP, GetRequiredEXP());
	OnLevelChanged.Broadcast(Level);
}

#pragma region Health Stats
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
#pragma endregion Health Stats

#pragma region Experience Stats
int32 UPlayerStatComponent::GetRequiredEXP() const
{
	if (Level >= MaxLevel)
		return 0;

	return Level * 10;
}

float UPlayerStatComponent::GetEXPPercent() const
{
	const int32 RequiredEXP = GetRequiredEXP();

	if (RequiredEXP <= 0)
		return 1.0f;

	return static_cast<float>(CurrentEXP) / static_cast<float>(RequiredEXP);
}

void UPlayerStatComponent::AddEXP(int32 Amount)
{
	if (Amount <= 0 || bIsDead)
		return;

	if (Level >= MaxLevel)
	{
		CurrentEXP = 0;
		OnEXPChanged.Broadcast(CurrentEXP, GetRequiredEXP());
		return;
	}

	CurrentEXP += Amount;

	while (Level < MaxLevel && CurrentEXP >= GetRequiredEXP())
	{
		CurrentEXP -= GetRequiredEXP();
		LevelUp();
	}

	if (Level >= MaxLevel)
	{
		Level = MaxLevel;
		CurrentEXP = 0;
	}

	OnEXPChanged.Broadcast(CurrentEXP, GetRequiredEXP());
}

void UPlayerStatComponent::LevelUp()
{
	if (Level >= MaxLevel)
	{
		Level = MaxLevel;
		CurrentEXP = 0;
		return;
	}

	Level++;

	OnLevelChanged.Broadcast(Level);
	OnEXPChanged.Broadcast(CurrentEXP, GetRequiredEXP());
}
#pragma endregion Experience Stats