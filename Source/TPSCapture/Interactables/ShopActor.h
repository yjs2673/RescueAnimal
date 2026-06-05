#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "TPSStructTypes.h"
#include "ShopActor.generated.h"

class UBoxComponent;
class USceneComponent;
class UStaticMeshComponent;
class USkeletalMeshComponent;
class UDataTable;
class UAnimMontage;

UCLASS()
class TPSCAPTURE_API AShopActor : public AActor
{
	GENERATED_BODY()

public:
	AShopActor();

	void Interact(AActor* InteractingActor);

	UFUNCTION(BlueprintPure, Category = "Shop")
	UDataTable* GetShopItemDataTable() const { return ShopItemDataTable; }

	UFUNCTION(BlueprintPure, Category = "Shop")
	FName GetCurrencyItemID() const { return CurrencyItemID; }

	UFUNCTION(BlueprintCallable, Category = "Shop")
	void OnShopClosed();

protected:
	virtual void BeginPlay() override;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Shop")
	TObjectPtr<USceneComponent> SceneRoot;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Shop")
	TObjectPtr<USkeletalMeshComponent> NPCMesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Shop")
	TObjectPtr<UStaticMeshComponent> DisplayMesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Shop")
	TObjectPtr<UBoxComponent> InteractionBox;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Shop")
	TObjectPtr<UDataTable> ShopItemDataTable;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Shop")
	FName CurrencyItemID = TEXT("Coin");

	UFUNCTION()
	void OnInteractionBeginOverlap(
		UPrimitiveComponent* OverlappedComponent,
		AActor* OtherActor,
		UPrimitiveComponent* OtherComp,
		int32 OtherBodyIndex,
		bool bFromSweep,
		const FHitResult& SweepResult
	);

	UFUNCTION()
	void OnInteractionEndOverlap(
		UPrimitiveComponent* OverlappedComponent,
		AActor* OtherActor,
		UPrimitiveComponent* OtherComp,
		int32 OtherBodyIndex
	);

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Shop|Animation")
	TObjectPtr<UAnimMontage> ShopClosedMontage;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Shop|Animation")
	float ShopClosedMontagePlayRate = 1.0f;
};