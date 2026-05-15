#include "TPSAnimalBase.h"
#include "Engine/DataTable.h"
#include "Components/WidgetComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "TimerManager.h"
#include "EnemyHPBarWidget.h"

AAnimalBase::AAnimalBase()
{
    PrimaryActorTick.bCanEverTick = false;

    HPWidgetComponent = CreateDefaultSubobject<UWidgetComponent>(TEXT("HPWidgetComponent"));
    HPWidgetComponent->SetupAttachment(GetRootComponent());
    HPWidgetComponent->SetRelativeLocation(FVector(0.0f, 0.0f, 100.0f));
    HPWidgetComponent->SetWidgetSpace(EWidgetSpace::Screen);
    HPWidgetComponent->SetDrawSize(FVector2D(100.0f, 20.0f));
    HPWidgetComponent->SetVisibility(false);
}

void AAnimalBase::BeginPlay()
{
    Super::BeginPlay();

    InitAnimalData();

    UpdateHPBar();
    HideHPBar();
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

float AAnimalBase::TakeDamage(
    float DamageAmount,
    FDamageEvent const& DamageEvent,
    AController* EventInstigator,
    AActor* DamageCauser
)
{
    const float ActualDamage = Super::TakeDamage(
        DamageAmount,
        DamageEvent,
        EventInstigator,
        DamageCauser
    );

    if (ActualDamage > 0.0f)
    {
        UpdateHPBar();
        ShowHPBar();
    }

    return ActualDamage;
}

void AAnimalBase::UpdateHPBar()
{
    if (!HPWidgetComponent)
    {
        return;
    }

    UEnemyHPBarWidget* HPWidget = Cast<UEnemyHPBarWidget>(HPWidgetComponent->GetUserWidgetObject());
    if (!HPWidget)
    {
        return;
    }

    const float HPPercent = MaxHP > 0.0f ? CurrentHP / MaxHP : 0.0f;

    HPWidget->SetHPPercent(HPPercent);
}

void AAnimalBase::ShowHPBar()
{
    if (!HPWidgetComponent)
    {
        return;
    }

    HPWidgetComponent->SetVisibility(true);

    GetWorldTimerManager().ClearTimer(HPBarHideTimerHandle);

    GetWorldTimerManager().SetTimer(
        HPBarHideTimerHandle,
        this,
        &AAnimalBase::HideHPBar,
        HPBarVisibleDuration,
        false
    );
}

void AAnimalBase::HideHPBar()
{
    if (!HPWidgetComponent)
    {
        return;
    }

    HPWidgetComponent->SetVisibility(false);
}