#include "AnimalBase.h"

#include "Components/AnimalAIComponent.h"
#include "Components/AnimalPresentationComponent.h"
#include "Components/AnimalRescueComponent.h"
#include "Components/AnimalStateComponent.h"

#include "Components/StaticMeshComponent.h"
#include "Components/WidgetComponent.h"

AAnimalBase::AAnimalBase()
{
	PrimaryActorTick.bCanEverTick = false;

	HPWidgetComponent = CreateDefaultSubobject<UWidgetComponent>(TEXT("HPWidgetComponent"));
	HPWidgetComponent->SetupAttachment(GetRootComponent());
	HPWidgetComponent->SetRelativeLocation(FVector(0.0f, 0.0f, 100.0f));
	HPWidgetComponent->SetWidgetSpace(EWidgetSpace::Screen);
	HPWidgetComponent->SetDrawSize(FVector2D(100.0f, 20.0f));
	HPWidgetComponent->SetVisibility(false);

	CageMeshComponent = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("CageMeshComponent"));
	CageMeshComponent->SetupAttachment(GetRootComponent());
	CageMeshComponent->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);

	SaveWidgetComponent = CreateDefaultSubobject<UWidgetComponent>(TEXT("SaveWidgetComponent"));
	SaveWidgetComponent->SetupAttachment(GetRootComponent());
	SaveWidgetComponent->SetRelativeLocation(FVector(0.0f, 0.0f, 130.0f));
	SaveWidgetComponent->SetWidgetSpace(EWidgetSpace::Screen);
	SaveWidgetComponent->SetDrawSize(FVector2D(120.0f, 40.0f));
	SaveWidgetComponent->SetVisibility(false);

	AnimalAIComponent = CreateDefaultSubobject<UAnimalAIComponent>(TEXT("AnimalAIComponent"));
	AnimalPresentationComponent = CreateDefaultSubobject<UAnimalPresentationComponent>(TEXT("AnimalPresentationComponent"));
	AnimalRescueComponent = CreateDefaultSubobject<UAnimalRescueComponent>(TEXT("AnimalRescueComponent"));
	AnimalStateComponent = CreateDefaultSubobject<UAnimalStateComponent>(TEXT("AnimalStateComponent"));
}

void AAnimalBase::BeginPlay()
{
	Super::BeginPlay();

	InitAnimalData();

	UpdateHPBar();
	HideHPBar();
	HideSaveWidget();

	if (bStartTrapped)
	{
		ApplyTrappedState();
	}
	else
	{
		StartWander();
	}
}

void AAnimalBase::InitAnimalData()
{
	if (AnimalStateComponent)
	{
		AnimalStateComponent->InitAnimalData();
	}
}

void AAnimalBase::SetAnimalState(EAnimalState NewState)
{
	if (AnimalStateComponent)
	{
		AnimalStateComponent->SetAnimalState(NewState);
		return;
	}

	AnimalState = NewState;
}

bool AAnimalBase::Rescue()
{
	return AnimalRescueComponent && AnimalRescueComponent->Rescue();
}

void AAnimalBase::SetOwningCamp(AEnemyCampActor* InCamp)
{
	if (AnimalRescueComponent)
	{
		AnimalRescueComponent->SetOwningCamp(InCamp);
		return;
	}

	OwningCamp = InCamp;
}

bool AAnimalBase::CanBeRescued() const
{
	return AnimalRescueComponent && AnimalRescueComponent->CanBeRescued();
}

void AAnimalBase::ApplyTrappedState()
{
	if (AnimalRescueComponent)
	{
		AnimalRescueComponent->ApplyTrappedState();
	}
}

void AAnimalBase::ApplyRescuedState()
{
	if (AnimalRescueComponent)
	{
		AnimalRescueComponent->ApplyRescuedState();
	}
}

void AAnimalBase::ApplyRuntimeRescuedState()
{
	if (AnimalRescueComponent)
	{
		AnimalRescueComponent->ApplyRuntimeRescuedState();
	}
}

void AAnimalBase::ShowSaveWidget()
{
	if (AnimalPresentationComponent)
	{
		AnimalPresentationComponent->ShowSaveWidget();
	}
}

void AAnimalBase::HideSaveWidget()
{
	if (AnimalPresentationComponent)
	{
		AnimalPresentationComponent->HideSaveWidget();
	}
}

float AAnimalBase::TakeDamage(
	float DamageAmount,
	FDamageEvent const& DamageEvent,
	AController* EventInstigator,
	AActor* DamageCauser
)
{
	if (AnimalStateComponent && !AnimalStateComponent->CanTakeDamage())
	{
		UE_LOG(LogTemp, Warning, TEXT("[AnimalBase] Damage ignored while trapped: %s / Damage: %.1f"),
			*GetName(),
			DamageAmount);
		return 0.0f;
	}

	const float ActualDamage = Super::TakeDamage(
		DamageAmount,
		DamageEvent,
		EventInstigator,
		DamageCauser
	);

	if (AnimalStateComponent)
	{
		AnimalStateComponent->HandleDamageTaken(ActualDamage, DamageCauser);
	}

	return ActualDamage;
}

void AAnimalBase::UpdateHPBar()
{
	if (AnimalPresentationComponent)
	{
		AnimalPresentationComponent->UpdateHPBar();
	}
}

void AAnimalBase::ShowHPBar()
{
	if (AnimalPresentationComponent)
	{
		AnimalPresentationComponent->ShowHPBar();
	}
}

void AAnimalBase::HideHPBar()
{
	if (AnimalPresentationComponent)
	{
		AnimalPresentationComponent->HideHPBar();
	}
}

void AAnimalBase::StartWander()
{
	if (AnimalAIComponent)
	{
		AnimalAIComponent->StartWander();
	}
}

void AAnimalBase::MoveToRandomLocation()
{
	if (AnimalAIComponent)
	{
		AnimalAIComponent->MoveToRandomLocation();
	}
}

void AAnimalBase::StartFlee(AActor* ThreatActor)
{
	if (AnimalAIComponent)
	{
		AnimalAIComponent->StartFlee(ThreatActor);
	}
}

void AAnimalBase::StopFlee()
{
	if (AnimalAIComponent)
	{
		AnimalAIComponent->StopFlee();
	}
}

void AAnimalBase::StopMovement()
{
	if (AnimalAIComponent)
	{
		AnimalAIComponent->StopMovement();
	}
}

void AAnimalBase::PlayAnimalDeathVisual()
{
	if (AnimalPresentationComponent)
	{
		AnimalPresentationComponent->PlayAnimalDeathVisual();
	}
}

void AAnimalBase::UpdateDeathFallRotation()
{
	if (AnimalPresentationComponent)
	{
		AnimalPresentationComponent->UpdateDeathFallRotation();
	}
}
