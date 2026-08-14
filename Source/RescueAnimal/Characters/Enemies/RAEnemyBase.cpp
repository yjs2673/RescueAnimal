#include "RAEnemyBase.h"
#include "Components/SphereComponent.h"
#include "DropItemActor.h"
#include "RACharacter.h"
#include "ArrowProjectile.h"
#include "WeaponBase.h"
#include "EnemyHPBarWidget.h"
#include "PlayerStatComponent.h"
#include "AnimalBase.h"
#include "RAGameInstance.h"
#include "RAWorldStateManager.h"

#include "EnemyAIComponent.h"
#include "EnemyCombatComponent.h"
#include "EnemyEquipmentComponent.h"
#include "EnemyRewardComponent.h"

#include "AIController.h"
#include "Animation/AnimInstance.h"
#include "Components/SceneComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/WidgetComponent.h"
#include "Engine/OverlapResult.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/ProjectileMovementComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Navigation/PathFollowingComponent.h"
#include "NavigationSystem.h"
#include "NiagaraComponent.h"
#include "NiagaraFunctionLibrary.h"
#include "Sound/SoundBase.h"
#include "TimerManager.h"
#include "EngineUtils.h"

ARAEnemyBase::ARAEnemyBase()
{
	PrimaryActorTick.bCanEverTick = true;

	DetectionSphere = CreateDefaultSubobject<USphereComponent>(TEXT("DetectionSphere"));
	DetectionSphere->SetupAttachment(RootComponent);
	DetectionSphere->SetSphereRadius(800.f);
	DetectionSphere->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	DetectionSphere->SetCollisionObjectType(ECC_WorldDynamic);
	DetectionSphere->SetCollisionResponseToAllChannels(ECR_Ignore);
	DetectionSphere->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);

	HPBarWidgetComponent = CreateDefaultSubobject<UWidgetComponent>(TEXT("HPBarWidgetComponent"));
	HPBarWidgetComponent->SetupAttachment(GetRootComponent());
	HPBarWidgetComponent->SetRelativeLocation(FVector(0.f, 0.f, 100.f));
	HPBarWidgetComponent->SetWidgetSpace(EWidgetSpace::Screen);
	HPBarWidgetComponent->SetDrawSize(FVector2D(100.f, 20.f));

	EnemyAIComponent = CreateDefaultSubobject<UEnemyAIComponent>(TEXT("EnemyAIComponent"));
	EnemyCombatComponent = CreateDefaultSubobject<UEnemyCombatComponent>(TEXT("EnemyCombatComponent"));
	EnemyEquipmentComponent = CreateDefaultSubobject<UEnemyEquipmentComponent>(TEXT("EnemyEquipmentComponent"));
	EnemyRewardComponent = CreateDefaultSubobject<UEnemyRewardComponent>(TEXT("EnemyRewardComponent"));

	if (GetCharacterMovement())
	{
		GetCharacterMovement()->bUseRVOAvoidance = true;
		GetCharacterMovement()->AvoidanceConsiderationRadius = EnemySeparationRadius;
	}
}

float ARAEnemyBase::TakeDamage(
	float DamageAmount,
	FDamageEvent const& DamageEvent,
	AController* EventInstigator,
	AActor* DamageCauser)
{
	LastDamageInstigator = EventInstigator;
	LastDamageCauser = DamageCauser;

	const float AppliedDamage = Super::TakeDamage(DamageAmount, DamageEvent, EventInstigator, DamageCauser);
	if (AppliedDamage > 0.f && !bIsDead)
	{
		if (ARACharacter* PlayerCharacter = ResolvePlayerFromDamage(EventInstigator, DamageCauser))
		{
			SetTargetActor(PlayerCharacter);
			UE_LOG(LogTemp, Warning, TEXT("[%s] Aggroed by damage from %s"), *GetName(), *PlayerCharacter->GetName());
		}
	}

	return AppliedDamage;
}

void ARAEnemyBase::BeginPlay()
{
	Super::BeginPlay();

	EquipDefaultWeapon();
	UpdateHPBar();
}

void ARAEnemyBase::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	UpdateHPBarVisibility();
}

void ARAEnemyBase::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	Super::EndPlay(EndPlayReason);
}

void ARAEnemyBase::OnDetectionSphereBeginOverlap(
	UPrimitiveComponent* OverlappedComponent,
	AActor* OtherActor,
	UPrimitiveComponent* OtherComp,
	int32 OtherBodyIndex,
	bool bFromSweep,
	const FHitResult& SweepResult)
{
	if (EnemyAIComponent)
	{
		EnemyAIComponent->OnDetectionSphereBeginOverlap(OverlappedComponent, OtherActor, OtherComp, OtherBodyIndex, bFromSweep, SweepResult);
	}
}

void ARAEnemyBase::OnDetectionSphereEndOverlap(
	UPrimitiveComponent* OverlappedComponent,
	AActor* OtherActor,
	UPrimitiveComponent* OtherComp,
	int32 OtherBodyIndex)
{
	if (EnemyAIComponent)
	{
		EnemyAIComponent->OnDetectionSphereEndOverlap(OverlappedComponent, OtherActor, OtherComp, OtherBodyIndex);
	}
}

void ARAEnemyBase::UpdateChase()
{
	if (EnemyAIComponent)
	{
		EnemyAIComponent->UpdateChase();
	}
}

void ARAEnemyBase::UpdateBowSpacing()
{
	if (EnemyAIComponent)
	{
		EnemyAIComponent->UpdateBowSpacing();
	}
}

void ARAEnemyBase::SetTargetActor(AActor* NewTarget)
{
	if (EnemyAIComponent)
	{
		EnemyAIComponent->SetTargetActor(NewTarget);
	}
}

void ARAEnemyBase::ClearTargetActor()
{
	if (EnemyAIComponent)
	{
		EnemyAIComponent->ClearTargetActor();
	}
}

bool ARAEnemyBase::HasValidTarget() const
{
	return EnemyAIComponent && EnemyAIComponent->HasValidTarget();
}

bool ARAEnemyBase::CanAttack() const
{
	return EnemyCombatComponent && EnemyCombatComponent->CanAttack();
}

void ARAEnemyBase::SetCampPatrolArea(const FVector& InCenter, float InRadius)
{
	if (EnemyAIComponent)
	{
		EnemyAIComponent->SetCampPatrolArea(InCenter, InRadius);
	}
}

void ARAEnemyBase::ClearCampPatrolArea()
{
	if (EnemyAIComponent)
	{
		EnemyAIComponent->ClearCampPatrolArea();
	}
}

void ARAEnemyBase::UpdateCampWander()
{
	if (EnemyAIComponent)
	{
		EnemyAIComponent->UpdateCampWander();
	}
}

void ARAEnemyBase::MoveToRandomCampLocation()
{
	if (EnemyAIComponent)
	{
		EnemyAIComponent->MoveToRandomCampLocation();
	}
}

bool ARAEnemyBase::IsValidCombatTarget(const AActor* InTargetActor) const
{
	return EnemyAIComponent && EnemyAIComponent->IsValidCombatTarget(InTargetActor);
}

ARACharacter* ARAEnemyBase::ResolvePlayerFromDamage(AController* EventInstigator, AActor* DamageCauser) const
{
	return EnemyAIComponent ? EnemyAIComponent->ResolvePlayerFromDamage(EventInstigator, DamageCauser) : nullptr;
}

float ARAEnemyBase::GetAttackStartRange() const
{
	return EnemyAIComponent ? EnemyAIComponent->GetAttackStartRange() : 0.f;
}

float ARAEnemyBase::GetAttackHitRange() const
{
	return EnemyAIComponent ? EnemyAIComponent->GetAttackHitRange() : 0.f;
}

float ARAEnemyBase::GetChaseAcceptanceRadius() const
{
	return EnemyAIComponent ? EnemyAIComponent->GetChaseAcceptanceRadius() : 10.f;
}

void ARAEnemyBase::UpdateMovementStuckCheck(float DeltaTime)
{
	if (EnemyAIComponent)
	{
		EnemyAIComponent->UpdateMovementStuckCheck(DeltaTime);
	}
}

void ARAEnemyBase::HandleMovementStuck()
{
	if (EnemyAIComponent)
	{
		EnemyAIComponent->HandleMovementStuck();
	}
}

bool ARAEnemyBase::TryMoveToStrafeLocationAroundTarget()
{
	return EnemyAIComponent && EnemyAIComponent->TryMoveToStrafeLocationAroundTarget();
}

void ARAEnemyBase::ApplySeparationFromNearbyEnemies(float DeltaTime)
{
	if (EnemyAIComponent)
	{
		EnemyAIComponent->ApplySeparationFromNearbyEnemies(DeltaTime);
	}
}

void ARAEnemyBase::UpdateAttack()
{
	if (EnemyCombatComponent)
	{
		EnemyCombatComponent->UpdateAttack();
	}
}

void ARAEnemyBase::EquipDefaultWeapon()
{
	if (EnemyEquipmentComponent)
	{
		EnemyEquipmentComponent->EquipDefaultWeapon();
	}
}

void ARAEnemyBase::EquipWeapon(AWeaponBase* NewWeapon)
{
	if (EnemyEquipmentComponent)
	{
		EnemyEquipmentComponent->EquipWeapon(NewWeapon);
	}
}

void ARAEnemyBase::SyncCombatDataFromWeapon()
{
	if (EnemyEquipmentComponent)
	{
		EnemyEquipmentComponent->SyncCombatDataFromWeapon();
	}
}

void ARAEnemyBase::PerformAttack()
{
	if (EnemyCombatComponent)
	{
		EnemyCombatComponent->PerformAttack();
	}
}

void ARAEnemyBase::PerformPunchAttack()
{
	if (EnemyCombatComponent)
	{
		EnemyCombatComponent->PerformPunchAttack();
	}
}

void ARAEnemyBase::PerformSwordAttack()
{
	if (EnemyCombatComponent)
	{
		EnemyCombatComponent->PerformSwordAttack();
	}
}

void ARAEnemyBase::PerformBowAttack()
{
	if (EnemyCombatComponent)
	{
		EnemyCombatComponent->PerformBowAttack();
	}
}

bool ARAEnemyBase::PlayAttackMontage(UAnimMontage* MontageToPlay)
{
	return EnemyCombatComponent && EnemyCombatComponent->PlayAttackMontage(MontageToPlay);
}

void ARAEnemyBase::ScheduleAttackEnd(float Delay)
{
	if (EnemyCombatComponent)
	{
		EnemyCombatComponent->ScheduleAttackEnd(Delay);
	}
}

void ARAEnemyBase::ReleaseBowChargeAtTarget()
{
	if (EnemyCombatComponent)
	{
		EnemyCombatComponent->ReleaseBowChargeAtTarget();
	}
}

void ARAEnemyBase::PlayBowWeaponMontageSection(FName SectionName)
{
	if (EnemyCombatComponent)
	{
		EnemyCombatComponent->PlayBowWeaponMontageSection(SectionName);
	}
}

void ARAEnemyBase::FaceTargetActor()
{
	if (EnemyCombatComponent)
	{
		EnemyCombatComponent->FaceTargetActor();
	}
}

void ARAEnemyBase::SetAttackMovementLocked(bool bLocked)
{
	if (EnemyCombatComponent)
	{
		EnemyCombatComponent->SetAttackMovementLocked(bLocked);
	}
}

void ARAEnemyBase::PlayMeleeHitEffects(const FVector& HitLocation)
{
	if (EnemyCombatComponent)
	{
		EnemyCombatComponent->PlayMeleeHitEffects(HitLocation);
	}
}

void ARAEnemyBase::SpawnHitVFX(
	UNiagaraSystem* NiagaraSystem,
	const FVector& SpawnLocation,
	const FRotator& SpawnRotation,
	const FLinearColor& Color,
	float Scale,
	float Lifetime)
{
	if (EnemyCombatComponent)
	{
		EnemyCombatComponent->SpawnHitVFX(NiagaraSystem, SpawnLocation, SpawnRotation, Color, Scale, Lifetime);
	}
}

void ARAEnemyBase::EndAttack()
{
	if (EnemyCombatComponent)
	{
		EnemyCombatComponent->EndAttack();
	}
}

void ARAEnemyBase::ApplyDamageToTarget()
{
	if (EnemyCombatComponent)
	{
		EnemyCombatComponent->ApplyDamageToTarget();
	}
}

void ARAEnemyBase::TriggerMeleeHit()
{
	if (EnemyCombatComponent)
	{
		EnemyCombatComponent->TriggerMeleeHit();
	}
}

void ARAEnemyBase::FireArrowAtTarget()
{
	if (EnemyCombatComponent)
	{
		EnemyCombatComponent->FireArrowAtTarget();
	}
}

void ARAEnemyBase::UpdateHPBar()
{
	if (!HPBarWidgetComponent)
	{
		return;
	}

	UUserWidget* UserWidget = HPBarWidgetComponent->GetUserWidgetObject();
	if (!UserWidget)
	{
		return;
	}

	UEnemyHPBarWidget* HPBarWidget = Cast<UEnemyHPBarWidget>(UserWidget);
	if (!HPBarWidget)
	{
		return;
	}

	const float HPPercent = (MaxHP > 0.f) ? (CurrentHP / MaxHP) : 0.f;
	HPBarWidget->SetHPPercent(HPPercent);
}

void ARAEnemyBase::UpdateHPBarVisibility()
{
	if (!HPBarWidgetComponent)
	{
		return;
	}

	ARACharacter* PlayerCharacter = Cast<ARACharacter>(UGameplayStatics::GetPlayerCharacter(this, 0));
	if (!PlayerCharacter)
	{
		return;
	}

	const float Distance = FVector::Dist(GetActorLocation(), PlayerCharacter->GetActorLocation());
	const bool bShouldShow = Distance <= 1200.f;

	HPBarWidgetComponent->SetVisibility(bShouldShow);
}

void ARAEnemyBase::Die()
{
	GrantEXPToKiller();
	SpawnDropItems();

#pragma region Runtime World State
	for (TActorIterator<ARAWorldStateManager> It(GetWorld()); It; ++It)
	{
		It->NotifyEnemyDefeated(ActorSaveID);
		break;
	}
#pragma endregion Runtime World State

	Super::Die();
}

void ARAEnemyBase::GrantEXPToKiller()
{
	if (EnemyRewardComponent)
	{
		EnemyRewardComponent->GrantEXPToKiller();
	}
}

void ARAEnemyBase::SpawnDropItems()
{
	if (EnemyRewardComponent)
	{
		EnemyRewardComponent->SpawnDropItems();
	}
}
