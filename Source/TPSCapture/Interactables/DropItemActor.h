#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "DropItemActor.generated.h"

class USphereComponent;
class USceneComponent;
class UStaticMeshComponent;
class UPrimitiveComponent;

UCLASS()
class TPSCAPTURE_API ADropItemActor : public AActor
{
	GENERATED_BODY()

public:
	ADropItemActor();

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<USceneComponent> SceneRoot;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UStaticMeshComponent> ItemMesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<USphereComponent> PickupCollision;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Item")
	FName ItemID = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Item")
	int32 Count = 1;

protected:
	virtual void BeginPlay() override;

	UFUNCTION()
	void OnPickupCollisionBeginOverlap(
		UPrimitiveComponent* OverlappedComponent,
		AActor* OtherActor,
		UPrimitiveComponent* OtherComp,
		int32 OtherBodyIndex,
		bool bFromSweep,
		const FHitResult& SweepResult
	);

public:
	UFUNCTION(BlueprintCallable, Category = "Item")
	void InitializeDropItem(FName InItemID, int32 InCount);
};
