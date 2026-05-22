#include "TPSAnimalBase.h"
#include "Engine/DataTable.h"
#include "Components/WidgetComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "TimerManager.h"
#include "EnemyHPBarWidget.h"

#include "AIController.h"
#include "NavigationSystem.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Materials/MaterialInstanceDynamic.h"

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

    StartWander();
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

        if (CurrentHP <= 0.0f)
        {
            PlayAnimalDeathVisual();
        }
        else
        {
            StartFlee(DamageCauser);
        }
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

void AAnimalBase::StartWander()
{
    if (AnimalState == EAnimalState::Dead || AnimalState == EAnimalState::Captured)
    {
        return;
    }

    GetWorldTimerManager().ClearTimer(FleeTimerHandle);

    SetAnimalState(EAnimalState::Wander);

    if (GetCharacterMovement())
    {
        GetCharacterMovement()->MaxWalkSpeed = WanderSpeed;
    }

    MoveToRandomLocation();

    GetWorldTimerManager().ClearTimer(WanderTimerHandle);
    GetWorldTimerManager().SetTimer(
        WanderTimerHandle,
        this,
        &AAnimalBase::MoveToRandomLocation,
        WanderInterval,
        true
    );
}

void AAnimalBase::MoveToRandomLocation()
{
    if (AnimalState == EAnimalState::Dead || AnimalState == EAnimalState::Captured)
    {
        return;
    }

    if (AnimalState == EAnimalState::Flee)
    {
        return;
    }

    UNavigationSystemV1* NavSystem = UNavigationSystemV1::GetCurrent(GetWorld());
    if (!NavSystem)
    {
        return;
    }

    FNavLocation RandomLocation;

    const bool bFoundLocation = NavSystem->GetRandomReachablePointInRadius(
        GetActorLocation(),
        WanderRadius,
        RandomLocation
    );

    if (!bFoundLocation)
    {
        return;
    }

    AAIController* AIController = Cast<AAIController>(GetController());
    if (!AIController)
    {
        return;
    }

    AIController->MoveToLocation(RandomLocation.Location);
}

void AAnimalBase::StartFlee(AActor* ThreatActor)
{
    if (AnimalState == EAnimalState::Dead || AnimalState == EAnimalState::Captured)
    {
        return;
    }

    if (!ThreatActor)
    {
        return;
    }

    GetWorldTimerManager().ClearTimer(WanderTimerHandle);
    GetWorldTimerManager().ClearTimer(FleeTimerHandle);

    SetAnimalState(EAnimalState::Flee);

    if (GetCharacterMovement())
    {
        GetCharacterMovement()->MaxWalkSpeed = FleeSpeed;
    }

    AAIController* AIController = Cast<AAIController>(GetController());
    if (!AIController)
    {
        return;
    }

    FVector FleeDirection = GetActorLocation() - ThreatActor->GetActorLocation();
    FleeDirection.Z = 0.0f;

    if (FleeDirection.IsNearlyZero())
    {
        FleeDirection = GetActorForwardVector();
    }

    FleeDirection.Normalize();

    const FVector DesiredLocation = GetActorLocation() + FleeDirection * FleeDistance;

    UNavigationSystemV1* NavSystem = UNavigationSystemV1::GetCurrent(GetWorld());
    if (!NavSystem)
    {
        return;
    }

    FNavLocation FleeLocation;

    const bool bFoundLocation = NavSystem->ProjectPointToNavigation(
        DesiredLocation,
        FleeLocation
    );

    if (bFoundLocation)
    {
        AIController->MoveToLocation(FleeLocation.Location);
    }

    GetWorldTimerManager().SetTimer(
        FleeTimerHandle,
        this,
        &AAnimalBase::StopFlee,
        FleeDuration,
        false
    );
}

void AAnimalBase::StopFlee()
{
    if (AnimalState == EAnimalState::Dead || AnimalState == EAnimalState::Captured)
    {
        return;
    }

    StopMovement();

    StartWander();
}

void AAnimalBase::StopMovement()
{
    AAIController* AIController = Cast<AAIController>(GetController());
    if (!AIController)
    {
        return;
    }

    AIController->StopMovement();
}

void AAnimalBase::PlayAnimalDeathVisual()
{
    if (bAnimalDeathVisualPlayed)
    {
        return;
    }

    bAnimalDeathVisualPlayed = true;

    SetAnimalState(EAnimalState::Dead);

    // 타이머 정리
    GetWorldTimerManager().ClearTimer(WanderTimerHandle);
    GetWorldTimerManager().ClearTimer(FleeTimerHandle);
    GetWorldTimerManager().ClearTimer(HPBarHideTimerHandle);

    HideHPBar();

    // AI 이동 정지
    if (AAIController* AIController = Cast<AAIController>(GetController()))
    {
        AIController->StopMovement();
    }

    // Character Movement 정지
    if (GetCharacterMovement())
    {
        GetCharacterMovement()->StopMovementImmediately();
        GetCharacterMovement()->DisableMovement();
    }

    if (GetMesh())
    {
        // 현재 애니메이션 프레임에서 정지
        GetMesh()->bPauseAnims = true;

        // 물리 시뮬레이션 X
        GetMesh()->SetSimulatePhysics(false);

        // 회전 보간 준비
        DeathStartRotation = GetMesh()->GetRelativeRotation();
        DeathTargetRotation = DeathStartRotation + DeathRotationOffset;
        DeathFallElapsedTime = 0.0f;

        GetWorldTimerManager().ClearTimer(DeathFallTimerHandle);
        GetWorldTimerManager().SetTimer(
            DeathFallTimerHandle,
            this,
            &AAnimalBase::UpdateDeathFallRotation,
            0.016f,
            true
        );
    }

    SetActorEnableCollision(false);
    SetLifeSpan(DeathLifeSpan);

    UE_LOG(LogTemp, Warning, TEXT("[AnimalBase] PlayAnimalDeathVisual: %s"), *GetName());
}

void AAnimalBase::UpdateDeathFallRotation()
{
    if (!GetMesh())
    {
        GetWorldTimerManager().ClearTimer(DeathFallTimerHandle);
        return;
    }

    DeathFallElapsedTime += 0.016f;

    const float Alpha = FMath::Clamp(
        DeathFallElapsedTime / DeathFallDuration,
        0.0f,
        1.0f
    );

    const float SmoothAlpha = FMath::InterpEaseOut(0.0f, 1.0f, Alpha, 2.0f);

    const FRotator NewRotation = FMath::Lerp(
        DeathStartRotation,
        DeathTargetRotation,
        SmoothAlpha
    );

    GetMesh()->SetRelativeRotation(NewRotation);

    if (Alpha >= 1.0f)
    {
        GetMesh()->SetRelativeRotation(DeathTargetRotation);
        GetWorldTimerManager().ClearTimer(DeathFallTimerHandle);
    }
}