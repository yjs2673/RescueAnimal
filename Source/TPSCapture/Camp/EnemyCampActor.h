#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "EnemyCampActor.generated.h"

class UBoxComponent;
class ATPSEnemyBase;
class AAnimalBase;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnEnemyCampCleared, AEnemyCampActor*, ClearedCamp);

UCLASS()
class TPSCAPTURE_API AEnemyCampActor : public AActor
{
	GENERATED_BODY()

public:
	AEnemyCampActor();

protected:
	virtual void BeginPlay() override;

public:
	UFUNCTION(BlueprintCallable, Category = "Camp")
	void RegisterEnemy(ATPSEnemyBase* Enemy);

	UFUNCTION(BlueprintCallable, Category = "Camp")
	void RegisterAnimal(AAnimalBase* Animal);

	UFUNCTION(BlueprintCallable, Category = "Camp")
	void RefreshCampMembers();

	UFUNCTION(BlueprintCallable, Category = "Camp")
	void CheckCampCleared();

	UFUNCTION(BlueprintPure, Category = "Camp")
	bool IsCampCleared() const { return bIsCampCleared; }

	UFUNCTION(BlueprintPure, Category = "Camp")
	int32 GetAliveEnemyCount() const;

	UFUNCTION(BlueprintPure, Category = "Camp")
	const TArray<ATPSEnemyBase*>& GetCampEnemies() const { return CampEnemies; }

	UFUNCTION(BlueprintPure, Category = "Camp")
	const TArray<AAnimalBase*>& GetCampAnimals() const { return CampAnimals; }

	UPROPERTY(BlueprintAssignable, Category = "Camp")
	FOnEnemyCampCleared OnEnemyCampCleared;

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Camp")
	TObjectPtr<UBoxComponent> CampBounds;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Camp")
	bool bAutoCollectMembersOnBeginPlay = true;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Camp", meta = (ClampMin = "0.1"))
	float ClearCheckInterval = 0.5f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Camp")
	TArray<TObjectPtr<ATPSEnemyBase>> CampEnemies;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Camp")
	TArray<TObjectPtr<AAnimalBase>> CampAnimals;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Camp")
	bool bIsCampCleared = false;

	FTimerHandle ClearCheckTimerHandle;

	bool IsActorInsideCampBounds(const AActor* Actor) const;
	void HandleCampCleared();
};