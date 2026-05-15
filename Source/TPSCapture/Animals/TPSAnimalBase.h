#pragma once

#include "CoreMinimal.h"
#include "TPSCreatureBase.h"
#include "TPSStructTypes.h"
#include "TPSGameEnums.h"
#include "TPSAnimalBase.generated.h"

class UDataTable;

UCLASS()
class TPSCAPTURE_API AAnimalBase : public ATPSCreatureBase
{
    GENERATED_BODY()

public:
    AAnimalBase();

protected:
    virtual void BeginPlay() override;

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
};