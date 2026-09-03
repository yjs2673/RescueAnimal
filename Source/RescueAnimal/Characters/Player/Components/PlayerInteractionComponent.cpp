#include "PlayerInteractionComponent.h"

#include "RACharacter.h"
#include "AnimalBase.h"
#include "AnimalRescueComponent.h"
#include "InventoryComponent.h"
#include "LobbyNPC.h"
#include "PlayerEquipmentComponent.h"
#include "PlayerStatComponent.h"
#include "PortalActor.h"
#include "QuickSlotComponent.h"
#include "RAGameInstance.h"
#include "RAStructTypes.h"
#include "ShopActor.h"
#include "WeaponBase.h"

#include "EngineUtils.h"
#include "Kismet/GameplayStatics.h"
#include "Sound/SoundBase.h"

UPlayerInteractionComponent::UPlayerInteractionComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UPlayerInteractionComponent::BeginPlay()
{
	Super::BeginPlay();
}

ARACharacter* UPlayerInteractionComponent::GetOwnerCharacter() const
{
	return Cast<ARACharacter>(GetOwner());
}

void UPlayerInteractionComponent::Interact()
{
	ARACharacter* Character = GetOwnerCharacter();
	if (!Character || Character->IsPortalTransitionInputLocked())
		return;

	if (Character->StatComponent && Character->StatComponent->IsDead())
		return;

	if (TryRescueNearbyAnimal())
		return;

	if (Character->CurrentShop)
	{
		Character->CurrentShop->Interact(Character);
		return;
	}

	if (Character->CurrentLobbyNPC)
	{
		Character->CurrentLobbyNPC->Interact();
		return;
	}

	if (Character->CurrentPortal)
	{
		Character->CurrentPortal->Interact(Character);
		return;
	}
}

bool UPlayerInteractionComponent::TryRescueNearbyAnimal()
{
	ARACharacter* Character = GetOwnerCharacter();
	if (!Character)
		return false;

	AAnimalBase* RescueAnimal = FindNearbyRescueAnimal();
	if (!RescueAnimal)
	{
		return false;
	}

	if (!IsRescueKitEquipped())
	{
		UE_LOG(LogTemplateCharacter, Warning, TEXT("Animal rescue failed: rescue kit is not equipped."));
		return true;
	}

	UAnimalRescueComponent* AnimalRescueComponent = RescueAnimal->GetAnimalRescueComponent();
	if (!AnimalRescueComponent || !AnimalRescueComponent->CanBeRescued())
	{
		UE_LOG(LogTemplateCharacter, Warning, TEXT("Animal rescue failed: camp is not cleared or animal is not trapped."));
		return true;
	}

	if (!AnimalRescueComponent->Rescue())
	{
		UE_LOG(LogTemplateCharacter, Warning, TEXT("Animal rescue failed: Rescue() returned false."));
		return true;
	}

	UE_LOG(LogTemplateCharacter, Warning, TEXT("Animal rescued: %s"), *RescueAnimal->GetName());
	return true;
}

AAnimalBase* UPlayerInteractionComponent::FindNearbyRescueAnimal() const
{
	const ARACharacter* Character = GetOwnerCharacter();
	UWorld* World = Character ? Character->GetWorld() : nullptr;
	if (!World)
	{
		return nullptr;
	}

	AAnimalBase* BestAnimal = nullptr;
	float BestDistanceSquared = FMath::Square(Character->AnimalRescueInteractDistance);

	for (TActorIterator<AAnimalBase> It(World); It; ++It)
	{
		AAnimalBase* Animal = *It;
		if (!IsValid(Animal) || !Animal->IsTrapped())
		{
			continue;
		}

		const float DistanceSquared = FVector::DistSquared(Character->GetActorLocation(), Animal->GetActorLocation());
		if (DistanceSquared <= BestDistanceSquared)
		{
			BestDistanceSquared = DistanceSquared;
			BestAnimal = Animal;
		}
	}

	return BestAnimal;
}

bool UPlayerInteractionComponent::IsRescueKitEquipped() const
{
	const ARACharacter* Character = GetOwnerCharacter();
	return Character && Character->CurrentWeapon && Character->CurrentWeapon->WeaponType == EWeaponType::Kit;
}

void UPlayerInteractionComponent::UseQuickSlotItem(int32 SlotIndex)
{
	ARACharacter* Character = GetOwnerCharacter();
	if (!Character || Character->IsPortalTransitionInputLocked() || !Character->QuickSlotComponent)
		return;

	const FName ItemID = Character->QuickSlotComponent->GetSlotItem(SlotIndex);
	if (ItemID.IsNone())
	{
		UE_LOG(LogTemplateCharacter, Warning, TEXT("QuickSlot %d is empty"), SlotIndex + 1);
		return;
	}

	UseInventoryItem(ItemID);
}

bool UPlayerInteractionComponent::UseInventoryItem(FName ItemID)
{
	ARACharacter* Character = GetOwnerCharacter();
	if (!Character || Character->IsPortalTransitionInputLocked() || ItemID.IsNone())
		return false;

	URAGameInstance* RAGameInstance = Character->GetGameInstance<URAGameInstance>();
	if (!RAGameInstance)
		return false;

	FItemData ItemData;
	if (!RAGameInstance->GetItemDataByID(ItemID, ItemData))
		return false;

	switch (ItemData.ItemType)
	{
	case EItemType::Consumable:
		return UseConsumableItem(ItemID);
	case EItemType::Weapon:
		if (UPlayerEquipmentComponent* PlayerEquipmentComponent = Character->GetPlayerEquipmentComponent())
		{
			return PlayerEquipmentComponent->EquipWeaponFromInventory(ItemID);
		}
		return false;
	default:
		return false;
	}
}

bool UPlayerInteractionComponent::UseConsumableItem(FName ItemID)
{
	ARACharacter* Character = GetOwnerCharacter();
	if (!Character || Character->IsPortalTransitionInputLocked())
		return false;

	if (Character->StatComponent && Character->StatComponent->IsDead())
		return false;

	if (ItemID.IsNone())
		return false;

	if (!Character->InventoryComponent || !Character->InventoryComponent->HasItem(ItemID, 1))
	{
		UE_LOG(LogTemplateCharacter, Warning, TEXT("Consumable item not found in inventory: %s"), *ItemID.ToString());
		return false;
	}

	URAGameInstance* RAGameInstance = Character->GetGameInstance<URAGameInstance>();
	if (!RAGameInstance)
	{
		UE_LOG(LogTemplateCharacter, Warning, TEXT("UseConsumableItem failed: RAGameInstance is null"));
		return false;
	}

	FItemData ItemData;
	if (!RAGameInstance->GetItemDataByID(ItemID, ItemData))
	{
		UE_LOG(LogTemplateCharacter, Warning, TEXT("UseConsumableItem failed: ItemData not found. ItemID=%s"), *ItemID.ToString());
		return false;
	}

	if (ItemData.ItemType != EItemType::Consumable)
	{
		UE_LOG(LogTemplateCharacter, Warning, TEXT("Item is not consumable: %s"), *ItemID.ToString());
		return false;
	}

	bool bUseSucceeded = false;

	switch (ItemData.ConsumableType)
	{
	case EConsumableType::Heal:
	{
		if (!Character->StatComponent)
		{
			UE_LOG(LogTemplateCharacter, Warning, TEXT("Heal item failed: StatComponent is null"));
			return false;
		}

		const float OldHP = Character->StatComponent->GetCurrentHP();
		Character->StatComponent->Heal(ItemData.HealAmount);
		const float NewHP = Character->StatComponent->GetCurrentHP();
		bUseSucceeded = true;

		UE_LOG(LogTemplateCharacter, Warning, TEXT("Heal item used: %s | HP %.1f -> %.1f"), *ItemID.ToString(), OldHP, NewHP);
		break;
	}
	case EConsumableType::Buff:
	{
		if (!Character->StatComponent)
		{
			UE_LOG(LogTemplateCharacter, Warning, TEXT("Buff item failed: StatComponent is null"));
			return false;
		}

		if (!Character->StatComponent->ApplyBuffItem(ItemData))
		{
			UE_LOG(LogTemplateCharacter, Warning, TEXT("Buff item failed: %s"), *ItemID.ToString());
			return false;
		}

		bUseSucceeded = true;
		UE_LOG(LogTemplateCharacter, Warning, TEXT("Buff item used: %s | Type=%d Value=%.2f Duration=%.2f"),
			*ItemID.ToString(),
			static_cast<uint8>(ItemData.BuffTargetType),
			ItemData.BuffValue,
			ItemData.BuffDuration);
		break;
	}
	case EConsumableType::Capture:
		UE_LOG(LogTemplateCharacter, Warning, TEXT("Capture consumable is not implemented yet: %s"), *ItemID.ToString());
		return false;
	default:
		UE_LOG(LogTemplateCharacter, Warning, TEXT("Unknown consumable type: %s"), *ItemID.ToString());
		return false;
	}

	if (!bUseSucceeded)
	{
		UE_LOG(LogTemplateCharacter, Warning, TEXT("Consumable use failed: %s"), *ItemID.ToString());
		return false;
	}

	if (!Character->InventoryComponent->RemoveItem(ItemID, 1))
	{
		UE_LOG(LogTemplateCharacter, Warning, TEXT("Failed to consume inventory item after use: %s"), *ItemID.ToString());
		return false;
	}

	if (Character->ConsumableUseSound)
	{
		UGameplayStatics::PlaySoundAtLocation(Character, Character->ConsumableUseSound, Character->GetActorLocation());
	}

	UE_LOG(LogTemplateCharacter, Warning, TEXT("Consumable item consumed: %s"), *ItemID.ToString());
	return true;
}

void UPlayerInteractionComponent::SetCurrentPortal(APortalActor* NewPortal)
{
	ARACharacter* Character = GetOwnerCharacter();
	if (!Character)
		return;

	Character->CurrentPortal = NewPortal;
	UE_LOG(LogTemp, Warning, TEXT("Current Portal Set"));
}

void UPlayerInteractionComponent::ClearCurrentPortal(APortalActor* PortalToClear)
{
	ARACharacter* Character = GetOwnerCharacter();
	if (!Character)
		return;

	if (Character->CurrentPortal == PortalToClear)
	{
		Character->CurrentPortal = nullptr;
		UE_LOG(LogTemp, Warning, TEXT("Current Portal Cleared"));
	}
}

void UPlayerInteractionComponent::SetCurrentShop(AShopActor* NewShop)
{
	ARACharacter* Character = GetOwnerCharacter();
	if (Character)
	{
		Character->CurrentShop = NewShop;
	}
}

void UPlayerInteractionComponent::ClearCurrentShop(AShopActor* ShopToClear)
{
	ARACharacter* Character = GetOwnerCharacter();
	if (Character && Character->CurrentShop == ShopToClear)
	{
		Character->CurrentShop = nullptr;
	}
}

void UPlayerInteractionComponent::SetCurrentLobbyNPC(ALobbyNPC* NewLobbyNPC)
{
	ARACharacter* Character = GetOwnerCharacter();
	if (Character)
	{
		Character->CurrentLobbyNPC = NewLobbyNPC;
	}
}

void UPlayerInteractionComponent::ClearCurrentLobbyNPC(ALobbyNPC* LobbyNPCToClear)
{
	ARACharacter* Character = GetOwnerCharacter();
	if (Character && Character->CurrentLobbyNPC == LobbyNPCToClear)
	{
		Character->CurrentLobbyNPC = nullptr;
	}
}
