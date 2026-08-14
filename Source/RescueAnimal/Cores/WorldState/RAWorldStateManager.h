#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "RAWorldStateManager.generated.h"

class USoundBase;

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnWorldProgressChanged);

UCLASS()
class RESCUEANIMAL_API ARAWorldStateManager : public AActor
{
	GENERATED_BODY()

public:
	ARAWorldStateManager();

protected:
	virtual void BeginPlay() override;

public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "World State")
	FName MapID = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Audio")
	TObjectPtr<USoundBase> MapBGM = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Audio", meta = (ClampMin = "0.0"))
	float BGMFadeInTime = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Audio", meta = (ClampMin = "0.0"))
	float BGMFadeOutTime = 1.0f;

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
