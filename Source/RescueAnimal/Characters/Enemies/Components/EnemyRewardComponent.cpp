#include "EnemyRewardComponent.h"

#include "RAEnemyBase.h"
#include "DropItemActor.h"
#include "Characters/Player/Components/PlayerStatComponent.h"
#include "RACharacter.h"
#include "RAGameInstance.h"
#include "RAWorldStateManager.h"

#include "EngineUtils.h"
#include "GameFramework/Controller.h"
#include "Kismet/GameplayStatics.h"

UEnemyRewardComponent::UEnemyRewardComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UEnemyRewardComponent::BeginPlay()
{
	Super::BeginPlay();
}

ARAEnemyBase* UEnemyRewardComponent::GetOwnerEnemy() const
{
	return Cast<ARAEnemyBase>(GetOwner());
}

void UEnemyRewardComponent::GrantEXPToKiller()
{
	ARAEnemyBase* Enemy = GetOwnerEnemy();
	if (!Enemy || Enemy->bHasGrantedEXP || Enemy->EXPReward <= 0)
	{
		return;
	}

	ARACharacter* PlayerCharacter = nullptr;

	if (Enemy->LastDamageInstigator)
	{
		PlayerCharacter = Cast<ARACharacter>(Enemy->LastDamageInstigator->GetPawn());
	}

	if (!PlayerCharacter && Enemy->LastDamageCauser)
	{
		PlayerCharacter = Cast<ARACharacter>(Enemy->LastDamageCauser);
	}

	if (!PlayerCharacter && Enemy->LastDamageCauser)
	{
		PlayerCharacter = Cast<ARACharacter>(Enemy->LastDamageCauser->GetOwner());
	}

	if (!PlayerCharacter && Enemy->LastDamageCauser)
	{
		PlayerCharacter = Cast<ARACharacter>(Enemy->LastDamageCauser->GetInstigator());
	}

	if (!PlayerCharacter)
	{
		return;
	}

	UPlayerStatComponent* PlayerStatComponent = PlayerCharacter->FindComponentByClass<UPlayerStatComponent>();
	if (!PlayerStatComponent)
	{
		UE_LOG(LogTemp, Warning, TEXT("[%s] Failed to grant EXP: PlayerStatComponent is missing"), *Enemy->ActorSaveID.ToString());
		return;
	}

	Enemy->bHasGrantedEXP = true;
	PlayerStatComponent->AddEXP(Enemy->EXPReward);

	UE_LOG(LogTemp, Warning, TEXT("[%s] Granted %d EXP to %s"),
		*Enemy->ActorSaveID.ToString(),
		Enemy->EXPReward,
		*PlayerCharacter->GetName());
}

void UEnemyRewardComponent::SpawnDropItems()
{
	ARAEnemyBase* Enemy = GetOwnerEnemy();
	if (!Enemy || Enemy->bHasDroppedItems)
	{
		return;
	}

	Enemy->bHasDroppedItems = true;

	if (Enemy->DropItems.Num() <= 0)
	{
		return;
	}

	if (!Enemy->DropItemActorClass)
	{
		UE_LOG(LogTemp, Warning, TEXT("[%s] DropItemActorClass is missing"), *Enemy->ActorSaveID.ToString());
		return;
	}

	UWorld* World = Enemy->GetWorld();
	if (!World)
	{
		UE_LOG(LogTemp, Warning, TEXT("[%s] Failed to spawn drop items: World is null"), *Enemy->ActorSaveID.ToString());
		return;
	}

	ARAWorldStateManager* WorldStateManager = nullptr;
	for (TActorIterator<ARAWorldStateManager> It(World); It; ++It)
	{
		WorldStateManager = *It;
		break;
	}

	int32 SpawnedDropNumber = 0;

	for (const FDropItemData& DropItemData : Enemy->DropItems)
	{
		if (DropItemData.ItemID.IsNone())
		{
			UE_LOG(LogTemp, Warning, TEXT("[%s] Skipped drop item: ItemID is None"), *Enemy->ActorSaveID.ToString());
			continue;
		}

		const float SafeDropRate = FMath::Clamp(DropItemData.DropRate, 0.0f, 1.0f);
		if (FMath::FRand() > SafeDropRate)
		{
			continue;
		}

		const int32 SafeMinCount = FMath::Max(1, DropItemData.MinCount);
		const int32 SafeMaxCount = FMath::Max(SafeMinCount, DropItemData.MaxCount);
		const int32 DropCount = FMath::RandRange(SafeMinCount, SafeMaxCount);

		const FVector RandomOffset(
			FMath::FRandRange(-80.f, 80.f),
			FMath::FRandRange(-80.f, 80.f),
			30.f
		);
		const FVector SpawnLocation = Enemy->GetActorLocation() + RandomOffset;
		const FRotator SpawnRotation = FRotator::ZeroRotator;
		const FTransform SpawnTransform(SpawnRotation, SpawnLocation);

		FActorSpawnParameters SpawnParams;
		SpawnParams.Owner = Enemy;
		SpawnParams.Instigator = Enemy;
		SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;

		ADropItemActor* DropItemActor = World->SpawnActorDeferred<ADropItemActor>(
			Enemy->DropItemActorClass,
			SpawnTransform,
			SpawnParams.Owner,
			SpawnParams.Instigator,
			SpawnParams.SpawnCollisionHandlingOverride
		);

		if (!DropItemActor)
		{
			UE_LOG(LogTemp, Warning, TEXT("[%s] Failed to spawn drop item: %s"),
				*Enemy->ActorSaveID.ToString(),
				*DropItemData.ItemID.ToString());
			continue;
		}

		++SpawnedDropNumber;
		const FName RuntimeItemSaveID(*FString::Printf(
			TEXT("%s_%s_%d"),
			*Enemy->ActorSaveID.ToString(),
			*DropItemData.ItemID.ToString(),
			SpawnedDropNumber
		));

		DropItemActor->InitializeRuntimeDropItem(
			DropItemData.ItemID,
			DropCount,
			RuntimeItemSaveID
		);

		if (WorldStateManager && !WorldStateManager->MapID.IsNone())
		{
			if (URAGameInstance* RAGameInstance = World->GetGameInstance<URAGameInstance>())
			{
				FSpawnedDropItemRuntimeData RuntimeDropData;
				RuntimeDropData.ItemSaveID = RuntimeItemSaveID;
				RuntimeDropData.ItemID = DropItemData.ItemID;
				RuntimeDropData.Count = DropCount;
				RuntimeDropData.SpawnTransform = SpawnTransform;
				RuntimeDropData.DropItemActorClass = DropItemActor->GetClass();

				RAGameInstance->RegisterSpawnedDropItem(WorldStateManager->MapID, RuntimeDropData);
			}
		}
		else
		{
			UE_LOG(LogTemp, Warning, TEXT("[%s] Runtime drop state was not registered: WorldStateManager or MapID is missing. ItemSaveID=%s"),
				*Enemy->ActorSaveID.ToString(),
				*RuntimeItemSaveID.ToString());
		}

		UGameplayStatics::FinishSpawningActor(DropItemActor, SpawnTransform);

		UE_LOG(LogTemp, Warning, TEXT("[%s] Spawned drop item: %s x%d / ItemSaveID=%s"),
			*Enemy->ActorSaveID.ToString(),
			*DropItemData.ItemID.ToString(),
			DropCount,
			*RuntimeItemSaveID.ToString());
	}
}
