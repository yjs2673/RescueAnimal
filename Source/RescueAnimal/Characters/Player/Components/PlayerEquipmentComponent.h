#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "RAGameEnums.h"
#include "PlayerEquipmentComponent.generated.h"

class ARACharacter;
class AWeaponBase;

UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class RESCUEANIMAL_API UPlayerEquipmentComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UPlayerEquipmentComponent();

	virtual void BeginPlay() override;

	void HandleWeaponInteract();
	void EquipWeapon(AWeaponBase* NewWeapon);
	void UnequipWeapon();
	void DropCurrentWeapon();
	void SetNearbyWeapon(AWeaponBase* NewWeapon);
	void ClearNearbyWeapon(AWeaponBase* WeaponToClear);
	bool EquipWeaponFromInventory(FName ItemID);
	bool UnequipCurrentWeaponToInventory();
	EWeaponType GetCurrentWeaponType() const;
	FName GetCurrentWeaponItemID() const;
	AWeaponBase* GetCurrentWeapon() const;

private:
	ARACharacter* GetOwnerCharacter() const;
};
