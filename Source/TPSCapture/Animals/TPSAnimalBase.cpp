#include "TPSAnimalBase.h"
#include "Engine/DataTable.h"

AAnimalBase::AAnimalBase()
{
    PrimaryActorTick.bCanEverTick = false;
}

void AAnimalBase::BeginPlay()
{
    Super::BeginPlay();

    InitAnimalData();
}

void AAnimalBase::InitAnimalData()
{
    if (!AnimalDataTable)
    {
        UE_LOG(LogTemp, Warning, TEXT("[AnimalBase] AnimalDataTable is null. Actor: %s"), *GetName());
        return;
    }

    if (AnimalID.IsNone())
    {
        UE_LOG(LogTemp, Warning, TEXT("[AnimalBase] AnimalID is None. Actor: %s"), *GetName());
        return;
    }

    const FAnimalData* FoundData = AnimalDataTable->FindRow<FAnimalData>(
        AnimalID,
        TEXT("AAnimalBase::InitAnimalData")
    );

    if (!FoundData)
    {
        UE_LOG(LogTemp, Warning, TEXT("[AnimalBase] Failed to find AnimalData. AnimalID: %s"), *AnimalID.ToString());
        return;
    }

    AnimalData = *FoundData;

    CaptureDifficulty = AnimalData.CaptureDifficulty;
    DropItemIDs = AnimalData.DropItemIDs;

    MaxHP = AnimalData.MaxHP;
    CurrentHP = MaxHP;

    SetAnimalState(EAnimalState::Idle);

    UE_LOG(LogTemp, Warning, TEXT("[AnimalBase] Loaded AnimalData: %s / HP: %.1f / CaptureDifficulty: %.2f"),
        *AnimalID.ToString(),
        AnimalData.MaxHP,
        CaptureDifficulty
    );
}

void AAnimalBase::SetAnimalState(EAnimalState NewState)
{
    AnimalState = NewState;
}