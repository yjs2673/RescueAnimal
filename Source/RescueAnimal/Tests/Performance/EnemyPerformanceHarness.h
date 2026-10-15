#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "EnemyPerformanceHarness.generated.h"

class ARAEnemyBase;
class AEnemyPerfTarget;
class AController;

UENUM(BlueprintType)
enum class EEnemyPerfScenario : uint8
{
	Idle,
	Wander,
	Chase,
	Attack,
	Mixed
};

// 테스트 맵에 명시적으로 배치
// 맵을 감지하거나 로드하지 않고 production AI를 변경하지 않음
UCLASS()
class RESCUEANIMAL_API AEnemyPerformanceHarness : public AActor
{
	GENERATED_BODY()

public:
	AEnemyPerformanceHarness();

	// 런타임에서만 사용, PIE의 Details 버튼 포함, 월드 로드당 한 번 실행
	UFUNCTION(BlueprintCallable, CallInEditor, Category = "Performance Test")
	void StartTest();

	UFUNCTION(BlueprintCallable, CallInEditor, Category = "Performance Test")
	void StopTest();

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Performance Test")
	bool bAutoStart = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Performance Test")
	FName RunLabel = TEXT("Baseline_R01");

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Performance Test")
	EEnemyPerfScenario Scenario = EEnemyPerfScenario::Idle;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Performance Test|Spawn")
	TSubclassOf<ARAEnemyBase> EnemyClass;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Performance Test|Spawn", meta = (ClampMin = "1", ClampMax = "200"))
	int32 EnemyCount = 10;

	// 고정 열: 카운트가 증가할 때 첫 번째 N 위치 유지
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Performance Test|Spawn", meta = (ClampMin = "1"))
	int32 GridColumns = 10;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Performance Test|Spawn", meta = (ClampMin = "1.0"))
	float GridSpacing = 250.f;

	// 로컬 ground 위치는 NavMesh 투영 전에 적용, 액터 스케일 무시
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Performance Test|Spawn", meta = (MakeEditWidget = "true"))
	FVector GridOrigin = FVector(-1125.f, -1125.f, 0.f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Performance Test|Spawn")
	FVector NavigationProjectionExtent = FVector(100.f, 100.f, 300.f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Performance Test|Target")
	TSubclassOf<AEnemyPerfTarget> TargetClass;

	// 로컬 CAPSULE CENTER 위치. 마지막 지점은 첫 번째 지점에 연결
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Performance Test|Target", meta = (MakeEditWidget = "true"))
	TArray<FVector> TargetRoute;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Performance Test|Target", meta = (ClampMin = "1.0"))
	float TargetSpeed = 450.f;

	// Per-waypoint stop supports attack windows in Mixed. Also applied at the first waypoint
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Performance Test|Target", meta = (ClampMin = "0.0"))
	float TargetPauseSeconds = 0.f;

	// Chase/Attack only: 처음에만 할당, 이후 자연스러운 감지/손실 계속
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Performance Test|Target")
	bool bAssignInitialTarget = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Performance Test|Patrol", meta = (ClampMin = "1.0"))
	float PatrolRadius = 2000.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Performance Test|Timing", meta = (ClampMin = "0.0"))
	float WarmupSeconds = 10.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Performance Test|Timing", meta = (ClampMin = "0.1"))
	float MeasurementSeconds = 30.f;

	// 0 = warmup, 1 = measuring, 2 = complete, -1 = stopped/not started.
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Performance Test|Status")
	int32 Phase = -1;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Performance Test|Status")
	int32 ActualSpawnedCount = 0;

protected:
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:
	UPROPERTY(Transient)
	TArray<TObjectPtr<ARAEnemyBase>> SpawnedEnemies;

	UPROPERTY(Transient)
	TArray<TObjectPtr<AController>> SpawnedControllers;

	UPROPERTY(Transient)
	TObjectPtr<AEnemyPerfTarget> Target;

	TArray<FVector> WorldRoute;
	TArray<double> SegmentDurations;
	double RouteDuration = 0.0;
	double RunStartSeconds = 0.0;
	double MeasurementStartSeconds = 0.0;
	bool bHasStarted = false;
	bool bMeasuring = false;
	EEnemyPerfScenario ActiveScenario = EEnemyPerfScenario::Idle;
	FString ActiveRunLabel;
	float ActiveWarmupSeconds = 0.f;
	float ActiveMeasurementSeconds = 0.f;
	float ActivePauseSeconds = 0.f;

	void UpdateTarget(double ElapsedSeconds);
	void RecordPopulation();
	void FailTest(const FString& Reason);
};
