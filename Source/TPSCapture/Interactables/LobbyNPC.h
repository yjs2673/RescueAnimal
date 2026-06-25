#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "LobbyNPC.generated.h"

class UBoxComponent;
class UPrimitiveComponent;
class USceneComponent;
class USkeletalMeshComponent;

UENUM(BlueprintType)
enum class ENPCDialogueState : uint8
{
	Intro UMETA(DisplayName = "Intro"),
	Progress UMETA(DisplayName = "Progress"),
	Ending UMETA(DisplayName = "Ending")
};

UCLASS()
class TPSCAPTURE_API ALobbyNPC : public AActor
{
	GENERATED_BODY()

public:
	ALobbyNPC();

	UFUNCTION(BlueprintPure, Category = "Lobby NPC|Dialogue")
	ENPCDialogueState GetCurrentDialogueState() const;

	UFUNCTION(BlueprintCallable, Category = "Lobby NPC|Dialogue")
	void Interact();

	UFUNCTION(BlueprintCallable, Category = "Lobby NPC|Dialogue")
	void StartIntroDialogue();

	UFUNCTION(BlueprintCallable, Category = "Lobby NPC|Dialogue")
	void ShowProgressDialogue();

	UFUNCTION(BlueprintCallable, Category = "Lobby NPC|Dialogue")
	void StartEndingDialogue();

protected:
	virtual void BeginPlay() override;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Lobby NPC")
	TObjectPtr<USceneComponent> SceneRoot;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Lobby NPC")
	TObjectPtr<USkeletalMeshComponent> NPCMesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Lobby NPC|Interaction")
	TObjectPtr<UBoxComponent> InteractionBox;

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
};
