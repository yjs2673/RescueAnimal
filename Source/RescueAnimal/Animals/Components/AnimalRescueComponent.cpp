#include "AnimalRescueComponent.h"

#include "AnimalAIComponent.h"
#include "AnimalBase.h"
#include "AnimalPresentationComponent.h"
#include "AnimalStateComponent.h"
#include "EnemyCampActor.h"
#include "RAGameInstance.h"
#include "RAWorldStateManager.h"

#include "Components/StaticMeshComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Kismet/GameplayStatics.h"
#include "NiagaraFunctionLibrary.h"
#include "NiagaraSystem.h"
#include "Sound/SoundBase.h"
#include "TimerManager.h"
#include "EngineUtils.h"

UAnimalRescueComponent::UAnimalRescueComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UAnimalRescueComponent::BeginPlay()
{
	Super::BeginPlay();
}

void UAnimalRescueComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	Super::EndPlay(EndPlayReason);
}

AAnimalBase* UAnimalRescueComponent::GetOwnerAnimal() const
{
	return Cast<AAnimalBase>(GetOwner());
}

bool UAnimalRescueComponent::Rescue()
{
	AAnimalBase* Animal = GetOwnerAnimal();
	if (!Animal)
	{
		return false;
	}

	if (!CanBeRescued())
	{
		return false;
	}

	ApplyRescuedState();

	if (URAGameInstance* RAGameInstance = Animal->GetGameInstance<URAGameInstance>())
	{
		const bool bNewUnlock = RAGameInstance->UnlockAnimal(Animal->AnimalID);
		UE_LOG(LogTemp, Warning, TEXT("[AnimalBase] Rescue collection result: AnimalID=%s NewUnlock=%s"),
			*Animal->AnimalID.ToString(),
			bNewUnlock ? TEXT("True") : TEXT("False"));
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("[AnimalBase] Rescue collection unlock skipped: GameInstance is not RAGameInstance. AnimalID=%s"),
			*Animal->AnimalID.ToString());
	}

	Animal->OnAnimalRescued.Broadcast(Animal);
	Animal->BP_OnRescued();

#pragma region Runtime World State
	for (TActorIterator<ARAWorldStateManager> It(Animal->GetWorld()); It; ++It)
	{
		It->NotifyAnimalRescued(Animal->AnimalSaveID);
		break;
	}
#pragma endregion Runtime World State

	return true;
}

void UAnimalRescueComponent::SetOwningCamp(AEnemyCampActor* InCamp)
{
	if (AAnimalBase* Animal = GetOwnerAnimal())
	{
		Animal->OwningCamp = InCamp;
	}
}

bool UAnimalRescueComponent::CanBeRescued() const
{
	const AAnimalBase* Animal = GetOwnerAnimal();
	if (!Animal)
	{
		return false;
	}

	if (!Animal->IsTrapped() || Animal->bHasBeenRescued || Animal->AnimalState == EAnimalState::Dead)
	{
		return false;
	}

	return !Animal->OwningCamp || Animal->OwningCamp->IsCampCleared();
}

void UAnimalRescueComponent::ApplyTrappedState()
{
	AAnimalBase* Animal = GetOwnerAnimal();
	if (!Animal)
	{
		return;
	}

	Animal->bHasBeenRescued = false;
	if (UAnimalStateComponent* AnimalStateComponent = Animal->GetAnimalStateComponent())
	{
		AnimalStateComponent->SetAnimalState(EAnimalState::Trapped);
	}

	Animal->GetWorldTimerManager().ClearTimer(Animal->WanderTimerHandle);
	Animal->GetWorldTimerManager().ClearTimer(Animal->FleeTimerHandle);

	if (UAnimalAIComponent* AnimalAIComponent = Animal->GetAnimalAIComponent())
	{
		AnimalAIComponent->StopMovement();
	}

	if (Animal->GetCharacterMovement())
	{
		Animal->GetCharacterMovement()->StopMovementImmediately();
		Animal->GetCharacterMovement()->DisableMovement();
	}

	if (Animal->CageMeshComponent)
	{
		Animal->CageMeshComponent->SetVisibility(true, true);
		Animal->CageMeshComponent->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	}

	if (UAnimalPresentationComponent* AnimalPresentationComponent = Animal->GetAnimalPresentationComponent())
	{
		AnimalPresentationComponent->HideSaveWidget();
	}
}

void UAnimalRescueComponent::ApplyRescuedState()
{
	AAnimalBase* Animal = GetOwnerAnimal();
	if (!Animal)
	{
		return;
	}

	Animal->bHasBeenRescued = true;
	if (UAnimalStateComponent* AnimalStateComponent = Animal->GetAnimalStateComponent())
	{
		AnimalStateComponent->SetAnimalState(EAnimalState::Rescued);
	}

	if (Animal->CageDisappearEffect)
	{
		UNiagaraFunctionLibrary::SpawnSystemAtLocation(
			Animal->GetWorld(),
			Animal->CageDisappearEffect,
			Animal->CageMeshComponent ? Animal->CageMeshComponent->GetComponentLocation() : Animal->GetActorLocation()
		);
	}

	if (Animal->CageDisappearSound)
	{
		UGameplayStatics::PlaySoundAtLocation(Animal, Animal->CageDisappearSound, Animal->GetActorLocation());
	}

	if (Animal->RescueSound)
	{
		UGameplayStatics::PlaySoundAtLocation(Animal, Animal->RescueSound, Animal->GetActorLocation());
	}

	if (Animal->CageMeshComponent)
	{
		Animal->CageMeshComponent->SetVisibility(false, true);
		Animal->CageMeshComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	}

	if (Animal->GetCharacterMovement())
	{
		Animal->GetCharacterMovement()->SetMovementMode(MOVE_Walking);
		Animal->GetCharacterMovement()->MaxWalkSpeed = Animal->WanderSpeed;
	}

	if (UAnimalPresentationComponent* AnimalPresentationComponent = Animal->GetAnimalPresentationComponent())
	{
		AnimalPresentationComponent->ShowSaveWidget();
	}

	if (UAnimalAIComponent* AnimalAIComponent = Animal->GetAnimalAIComponent())
	{
		AnimalAIComponent->StartWander();
	}
}

void UAnimalRescueComponent::ApplyRuntimeRescuedState()
{
	AAnimalBase* Animal = GetOwnerAnimal();
	if (!Animal)
	{
		return;
	}

	Animal->bStartTrapped = false;
	Animal->bHasBeenRescued = true;
	if (UAnimalStateComponent* AnimalStateComponent = Animal->GetAnimalStateComponent())
	{
		AnimalStateComponent->SetAnimalState(EAnimalState::Rescued);
	}

	Animal->GetWorldTimerManager().ClearTimer(Animal->WanderTimerHandle);
	Animal->GetWorldTimerManager().ClearTimer(Animal->FleeTimerHandle);
	Animal->GetWorldTimerManager().ClearTimer(Animal->SaveWidgetHideTimerHandle);

	if (Animal->CageMeshComponent)
	{
		Animal->CageMeshComponent->SetVisibility(false, true);
		Animal->CageMeshComponent->SetHiddenInGame(true);
		Animal->CageMeshComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	}

	if (UAnimalPresentationComponent* AnimalPresentationComponent = Animal->GetAnimalPresentationComponent())
	{
		AnimalPresentationComponent->HideSaveWidget();
	}

	if (Animal->GetCharacterMovement())
	{
		Animal->GetCharacterMovement()->SetMovementMode(MOVE_Walking);
		Animal->GetCharacterMovement()->MaxWalkSpeed = Animal->WanderSpeed;
	}

	if (UAnimalAIComponent* AnimalAIComponent = Animal->GetAnimalAIComponent())
	{
		AnimalAIComponent->StartWander();
	}
}
