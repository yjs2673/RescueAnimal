#include "RAWorldStateManager.h"

#include "DropItemActor.h"
#include "RAEnemyBase.h"
#include "AnimalBase.h"
#include "RAGameInstance.h"
#include "RAPlayerController.h"

#include "RAAudioSubsystem.h"
#include "Sound/SoundBase.h"

#include "Engine/World.h"
#include "EngineUtils.h"
#include "Kismet/GameplayStatics.h"
#include "UObject/UnrealType.h"

ARAWorldStateManager::ARAWorldStateManager()
{
	PrimaryActorTick.bCanEverTick = false;
}

void ARAWorldStateManager::BeginPlay()
{
	Super::BeginPlay();

	if (MapID.IsNone())
	{
		UE_LOG(LogTemp, Warning, TEXT("[WorldStateManager] MapID is None. Actor=%s"), *GetName());
	}

	ValidateDuplicateSaveIDs();
	ApplySavedWorldState();
	CheckAndHandleMapClear();

	if (UGameInstance* GameInstance = GetGameInstance())
	{
		if (URAAudioSubsystem* AudioSubsystem = GameInstance->GetSubsystem<URAAudioSubsystem>())
		{
			AudioSubsystem->PlayBGM(MapBGM, BGMFadeInTime, BGMFadeOutTime);
		}
	}
}

void ARAWorldStateManager::ApplySavedWorldState()
{
	AliveEnemyCount = 0;
	UnrescuedAnimalCount = 0;
	TotalAnimalCount = 0;

	URAGameInstance* RAGameInstance = GetWorld() ? GetWorld()->GetGameInstance<URAGameInstance>() : nullptr;
	if (!RAGameInstance)
	{
		UE_LOG(LogTemp, Warning, TEXT("[WorldStateManager] RAGameInstance is null."));
		return;
	}

	for (TActorIterator<ARAEnemyBase> It(GetWorld()); It; ++It)
	{
		ARAEnemyBase* Enemy = *It;
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

		if (RAGameInstance->IsEnemyDefeated(MapID, ActorSaveID))
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

		TotalAnimalCount++;

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

#pragma region Runtime Animal Restore
		if (RAGameInstance->IsAnimalRescued(MapID, AnimalSaveID))
		{
			Animal->ApplyRuntimeRescuedState();
			continue;
		}
#pragma endregion Runtime Animal Restore

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

		if (RAGameInstance->IsItemPicked(MapID, ItemSaveID))
		{
			DropItem->Destroy();
		}
	}

	RestoreSpawnedDropItems();
	OnWorldProgressChanged.Broadcast();
}

#pragma region Runtime Spawned Drop Items
void ARAWorldStateManager::RestoreSpawnedDropItems()
{
	if (MapID.IsNone())
	{
		UE_LOG(LogTemp, Warning, TEXT("[WorldStateManager] Runtime drop restore skipped: MapID is None."));
		return;
	}

	UWorld* World = GetWorld();
	URAGameInstance* RAGameInstance = World ? World->GetGameInstance<URAGameInstance>() : nullptr;
	if (!World || !RAGameInstance)
	{
		UE_LOG(LogTemp, Warning, TEXT("[WorldStateManager] Runtime drop restore skipped: World or RAGameInstance is null."));
		return;
	}

	TSet<FName> ExistingItemSaveIDs;
	for (TActorIterator<ADropItemActor> It(World); It; ++It)
	{
		if (ADropItemActor* ExistingDropItem = *It)
		{
			const FName ExistingItemSaveID = ExistingDropItem->GetItemSaveID();
			if (!ExistingItemSaveID.IsNone())
			{
				ExistingItemSaveIDs.Add(ExistingItemSaveID);
			}
		}
	}

	const TArray<FSpawnedDropItemRuntimeData> SpawnedDropItems = RAGameInstance->GetSpawnedDropItems(MapID);
	for (const FSpawnedDropItemRuntimeData& DropItemData : SpawnedDropItems)
	{
		if (DropItemData.ItemSaveID.IsNone() || DropItemData.ItemID.IsNone())
		{
			UE_LOG(LogTemp, Warning, TEXT("[WorldStateManager] Invalid runtime drop data skipped."));
			continue;
		}

		if (RAGameInstance->IsItemPicked(MapID, DropItemData.ItemSaveID) ||
			ExistingItemSaveIDs.Contains(DropItemData.ItemSaveID))
		{
			continue;
		}

		if (!DropItemData.DropItemActorClass)
		{
			UE_LOG(LogTemp, Warning, TEXT("[WorldStateManager] Runtime drop class is null. ItemSaveID=%s"),
				*DropItemData.ItemSaveID.ToString());
			continue;
		}

		ADropItemActor* RestoredDropItem = World->SpawnActorDeferred<ADropItemActor>(
			DropItemData.DropItemActorClass,
			DropItemData.SpawnTransform,
			nullptr,
			nullptr,
			ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn
		);

		if (!RestoredDropItem)
		{
			UE_LOG(LogTemp, Warning, TEXT("[WorldStateManager] Failed to restore runtime drop. ItemSaveID=%s"),
				*DropItemData.ItemSaveID.ToString());
			continue;
		}

		RestoredDropItem->InitializeRuntimeDropItem(
			DropItemData.ItemID,
			DropItemData.Count,
			DropItemData.ItemSaveID
		);
		UGameplayStatics::FinishSpawningActor(RestoredDropItem, DropItemData.SpawnTransform);
		ExistingItemSaveIDs.Add(DropItemData.ItemSaveID);

		UE_LOG(LogTemp, Warning, TEXT("[WorldStateManager] Restored runtime drop. ItemSaveID=%s ItemID=%s Count=%d"),
			*DropItemData.ItemSaveID.ToString(),
			*DropItemData.ItemID.ToString(),
			DropItemData.Count);
	}
}
#pragma endregion Runtime Spawned Drop Items

void ARAWorldStateManager::NotifyEnemyDefeated(FName ActorSaveID)
{
	if (MapID.IsNone() || ActorSaveID.IsNone())
	{
		UE_LOG(LogTemp, Warning, TEXT("[WorldStateManager] NotifyEnemyDefeated failed: MapID or ActorSaveID is None."));
		return;
	}

	if (URAGameInstance* RAGameInstance = GetWorld() ? GetWorld()->GetGameInstance<URAGameInstance>() : nullptr)
	{
		RAGameInstance->RegisterDefeatedEnemy(MapID, ActorSaveID);
	}

	AliveEnemyCount = FMath::Max(0, AliveEnemyCount - 1);
	CheckAndHandleMapClear();
}

void ARAWorldStateManager::NotifyAnimalRescued(FName AnimalSaveID)
{
	if (MapID.IsNone() || AnimalSaveID.IsNone())
	{
		UE_LOG(LogTemp, Warning, TEXT("[WorldStateManager] NotifyAnimalRescued failed: MapID or AnimalSaveID is None."));
		return;
	}

	bool bWasAlreadyRescued = false;
	if (URAGameInstance* RAGameInstance = GetWorld() ? GetWorld()->GetGameInstance<URAGameInstance>() : nullptr)
	{
		bWasAlreadyRescued = RAGameInstance->IsAnimalRescued(MapID, AnimalSaveID);
		RAGameInstance->RegisterRescuedAnimal(MapID, AnimalSaveID);
	}

	if (!bWasAlreadyRescued)
	{
		UnrescuedAnimalCount = FMath::Max(0, UnrescuedAnimalCount - 1);
	}

	OnWorldProgressChanged.Broadcast();
	CheckAndHandleMapClear();
}

void ARAWorldStateManager::NotifyItemPicked(FName ItemSaveID)
{
	if (MapID.IsNone() || ItemSaveID.IsNone())
	{
		UE_LOG(LogTemp, Warning, TEXT("[WorldStateManager] NotifyItemPicked failed: MapID or ItemSaveID is None."));
		return;
	}

	if (URAGameInstance* RAGameInstance = GetWorld() ? GetWorld()->GetGameInstance<URAGameInstance>() : nullptr)
	{
		RAGameInstance->RegisterPickedItem(MapID, ItemSaveID);
	}
}

void ARAWorldStateManager::CheckAndHandleMapClear()
{
#pragma region Game Progress
	if (UnrescuedAnimalCount > 0)
	{
		return;
	}

	if (MapID.IsNone())
	{
		UE_LOG(LogTemp, Warning, TEXT("[WorldStateManager] Map clear skipped: MapID is None."));
		return;
	}

	URAGameInstance* RAGameInstance = GetWorld() ? GetWorld()->GetGameInstance<URAGameInstance>() : nullptr;
	if (!RAGameInstance)
	{
		UE_LOG(LogTemp, Warning, TEXT("[WorldStateManager] Map clear skipped: RAGameInstance is null."));
		return;
	}

	if (!RAGameInstance->IsMapCleared(MapID))
	{
		RAGameInstance->SetMapCleared(MapID, true);
		UE_LOG(LogTemp, Warning, TEXT("[WorldStateManager] Map cleared by rescued-animal condition. MapID=%s"), *MapID.ToString());

#pragma region Game Progress Message
		if (ARAPlayerController* RAPlayerController = Cast<ARAPlayerController>(
			UGameplayStatics::GetPlayerController(this, 0)))
		{
			RAPlayerController->ShowFieldClearMessage(MapID);
		}
		else
		{
			UE_LOG(LogTemp, Warning, TEXT("[GameProgress] Field clear message skipped: RAPlayerController is null."));
		}
#pragma endregion Game Progress Message
	}
#pragma endregion Game Progress
}

void ARAWorldStateManager::ValidateDuplicateSaveIDs()
{
	TMap<FName, const AActor*> SeenIDs;

	for (TActorIterator<ARAEnemyBase> It(GetWorld()); It; ++It)
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

FName ARAWorldStateManager::GetSaveIDFromActor(const AActor* Actor, FName PropertyName) const
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

void ARAWorldStateManager::ValidateDuplicateID(
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
