#include "AnimalAIComponent.h"

#include "AnimalBase.h"
#include "AnimalStateComponent.h"

#include "AIController.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "NavigationSystem.h"
#include "TimerManager.h"

UAnimalAIComponent::UAnimalAIComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UAnimalAIComponent::BeginPlay()
{
	Super::BeginPlay();
}

void UAnimalAIComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (AAnimalBase* Animal = GetOwnerAnimal())
	{
		Animal->GetWorldTimerManager().ClearTimer(Animal->WanderTimerHandle);
		Animal->GetWorldTimerManager().ClearTimer(Animal->FleeTimerHandle);
	}

	Super::EndPlay(EndPlayReason);
}

AAnimalBase* UAnimalAIComponent::GetOwnerAnimal() const
{
	return Cast<AAnimalBase>(GetOwner());
}

void UAnimalAIComponent::StartWander()
{
	AAnimalBase* Animal = GetOwnerAnimal();
	if (!Animal)
	{
		return;
	}

	if (Animal->AnimalState == EAnimalState::Dead || Animal->AnimalState == EAnimalState::Captured || Animal->AnimalState == EAnimalState::Trapped)
	{
		return;
	}

	Animal->GetWorldTimerManager().ClearTimer(Animal->FleeTimerHandle);

	if (UAnimalStateComponent* AnimalStateComponent = Animal->GetAnimalStateComponent())
	{
		AnimalStateComponent->SetAnimalState(Animal->bHasBeenRescued ? EAnimalState::Rescued : EAnimalState::Wander);
	}

	if (Animal->GetCharacterMovement())
	{
		Animal->GetCharacterMovement()->MaxWalkSpeed = Animal->WanderSpeed;
	}

	MoveToRandomLocation();

	Animal->GetWorldTimerManager().ClearTimer(Animal->WanderTimerHandle);
	Animal->GetWorldTimerManager().SetTimer(
		Animal->WanderTimerHandle,
		this,
		&UAnimalAIComponent::MoveToRandomLocation,
		Animal->WanderInterval,
		true
	);
}

void UAnimalAIComponent::MoveToRandomLocation()
{
	AAnimalBase* Animal = GetOwnerAnimal();
	if (!Animal)
	{
		return;
	}

	if (Animal->AnimalState == EAnimalState::Dead || Animal->AnimalState == EAnimalState::Captured || Animal->AnimalState == EAnimalState::Trapped)
	{
		return;
	}

	if (Animal->AnimalState == EAnimalState::Flee)
	{
		return;
	}

	UNavigationSystemV1* NavSystem = UNavigationSystemV1::GetCurrent(Animal->GetWorld());
	if (!NavSystem)
	{
		return;
	}

	FNavLocation RandomLocation;

	const bool bFoundLocation = NavSystem->GetRandomReachablePointInRadius(
		Animal->GetActorLocation(),
		Animal->WanderRadius,
		RandomLocation
	);

	if (!bFoundLocation)
	{
		return;
	}

	AAIController* AIController = Cast<AAIController>(Animal->GetController());
	if (!AIController)
	{
		return;
	}

	AIController->MoveToLocation(RandomLocation.Location);
}

void UAnimalAIComponent::StartFlee(AActor* ThreatActor)
{
	AAnimalBase* Animal = GetOwnerAnimal();
	if (!Animal)
	{
		return;
	}

	if (Animal->AnimalState == EAnimalState::Dead || Animal->AnimalState == EAnimalState::Captured || Animal->AnimalState == EAnimalState::Trapped)
	{
		return;
	}

	if (!ThreatActor)
	{
		return;
	}

	Animal->GetWorldTimerManager().ClearTimer(Animal->WanderTimerHandle);
	Animal->GetWorldTimerManager().ClearTimer(Animal->FleeTimerHandle);

	if (UAnimalStateComponent* AnimalStateComponent = Animal->GetAnimalStateComponent())
	{
		AnimalStateComponent->SetAnimalState(EAnimalState::Flee);
	}

	if (Animal->GetCharacterMovement())
	{
		Animal->GetCharacterMovement()->MaxWalkSpeed = Animal->FleeSpeed;
	}

	AAIController* AIController = Cast<AAIController>(Animal->GetController());
	if (!AIController)
	{
		return;
	}

	FVector FleeDirection = Animal->GetActorLocation() - ThreatActor->GetActorLocation();
	FleeDirection.Z = 0.0f;

	if (FleeDirection.IsNearlyZero())
	{
		FleeDirection = Animal->GetActorForwardVector();
	}

	FleeDirection.Normalize();

	const FVector DesiredLocation = Animal->GetActorLocation() + FleeDirection * Animal->FleeDistance;

	UNavigationSystemV1* NavSystem = UNavigationSystemV1::GetCurrent(Animal->GetWorld());
	if (!NavSystem)
	{
		return;
	}

	FNavLocation FleeLocation;

	const bool bFoundLocation = NavSystem->ProjectPointToNavigation(
		DesiredLocation,
		FleeLocation
	);

	if (bFoundLocation)
	{
		AIController->MoveToLocation(FleeLocation.Location);
	}

	Animal->GetWorldTimerManager().SetTimer(
		Animal->FleeTimerHandle,
		this,
		&UAnimalAIComponent::StopFlee,
		Animal->FleeDuration,
		false
	);
}

void UAnimalAIComponent::StopFlee()
{
	AAnimalBase* Animal = GetOwnerAnimal();
	if (!Animal)
	{
		return;
	}

	if (Animal->AnimalState == EAnimalState::Dead || Animal->AnimalState == EAnimalState::Captured || Animal->AnimalState == EAnimalState::Trapped)
	{
		return;
	}

	StopMovement();
	StartWander();
}

void UAnimalAIComponent::StopMovement()
{
	AAnimalBase* Animal = GetOwnerAnimal();
	if (!Animal)
	{
		return;
	}

	AAIController* AIController = Cast<AAIController>(Animal->GetController());
	if (!AIController)
	{
		return;
	}

	AIController->StopMovement();
}
