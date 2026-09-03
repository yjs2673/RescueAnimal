#include "PortalActor.h"
#include "Animation/AnimationAsset.h"
#include "Camera/PlayerCameraManager.h"
#include "Components/BoxComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/SkeletalMesh.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"
#include "RACharacter.h"
#include "RAGameInstance.h"
#include "RAPlayerController.h"
#include "PlayerUIFlowComponent.h"
#include "UObject/ConstructorHelpers.h"

APortalActor::APortalActor()
{
	PrimaryActorTick.bCanEverTick = false;

	Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	SetRootComponent(Root);

	PortalMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("PortalMesh"));
	PortalMesh->SetupAttachment(Root);
	PortalMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	PortalSkeletalMesh = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("PortalSkeletalMesh"));
	PortalSkeletalMesh->SetupAttachment(Root);
	PortalSkeletalMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	PortalSkeletalMesh->SetGenerateOverlapEvents(false);

	static ConstructorHelpers::FObjectFinder<USkeletalMesh> PortalMeshAsset(TEXT("/Game/External/Portal_effect/portal_effect.portal_effect"));
	if (PortalMeshAsset.Succeeded())
	{
		PortalSkeletalMesh->SetSkeletalMesh(PortalMeshAsset.Object);
	}

	static ConstructorHelpers::FObjectFinder<UAnimationAsset> PortalAnimAsset(TEXT("/Game/External/Portal_effect/portal_effect_Anim.portal_effect_Anim"));
	if (PortalAnimAsset.Succeeded())
	{
		PortalSkeletalMesh->SetAnimationMode(EAnimationMode::AnimationSingleNode);
		PortalSkeletalMesh->SetAnimation(PortalAnimAsset.Object);
	}

	TriggerBox = CreateDefaultSubobject<UBoxComponent>(TEXT("TriggerBox"));
	TriggerBox->SetupAttachment(Root);
	TriggerBox->SetBoxExtent(FVector(100.f, 100.f, 150.f));
	TriggerBox->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	TriggerBox->SetCollisionResponseToAllChannels(ECR_Ignore);
	TriggerBox->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
}

void APortalActor::BeginPlay()
{
	Super::BeginPlay();

	HideLegacyPortalSkeletalMeshes();
	ApplyPortalColor();
	RefreshClearedMapVisual();
	if (PortalSkeletalMesh)
	{
		PortalSkeletalMesh->Play(true);
	}

	if (TriggerBox)
	{
		TriggerBox->OnComponentBeginOverlap.AddDynamic(this, &APortalActor::OnOverlapBegin);
		TriggerBox->OnComponentEndOverlap.AddDynamic(this, &APortalActor::OnOverlapEnd);
	}
}

#pragma region Game Progress
void APortalActor::RefreshClearedMapVisual()
{
	if (DestinationLevelName.IsNone())
	{
		return;
	}

	const URAGameInstance* RAGameInstance = GetGameInstance<URAGameInstance>();
	if (!RAGameInstance || !RAGameInstance->IsMapCleared(DestinationLevelName))
	{
		return;
	}

	PortalColor = FLinearColor(1.0f, 1.0f, 1.0f, 1.0f);
	ApplyPortalColor();

	UE_LOG(LogTemp, Warning, TEXT("[GameProgress] Cleared-map portal color applied. Destination=%s Color=(1,1,1)"),
		*DestinationLevelName.ToString());
}
#pragma endregion Game Progress

void APortalActor::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);

	HideLegacyPortalSkeletalMeshes();
	ApplyPortalColor();
}

void APortalActor::OnOverlapBegin(
	UPrimitiveComponent* OverlappedComponent,
	AActor* OtherActor,
	UPrimitiveComponent* OtherComp,
	int32 OtherBodyIndex,
	bool bFromSweep,
	const FHitResult& SweepResult)
{
	if (!OtherActor)
	{
		return;
	}

	ARACharacter* PlayerCharacter = Cast<ARACharacter>(OtherActor);
	if (!PlayerCharacter)
	{
		return;
	}

	PlayerCharacter->SetCurrentPortal(this);
	UE_LOG(LogTemp, Warning, TEXT("Entered Portal Range"));
}

void APortalActor::OnOverlapEnd(
	UPrimitiveComponent* OverlappedComponent,
	AActor* OtherActor,
	UPrimitiveComponent* OtherComp,
	int32 OtherBodyIndex)
{
	if (!OtherActor)
	{
		return;
	}

	ARACharacter* PlayerCharacter = Cast<ARACharacter>(OtherActor);
	if (!PlayerCharacter)
	{
		return;
	}

	PlayerCharacter->ClearCurrentPortal(this);
	UE_LOG(LogTemp, Warning, TEXT("Exited Portal Range"));
}

void APortalActor::Interact(AActor* InteractingActor)
{
	if (!InteractingActor || bIsTeleporting || bIsTransitioning)
	{
		return;
	}

	TeleportPlayer(InteractingActor);
}

void APortalActor::TeleportPlayer(AActor* OverlappingActor)
{
	if (DestinationLevelName.IsNone())
	{
		UE_LOG(LogTemp, Warning, TEXT("PortalActor: DestinationLevelName is None."));
		return;
	}

	APlayerController* PlayerController = nullptr;
	if (APawn* PlayerPawn = Cast<APawn>(OverlappingActor))
	{
		PlayerController = Cast<APlayerController>(PlayerPawn->GetController());
	}

	if (!PlayerController)
	{
		PlayerController = UGameplayStatics::GetPlayerController(this, 0);
	}

	if (!PlayerController)
	{
		UE_LOG(LogTemp, Warning, TEXT("PortalActor: PlayerController is null."));
		return;
	}

	bIsTransitioning = true;

	if (bOneShotTeleport)
	{
		bIsTeleporting = true;
	}

	if (ARAPlayerController* RAPlayerController = Cast<ARAPlayerController>(PlayerController))
	{
		if (UPlayerUIFlowComponent* PlayerUIFlowComponent = RAPlayerController->GetPlayerUIFlowComponent())
		{
			PlayerUIFlowComponent->SetPortalTransitionInputLocked(true);
			PlayerUIFlowComponent->HideMainHUD();
		}
	}
	else
	{
		PlayerController->SetIgnoreMoveInput(true);
		PlayerController->SetIgnoreLookInput(true);
	}

	if (PortalEnterSound)
	{
		UGameplayStatics::PlaySound2D(this, PortalEnterSound);
	}

	if (PlayerController->PlayerCameraManager)
	{
		PlayerController->PlayerCameraManager->StartCameraFade(
			0.f,
			1.f,
			FMath::Max(0.f, FadeOutDuration),
			FLinearColor::Black,
			false,
			true
		);
	}

	const float TransitionDelay = FMath::Max(KINDA_SMALL_NUMBER, FadeOutDuration);
	GetWorldTimerManager().SetTimer(
		TransitionTimerHandle,
		this,
		&APortalActor::TravelToTargetLevel,
		TransitionDelay,
		false
	);
}

void APortalActor::TravelToTargetLevel()
{
	if (DestinationLevelName.IsNone())
	{
		UE_LOG(LogTemp, Warning, TEXT("PortalActor: DestinationLevelName is None."));
		bIsTransitioning = false;
		return;
	}

	if (URAGameInstance* RAGameInstance = Cast<URAGameInstance>(UGameplayStatics::GetGameInstance(this)))
	{
		RAGameInstance->bPendingPortalTransition = true;
	}

#pragma region Runtime Data
	if (ARACharacter* PlayerCharacter = Cast<ARACharacter>(UGameplayStatics::GetPlayerCharacter(this, 0)))
	{
		PlayerCharacter->SaveRuntimeDataToGameInstance();
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("PortalActor: Runtime data save skipped because player character is null."));
	}
#pragma endregion Runtime Data

	UE_LOG(LogTemp, Warning, TEXT("Teleporting to level: %s"), *DestinationLevelName.ToString());
	UGameplayStatics::OpenLevel(this, DestinationLevelName);
}

void APortalActor::ApplyPortalColor()
{
	ApplyPortalColorToMaterial(GlowMaterialSlotName, GlowColorParameterNames);
	ApplyPortalColorToMaterial(FloorMaterialSlotName, FloorColorParameterNames);
}

void APortalActor::ApplyPortalColorToMaterial(FName MaterialSlotName, const TArray<FName>& ParameterNames)
{
	if (!PortalSkeletalMesh || MaterialSlotName.IsNone() || ParameterNames.IsEmpty())
	{
		return;
	}

	const int32 MaterialIndex = FindPortalMaterialIndex(MaterialSlotName);
	if (MaterialIndex == INDEX_NONE)
	{
		return;
	}

	if (!PortalMaterialInstances.IsValidIndex(MaterialIndex) || !PortalMaterialInstances[MaterialIndex])
	{
		PortalMaterialInstances.SetNum(FMath::Max(PortalMaterialInstances.Num(), MaterialIndex + 1));
		PortalMaterialInstances[MaterialIndex] = PortalSkeletalMesh->CreateDynamicMaterialInstance(MaterialIndex);
	}

	UMaterialInstanceDynamic* MaterialInstance = PortalMaterialInstances.IsValidIndex(MaterialIndex) ? PortalMaterialInstances[MaterialIndex] : nullptr;
	if (MaterialInstance)
	{
		for (const FName& ParameterName : ParameterNames)
		{
			if (!ParameterName.IsNone())
			{
				MaterialInstance->SetVectorParameterValue(ParameterName, PortalColor);
			}
		}
	}
}

int32 APortalActor::FindPortalMaterialIndex(FName MaterialSlotName) const
{
	if (!PortalSkeletalMesh || MaterialSlotName.IsNone())
	{
		return INDEX_NONE;
	}

	int32 MaterialIndex = PortalSkeletalMesh->GetMaterialIndex(MaterialSlotName);
	if (MaterialIndex != INDEX_NONE)
	{
		return MaterialIndex;
	}

	const FString MaterialNameKey = MaterialSlotName.ToString();
	for (int32 Index = 0; Index < PortalSkeletalMesh->GetNumMaterials(); ++Index)
	{
		UMaterialInterface* Material = PortalSkeletalMesh->GetMaterial(Index);
		if (Material && Material->GetName().Contains(MaterialNameKey))
		{
			return Index;
		}
	}

	return INDEX_NONE;
}

void APortalActor::HideLegacyPortalSkeletalMeshes()
{
	TArray<USkeletalMeshComponent*> SkeletalMeshComponents;
	GetComponents(SkeletalMeshComponents);

	for (USkeletalMeshComponent* SkeletalMeshComponent : SkeletalMeshComponents)
	{
		if (SkeletalMeshComponent && SkeletalMeshComponent != PortalSkeletalMesh)
		{
			SkeletalMeshComponent->SetVisibility(false);
			SkeletalMeshComponent->SetHiddenInGame(true);
		}
	}
}
