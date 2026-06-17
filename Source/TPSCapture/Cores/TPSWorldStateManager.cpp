#include "TPSWorldStateManager.h"

#include "DropItemActor.h"
#include "TPSEnemyBase.h"
#include "TPSAnimalBase.h"
#include "TPSGameInstance.h"

#include "Engine/World.h"
#include "EngineUtils.h"
#include "UObject/UnrealType.h"

ATPSWorldStateManager::ATPSWorldStateManager()
{
	PrimaryActorTick.bCanEverTick = false;
}

void ATPSWorldStateManager::BeginPlay()
{
	Super::BeginPlay();

	if (MapID.IsNone())
	{
		UE_LOG(LogTemp, Warning, TEXT("[WorldStateManager] MapID is None. Actor=%s"), *GetName());
	}

	ValidateDuplicateSaveIDs();
	ApplySavedWorldState();
	CheckAndHandleMapClear();
}

void ATPSWorldStateManager::ApplySavedWorldState()
{
	AliveEnemyCount = 0;
	UnrescuedAnimalCount = 0;

	UTPSGameInstance* TPSGameInstance = GetWorld() ? GetWorld()->GetGameInstance<UTPSGameInstance>() : nullptr;
	if (!TPSGameInstance)
	{
		UE_LOG(LogTemp, Warning, TEXT("[WorldStateManager] TPSGameInstance is null."));
		return;
	}

	for (TActorIterator<ATPSEnemyBase> It(GetWorld()); It; ++It)
	{
		ATPSEnemyBase* Enemy = *It;
		if (!Enemy)
		{
			continue;
		}

		const FName ActorSaveID = GetSaveIDFromActor(Enemy, TEXT("ActorSaveID"));
		if (ActorSaveID.IsNone())
		{
			UE_LOG(LogTemp, Warning, TEXT("[WorldStateManager] Enemy ActorSaveID is None. Actor=%s"), *Enemy->GetName());
			AliveEnemyCount++;
			continue;
		}

		if (TPSGameInstance->IsEnemyDefeated(MapID, ActorSaveID))
		{
			Enemy->Destroy();
			continue;
		}

		AliveEnemyCount++;
	}

	for (TActorIterator<AAnimalBase> It(GetWorld()); It; ++It)
	{
		AAnimalBase* Animal = *It;
		if (!Animal)
		{
			continue;
		}

		const FName AnimalSaveID = GetSaveIDFromActor(Animal, TEXT("AnimalSaveID"));
		if (AnimalSaveID.IsNone())
		{
			UE_LOG(LogTemp, Warning, TEXT("[WorldStateManager] AnimalSaveID is None. Actor=%s"), *Animal->GetName());

			if (!Animal->IsRescued())
			{
				UnrescuedAnimalCount++;
			}
			continue;
		}

		if (TPSGameInstance->IsAnimalRescued(MapID, AnimalSaveID))
		{
#pragma region Runtime Animal Restore
			Animal->ApplyRuntimeRescuedState();
#pragma endregion Runtime Animal Restore
			continue;
		}

		if (!Animal->IsRescued())
		{
			UnrescuedAnimalCount++;
		}
	}

	for (TActorIterator<ADropItemActor> It(GetWorld()); It; ++It)
	{
		ADropItemActor* DropItem = *It;
		if (!DropItem)
		{
			continue;
		}

		const FName ItemSaveID = GetSaveIDFromActor(DropItem, TEXT("ItemSaveID"));
		if (ItemSaveID.IsNone())
		{
			UE_LOG(LogTemp, Warning, TEXT("[WorldStateManager] ItemSaveID is None. Actor=%s"), *DropItem->GetName());
			continue;
		}

		if (TPSGameInstance->IsItemPicked(MapID, ItemSaveID))
		{
			DropItem->Destroy();
		}
	}
}

void ATPSWorldStateManager::NotifyEnemyDefeated(FName ActorSaveID)
{
	if (MapID.IsNone() || ActorSaveID.IsNone())
	{
		UE_LOG(LogTemp, Warning, TEXT("[WorldStateManager] NotifyEnemyDefeated failed: MapID or ActorSaveID is None."));
		return;
	}

	if (UTPSGameInstance* TPSGameInstance = GetWorld() ? GetWorld()->GetGameInstance<UTPSGameInstance>() : nullptr)
	{
		TPSGameInstance->RegisterDefeatedEnemy(MapID, ActorSaveID);
	}

	AliveEnemyCount = FMath::Max(0, AliveEnemyCount - 1);
	CheckAndHandleMapClear();
}

void ATPSWorldStateManager::NotifyAnimalRescued(FName AnimalSaveID)
{
	if (MapID.IsNone() || AnimalSaveID.IsNone())
	{
		UE_LOG(LogTemp, Warning, TEXT("[WorldStateManager] NotifyAnimalRescued failed: MapID or AnimalSaveID is None."));
		return;
	}

	if (UTPSGameInstance* TPSGameInstance = GetWorld() ? GetWorld()->GetGameInstance<UTPSGameInstance>() : nullptr)
	{
		TPSGameInstance->RegisterRescuedAnimal(MapID, AnimalSaveID);
	}

	UnrescuedAnimalCount = FMath::Max(0, UnrescuedAnimalCount - 1);
	CheckAndHandleMapClear();
}

void ATPSWorldStateManager::NotifyItemPicked(FName ItemSaveID)
{
	if (MapID.IsNone() || ItemSaveID.IsNone())
	{
		UE_LOG(LogTemp, Warning, TEXT("[WorldStateManager] NotifyItemPicked failed: MapID or ItemSaveID is None."));
		return;
	}

	if (UTPSGameInstance* TPSGameInstance = GetWorld() ? GetWorld()->GetGameInstance<UTPSGameInstance>() : nullptr)
	{
		TPSGameInstance->RegisterPickedItem(MapID, ItemSaveID);
	}
}

void ATPSWorldStateManager::CheckAndHandleMapClear()
{
	if (AliveEnemyCount > 0 || UnrescuedAnimalCount > 0)
	{
		return;
	}

	if (MapID.IsNone())
	{
		UE_LOG(LogTemp, Warning, TEXT("[WorldStateManager] Map clear skipped: MapID is None."));
		return;
	}

	if (UTPSGameInstance* TPSGameInstance = GetWorld() ? GetWorld()->GetGameInstance<UTPSGameInstance>() : nullptr)
	{
		TPSGameInstance->SetMapCleared(MapID, true);
	}

	UE_LOG(LogTemp, Warning, TEXT("[WorldStateManager] Map cleared. MapID=%s"), *MapID.ToString());
}

void ATPSWorldStateManager::ValidateDuplicateSaveIDs()
{
	TMap<FName, const AActor*> SeenIDs;

	for (TActorIterator<ATPSEnemyBase> It(GetWorld()); It; ++It)
	{
		ValidateDuplicateID(GetSaveIDFromActor(*It, TEXT("ActorSaveID")), *It, SeenIDs, TEXT("Enemy"));
	}

	for (TActorIterator<AAnimalBase> It(GetWorld()); It; ++It)
	{
		ValidateDuplicateID(GetSaveIDFromActor(*It, TEXT("AnimalSaveID")), *It, SeenIDs, TEXT("Animal"));
	}

	for (TActorIterator<ADropItemActor> It(GetWorld()); It; ++It)
	{
		ValidateDuplicateID(GetSaveIDFromActor(*It, TEXT("ItemSaveID")), *It, SeenIDs, TEXT("Item"));
	}
}

FName ATPSWorldStateManager::GetSaveIDFromActor(const AActor* Actor, FName PropertyName) const
{
	if (!Actor || PropertyName.IsNone())
	{
		return NAME_None;
	}

	const FNameProperty* SaveIDProperty = FindFProperty<FNameProperty>(Actor->GetClass(), PropertyName);
	if (!SaveIDProperty)
	{
		UE_LOG(LogTemp, Warning, TEXT("[WorldStateManager] SaveID property not found. Actor=%s Property=%s"),
			*Actor->GetName(),
			*PropertyName.ToString());
		return NAME_None;
	}

	return SaveIDProperty->GetPropertyValue_InContainer(Actor);
}

void ATPSWorldStateManager::ValidateDuplicateID(
	FName SaveID,
	const AActor* Actor,
	TMap<FName, const AActor*>& SeenIDs,
	const FString& TypeName
) const
{
	if (!Actor)
	{
		return;
	}

	if (SaveID.IsNone())
	{
		UE_LOG(LogTemp, Warning, TEXT("[WorldStateManager] %s SaveID is None. Actor=%s"),
			*TypeName,
			*Actor->GetName());
		return;
	}

	if (const AActor** ExistingActor = SeenIDs.Find(SaveID))
	{
		UE_LOG(LogTemp, Warning, TEXT("[WorldStateManager] Duplicate SaveID detected. SaveID=%s Existing=%s Duplicate=%s Type=%s"),
			*SaveID.ToString(),
			*GetNameSafe(*ExistingActor),
			*GetNameSafe(Actor),
			*TypeName);
		return;
	}

	SeenIDs.Add(SaveID, Actor);
}
