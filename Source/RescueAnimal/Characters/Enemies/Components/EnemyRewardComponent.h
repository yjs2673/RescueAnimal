#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "EnemyRewardComponent.generated.h"

class ARAEnemyBase;

UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class RESCUEANIMAL_API UEnemyRewardComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UEnemyRewardComponent();

	virtual void BeginPlay() override;

	void GrantEXPToKiller();
	void SpawnDropItems();

private:
	ARAEnemyBase* GetOwnerEnemy() const;
};
