#include "EnemyCampActor.h"

#include "Components/BoxComponent.h"
#include "Engine/OverlapResult.h"
#include "TimerManager.h"

#include "TPSEnemyBase.h"
#include "TPSAnimalBase.h"

AEnemyCampActor::AEnemyCampActor()
{
	PrimaryActorTick.bCanEverTick = false;

	CampBounds = CreateDefaultSubobject<UBoxComponent>(TEXT("CampBounds"));
	RootComponent = CampBounds;

	CampBounds->SetBoxExtent(FVector(1000.0f, 1000.0f, 300.0f));
	CampBounds->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	CampBounds->SetCollisionObjectType(ECC_WorldDynamic);
	CampBounds->SetCollisionResponseToAllChannels(ECR_Ignore);
	CampBounds->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
}

void AEnemyCampActor::BeginPlay()
{
	Super::BeginPlay();

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

void AEnemyCampActor::RegisterEnemy(ATPSEnemyBase* Enemy)
{
	if (!Enemy)
	{
		return;
	}

	CampEnemies.AddUnique(Enemy);
	CheckCampCleared();
}

void AEnemyCampActor::RegisterAnimal(AAnimalBase* Animal)
{
	if (!Animal)
	{
		return;
	}

	CampAnimals.AddUnique(Animal);
}

void AEnemyCampActor::RefreshCampMembers()
{
	CampEnemies.Reset();
	CampAnimals.Reset();

	if (!CampBounds || !GetWorld())
	{
		return;
	}

	TArray<FOverlapResult> OverlapResults;

	FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(EnemyCampActorRefresh), false, this);

	const bool bHasOverlaps = GetWorld()->OverlapMultiByObjectType(
		OverlapResults,
		CampBounds->GetComponentLocation(),
		CampBounds->GetComponentQuat(),
		FCollisionObjectQueryParams(ECC_Pawn),
		FCollisionShape::MakeBox(CampBounds->GetScaledBoxExtent()),
		QueryParams
	);

	if (!bHasOverlaps)
	{
		return;
	}

	for (const FOverlapResult& Result : OverlapResults)
	{
		AActor* OverlappedActor = Result.GetActor();
		if (!OverlappedActor)
		{
			continue;
		}

		if (ATPSEnemyBase* Enemy = Cast<ATPSEnemyBase>(OverlappedActor))
		{
			CampEnemies.AddUnique(Enemy);
			continue;
		}

		if (AAnimalBase* Animal = Cast<AAnimalBase>(OverlappedActor))
		{
			CampAnimals.AddUnique(Animal);
		}
	}
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

	for (ATPSEnemyBase* Enemy : CampEnemies)
	{
		if (IsValid(Enemy))
		{
			++AliveCount;
		}
	}

	return AliveCount;
}

bool AEnemyCampActor::IsActorInsideCampBounds(const AActor* Actor) const
{
	if (!Actor || !CampBounds)
	{
		return false;
	}

	const FTransform BoundsTransform = CampBounds->GetComponentTransform();
	const FVector LocalLocation = BoundsTransform.InverseTransformPosition(Actor->GetActorLocation());
	const FVector BoxExtent = CampBounds->GetUnscaledBoxExtent();

	return FMath::Abs(LocalLocation.X) <= BoxExtent.X
		&& FMath::Abs(LocalLocation.Y) <= BoxExtent.Y
		&& FMath::Abs(LocalLocation.Z) <= BoxExtent.Z;
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