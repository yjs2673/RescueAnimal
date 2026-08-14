#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "TimerManager.h"
#include "PortalActor.generated.h"

class UBoxComponent;
class UMaterialInstanceDynamic;
class USoundBase;
class USkeletalMeshComponent;
class UStaticMeshComponent;

UCLASS()
class RESCUEANIMAL_API APortalActor : public AActor
{
	GENERATED_BODY()

public:
	APortalActor();

	void Interact(AActor* InteractingActor);

#pragma region Game Progress
	UFUNCTION(BlueprintCallable, Category = "Portal|Game Progress")
	void RefreshClearedMapVisual();
#pragma endregion Game Progress

protected:
	virtual void BeginPlay() override;
	virtual void OnConstruction(const FTransform& Transform) override;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Portal")
	USceneComponent* Root;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Portal")
	UStaticMeshComponent* PortalMesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Portal")
	USkeletalMeshComponent* PortalSkeletalMesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Portal")
	UBoxComponent* TriggerBox;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Portal")
	FName DestinationLevelName;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Portal")
	bool bOneShotTeleport = true;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Portal|Transition")
	float FadeOutDuration = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Portal|Transition")
	USoundBase* PortalEnterSound = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Portal|Visual")
	FLinearColor PortalColor = FLinearColor(0.f, 0.5f, 1.f, 1.f);

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Portal|Visual")
	FName GlowMaterialSlotName = TEXT("glow");

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Portal|Visual")
	TArray<FName> GlowColorParameterNames = { TEXT("BaseColorFactor"), TEXT("EmissiveFactor") };

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Portal|Visual")
	FName FloorMaterialSlotName = TEXT("floor");

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Portal|Visual")
	TArray<FName> FloorColorParameterNames = { TEXT("BaseColorFactor") };

	bool bIsTeleporting = false;
	bool bIsTransitioning = false;

	FTimerHandle TransitionTimerHandle;

	UPROPERTY(Transient)
	TArray<UMaterialInstanceDynamic*> PortalMaterialInstances;

	UFUNCTION()
	void OnOverlapBegin(
		UPrimitiveComponent* OverlappedComponent,
		AActor* OtherActor,
		UPrimitiveComponent* OtherComp,
		int32 OtherBodyIndex,
		bool bFromSweep,
		const FHitResult& SweepResult
	);

	UFUNCTION()
	void OnOverlapEnd(
		UPrimitiveComponent* OverlappedComponent,
		AActor* OtherActor,
		UPrimitiveComponent* OtherComp,
		int32 OtherBodyIndex
	);

	void TeleportPlayer(AActor* OverlappingActor);
	void TravelToTargetLevel();
	void ApplyPortalColor();
	void ApplyPortalColorToMaterial(FName MaterialSlotName, const TArray<FName>& ParameterNames);
	int32 FindPortalMaterialIndex(FName MaterialSlotName) const;
	void HideLegacyPortalSkeletalMeshes();
};
