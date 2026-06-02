#include "PlayerStatComponent.h"

#include "NiagaraComponent.h"
#include "NiagaraFunctionLibrary.h"

#include "Components/SceneComponent.h"
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
	MaxHP = BaseMaxHP + LevelBonusMaxHP;
	CurrentHP = FMath::Clamp(CurrentHP, 0.f, MaxHP);
}
#pragma region Bonus Stats

float UPlayerStatComponent::GetFinalAttackPower(float BaseAttack) const
{
	const float AdditiveAttack = BaseAttack + LevelBonusAttack;
	return FMath::Max(0.0f, AdditiveAttack * AttackBuffMultiplier);
}

void UPlayerStatComponent::AddAttackBuffMultiplier(float Multiplier)
{
	if (Multiplier <= 0.0f)
		return;

	AttackBuffMultiplier *= Multiplier;
}

void UPlayerStatComponent::RemoveAttackBuffMultiplier(float Multiplier)
{
	if (Multiplier <= 0.0f)
		return;

	AttackBuffMultiplier = FMath::Max(1.0f, AttackBuffMultiplier / Multiplier);
}

void UPlayerStatComponent::AddDefenseBuffMultiplier(float Multiplier)
{
	if (Multiplier <= 0.0f)
		return;

	DefenseBuffMultiplier *= Multiplier;
}

void UPlayerStatComponent::RemoveDefenseBuffMultiplier(float Multiplier)
{
	if (Multiplier <= 0.0f)
		return;

	DefenseBuffMultiplier = FMath::Max(1.0f, DefenseBuffMultiplier / Multiplier);
}

void UPlayerStatComponent::AddJumpBuffMultiplier(float Multiplier)
{
	if (Multiplier <= 0.0f)
		return;

	JumpBuffMultiplier *= Multiplier;
}

void UPlayerStatComponent::RemoveJumpBuffMultiplier(float Multiplier)
{
	if (Multiplier <= 0.0f)
		return;

	JumpBuffMultiplier = FMath::Max(1.0f, JumpBuffMultiplier / Multiplier);
}

void UPlayerStatComponent::AddMoveSpeedBuffMultiplier(float Multiplier)
{
	if (Multiplier <= 0.0f)
		return;

	MoveSpeedBuffMultiplier *= Multiplier;
}

void UPlayerStatComponent::RemoveMoveSpeedBuffMultiplier(float Multiplier)
{
	if (Multiplier <= 0.0f)
		return;

	MoveSpeedBuffMultiplier = FMath::Max(1.0f, MoveSpeedBuffMultiplier / Multiplier);
}

#pragma region Health Stats
float UPlayerStatComponent::ApplyDamage(float Amount)
{
	if (bIsDead || Amount <= 0.f)
		return CurrentHP;

	const float FinalDamage = GetFinalDamageAfterDefense(Amount);

	CurrentHP = FMath::Clamp(CurrentHP - FinalDamage, 0.f, MaxHP);
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

float UPlayerStatComponent::GetFinalDefense() const
{
	const float AdditiveDefense = BaseDefense + LevelBonusDefense;
	return FMath::Max(0.0f, AdditiveDefense * DefenseBuffMultiplier);
}

float UPlayerStatComponent::GetFinalDamageAfterDefense(float DamageAmount) const
{
	if (DamageAmount <= 0.0f)
		return 0.0f;

	const float DefenseReductionPercent = FMath::Clamp(GetFinalDefense(), 0.0f, 100.0f);
	return DamageAmount * (1.0f - (DefenseReductionPercent / 100.0f));
}

float UPlayerStatComponent::GetFinalJumpZVelocity() const
{
	const float AdditiveJumpZVelocity = BaseJumpZVelocity + LevelBonusJumpZVelocity;
	return FMath::Max(0.0f, AdditiveJumpZVelocity * JumpBuffMultiplier);
}

float UPlayerStatComponent::GetFinalMoveSpeed() const
{
	const float AdditiveMoveSpeed = BaseMoveSpeed + LevelBonusMoveSpeed;
	return FMath::Max(0.0f, AdditiveMoveSpeed * MoveSpeedBuffMultiplier);
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

	LevelBonusMaxHP += 10.f;
	LevelBonusAttack += 2.f;

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

	if (USceneComponent* OwnerRootComponent = Owner->GetRootComponent())
	{
		ActiveLevelUpVFX = UNiagaraFunctionLibrary::SpawnSystemAttached(
			LevelUpVFX,
			OwnerRootComponent,
			NAME_None,
			FVector::ZeroVector,
			FRotator::ZeroRotator,
			EAttachLocation::KeepRelativeOffset,
			false,
			true,
			ENCPoolMethod::None,
			true
		);
	}

	if (!ActiveLevelUpVFX)
	{
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
	}

	if (!ActiveLevelUpVFX)
		return;

	ActiveLevelUpVFX->SetRelativeLocation(FVector::ZeroVector);
	ActiveLevelUpVFX->SetRelativeRotation(FRotator::ZeroRotator);
	ActiveLevelUpVFX->SetRelativeScale3D(FVector(1.0f));

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
