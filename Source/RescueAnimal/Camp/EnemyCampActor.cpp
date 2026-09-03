#include "EnemyCampActor.h"

#include "Components/SceneComponent.h"
#include "Components/SphereComponent.h"
#include "EngineUtils.h"
#include "TimerManager.h"

#include "RAEnemyBase.h"
#include "AnimalBase.h"
#include "AnimalRescueComponent.h"
#include "EnemyAIComponent.h"

AEnemyCampActor::AEnemyCampActor()
{
	PrimaryActorTick.bCanEverTick = false;

	DefaultRoot = CreateDefaultSubobject<USceneComponent>(TEXT("DefaultRoot"));
	RootComponent = DefaultRoot;

	CampBounds = CreateDefaultSubobject<USphereComponent>(TEXT("CampBounds"));
	CampBounds->SetupAttachment(DefaultRoot);

	CampBounds->SetRelativeLocation(FVector::ZeroVector);
	CampBounds->SetSphereRadius(CampRadius);
	CampBounds->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	CampBounds->SetCollisionObjectType(ECC_WorldDynamic);
	CampBounds->SetCollisionResponseToAllChannels(ECR_Ignore);
	CampBounds->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);

	ApplyCampBoundsSettings();
}

void AEnemyCampActor::BeginPlay()
{
	Super::BeginPlay();

	ApplyCampBoundsSettings();

	if (bAutoCollectMembersOnBeginPlay)
	{
		RefreshCampMembers();
	}

	CheckCampCleared();

	if (!bIsCampCleared && ClearCheckInterval > 0.0f)
	{
		GetWorldTimerManager().SetTimer(
			ClearCheckTimerHandle,
			this,
			&AEnemyCampActor::CheckCampCleared,
			ClearCheckInterval,
			true
		);
	}
}

void AEnemyCampActor::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);

	ApplyCampBoundsSettings();
}

#if WITH_EDITOR
void AEnemyCampActor::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);

	ApplyCampBoundsSettings();
}
#endif

void AEnemyCampActor::ApplyCampBoundsSettings()
{
	if (CampBounds)
	{
		CampBounds->SetRelativeLocation(FVector::ZeroVector);
		CampBounds->SetSphereRadius(CampRadius, true);
		CampBounds->UpdateBounds();
		CampBounds->MarkRenderStateDirty();
	}
}

void AEnemyCampActor::RegisterEnemy(ARAEnemyBase* Enemy)
{
	if (!Enemy)
	{
		return;
	}

	CampEnemies.AddUnique(Enemy);
	if (UEnemyAIComponent* EnemyAIComponent = Enemy->GetEnemyAIComponent())
	{
		EnemyAIComponent->SetCampPatrolArea(CampBounds ? CampBounds->GetComponentLocation() : GetActorLocation(), GetCampRadius());
	}
	CheckCampCleared();
}

void AEnemyCampActor::RegisterAnimal(AAnimalBase* Animal)
{
	if (!Animal)
	{
		return;
	}

	CampAnimals.AddUnique(Animal);
	if (UAnimalRescueComponent* AnimalRescueComponent = Animal->GetAnimalRescueComponent())
	{
		AnimalRescueComponent->SetOwningCamp(this);
	}
}

void AEnemyCampActor::RefreshCampMembers()
{
	CampEnemies.Reset();
	CampAnimals.Reset();

	if (!CampBounds || !GetWorld())
	{
		return;
	}

	for (TActorIterator<ARAEnemyBase> It(GetWorld()); It; ++It)
	{
		ARAEnemyBase* Enemy = *It;
		if (!IsValid(Enemy) || !IsActorInsideCampBounds(Enemy))
		{
			continue;
		}

		CampEnemies.AddUnique(Enemy);
		if (UEnemyAIComponent* EnemyAIComponent = Enemy->GetEnemyAIComponent())
		{
			EnemyAIComponent->SetCampPatrolArea(CampBounds->GetComponentLocation(), GetCampRadius());
		}
	}

	for (TActorIterator<AAnimalBase> It(GetWorld()); It; ++It)
	{
		AAnimalBase* Animal = *It;
		if (!IsValid(Animal) || !IsActorInsideCampBounds(Animal))
		{
			continue;
		}

		CampAnimals.AddUnique(Animal);
		if (UAnimalRescueComponent* AnimalRescueComponent = Animal->GetAnimalRescueComponent())
		{
			AnimalRescueComponent->SetOwningCamp(this);
		}
	}

	UE_LOG(LogTemp, Warning, TEXT("[EnemyCampActor] Refreshed members: %s / Enemies=%d Animals=%d"),
		*GetName(),
		CampEnemies.Num(),
		CampAnimals.Num());
}

void AEnemyCampActor::CheckCampCleared()
{
	if (bIsCampCleared)
	{
		return;
	}

	for (int32 Index = CampEnemies.Num() - 1; Index >= 0; --Index)
	{
		if (!IsValid(CampEnemies[Index]))
		{
			CampEnemies.RemoveAt(Index);
		}
	}

	if (CampEnemies.Num() <= 0)
	{
		HandleCampCleared();
	}
}

int32 AEnemyCampActor::GetAliveEnemyCount() const
{
	int32 AliveCount = 0;

	for (const TObjectPtr<ARAEnemyBase>& Enemy : CampEnemies)
	{
		if (IsValid(Enemy))
		{
			++AliveCount;
		}
	}

	return AliveCount;
}

TArray<ARAEnemyBase*> AEnemyCampActor::GetCampEnemies() const
{
	TArray<ARAEnemyBase*> Enemies;
	Enemies.Reserve(CampEnemies.Num());

	for (const TObjectPtr<ARAEnemyBase>& Enemy : CampEnemies)
	{
		if (IsValid(Enemy))
		{
			Enemies.Add(Enemy.Get());
		}
	}

	return Enemies;
}

TArray<AAnimalBase*> AEnemyCampActor::GetCampAnimals() const
{
	TArray<AAnimalBase*> Animals;
	Animals.Reserve(CampAnimals.Num());

	for (const TObjectPtr<AAnimalBase>& Animal : CampAnimals)
	{
		if (IsValid(Animal))
		{
			Animals.Add(Animal.Get());
		}
	}

	return Animals;
}

float AEnemyCampActor::GetCampRadius() const
{
	return CampBounds ? CampBounds->GetScaledSphereRadius() : 0.0f;
}

bool AEnemyCampActor::IsActorInsideCampBounds(const AActor* Actor) const
{
	if (!Actor || !CampBounds)
	{
		return false;
	}

	return FVector::DistSquared(Actor->GetActorLocation(), CampBounds->GetComponentLocation())
		<= FMath::Square(CampBounds->GetScaledSphereRadius());
}

void AEnemyCampActor::HandleCampCleared()
{
	if (bIsCampCleared)
	{
		return;
	}

	bIsCampCleared = true;
	GetWorldTimerManager().ClearTimer(ClearCheckTimerHandle);

	OnEnemyCampCleared.Broadcast(this);

	UE_LOG(LogTemp, Warning, TEXT("[EnemyCampActor] Camp cleared: %s"), *GetName());
}
