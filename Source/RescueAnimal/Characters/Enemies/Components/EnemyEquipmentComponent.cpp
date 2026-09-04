#include "EnemyEquipmentComponent.h"

#include "RAEnemyBase.h"
#include "WeaponBase.h"

#include "Components/SceneComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/StaticMeshComponent.h"

UEnemyEquipmentComponent::UEnemyEquipmentComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UEnemyEquipmentComponent::BeginPlay()
{
	Super::BeginPlay();
}

void UEnemyEquipmentComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (ARAEnemyBase* Enemy = GetOwnerEnemy())
	{
		if (Enemy->CurrentWeapon)
		{
			Enemy->CurrentWeapon->Destroy();
			Enemy->CurrentWeapon = nullptr;
		}
	}

	Super::EndPlay(EndPlayReason);
}

ARAEnemyBase* UEnemyEquipmentComponent::GetOwnerEnemy() const
{
	return Cast<ARAEnemyBase>(GetOwner());
}

void UEnemyEquipmentComponent::EquipDefaultWeapon()
{
	ARAEnemyBase* Enemy = GetOwnerEnemy();
	if (!Enemy || !Enemy->EnemyWeaponClass || Enemy->CurrentWeapon)
		return;

	FActorSpawnParameters SpawnParams;
	SpawnParams.Owner = Enemy;
	SpawnParams.Instigator = Enemy;

	AWeaponBase* SpawnedWeapon = Enemy->GetWorld()->SpawnActor<AWeaponBase>(
		Enemy->EnemyWeaponClass,
		Enemy->GetActorTransform(),
		SpawnParams
	);
	if (!SpawnedWeapon)
	{
		UE_LOG(LogTemp, Warning, TEXT("[%s] Failed to spawn enemy weapon"), *Enemy->ActorSaveID.ToString());
		return;
	}

	EquipWeapon(SpawnedWeapon);
}

void UEnemyEquipmentComponent::EquipWeapon(AWeaponBase* NewWeapon)
{
	ARAEnemyBase* Enemy = GetOwnerEnemy();
	if (!Enemy || !NewWeapon || !Enemy->GetMesh())
		return;

	if (Enemy->CurrentWeapon && Enemy->CurrentWeapon != NewWeapon)
	{
		Enemy->CurrentWeapon->Destroy();
	}

	Enemy->CurrentWeapon = NewWeapon;
	Enemy->CurrentWeapon->SetOwner(Enemy);
	Enemy->CurrentWeapon->SetInstigator(Enemy);
	Enemy->CurrentWeapon->SetPickupEnabled(false);
	Enemy->CurrentWeapon->UpdateWeaponVisualState();

	if (Enemy->CurrentWeapon->WeaponMesh)
	{
		Enemy->CurrentWeapon->WeaponMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		Enemy->CurrentWeapon->WeaponMesh->SetGenerateOverlapEvents(false);
		Enemy->CurrentWeapon->WeaponMesh->SetSimulatePhysics(false);
	}

	if (Enemy->CurrentWeapon->WeaponSkeletalMesh)
	{
		Enemy->CurrentWeapon->WeaponSkeletalMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		Enemy->CurrentWeapon->WeaponSkeletalMesh->SetGenerateOverlapEvents(false);
		Enemy->CurrentWeapon->WeaponSkeletalMesh->SetSimulatePhysics(false);
	}

	const FName AttachSocketName =
		(Enemy->CurrentWeapon->WeaponType == EWeaponType::Bow)
		? Enemy->LeftWeaponSocketName : Enemy->RightWeaponSocketName;

	Enemy->CurrentWeapon->AttachToComponent(
		Enemy->GetMesh(),
		FAttachmentTransformRules::SnapToTargetNotIncludingScale,
		AttachSocketName
	);

	if (USceneComponent* ActiveVisual = Enemy->CurrentWeapon->GetActiveVisualComponent())
	{
		ActiveVisual->SetRelativeLocation(Enemy->CurrentWeapon->EquipRelativeLocation);
		ActiveVisual->SetRelativeRotation(Enemy->CurrentWeapon->EquipRelativeRotation);
		ActiveVisual->SetRelativeScale3D(Enemy->CurrentWeapon->EquipRelativeScale);
	}

	SyncCombatDataFromWeapon();

	UE_LOG(LogTemp, Warning, TEXT("[%s] Equipped Enemy Weapon: %s | Socket: %s"),
		*Enemy->ActorSaveID.ToString(),
		*Enemy->CurrentWeapon->GetName(),
		*AttachSocketName.ToString());
}

void UEnemyEquipmentComponent::SyncCombatDataFromWeapon()
{
	ARAEnemyBase* Enemy = GetOwnerEnemy();
	if (!Enemy || !Enemy->CurrentWeapon || !Enemy->bUseEquippedWeaponCombatData)
		return;

	Enemy->AttackDamage = Enemy->CurrentWeapon->AttackDamage;

	if (Enemy->CurrentWeapon->AttackMontage)
	{
		Enemy->AttackMontage = Enemy->CurrentWeapon->AttackMontage;
	}

	switch (Enemy->CurrentWeapon->WeaponType)
	{
	case EWeaponType::Sword:
		Enemy->AttackType = EEnemyAttackType::Sword;
		if (Enemy->CurrentWeapon->AttackMontage)
		{
			Enemy->SwordAttackMontage = Enemy->CurrentWeapon->AttackMontage;
		}
		break;
	case EWeaponType::Bow:
		Enemy->AttackType = EEnemyAttackType::Bow;
		Enemy->AttackRange = Enemy->BowAttackRange;
		if (Enemy->CurrentWeapon->AttackMontage)
		{
			Enemy->BowAttackMontage = Enemy->CurrentWeapon->AttackMontage;
		}
		if (Enemy->CurrentWeapon->ProjectileClass)
		{
			Enemy->BowProjectileClass = Enemy->CurrentWeapon->ProjectileClass;
		}
		Enemy->BowProjectileSpeed = Enemy->CurrentWeapon->ProjectileSpeed;
		break;
	default:
		break;
	}
}
