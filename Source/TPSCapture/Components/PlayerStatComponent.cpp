#include "PlayerStatComponent.h"

#include "NiagaraComponent.h"
#include "NiagaraFunctionLibrary.h"

#include "Kismet/GameplayStatics.h"
#include "TimerManager.h"

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

void UPlayerStatComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	StopLevelUpVFX();

	Super::EndPlay(EndPlayReason);
}

#pragma region Bonus Stats
void UPlayerStatComponent::RecalculateStats()
{
	MaxHP = BaseMaxHP + BonusMaxHP;
	CurrentHP = FMath::Clamp(CurrentHP, 0.f, MaxHP);
}
#pragma region Bonus Stats

float UPlayerStatComponent::GetFinalAttackPower(float BaseAttack) const
{
	return BaseAttack + BonusAttack;
}

#pragma region Health Stats
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

	if (LevelUpSound)
	{
		const AActor* Owner = GetOwner();
		const FVector SoundLocation = Owner ? Owner->GetActorLocation() : FVector::ZeroVector;
		UGameplayStatics::PlaySoundAtLocation(this, LevelUpSound, SoundLocation);
	}

	PlayLevelUpVFX();

	BonusMaxHP += 10.f;
	BonusAttack += 2.f;

	RecalculateStats();
	CurrentHP = MaxHP;

	OnHPChanged.Broadcast(CurrentHP, MaxHP);
	OnLevelChanged.Broadcast(Level);
	OnEXPChanged.Broadcast(CurrentEXP, GetRequiredEXP());
}
#pragma endregion Experience Stats

void UPlayerStatComponent::PlayLevelUpVFX()
{
	if (!LevelUpVFX)
		return;

	UWorld* World = GetWorld();
	if (!World)
		return;

	AActor* Owner = GetOwner();
	if (!Owner)
		return;

	StopLevelUpVFX();

	const FVector SpawnLocation = Owner->GetActorLocation();

	ActiveLevelUpVFX = UNiagaraFunctionLibrary::SpawnSystemAtLocation(
		World,
		LevelUpVFX,
		SpawnLocation,
		Owner->GetActorRotation(),
		FVector(1.0f),
		false,
		true,
		ENCPoolMethod::None,
		true
	);

	if (!ActiveLevelUpVFX)
		return;

	ActiveLevelUpVFX->SetVariableLinearColor(TEXT("Color"), LevelUpVFXColor);
	ActiveLevelUpVFX->SetVariableFloat(TEXT("Scale"), LevelUpVFXScale);
	ActiveLevelUpVFX->SetVariableFloat(TEXT("Lifetime"), LevelUpVFXLifetime);
	ActiveLevelUpVFX->SetVariableFloat(LevelUpVFXRevealParameterName, 0.0f);

	LevelUpVFXElapsedTime = 0.0f;

	World->GetTimerManager().SetTimer(
		LevelUpVFXTimerHandle,
		this,
		&UPlayerStatComponent::UpdateLevelUpVFX,
		1.0f / 60.0f,
		true
	);
}

void UPlayerStatComponent::UpdateLevelUpVFX()
{
	if (!ActiveLevelUpVFX)
	{
		StopLevelUpVFX();
		return;
	}

	UWorld* World = GetWorld();
	if (!World)
	{
		StopLevelUpVFX();
		return;
	}

	const float RevealDuration = FMath::Max(LevelUpVFXRevealDuration, 0.01f);
	const float HoldDuration = FMath::Max(LevelUpVFXHoldDuration, 0.0f);
	const float TotalDuration = RevealDuration + HoldDuration + RevealDuration;

	LevelUpVFXElapsedTime += World->GetDeltaSeconds();

	float RevealAmount = 0.0f;
	if (LevelUpVFXElapsedTime <= RevealDuration)
	{
		RevealAmount = LevelUpVFXElapsedTime / RevealDuration;
	}
	else if (LevelUpVFXElapsedTime <= RevealDuration + HoldDuration)
	{
		RevealAmount = 1.0f;
	}
	else if (LevelUpVFXElapsedTime <= TotalDuration)
	{
		RevealAmount = 1.0f - ((LevelUpVFXElapsedTime - RevealDuration - HoldDuration) / RevealDuration);
	}
	else
	{
		StopLevelUpVFX();
		return;
	}

	ActiveLevelUpVFX->SetVariableFloat(
		LevelUpVFXRevealParameterName,
		FMath::Clamp(RevealAmount, 0.0f, 1.0f)
	);
}

void UPlayerStatComponent::StopLevelUpVFX()
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(LevelUpVFXTimerHandle);
	}

	if (ActiveLevelUpVFX)
	{
		ActiveLevelUpVFX->DeactivateImmediate();
		ActiveLevelUpVFX->DestroyComponent();
		ActiveLevelUpVFX = nullptr;
	}

	LevelUpVFXElapsedTime = 0.0f;
}
