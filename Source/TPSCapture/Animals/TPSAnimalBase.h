#pragma once

#include "CoreMinimal.h"
#include "TPSCreatureBase.h"
#include "TPSStructTypes.h"
#include "TPSGameEnums.h"
#include "TPSAnimalBase.generated.h"

class UDataTable;
class UWidgetComponent;
class UEnemyHPBarWidget;
class AAIController;
class UMaterialInstanceDynamic;
class UStaticMeshComponent;
class UNiagaraSystem;
class USoundBase;
class AAnimalBase;
class AEnemyCampActor;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnAnimalRescued, AAnimalBase*, RescuedAnimal);

UCLASS()
class TPSCAPTURE_API AAnimalBase : public ATPSCreatureBase
{
    GENERATED_BODY()

public:
    AAnimalBase();

    UFUNCTION(BlueprintCallable, Category = "Animal|Rescue")
    bool Rescue();

    UFUNCTION(BlueprintPure, Category = "Animal|Rescue")
    bool IsTrapped() const { return AnimalState == EAnimalState::Trapped; }

    UFUNCTION(BlueprintPure, Category = "Animal|Rescue")
    bool IsRescued() const { return bHasBeenRescued; }

    UFUNCTION(BlueprintCallable, Category = "Animal|Rescue")
    void SetOwningCamp(AEnemyCampActor* InCamp);

    UFUNCTION(BlueprintPure, Category = "Animal|Rescue")
    AEnemyCampActor* GetOwningCamp() const { return OwningCamp; }

    UFUNCTION(BlueprintPure, Category = "Animal|Rescue")
    bool CanBeRescued() const;

    UPROPERTY(BlueprintAssignable, Category = "Animal|Rescue")
    FOnAnimalRescued OnAnimalRescued;

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

    UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Category = "World State")
    FName AnimalSaveID = NAME_None;

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

protected:
    UFUNCTION(BlueprintCallable, Category = "Animal|Data")
    void InitAnimalData();

    UFUNCTION(BlueprintCallable, Category = "Animal|State")
    void SetAnimalState(EAnimalState NewState);

    UFUNCTION(BlueprintCallable, Category = "Animal|Rescue")
    void ApplyTrappedState();

    UFUNCTION(BlueprintCallable, Category = "Animal|Rescue")
    void ApplyRescuedState();

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

    UFUNCTION(BlueprintCallable, Category = "Animal|Rescue")
    void ShowSaveWidget();

    UFUNCTION(BlueprintCallable, Category = "Animal|Rescue")
    void HideSaveWidget();

protected: // HP Bar
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Animal|UI")
    UWidgetComponent* HPWidgetComponent;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Animal|UI")
    float HPBarVisibleDuration = 3.0f;

    FTimerHandle HPBarHideTimerHandle;

    UFUNCTION(BlueprintCallable, Category = "Animal|UI")
    void UpdateHPBar();

    UFUNCTION(BlueprintCallable, Category = "Animal|UI")
    void ShowHPBar();

    UFUNCTION(BlueprintCallable, Category = "Animal|UI")
    void HideHPBar();

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

    UFUNCTION(BlueprintCallable, Category = "Animal|AI")
    void StartWander();

    UFUNCTION(BlueprintCallable, Category = "Animal|AI")
    void MoveToRandomLocation();

    UFUNCTION(BlueprintCallable, Category = "Animal|AI")
    void StartFlee(AActor* ThreatActor);

    UFUNCTION(BlueprintCallable, Category = "Animal|AI")
    void StopFlee();

    UFUNCTION(BlueprintCallable, Category = "Animal|AI")
    void StopMovement();

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

    UFUNCTION(BlueprintCallable, Category = "Animal|Death")
    void PlayAnimalDeathVisual();

    UFUNCTION()
    void UpdateDeathFallRotation();
};
