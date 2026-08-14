#include "AnimalPresentationComponent.h"

#include "AnimalBase.h"
#include "EnemyHPBarWidget.h"

#include "AIController.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/WidgetComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "TimerManager.h"

UAnimalPresentationComponent::UAnimalPresentationComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UAnimalPresentationComponent::BeginPlay()
{
	Super::BeginPlay();
}

void UAnimalPresentationComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (AAnimalBase* Animal = GetOwnerAnimal())
	{
		Animal->GetWorldTimerManager().ClearTimer(Animal->HPBarHideTimerHandle);
		Animal->GetWorldTimerManager().ClearTimer(Animal->SaveWidgetHideTimerHandle);
		Animal->GetWorldTimerManager().ClearTimer(Animal->DeathFallTimerHandle);
	}

	Super::EndPlay(EndPlayReason);
}

AAnimalBase* UAnimalPresentationComponent::GetOwnerAnimal() const
{
	return Cast<AAnimalBase>(GetOwner());
}

void UAnimalPresentationComponent::ShowSaveWidget()
{
	AAnimalBase* Animal = GetOwnerAnimal();
	if (!Animal || !Animal->SaveWidgetComponent)
	{
		return;
	}

	Animal->SaveWidgetComponent->SetVisibility(true);

	Animal->GetWorldTimerManager().ClearTimer(Animal->SaveWidgetHideTimerHandle);
	Animal->GetWorldTimerManager().SetTimer(
		Animal->SaveWidgetHideTimerHandle,
		this,
		&UAnimalPresentationComponent::HideSaveWidget,
		Animal->SaveWidgetVisibleDuration,
		false
	);
}

void UAnimalPresentationComponent::HideSaveWidget()
{
	AAnimalBase* Animal = GetOwnerAnimal();
	if (!Animal || !Animal->SaveWidgetComponent)
	{
		return;
	}

	Animal->SaveWidgetComponent->SetVisibility(false);
}

void UAnimalPresentationComponent::UpdateHPBar()
{
	AAnimalBase* Animal = GetOwnerAnimal();
	if (!Animal || !Animal->HPWidgetComponent)
	{
		return;
	}

	UEnemyHPBarWidget* HPWidget = Cast<UEnemyHPBarWidget>(Animal->HPWidgetComponent->GetUserWidgetObject());
	if (!HPWidget)
	{
		return;
	}

	const float HPPercent = Animal->MaxHP > 0.0f ? Animal->CurrentHP / Animal->MaxHP : 0.0f;

	HPWidget->SetHPPercent(HPPercent);
}

void UAnimalPresentationComponent::ShowHPBar()
{
	AAnimalBase* Animal = GetOwnerAnimal();
	if (!Animal || !Animal->HPWidgetComponent)
	{
		return;
	}

	Animal->HPWidgetComponent->SetVisibility(true);

	Animal->GetWorldTimerManager().ClearTimer(Animal->HPBarHideTimerHandle);

	Animal->GetWorldTimerManager().SetTimer(
		Animal->HPBarHideTimerHandle,
		this,
		&UAnimalPresentationComponent::HideHPBar,
		Animal->HPBarVisibleDuration,
		false
	);
}

void UAnimalPresentationComponent::HideHPBar()
{
	AAnimalBase* Animal = GetOwnerAnimal();
	if (!Animal || !Animal->HPWidgetComponent)
	{
		return;
	}

	Animal->HPWidgetComponent->SetVisibility(false);
}

void UAnimalPresentationComponent::PlayAnimalDeathVisual()
{
	AAnimalBase* Animal = GetOwnerAnimal();
	if (!Animal)
	{
		return;
	}

	if (Animal->bAnimalDeathVisualPlayed)
	{
		return;
	}

	Animal->bAnimalDeathVisualPlayed = true;

	Animal->SetAnimalState(EAnimalState::Dead);

	Animal->GetWorldTimerManager().ClearTimer(Animal->WanderTimerHandle);
	Animal->GetWorldTimerManager().ClearTimer(Animal->FleeTimerHandle);
	Animal->GetWorldTimerManager().ClearTimer(Animal->HPBarHideTimerHandle);
	Animal->GetWorldTimerManager().ClearTimer(Animal->SaveWidgetHideTimerHandle);

	HideHPBar();
	HideSaveWidget();

	if (AAIController* AIController = Cast<AAIController>(Animal->GetController()))
	{
		AIController->StopMovement();
	}

	if (Animal->GetCharacterMovement())
	{
		Animal->GetCharacterMovement()->StopMovementImmediately();
		Animal->GetCharacterMovement()->DisableMovement();
	}

	if (Animal->GetMesh())
	{
		Animal->GetMesh()->bPauseAnims = true;
		Animal->GetMesh()->SetSimulatePhysics(false);

		Animal->DeathStartRotation = Animal->GetMesh()->GetRelativeRotation();
		Animal->DeathTargetRotation = Animal->DeathStartRotation + Animal->DeathRotationOffset;
		Animal->DeathFallElapsedTime = 0.0f;

		Animal->GetWorldTimerManager().ClearTimer(Animal->DeathFallTimerHandle);
		Animal->GetWorldTimerManager().SetTimer(
			Animal->DeathFallTimerHandle,
			this,
			&UAnimalPresentationComponent::UpdateDeathFallRotation,
			0.016f,
			true
		);
	}

	Animal->SetActorEnableCollision(false);
	Animal->SetLifeSpan(Animal->DeathLifeSpan);

	UE_LOG(LogTemp, Warning, TEXT("[AnimalBase] PlayAnimalDeathVisual: %s"), *Animal->GetName());
}

void UAnimalPresentationComponent::UpdateDeathFallRotation()
{
	AAnimalBase* Animal = GetOwnerAnimal();
	if (!Animal || !Animal->GetMesh())
	{
		if (Animal)
		{
			Animal->GetWorldTimerManager().ClearTimer(Animal->DeathFallTimerHandle);
		}
		return;
	}

	Animal->DeathFallElapsedTime += 0.016f;

	const float Alpha = FMath::Clamp(
		Animal->DeathFallElapsedTime / Animal->DeathFallDuration,
		0.0f,
		1.0f
	);

	const float SmoothAlpha = FMath::InterpEaseOut(0.0f, 1.0f, Alpha, 2.0f);

	const FRotator NewRotation = FMath::Lerp(
		Animal->DeathStartRotation,
		Animal->DeathTargetRotation,
		SmoothAlpha
	);

	Animal->GetMesh()->SetRelativeRotation(NewRotation);

	if (Alpha >= 1.0f)
	{
		Animal->GetMesh()->SetRelativeRotation(Animal->DeathTargetRotation);
		Animal->GetWorldTimerManager().ClearTimer(Animal->DeathFallTimerHandle);
	}
}
