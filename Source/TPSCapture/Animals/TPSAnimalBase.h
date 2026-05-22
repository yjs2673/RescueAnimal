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

UCLASS()
class TPSCAPTURE_API AAnimalBase : public ATPSCreatureBase
{
    GENERATED_BODY()

public:
    AAnimalBase();

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

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Animal|Data")
    UDataTable* AnimalDataTable;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Animal|Data")
    FAnimalData AnimalData;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Animal|State")
    EAnimalState AnimalState = EAnimalState::Idle;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Animal|Capture")
    float CaptureDifficulty = 1.0f;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Animal|Drop")
    TArray<FName> DropItemIDs;

protected:
    UFUNCTION(BlueprintCallable, Category = "Animal|Data")
    void InitAnimalData();

    UFUNCTION(BlueprintCallable, Category = "Animal|State")
    void SetAnimalState(EAnimalState NewState);

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
};