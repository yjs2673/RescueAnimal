#include "EnemyPerformanceHarness.h"

#include "EnemyPerfTarget.h"
#include "RAEnemyBase.h"
#include "EnemyAIComponent.h"
#include "AIController.h"
#include "Components/CapsuleComponent.h"
#include "Components/SceneComponent.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "NavigationSystem.h"
#include "ProfilingDebugging/CpuProfilerTrace.h"
#include "ProfilingDebugging/CsvProfiler.h"
#include "TimerManager.h"

DEFINE_LOG_CATEGORY_STATIC(LogEnemyPerformance, Log, All);
CSV_DEFINE_CATEGORY(EnemyPerf, true);

AEnemyPerformanceHarness::AEnemyPerformanceHarness()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = false;
	PrimaryActorTick.TickGroup = TG_PostUpdateWork;

	SetRootComponent(CreateDefaultSubobject<USceneComponent>(TEXT("Root")));

	TargetClass = AEnemyPerfTarget::StaticClass();
	TargetRoute = { FVector(1800.f, 0.f, 100.f), FVector(1800.f, 1800.f, 100.f), FVector(0.f, 1800.f, 100.f) };
}

void AEnemyPerformanceHarness::BeginPlay()
{
	Super::BeginPlay();

	if (bAutoStart)
	{
		// 다른 액터들이 BeginPlay를 완료한 후 테스트 월드를 검증
		GetWorldTimerManager().SetTimerForNextTick(this, &AEnemyPerformanceHarness::StartTest);
	}
}

void AEnemyPerformanceHarness::StartTest()
{
	UWorld* World = GetWorld();

	if (!World || !World->IsGameWorld() || bHasStarted)
	{
		UE_LOG(LogEnemyPerformance, Warning, TEXT("StartTest requires a running game and a fresh world load."));
		return;
	}

	if (!EnemyClass || EnemyClass->HasAnyClassFlags(CLASS_Abstract) || EnemyCount < 1 || EnemyCount > 200 ||
		GridColumns < 1 || !FMath::IsFinite(GridSpacing) || GridSpacing <= 0.f ||
		!FMath::IsFinite(WarmupSeconds) || WarmupSeconds < 0.f ||
		!FMath::IsFinite(MeasurementSeconds) || MeasurementSeconds <= 0.f ||
		GridOrigin.ContainsNaN() || NavigationProjectionExtent.ContainsNaN() ||
		NavigationProjectionExtent.GetMin() <= 0.f)
	{
		FailTest(TEXT("Invalid EnemyClass, count, grid, projection extent or timing settings."));
		return;
	}

	const bool bNeedsTarget = Scenario == EEnemyPerfScenario::Chase ||
		Scenario == EEnemyPerfScenario::Attack || Scenario == EEnemyPerfScenario::Mixed;
	const bool bPatrol = Scenario == EEnemyPerfScenario::Wander || Scenario == EEnemyPerfScenario::Mixed;

	if ((bPatrol && (!FMath::IsFinite(PatrolRadius) || PatrolRadius <= 0.f)) ||
		(bNeedsTarget && (!TargetClass || TargetClass->HasAnyClassFlags(CLASS_Abstract) ||
		TargetRoute.IsEmpty() || !FMath::IsFinite(TargetSpeed) || TargetSpeed <= 0.f ||
		!FMath::IsFinite(TargetPauseSeconds) || TargetPauseSeconds < 0.f)))
	{
		FailTest(TEXT("Invalid patrol or target settings. TargetRoute needs at least one capsule-center point."));
		return;
	}
	for (TActorIterator<AEnemyPerformanceHarness> It(World); It; ++It)
	{
		if (*It != this)
		{
			FailTest(TEXT("Use exactly one harness per world."));
			return;
		}
	}

	if (TActorIterator<ARAEnemyBase>(World) || TActorIterator<ARACharacter>(World))
	{
		FailTest(TEXT("Existing Enemy/RACharacter would contaminate the run. Use an empty test map with a spectator pawn."));
		return;
	}

	UNavigationSystemV1* Nav = UNavigationSystemV1::GetCurrent(World);

	if (!Nav || UNavigationSystemV1::IsNavigationBeingBuiltOrLocked(World))
	{
		FailTest(TEXT("Navigation is unavailable or still building. Build the test NavMesh before StartTest."));
		return;
	}

	// Snapshot the settings that are used during Tick. Changing Details mid-run is not a new condition.
	bHasStarted = true;
	ActiveScenario = Scenario;
	ActiveRunLabel = RunLabel.ToString();
	ActiveWarmupSeconds = WarmupSeconds;
	ActiveMeasurementSeconds = MeasurementSeconds;
	ActivePauseSeconds = TargetPauseSeconds;

	const FTransform LayoutTransform(FRotator(0.f, GetActorRotation().Yaw, 0.f), GetActorLocation());

	WorldRoute.Reset();
	SegmentDurations.Reset();
	RouteDuration = 0.0;

	if (bNeedsTarget)
	{
		for (const FVector& Point : TargetRoute)
		{
			if (Point.ContainsNaN())
			{
				FailTest(TEXT("TargetRoute contains an invalid point."));
				return;
			}
			WorldRoute.Add(LayoutTransform.TransformPosition(Point));
		}

		for (int32 Index = 0; Index < WorldRoute.Num(); ++Index)
		{
			const double Duration = FVector::Distance(WorldRoute[Index], WorldRoute[(Index + 1) % WorldRoute.Num()]) / TargetSpeed;
			SegmentDurations.Add(Duration);
			RouteDuration += ActivePauseSeconds + Duration;
		}

		FActorSpawnParameters TargetParams;
		TargetParams.Owner = this;
		TargetParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::DontSpawnIfColliding;
		Target = World->SpawnActor<AEnemyPerfTarget>(TargetClass, WorldRoute[0], FRotator::ZeroRotator, TargetParams);

		if (!IsValid(Target) || Target->GetController())
		{
			FailTest(TEXT("Dummy spawn failed or it was possessed. Disable Auto Possess in its Blueprint."));
			return;
		}
	}

	UE_LOG(LogEnemyPerformance, Display, TEXT("Setup %s Scenario=%d EnemyClass=%s Requested=%d Columns=%d Spacing=%.1f Warmup=%.2f Measure=%.2f TargetClass=%s Speed=%.1f Pause=%.2f PatrolRadius=%.1f InitialTarget=%d"),
		*ActiveRunLabel, static_cast<int32>(ActiveScenario), *EnemyClass->GetPathName(), EnemyCount,
		GridColumns, GridSpacing, WarmupSeconds, MeasurementSeconds, *GetNameSafe(TargetClass.Get()),
		TargetSpeed, TargetPauseSeconds, PatrolRadius, bAssignInitialTarget);

	for (int32 Index = 0; Index < WorldRoute.Num(); ++Index)
	{
		UE_LOG(LogEnemyPerformance, Display, TEXT("Route[%d]=%s"), Index, *WorldRoute[Index].ToString());
	}

	for (int32 Index = 0; Index < EnemyCount; ++Index)
	{
		const FVector LocalGround = GridOrigin + FVector((Index % GridColumns) * GridSpacing, (Index / GridColumns) * GridSpacing, 0.f);
		FNavLocation Projected;

		if (!Nav->ProjectPointToNavigation(LayoutTransform.TransformPosition(LocalGround), Projected, NavigationProjectionExtent))
		{
			FailTest(FString::Printf(TEXT("No NavMesh at grid index %d. No partial run will be measured."), Index));
			return;
		}

		const float HalfHeight = EnemyClass->GetDefaultObject<ARAEnemyBase>()->GetCapsuleComponent()->GetScaledCapsuleHalfHeight();
		const FVector SpawnLocation = Projected.Location + FVector(0.f, 0.f, HalfHeight + 2.f);

		FActorSpawnParameters Params;
		Params.Owner = this;
		Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::DontSpawnIfColliding;

		ARAEnemyBase* Enemy = World->SpawnActor<ARAEnemyBase>(EnemyClass, SpawnLocation, LayoutTransform.Rotator(), Params);

		if (!IsValid(Enemy))
		{
			FailTest(FString::Printf(TEXT("Spawn blocked at index %d (%s). Check spacing/floor/obstacles."), Index, *SpawnLocation.ToString()));
			return;
		}

		SpawnedEnemies.Add(Enemy);

		if (!Enemy->GetController())
		{
			Enemy->SpawnDefaultController();
		}

		if (AAIController* Controller = Cast<AAIController>(Enemy->GetController()))
		{
			SpawnedControllers.Add(Controller);
		}

		UEnemyAIComponent* AI = Enemy->GetEnemyAIComponent();

		if (!Cast<AAIController>(Enemy->GetController()) || !AI || !AI->IsComponentTickEnabled() ||
			Enemy->GetEnemyAIState() == EEnemyAIState::Dead)
		{
			FailTest(TEXT("Enemy requires a live AI Component and an AIController. Check the actual Enemy Blueprint."));
			return;
		}

		if (bPatrol)
		{
			AI->SetCampPatrolArea(GetActorLocation(), PatrolRadius);
		}
		else
		{
			AI->ClearCampPatrolArea();
		}

		if (bAssignInitialTarget && (ActiveScenario == EEnemyPerfScenario::Chase || ActiveScenario == EEnemyPerfScenario::Attack))
		{
			AI->SetTargetActor(Target);
		}

		UE_LOG(LogEnemyPerformance, Display, TEXT("Spawn[%d]=%s"), Index, *Enemy->GetActorLocation().ToString());
	}

	ActualSpawnedCount = SpawnedEnemies.Num();
	Phase = 0;
	RunStartSeconds = World->GetTimeSeconds();
	bMeasuring = false;

	TRACE_BOOKMARK(TEXT("EnemyPerf Warmup Run=%s Scenario=%d N=%d"), *ActiveRunLabel, static_cast<int32>(ActiveScenario), ActualSpawnedCount);
	SetActorTickEnabled(true);
}

void AEnemyPerformanceHarness::Tick(float DeltaSeconds)
{
	TRACE_CPUPROFILER_EVENT_SCOPE(PerfHarness_Update);
	Super::Tick(DeltaSeconds);

	const double Now = GetWorld()->GetTimeSeconds();
	const double Elapsed = Now - RunStartSeconds;

	if (Phase == 0 && Elapsed >= ActiveWarmupSeconds)
	{
		Phase = 1;
		bMeasuring = true;
		MeasurementStartSeconds = Now;
		TRACE_BOOKMARK(TEXT("EnemyPerf MeasureBegin Run=%s Scenario=%d N=%d"), *ActiveRunLabel, static_cast<int32>(ActiveScenario), ActualSpawnedCount);
		UE_LOG(LogEnemyPerformance, Display, TEXT("MeasureBegin %s N=%d"), *ActiveRunLabel, ActualSpawnedCount);
	}
	else if (Phase == 1 && Now - MeasurementStartSeconds >= ActiveMeasurementSeconds)
	{
		Phase = 2;
		bMeasuring = false;
		TRACE_BOOKMARK(TEXT("EnemyPerf MeasureEnd Run=%s Seconds=%.3f"), *ActiveRunLabel, Now - MeasurementStartSeconds);
		UE_LOG(LogEnemyPerformance, Display, TEXT("MeasureEnd %s ActualGameSeconds=%.3f. Stop captures; reload the map for the next run."),
			*ActiveRunLabel, Now - MeasurementStartSeconds);
	}

	UpdateTarget(Elapsed);
	RecordPopulation();
}

void AEnemyPerformanceHarness::UpdateTarget(double ElapsedSeconds)
{
	if (!IsValid(Target) || ActiveScenario == EEnemyPerfScenario::Attack || WorldRoute.Num() < 2 || RouteDuration <= UE_SMALL_NUMBER)
	{
		return;
	}

	double Remaining = FMath::Fmod(ElapsedSeconds, RouteDuration);

	for (int32 Index = 0; Index < WorldRoute.Num(); ++Index)
	{
		if (Remaining < ActivePauseSeconds)
		{
			Target->SetActorLocation(WorldRoute[Index], false);
			return;
		}

		Remaining -= ActivePauseSeconds;
		const double Duration = SegmentDurations[Index];

		if (Duration > UE_SMALL_NUMBER && Remaining < Duration)
		{
			const FVector Next = WorldRoute[(Index + 1) % WorldRoute.Num()];
			Target->SetActorLocation(FMath::Lerp(WorldRoute[Index], Next, Remaining / Duration), false);
			Target->SetActorRotation((Next - WorldRoute[Index]).Rotation());
			return;
		}

		Remaining -= Duration;
	}

	Target->SetActorLocation(WorldRoute[0], false);
}

void AEnemyPerformanceHarness::RecordPopulation()
{
	TRACE_CPUPROFILER_EVENT_SCOPE(PerfHarness_Population);
	int32 Counts[5] = {};
	int32 Missing = 0;

	for (const ARAEnemyBase* Enemy : SpawnedEnemies)
	{
		if (IsValid(Enemy) && static_cast<uint8>(Enemy->GetEnemyAIState()) < UE_ARRAY_COUNT(Counts))
		{
			++Counts[static_cast<uint8>(Enemy->GetEnemyAIState())];
		}
		else
		{
			++Missing;
		}
	}

	const int32 Alive = Counts[0] + Counts[1] + Counts[2] + Counts[3];
	int32 Matching = Alive;

	switch (ActiveScenario)
	{
	case EEnemyPerfScenario::Idle: Matching = Counts[0]; break;
	case EEnemyPerfScenario::Wander: Matching = Counts[1]; break;
	case EEnemyPerfScenario::Chase: Matching = Counts[2]; break;
	case EEnemyPerfScenario::Attack: Matching = Counts[3]; break;
	default: break;
	}

	CSV_CUSTOM_STAT(EnemyPerf, Phase, Phase, ECsvCustomStatOp::Set);
	CSV_CUSTOM_STAT(EnemyPerf, Scenario, static_cast<int32>(ActiveScenario), ECsvCustomStatOp::Set);
	CSV_CUSTOM_STAT(EnemyPerf, Spawned, ActualSpawnedCount, ECsvCustomStatOp::Set);
	CSV_CUSTOM_STAT(EnemyPerf, Alive, Alive, ECsvCustomStatOp::Set);
	CSV_CUSTOM_STAT(EnemyPerf, Idle, Counts[0], ECsvCustomStatOp::Set);
	CSV_CUSTOM_STAT(EnemyPerf, Patrol, Counts[1], ECsvCustomStatOp::Set);
	CSV_CUSTOM_STAT(EnemyPerf, Chase, Counts[2], ECsvCustomStatOp::Set);
	CSV_CUSTOM_STAT(EnemyPerf, Attack, Counts[3], ECsvCustomStatOp::Set);
	CSV_CUSTOM_STAT(EnemyPerf, Dead, Counts[4], ECsvCustomStatOp::Set);
	CSV_CUSTOM_STAT(EnemyPerf, Missing, Missing, ECsvCustomStatOp::Set);
	CSV_CUSTOM_STAT(EnemyPerf, MatchingState, Matching, ECsvCustomStatOp::Set);
	CSV_CUSTOM_STAT(EnemyPerf, TargetPresent, IsValid(Target) ? 1 : 0, ECsvCustomStatOp::Set);
}

void AEnemyPerformanceHarness::FailTest(const FString& Reason)
{
	UE_LOG(LogEnemyPerformance, Error, TEXT("Run rejected: %s"), *Reason);
	TRACE_BOOKMARK(TEXT("EnemyPerf Rejected: %s"), *Reason);
	StopTest();
}

void AEnemyPerformanceHarness::StopTest()
{
	if (!GetWorld() || !GetWorld()->IsGameWorld())
	{
		return;
	}

	GetWorldTimerManager().ClearAllTimersForObject(this);

	if (bMeasuring)
	{
		TRACE_BOOKMARK(TEXT("EnemyPerf MeasureAborted Run=%s"), *ActiveRunLabel);
	}

	bMeasuring = false;
	Phase = -1;
	SetActorTickEnabled(false);

	for (ARAEnemyBase* Enemy : SpawnedEnemies)
	{
		if (IsValid(Enemy))
		{
			Enemy->Destroy();
		}
	}

	for (AController* Controller : SpawnedControllers)
	{
		if (IsValid(Controller))
		{
			Controller->Destroy();
		}
	}

	if (IsValid(Target))
	{
		Target->Destroy();
	}

	SpawnedEnemies.Reset();
	SpawnedControllers.Reset();
	Target = nullptr;
	ActualSpawnedCount = 0;
}

void AEnemyPerformanceHarness::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	GetWorldTimerManager().ClearAllTimersForObject(this);
	StopTest();
	Super::EndPlay(EndPlayReason);
}
