#include "AnimalStateComponent.h"

#include "AnimalBase.h"

#include "Engine/DataTable.h"

UAnimalStateComponent::UAnimalStateComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UAnimalStateComponent::BeginPlay()
{
	Super::BeginPlay();
}

AAnimalBase* UAnimalStateComponent::GetOwnerAnimal() const
{
	return Cast<AAnimalBase>(GetOwner());
}

void UAnimalStateComponent::InitAnimalData()
{
	AAnimalBase* Animal = GetOwnerAnimal();
	if (!Animal)
	{
		return;
	}

	if (!Animal->AnimalDataTable)
	{
		UE_LOG(LogTemp, Warning, TEXT("[AnimalBase] AnimalDataTable is null. Actor: %s"), *Animal->GetName());
		return;
	}

	if (Animal->AnimalID.IsNone())
	{
		UE_LOG(LogTemp, Warning, TEXT("[AnimalBase] AnimalID is None. Actor: %s"), *Animal->GetName());
		return;
	}

	const FAnimalData* FoundData = Animal->AnimalDataTable->FindRow<FAnimalData>(
		Animal->AnimalID,
		TEXT("AAnimalBase::InitAnimalData")
	);

	if (!FoundData)
	{
		UE_LOG(LogTemp, Warning, TEXT("[AnimalBase] Failed to find AnimalData. AnimalID: %s"), *Animal->AnimalID.ToString());
		return;
	}

	Animal->AnimalData = *FoundData;

	Animal->CaptureDifficulty = Animal->AnimalData.CaptureDifficulty;
	Animal->DropItemIDs = Animal->AnimalData.DropItemIDs;

	Animal->MaxHP = Animal->AnimalData.MaxHP;
	Animal->CurrentHP = Animal->MaxHP;

	SetAnimalState(Animal->bStartTrapped ? EAnimalState::Trapped : EAnimalState::Idle);

	UE_LOG(LogTemp, Warning, TEXT("[AnimalBase] Loaded AnimalData: %s / HP: %.1f / CaptureDifficulty: %.2f"),
		*Animal->AnimalID.ToString(),
		Animal->AnimalData.MaxHP,
		Animal->CaptureDifficulty
	);
}

void UAnimalStateComponent::SetAnimalState(EAnimalState NewState)
{
	if (AAnimalBase* Animal = GetOwnerAnimal())
	{
		Animal->AnimalState = NewState;
	}
}

bool UAnimalStateComponent::CanTakeDamage() const
{
	const AAnimalBase* Animal = GetOwnerAnimal();
	if (!Animal)
	{
		return false;
	}

	if (Animal->IsTrapped())
	{
		return false;
	}

	return true;
}

void UAnimalStateComponent::HandleDamageTaken(float ActualDamage, AActor* DamageCauser)
{
	AAnimalBase* Animal = GetOwnerAnimal();
	if (!Animal || ActualDamage <= 0.0f)
	{
		return;
	}

	Animal->UpdateHPBar();
	Animal->ShowHPBar();

	if (Animal->CurrentHP <= 0.0f)
	{
		Animal->PlayAnimalDeathVisual();
	}
	else
	{
		Animal->StartFlee(DamageCauser);
	}
}
