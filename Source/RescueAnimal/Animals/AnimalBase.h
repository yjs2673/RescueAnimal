#pragma once

#include "CoreMinimal.h"
#include "RACreatureBase.h"
#include "RAStructTypes.h"
#include "RAGameEnums.h"
#include "AnimalBase.generated.h"

class UDataTable;
class UWidgetComponent;
class UEnemyHPBarWidget;
class UStaticMeshComponent;
class UNiagaraSystem;
class USoundBase;
class AAnimalBase;
class AEnemyCampActor;
class UAnimalAIComponent;
class UAnimalPresentationComponent;
class UAnimalRescueComponent;
class UAnimalStateComponent;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnAnimalRescued, AAnimalBase*, RescuedAnimal);

UCLASS()
class RESCUEANIMAL_API AAnimalBase : public ARACreatureBase
{
    GENERATED_BODY()

    friend class UAnimalAIComponent;
    friend class UAnimalPresentationComponent;
    friend class UAnimalRescueComponent;
    friend class UAnimalStateComponent;

public:
    AAnimalBase();

    UFUNCTION(BlueprintCallable, Category = "Animal|Rescue")
    bool Rescue();

    UFUNCTION(BlueprintPure, Category = "Animal|Rescue")
    bool IsTrapped() const { return AnimalState == EAnimalState::Trapped; }

    UFUNCTION(BlueprintPure, Category = "Animal|Rescue")
    bool IsRescued() const { return bHasBeenRescued; }

    UFUNCTION(BlueprintPure, Category = "Animal|Rescue")
    AEnemyCampActor* GetOwningCamp() const { return OwningCamp; }

    UPROPERTY(BlueprintAssignable, Category = "Animal|Rescue")
    FOnAnimalRescued OnAnimalRescued;

    UFUNCTION(BlueprintPure, Category = "Components")
    FORCEINLINE UAnimalAIComponent* GetAnimalAIComponent() const { return AnimalAIComponent; }

    UFUNCTION(BlueprintPure, Category = "Components")
    FORCEINLINE UAnimalPresentationComponent* GetAnimalPresentationComponent() const { return AnimalPresentationComponent; }

    UFUNCTION(BlueprintPure, Category = "Components")
    FORCEINLINE UAnimalRescueComponent* GetAnimalRescueComponent() const { return AnimalRescueComponent; }

    UFUNCTION(BlueprintPure, Category = "Components")
    FORCEINLINE UAnimalStateComponent* GetAnimalStateComponent() const { return AnimalStateComponent; }

protected:
    virtual void BeginPlay() override;

public:
    virtual float TakeDamage(
        float DamageAmount,
        struct FDamageEvent const& DamageEvent,
        AController* EventInstigator,
        AActor* DamageCauser
    ) override;

protected:
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Animal|Data")
    FName AnimalID;

#pragma region Runtime World State
    UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Category = "World State")
    FName AnimalSaveID = NAME_None;
#pragma endregion Runtime World State

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Animal|Data")
    UDataTable* AnimalDataTable;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Animal|Data")
    FAnimalData AnimalData;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Animal|State")
    EAnimalState AnimalState = EAnimalState::Idle;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Animal|Rescue")
    bool bStartTrapped = false;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Animal|Rescue")
    bool bHasBeenRescued = false;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Animal|Rescue")
    TObjectPtr<AEnemyCampActor> OwningCamp = nullptr;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Animal|Capture")
    float CaptureDifficulty = 1.0f;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Animal|Drop")
    TArray<FName> DropItemIDs;

protected: // Components
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
    TObjectPtr<UAnimalAIComponent> AnimalAIComponent;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
    TObjectPtr<UAnimalPresentationComponent> AnimalPresentationComponent;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
    TObjectPtr<UAnimalRescueComponent> AnimalRescueComponent;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
    TObjectPtr<UAnimalStateComponent> AnimalStateComponent;

protected:
    UFUNCTION(BlueprintImplementableEvent, Category = "Animal|Rescue")
    void BP_OnRescued();

protected: // Rescue Visual
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Animal|Rescue")
    UStaticMeshComponent* CageMeshComponent;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Animal|Rescue")
    UWidgetComponent* SaveWidgetComponent;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Animal|Rescue")
    UNiagaraSystem* CageDisappearEffect;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Animal|Rescue")
    USoundBase* CageDisappearSound;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Animal|Rescue")
    USoundBase* RescueSound;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Animal|Rescue")
    float SaveWidgetVisibleDuration = 1.0f;

    FTimerHandle SaveWidgetHideTimerHandle;

protected: // HP Bar
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Animal|UI")
    UWidgetComponent* HPWidgetComponent;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Animal|UI")
    float HPBarVisibleDuration = 3.0f;

    FTimerHandle HPBarHideTimerHandle;

    UFUNCTION(BlueprintCallable, Category = "Animal|UI")
    void UpdateHPBar();

protected: // Animal AI
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Animal|AI")
    float WanderRadius = 500.0f;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Animal|AI")
    float WanderInterval = 3.0f;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Animal|AI")
    float WanderSpeed = 120.0f;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Animal|AI")
    float FleeDistance = 800.0f;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Animal|AI")
    float FleeDuration = 3.0f;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Animal|AI")
    float FleeSpeed = 450.0f;

    FTimerHandle WanderTimerHandle;
    FTimerHandle FleeTimerHandle;

protected: // Death Visual
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Animal|Death")
    float DeathLifeSpan = 3.0f;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Animal|Death")
    FRotator DeathRotationOffset = FRotator(90.0f, 0.0f, 0.0f);

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Animal|Death")
    float DeathFallDuration = 0.35f;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Animal|Death")
    bool bAnimalDeathVisualPlayed = false;

    FTimerHandle DeathFallTimerHandle;

    FRotator DeathStartRotation;
    FRotator DeathTargetRotation;

    float DeathFallElapsedTime = 0.0f;

};
