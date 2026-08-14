#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "DropItemActor.generated.h"

class USphereComponent;
class USceneComponent;
class UStaticMeshComponent;
class USkeletalMeshComponent;
class UPrimitiveComponent;
class USoundBase;

UCLASS()
class RESCUEANIMAL_API ADropItemActor : public AActor
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
	TObjectPtr<USkeletalMeshComponent> ItemSkeletalMesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<USphereComponent> PickupCollision;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Item")
	FName ItemID = NAME_None;

#pragma region Runtime World State
	UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Category = "World State")
	FName ItemSaveID = NAME_None;
#pragma endregion Runtime World State

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Item")
	int32 Count = 1;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Item|SFX")
	TObjectPtr<USoundBase> PickupSound = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Item|SFX")
	float PickupSoundVolume = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Item|SFX")
	float PickupSoundPitch = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Item|Motion")
	bool bEnablePickupMotion = true;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Item|Motion")
	float PickupBobAmplitude = 15.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Item|Motion")
	float PickupBobSpeed = 2.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Item|Motion")
	float PickupRotationSpeed = 90.0f;

protected:
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaTime) override;

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

#pragma region Runtime Spawned Drop Item
	UFUNCTION(BlueprintCallable, Category = "World State")
	void InitializeRuntimeDropItem(FName InItemID, int32 InCount, FName InItemSaveID);

	UFUNCTION(BlueprintPure, Category = "World State")
	FName GetItemSaveID() const { return ItemSaveID; }
#pragma endregion Runtime Spawned Drop Item

protected:
	void ApplyItemDataToDropVisual();
	void CacheInitialPickupMotionTransform();
	void ApplyPickupMotion(float DeltaTime);
	USceneComponent* GetActiveVisualComponent() const;

	FVector InitialMeshRelativeLocation = FVector::ZeroVector;
	FRotator InitialMeshRelativeRotation = FRotator::ZeroRotator;
	FRotator InitialActorRotation = FRotator::ZeroRotator;
	float PickupMotionElapsedTime = 0.0f;
	float PickupMotionRotationYaw = 0.0f;
};
