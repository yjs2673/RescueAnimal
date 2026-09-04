#include "RAEnemyBase.h"
#include "Components/SphereComponent.h"
#include "DropItemActor.h"
#include "RACharacter.h"
#include "ArrowProjectile.h"
#include "WeaponBase.h"
#include "UI/Enemy/EnemyHPBarWidget.h"
#include "Characters/Player/Components/PlayerStatComponent.h"
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
		ARACharacter* PlayerCharacter = EnemyAIComponent
			? EnemyAIComponent->ResolvePlayerFromDamage(EventInstigator, DamageCauser)
			: nullptr;
		if (PlayerCharacter)
		{
			if (EnemyAIComponent->SetTargetActor(PlayerCharacter))
			{
				UE_LOG(LogTemp, Warning, TEXT("적 데미지 감지"));
			}
		}
	}

	return AppliedDamage;
}

void ARAEnemyBase::SetEnemyAIState(EEnemyAIState NewState)
{
	if (CurrentAIState == NewState)
	{
		return;
	}

	const FString PreviousStateName = StaticEnum<EEnemyAIState>()
		? StaticEnum<EEnemyAIState>()->GetNameStringByValue(static_cast<int64>(CurrentAIState))
		: TEXT("Unknown");
	const FString NewStateName = StaticEnum<EEnemyAIState>()
		? StaticEnum<EEnemyAIState>()->GetNameStringByValue(static_cast<int64>(NewState))
		: TEXT("Unknown");

	UE_LOG(LogTemp, Log, TEXT("[EnemyFSM] %s state changed: %s -> %s"), *ActorSaveID.ToString(), *PreviousStateName, *NewStateName);
	CurrentAIState = NewState;
}

void ARAEnemyBase::BeginPlay()
{
	Super::BeginPlay();

	if (EnemyEquipmentComponent)
	{
		EnemyEquipmentComponent->EquipDefaultWeapon();
	}

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

FString ARAEnemyBase::GetDebugName() const
{
	return ActorSaveID.ToString();
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
	if (EnemyCombatComponent)
	{
		EnemyCombatComponent->CancelAttackForDeath();
	}

	if (EnemyAIComponent)
	{
		EnemyAIComponent->NotifyOwnerDied();
	}
	else
	{
		SetEnemyAIState(EEnemyAIState::Dead);
	}

	if (EnemyRewardComponent)
	{
		EnemyRewardComponent->GrantEXPToKiller();
		EnemyRewardComponent->SpawnDropItems();
	}

#pragma region Runtime World State
	for (TActorIterator<ARAWorldStateManager> It(GetWorld()); It; ++It)
	{
		It->NotifyEnemyDefeated(ActorSaveID);
		break;
	}
#pragma endregion Runtime World State

	Super::Die();
}
