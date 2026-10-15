#include "EnemyPerfTarget.h"

#include "Components/CapsuleComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "ProfilingDebugging/CpuProfilerTrace.h"
#include "UObject/ConstructorHelpers.h"

AEnemyPerfTarget::AEnemyPerfTarget()
{
	AutoPossessPlayer = EAutoReceiveInput::Disabled;
	AutoPossessAI = EAutoPossessAI::Disabled;

	UStaticMeshComponent* Marker = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("TargetMarker"));
	Marker->SetupAttachment(GetRootComponent());
	Marker->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Marker->SetGenerateOverlapEvents(false);
	Marker->SetCanEverAffectNavigation(false);
	Marker->SetRelativeScale3D(FVector(0.6f));

	static ConstructorHelpers::FObjectFinder<UStaticMesh> Sphere(TEXT("/Engine/BasicShapes/Sphere.Sphere"));
	if (Sphere.Succeeded())
	{
		Marker->SetStaticMesh(Sphere.Object);
	}
}

void AEnemyPerfTarget::BeginPlay()
{
	Super::BeginPlay();

	// 캐릭터 physics가 이동하지 않도록, 절대적인 time based route를 제공하는 harness
	GetCharacterMovement()->DisableMovement();
	GetCharacterMovement()->SetComponentTickEnabled(false);
	GetCapsuleComponent()->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
	GetCapsuleComponent()->SetCanEverAffectNavigation(false);
}

float AEnemyPerfTarget::TakeDamage(float DamageAmount, const FDamageEvent& DamageEvent,
	AController* EventInstigator, AActor* DamageCauser)
{
	TRACE_CPUPROFILER_EVENT_SCOPE(PerfHarness_DummyDamage);

	return 0.f;
}
