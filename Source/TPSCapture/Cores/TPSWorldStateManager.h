#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "TPSWorldStateManager.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnWorldProgressChanged);

UCLASS()
class TPSCAPTURE_API ATPSWorldStateManager : public AActor
{
	GENERATED_BODY()

public:
	ATPSWorldStateManager();

protected:
	virtual void BeginPlay() override;

public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "World State")
	FName MapID = NAME_None;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "World State")
	int32 AliveEnemyCount = 0;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "World State")
	int32 UnrescuedAnimalCount = 0;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "World State")
	int32 TotalAnimalCount = 0;

	UPROPERTY(BlueprintAssignable, Category = "World State")
	FOnWorldProgressChanged OnWorldProgressChanged;

	UFUNCTION(BlueprintPure, Category = "World State")
	int32 GetRescuedAnimalCount() const
	{
		return FMath::Clamp(TotalAnimalCount - UnrescuedAnimalCount, 0, TotalAnimalCount);
	}

public:
	UFUNCTION(BlueprintCallable, Category = "World State")
	void NotifyEnemyDefeated(FName ActorSaveID);

	UFUNCTION(BlueprintCallable, Category = "World State")
	void NotifyAnimalRescued(FName AnimalSaveID);

	UFUNCTION(BlueprintCallable, Category = "World State")
	void NotifyItemPicked(FName ItemSaveID);

	UFUNCTION(BlueprintCallable, Category = "World State")
	void CheckAndHandleMapClear();

	UFUNCTION(BlueprintCallable, Category = "World State")
	void ValidateDuplicateSaveIDs();

protected:
	void ApplySavedWorldState();

#pragma region Runtime Spawned Drop Items
	void RestoreSpawnedDropItems();
#pragma endregion Runtime Spawned Drop Items

	FName GetSaveIDFromActor(const AActor* Actor, FName PropertyName) const;

	void ValidateDuplicateID(
		FName SaveID,
		const AActor* Actor,
		TMap<FName, const AActor*>& SeenIDs,
		const FString& TypeName
	) const;
};
