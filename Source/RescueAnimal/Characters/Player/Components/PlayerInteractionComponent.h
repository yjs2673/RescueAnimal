#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "PlayerInteractionComponent.generated.h"

class AAnimalBase;
class APortalActor;
class AShopActor;
class ALobbyNPC;
class ARACharacter;

UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class RESCUEANIMAL_API UPlayerInteractionComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UPlayerInteractionComponent();

	virtual void BeginPlay() override;

	void Interact();
	bool TryRescueNearbyAnimal();
	AAnimalBase* FindNearbyRescueAnimal() const;
	bool IsRescueKitEquipped() const;
	void UseQuickSlotItem(int32 SlotIndex);
	bool UseInventoryItem(FName ItemID);
	bool UseConsumableItem(FName ItemID);

	void SetCurrentPortal(APortalActor* NewPortal);
	void ClearCurrentPortal(APortalActor* PortalToClear);
	void SetCurrentShop(AShopActor* NewShop);
	void ClearCurrentShop(AShopActor* ShopToClear);
	void SetCurrentLobbyNPC(ALobbyNPC* NewLobbyNPC);
	void ClearCurrentLobbyNPC(ALobbyNPC* LobbyNPCToClear);

private:
	ARACharacter* GetOwnerCharacter() const;
};
