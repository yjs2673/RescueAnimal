#include "PlayerEquipmentComponent.h"

#include "RACharacter.h"
#include "InventoryComponent.h"
#include "PlayerCombatComponent.h"
#include "PlayerMovementComponent.h"
#include "PlayerStatComponent.h"
#include "RAGameInstance.h"
#include "RAStructTypes.h"
#include "WeaponBase.h"

#include "Components/SceneComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Sound/SoundBase.h"

UPlayerEquipmentComponent::UPlayerEquipmentComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UPlayerEquipmentComponent::BeginPlay()
{
	Super::BeginPlay();
}

ARACharacter* UPlayerEquipmentComponent::GetOwnerCharacter() const
{
	return Cast<ARACharacter>(GetOwner());
}

AWeaponBase* UPlayerEquipmentComponent::GetCurrentWeapon() const
{
	const ARACharacter* Character = GetOwnerCharacter();
	return Character ? Character->CurrentWeapon : nullptr;
}

void UPlayerEquipmentComponent::HandleWeaponInteract()
{
	ARACharacter* Character = GetOwnerCharacter();
	if (!Character || (Character->PlayerCombatComponent && Character->PlayerCombatComponent->IsAttacking()))
		return;

	if (Character->NearbyWeapon)
	{
		if (!Character->InventoryComponent)
			return;

		const FName WeaponItemID = Character->NearbyWeapon->WeaponID;
		if (WeaponItemID.IsNone())
		{
			UE_LOG(LogTemplateCharacter, Warning, TEXT("Pickup weapon failed: WeaponID is None. Weapon=%s"), *GetNameSafe(Character->NearbyWeapon));
			return;
		}

		URAGameInstance* RAGameInstance = Character->GetGameInstance<URAGameInstance>();
		FItemData ItemData;
		if (!RAGameInstance || !RAGameInstance->GetItemDataByID(WeaponItemID, ItemData) || ItemData.ItemType != EItemType::Weapon)
		{
			UE_LOG(LogTemplateCharacter, Warning, TEXT("Pickup weapon failed: ItemData is not weapon. ItemID=%s"), *WeaponItemID.ToString());
			return;
		}

		AWeaponBase* WeaponToPickup = Character->NearbyWeapon;
		Character->NearbyWeapon = nullptr;

		Character->InventoryComponent->AddItem(WeaponItemID, 1);
		WeaponToPickup->Destroy();

		if (Character->EquipmentSound)
		{
			UGameplayStatics::PlaySoundAtLocation(Character, Character->EquipmentSound, Character->GetActorLocation());
		}

		UE_LOG(LogTemplateCharacter, Warning, TEXT("Picked up weapon item: %s"), *WeaponItemID.ToString());
		return;
	}

	if (Character->CurrentWeapon)
	{
		DropCurrentWeapon();

		if (Character->EquipmentSound)
		{
			UGameplayStatics::PlaySoundAtLocation(Character, Character->EquipmentSound, Character->GetActorLocation());
		}
	}
}

void UPlayerEquipmentComponent::EquipWeapon(AWeaponBase* NewWeapon)
{
	ARACharacter* Character = GetOwnerCharacter();
	if (!Character)
		return;

	if (!NewWeapon)
	{
		UE_LOG(LogTemp, Warning, TEXT("EquipWeapon: NewWeapon is null"));
		return;
	}

	if (Character->CurrentWeapon == NewWeapon)
	{
		UE_LOG(LogTemp, Warning, TEXT("EquipWeapon: Weapon is already equipped"));
		return;
	}

	if (Character->CurrentWeapon)
	{
		UnequipWeapon();
	}

	Character->CurrentWeapon = NewWeapon;
	Character->CurrentWeapon->SetOwner(Character);
	Character->CurrentWeapon->SetPickupEnabled(false);
	Character->CurrentWeapon->UpdateWeaponVisualState();

	if (Character->CurrentWeapon->WeaponMesh)
	{
		Character->CurrentWeapon->WeaponMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		Character->CurrentWeapon->WeaponMesh->SetSimulatePhysics(false);
	}

	if (Character->CurrentWeapon->WeaponSkeletalMesh)
	{
		Character->CurrentWeapon->WeaponSkeletalMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		Character->CurrentWeapon->WeaponSkeletalMesh->SetSimulatePhysics(false);
	}

	const FName AttachSocketName =
		(Character->CurrentWeapon->WeaponType == EWeaponType::Bow)
		? Character->LeftWeaponSocketName : Character->RightWeaponSocketName;

	Character->CurrentWeapon->AttachToComponent(
		Character->GetMesh(),
		FAttachmentTransformRules::SnapToTargetNotIncludingScale,
		AttachSocketName
	);

	if (USceneComponent* ActiveVisual = Character->CurrentWeapon->GetActiveVisualComponent())
	{
		ActiveVisual->SetRelativeLocation(Character->CurrentWeapon->EquipRelativeLocation);
		ActiveVisual->SetRelativeRotation(Character->CurrentWeapon->EquipRelativeRotation);
		ActiveVisual->SetRelativeScale3D(Character->CurrentWeapon->EquipRelativeScale);
	}

	Character->OnWeaponChanged.Broadcast(GetCurrentWeaponType());
	UE_LOG(LogTemp, Warning, TEXT("Weapon Changed Broadcast: %s"), *UEnum::GetValueAsString(GetCurrentWeaponType()));
	UE_LOG(LogTemp, Warning, TEXT("Equipped Weapon: %s | Socket: %s"), *Character->CurrentWeapon->GetName(), *AttachSocketName.ToString());
}

void UPlayerEquipmentComponent::UnequipWeapon()
{
	ARACharacter* Character = GetOwnerCharacter();
	if (!Character || !Character->CurrentWeapon)
		return;

	Character->CurrentWeapon->DetachFromActor(FDetachmentTransformRules::KeepWorldTransform);
	Character->CurrentWeapon->SetOwner(nullptr);
	Character->CurrentWeapon->UpdateWeaponVisualState();

	if (Character->CurrentWeapon->WeaponMesh)
	{
		Character->CurrentWeapon->WeaponMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		Character->CurrentWeapon->WeaponMesh->SetSimulatePhysics(false);
	}

	if (Character->CurrentWeapon->WeaponSkeletalMesh)
	{
		Character->CurrentWeapon->WeaponSkeletalMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		Character->CurrentWeapon->WeaponSkeletalMesh->SetSimulatePhysics(false);
	}

	Character->CurrentWeapon->SetPickupEnabled(true);
	UE_LOG(LogTemp, Warning, TEXT("Unequipped Weapon: %s"), *Character->CurrentWeapon->GetName());

	Character->CurrentWeapon = nullptr;
	Character->OnWeaponChanged.Broadcast(GetCurrentWeaponType());
}

void UPlayerEquipmentComponent::DropCurrentWeapon()
{
	ARACharacter* Character = GetOwnerCharacter();
	if (!Character || !Character->CurrentWeapon)
		return;

	if (Character->PlayerCombatComponent && (Character->PlayerCombatComponent->IsBowCharging() || Character->PlayerCombatComponent->IsBowAiming()))
	{
		Character->PlayerCombatComponent->EndBowAim();
		if (Character->PlayerMovementComponent)
		{
			Character->PlayerMovementComponent->ApplyMovementStats();
		}
	}

	AWeaponBase* WeaponToDrop = Character->CurrentWeapon;

	WeaponToDrop->DetachFromActor(FDetachmentTransformRules::KeepWorldTransform);
	WeaponToDrop->SetOwner(nullptr);
	WeaponToDrop->UpdateWeaponVisualState();

	const FVector ForwardOffset = Character->GetActorForwardVector() * 80.0f;
	const FVector TraceStart = Character->GetActorLocation() + ForwardOffset + FVector(0.0f, 0.0f, 100.0f);
	const FVector TraceEnd = TraceStart - FVector(0.0f, 0.0f, 500.0f);

	FHitResult HitResult;
	FCollisionQueryParams QueryParams;
	QueryParams.AddIgnoredActor(Character);
	QueryParams.AddIgnoredActor(WeaponToDrop);

	FVector DropLocation = Character->GetActorLocation() + ForwardOffset + FVector(0.0f, 0.0f, 15.0f);
	if (Character->GetWorld()->LineTraceSingleByChannel(HitResult, TraceStart, TraceEnd, ECC_Visibility, QueryParams))
	{
		DropLocation = HitResult.ImpactPoint + FVector(0.0f, 0.0f, 15.0f);
	}

	WeaponToDrop->SetActorLocation(DropLocation);
	WeaponToDrop->SetActorRotation(FRotator(0.0f, Character->GetActorRotation().Yaw + 25.0f, 0.0f));

	if (WeaponToDrop->WeaponMesh)
	{
		WeaponToDrop->WeaponMesh->SetSimulatePhysics(false);
		WeaponToDrop->WeaponMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	}

	WeaponToDrop->SetPickupEnabled(true);
	WeaponToDrop->EnablePickupAfterDrop();

	Character->CurrentWeapon = nullptr;
	Character->OnWeaponChanged.Broadcast(GetCurrentWeaponType());

	UE_LOG(LogTemp, Warning, TEXT("Dropped Weapon: %s"), *WeaponToDrop->GetName());
}

void UPlayerEquipmentComponent::SetNearbyWeapon(AWeaponBase* NewWeapon)
{
	ARACharacter* Character = GetOwnerCharacter();
	if (!Character)
		return;

	Character->NearbyWeapon = NewWeapon;
	UE_LOG(LogTemp, Warning, TEXT("Nearby Weapon Set: %s"), *GetNameSafe(NewWeapon));
}

void UPlayerEquipmentComponent::ClearNearbyWeapon(AWeaponBase* WeaponToClear)
{
	ARACharacter* Character = GetOwnerCharacter();
	if (!Character)
		return;

	if (Character->NearbyWeapon == WeaponToClear)
	{
		Character->NearbyWeapon = nullptr;
		UE_LOG(LogTemp, Warning, TEXT("Nearby Weapon Cleared"));
	}
}

bool UPlayerEquipmentComponent::UnequipCurrentWeaponToInventory()
{
	ARACharacter* Character = GetOwnerCharacter();
	if (!Character)
		return false;

	if (Character->StatComponent && Character->StatComponent->IsDead())
		return false;

	if (!Character->CurrentWeapon || !Character->InventoryComponent)
		return false;

	const FName PreviousWeaponID = Character->CurrentWeapon->WeaponID;
	if (PreviousWeaponID.IsNone())
		return false;

	if (Character->PlayerCombatComponent && (Character->PlayerCombatComponent->IsBowCharging() || Character->PlayerCombatComponent->IsBowAiming()))
	{
		Character->PlayerCombatComponent->EndBowAim();
		if (Character->PlayerMovementComponent)
		{
			Character->PlayerMovementComponent->ApplyMovementStats();
		}
	}

	AWeaponBase* OldWeapon = Character->CurrentWeapon;
	Character->CurrentWeapon = nullptr;
	OldWeapon->Destroy();

	if (!Character->InventoryComponent->HasItem(PreviousWeaponID, 1))
	{
		Character->InventoryComponent->AddItem(PreviousWeaponID, 1);
	}

	Character->OnWeaponChanged.Broadcast(GetCurrentWeaponType());

	if (Character->EquipmentSound)
	{
		UGameplayStatics::PlaySoundAtLocation(Character, Character->EquipmentSound, Character->GetActorLocation());
	}

	UE_LOG(LogTemplateCharacter, Warning, TEXT("Unequipped weapon to inventory: %s"), *PreviousWeaponID.ToString());
	return true;
}

bool UPlayerEquipmentComponent::EquipWeaponFromInventory(FName ItemID)
{
	ARACharacter* Character = GetOwnerCharacter();
	if (!Character)
		return false;

	if (Character->StatComponent && Character->StatComponent->IsDead())
		return false;

	if (ItemID.IsNone())
		return false;

	if (Character->CurrentWeapon && Character->CurrentWeapon->WeaponID == ItemID)
	{
		return UnequipCurrentWeaponToInventory();
	}

	if (!Character->InventoryComponent || !Character->InventoryComponent->HasItem(ItemID, 1))
	{
		UE_LOG(LogTemplateCharacter, Warning, TEXT("EquipWeaponFromInventory failed: item not found. ItemID=%s"), *ItemID.ToString());
		return false;
	}

	URAGameInstance* RAGameInstance = Character->GetGameInstance<URAGameInstance>();
	if (!RAGameInstance)
	{
		UE_LOG(LogTemplateCharacter, Warning, TEXT("EquipWeaponFromInventory failed: RAGameInstance is null"));
		return false;
	}

	FItemData ItemData;
	if (!RAGameInstance->GetItemDataByID(ItemID, ItemData) || ItemData.ItemType != EItemType::Weapon)
	{
		UE_LOG(LogTemplateCharacter, Warning, TEXT("EquipWeaponFromInventory failed: item is not weapon. ItemID=%s"), *ItemID.ToString());
		return false;
	}

	FWeaponData WeaponData;
	if (!RAGameInstance->GetWeaponDataByID(ItemID, WeaponData) || !WeaponData.WeaponClass)
	{
		UE_LOG(LogTemplateCharacter, Warning, TEXT("EquipWeaponFromInventory failed: WeaponData or WeaponClass is missing. WeaponID=%s"), *ItemID.ToString());
		return false;
	}

	UWorld* World = Character->GetWorld();
	if (!World)
		return false;

	AWeaponBase* NewWeapon = World->SpawnActor<AWeaponBase>(WeaponData.WeaponClass);
	if (!NewWeapon)
	{
		UE_LOG(LogTemplateCharacter, Warning, TEXT("EquipWeaponFromInventory failed: spawn failed. WeaponID=%s"), *ItemID.ToString());
		return false;
	}

	NewWeapon->WeaponID = WeaponData.WeaponID.IsNone() ? ItemID : WeaponData.WeaponID;
	NewWeapon->WeaponType = WeaponData.WeaponType;
	NewWeapon->AttackType = WeaponData.AttackType;

	if (Character->CurrentWeapon)
	{
		if (Character->PlayerCombatComponent && (Character->PlayerCombatComponent->IsBowCharging() || Character->PlayerCombatComponent->IsBowAiming()))
		{
			Character->PlayerCombatComponent->EndBowAim();
			if (Character->PlayerMovementComponent)
			{
				Character->PlayerMovementComponent->ApplyMovementStats();
			}
		}

		AWeaponBase* OldWeapon = Character->CurrentWeapon;
		Character->CurrentWeapon = nullptr;
		OldWeapon->Destroy();
	}

	EquipWeapon(NewWeapon);

	if (Character->EquipmentSound)
	{
		UGameplayStatics::PlaySoundAtLocation(Character, Character->EquipmentSound, Character->GetActorLocation());
	}

	UE_LOG(LogTemplateCharacter, Warning, TEXT("Equipped weapon from inventory: %s"), *ItemID.ToString());
	return true;
}

EWeaponType UPlayerEquipmentComponent::GetCurrentWeaponType() const
{
	const ARACharacter* Character = GetOwnerCharacter();
	if (!Character || !Character->CurrentWeapon)
	{
		return EWeaponType::None;
	}

	return Character->CurrentWeapon->WeaponType;
}

FName UPlayerEquipmentComponent::GetCurrentWeaponItemID() const
{
	const ARACharacter* Character = GetOwnerCharacter();
	return Character && Character->CurrentWeapon ? Character->CurrentWeapon->WeaponID : NAME_None;
}
