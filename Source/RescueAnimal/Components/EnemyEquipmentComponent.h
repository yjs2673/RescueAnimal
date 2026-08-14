#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "EnemyEquipmentComponent.generated.h"

class ARAEnemyBase;
class AWeaponBase;

UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class RESCUEANIMAL_API UEnemyEquipmentComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UEnemyEquipmentComponent();

	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	void EquipDefaultWeapon();
	void EquipWeapon(AWeaponBase* NewWeapon);
	void SyncCombatDataFromWeapon();

private:
	ARAEnemyBase* GetOwnerEnemy() const;
};
