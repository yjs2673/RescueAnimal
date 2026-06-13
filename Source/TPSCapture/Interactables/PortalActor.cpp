#include "PortalActor.h"
#include "Animation/AnimationAsset.h"
#include "Components/BoxComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/SkeletalMesh.h"
#include "Kismet/GameplayStatics.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"
#include "TPSCaptureCharacter.h"
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

	ATPSCaptureCharacter* PlayerCharacter = Cast<ATPSCaptureCharacter>(OtherActor);
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

	ATPSCaptureCharacter* PlayerCharacter = Cast<ATPSCaptureCharacter>(OtherActor);
	if (!PlayerCharacter)
	{
		return;
	}

	PlayerCharacter->ClearCurrentPortal(this);
	UE_LOG(LogTemp, Warning, TEXT("Exited Portal Range"));
}

void APortalActor::Interact(AActor* InteractingActor)
{
	if (!InteractingActor || bIsTeleporting)
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

	if (bOneShotTeleport)
	{
		bIsTeleporting = true;
	}

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
