# Enemy AI 성능 연구 — PHASE 1 / PHASE 2

- 작성일: 2026-10-15. 분석 기준 커밋: `8c811f1` (`Enemy: ABP 로직 수정`).
- 엔진: 프로젝트의 `EngineAssociation` 및 로컬 설치 기준 UE 5.5.
- 현재 단계: C++ 구조 분석 및 CPU Trace Instrumentation. 성능 데이터는 아직 없음.
- 이번 변경에는 최적화, Test Harness, 테스트 맵, 판단 주기 변경, CSV 전용 계측을 포함하지 않는다.
- 아래 수치 중 시간·거리 기본값은 소스 설정값이며 실측 성능이 아니다. Blueprint/인스턴스에서 덮어쓸 수 있다.

## 1. 분석 범위와 확인 한계

Source 전체 파일 목록과 Enemy 참조·Spawn 호출을 검색하고 Enemy, Creature, Camp, WorldState, GameMode 및 공유 화살 경로를 읽었다. `.uasset`/`.umap`의 Blueprint 그래프와 인스턴스 설정은 이 분석에서 검사하지 않았다. 실제 Enemy Blueprint, AIController/Auto Possess AI, NavMesh, Animation Blueprint, AnimNotify 연결은 Editor에서 확인해야 한다.

### [Enemy AI 구조]

```text
ACharacter
└─ ARACreatureBase
   ├─ 공통 HP, 피격 몽타주, 사망, DestroyDelay
   ├─ 생성자에서 Actor Tick 비활성
   └─ ARAEnemyBase
      ├─ Actor Tick 활성: Super::Tick → UpdateHPBarVisibility
      ├─ TargetActor, CurrentAIState, AI/전투 설정 데이터 보유
      ├─ DetectionSphere / HPBarWidgetComponent
      ├─ UEnemyAIComponent: 자체 Tick 활성
      │  ├─ UpdateChase
      │  ├─ UpdateMovementStuckCheck
      │  └─ ApplySeparationFromNearbyEnemies
      ├─ UEnemyCombatComponent: 자체 Tick 활성
      │  ├─ 활 차징 중 FaceTargetActor
      │  └─ UpdateAttack → CanAttack → 조건 충족 시 PerformAttack
      ├─ UEnemyEquipmentComponent: 자체 Tick 없음
      │  └─ BeginPlay에서 장착, 장비 데이터 반영, EndPlay에서 정리
      └─ UEnemyRewardComponent: 자체 Tick 없음
         └─ 사망 이벤트에서 경험치 지급 / 드롭 생성

AEnemyCampActor
├─ Tick 없음, BeginPlay에서 영역 내 Enemy/Animal 수집
├─ RegisterEnemy / RefreshCampMembers → SetCampPatrolArea
└─ 기본 0.5초 Timer → CheckCampCleared

ARAWorldStateManager
├─ Tick 없음, 저장 상태에 따라 기존 Enemy 제거
└─ Enemy 사망 기록 및 맵 진행 상태 관리

ATestEnemy : AActor
└─ 데미지 로그용 별도 Actor, Tick 없음. 실제 Enemy FSM의 하위 클래스가 아님
```

코드 경로는 `Source/RescueAnimal/Characters/Enemies`, `Creatures`, `Camp`, `Cores/WorldState`이다. Actor Tick과 각 Component Tick은 별도 스케줄링 단위이며, 위 나열을 이들 사이의 확정 실행 순서로 해석하지 않는다.

### FSM / 탐지 / 거리 / 이동

- 실제 enum: `EEnemyAIState::{Idle, Patrol, Chase, Attack, Dead}`. **Wander, Return, Stuck Recovery라는 독립 상태는 없다.**
- 전환 경로: `ChangeAIState` → `ChangeAIStateInternal` → `CanChangeAIState` → Exit → `ARAEnemyBase::SetEnemyAIState` → Enter. 같은 상태, Dead, Attack 중 부적절한 전환은 거부한다. 공격 종료는 force 경로를 사용한다. 공개 `SetEnemyAIState`를 직접 호출하는 Blueprint 경로는 별도 확인 대상이다.
- 탐지: `BeginPlay`에서 DetectionSphere Begin/End Overlap에 Component 함수를 연결한다. BeginOverlap → `ARACharacter` 확인 → `SetTargetActor` → `RefreshAIStateFromTarget`. EndOverlap → 현재 타깃과 일치하면 `ClearTargetActor` → 이동 중단 → Patrol 또는 Idle. **탐지는 이미 이벤트 기반이다.**
- 피격도 `ARAEnemyBase::TakeDamage` → `ResolvePlayerFromDamage` → `SetTargetActor`로 타깃을 획득한다. 탐지 Sphere 콜백 Scope는 이 경로를 포함하지 않는다.
- 유효 전투 타깃은 현재 `ARACharacter`만 허용한다. 일반 Actor Dummy를 지정하는 것으로는 Chase 테스트가 성립하지 않는다.
- 근접 추적: `UpdateChase`에서 `FVector::Dist` 및 공격 시작 거리 비교 후 `MoveToActor` 또는 `StopMovement`. 해당 분기에 있는 동안 프레임마다 이동 요청을 호출한다. 이것이 매번 전체 경로 재계산을 의미하는지는 엔진 Trace로 확인해야 한다.
- 활: `UpdateBowSpacing`에서 목표 거리와 허용 오차에 따라 `MoveToActor`, 후퇴 `MoveToLocation`, 또는 정지한다.
- 순찰: `UpdateCampWander`는 Tick 경로에서 호출되지만 `CampWanderInterval`(기본 3초) 검사 후에만 `MoveToRandomCampLocation`을 실행한다. `GetRandomReachablePointInRadius`로 캠프 내 지점을 구하고 `MoveToLocation`을 요청한다. Timer가 아니라 Tick 내부 시간 검사다.
- 거리 계산은 추적·활 간격, `CanAttack`, 실제 근접 명중, HP Bar 표시, Separation 가중치, 캠프 포함 검사에 존재한다. 짧은 거리 연산마다 별도 Scope를 만들지 않는다.

### 공격 / Collision / Timer

- `UpdateAttack`는 매 Tick `CanAttack`(생존·타깃·현재 상태·거리)을 확인하고 쿨다운을 검사한다. 성공하면 Attack 상태 진입, 이동 중단, 방향 갱신, 무기별 공격을 실행한다.
- Punch/Sword: 몽타주 재생 및 `MeleeHitTimerHandle` 설정. 기본 타격 지연은 각각 0.35초. `AttackEndTimerHandle`로 공격 종료를 예약한다. Blueprint 호출 가능한 `ApplyDamageToTarget` / `TriggerMeleeHit` / `EndAttack`도 존재하므로 실제 Notify 사용은 Editor 확인 대상이다.
- 근접 명중은 거리, 회피, 중복 타격 여부를 검사한 후 VFX/SFX와 `ApplyDamage`를 호출한다. 해당 C++ 경로에는 공격 Sweep/Overlap Query가 없다. Enemy 장착 무기 Mesh의 Collision도 비활성화한다.
- Bow: `BowFireTimerHandle`로 차징 종료(기본 1.5초), `ReleaseBowChargeAtTarget` → `FireArrowAtTarget` → 공격 종료 예약(기본 0.8초).
- `AArrowProjectile`은 Player/Enemy 공유 클래스다. Actor Tick은 없지만 ProjectileMovement와 Collision이 동작하며 `OnArrowOverlap`에서 필터링·데미지·효과·파괴를 처리한다. 이번 Enemy Scope에는 발사 후 화살 이동/명중 전체 비용이 포함되지 않는다.
- Separation은 `OverlapMultiByObjectType(ECC_Pawn)` 후 살아 있는 `ARAEnemyBase`를 골라 밀어내는 입력을 합산한다. CharacterMovement RVO도 기본 사용한다. 두 경로의 비용과 효과는 별도 관찰 대상이다.

### Death / Stuck Recovery / Spawn

- Death: `ARAEnemyBase::Die` → 공격 Timer 취소 → 타깃 해제·이동 중단·Dead 전환 → 보상/드롭 → WorldState 알림 → `ARACreatureBase::Die`.
- 부모 Death는 `bIsDead` 설정, Capsule Collision/Movement 비활성화, 사망 몽타주, `SetLifeSpan`(기본 DestroyDelay 2초)을 수행한다. Actor/AI/Combat Tick을 직접 끄지는 않는다. 생존 검사로 조기 반환하는 경로와 HP Bar Tick이 파괴 전까지 남을 수 있다. 이번에는 기존 동작을 유지한다.
- Stuck: 매 Tick 이동 의도 또는 PathFollowing Moving 확인 → 속도 임계값(기본 12) 이하의 정체 시간 누적 → 기본 0.8초 및 복구 쿨다운 1초 조건 검사 → `HandleMovementStuck`.
- 복구: 이동 중단 → 타깃 주변 좌/우 후보를 `ProjectPointToNavigation` 후 `MoveToLocation` → 실패 시 `MoveToRandomCampLocation`. 좌우 선택과 순찰 목적지는 난수이므로 경로가 완전히 결정적이지 않다.
- Source 전체 Spawn 검색에서 Enemy 본체 생성 C++ 코드는 발견되지 않았다. Enemy 장비·화살·보상 아이템 생성은 존재한다. 캠프는 기존 Actor 수집/등록 시스템이다. README의 필드 배치 설명과 일치하지만 Blueprint Spawn 유무는 확정하지 않는다.
- WorldState는 저장된 처치 기록에 따라 배치 Enemy를 제거한다. 측정 시작 시 실제 살아 있는 Enemy 수와 저장 상태를 확인해야 한다. 캠프의 `GetAliveEnemyCount`/클리어 판정은 HP 대신 Actor 유효성을 기준으로 하므로 사망 몽타주 중 Actor를 포함할 수 있다.

### [매 Frame 실행되는 로직]

1. Actor: 플레이어 조회 → HP Bar용 거리 계산 → 표시 여부 설정.
2. AI Component: 이동 의도 초기화 → 상태/타깃 판단 및 추적·순찰 분기 → Stuck 검사 → Separation 검사/Query/주변 순회.
3. Combat Component: 활 차징 시 타깃 방향 회전 → 공격 가능 여부/쿨다운 검사 → 조건부 공격 시작.
4. 엔진 측: CharacterMovement, PathFollowing, RVO, SkeletalMesh/Animation, Collision/Overlap, Widget 작업. 사용자 함수 Scope 밖에서 실행되는 부분을 별도로 확인한다.

### [잠재적인 비용 후보]

| 후보 | 코드상 근거 | 측정에서 확인할 점 |
| --- | --- | --- |
| Separation Query/순회 | 이동 가능한 Enemy별 Pawn Overlap 및 결과 순회 | Enemy 수뿐 아니라 밀집도·겹친 컴포넌트 수에 따른 변화 |
| 이동 요청/Navigation | Chase/Bow 분기의 반복 MoveTo, 순찰/복구 Nav Query | 호출 횟수, 동기 비용, 엔진 PathFollowing/Navigation 비용 |
| CharacterMovement/RVO/Animation | 각 캐릭터의 엔진 Component 작업 | Game Thread와 Worker Thread를 구분해 관찰 |
| 공격 판단 및 이벤트 | 매 Tick 공격 가능 검사, Timer/Notify, 몽타주·투사체·효과 | 정상 추적과 공격 집중 구간 비교 |
| HP Bar 및 로그 | 매 Tick 표시 갱신, 전환/탐지/공격 시 기존 로그 | UI 비용 및 로그 정책을 동일하게 고정 |
| 죽은 Actor의 잔여 Tick | Tick 비활성화 없이 지연 파괴 | Dead 구간 Scope 호출과 조기 반환 비용 |

모두 **후보**이며 순위나 병목 여부, 복잡도, 개선율을 실측 없이 확정하지 않는다.

## 2. Instrumentation 변경

세 `.cpp`에 `#include "ProfilingDebugging/CpuProfilerTrace.h"`와 아래 함수 진입 Scope만 추가했다. 정적 식별자 `TRACE_CPUPROFILER_EVENT_SCOPE(Enemy_AI_Update)` 형식이며 Actor별 동적 문자열이나 매 프레임 로그를 추가하지 않았다. Header, Build.cs, Gameplay 분기, Tick 주기, Timer, 로그 정책은 변경하지 않았다.

| 파일 | 함수 → Scope | 목적 / Editor 확인 |
| --- | --- | --- |
| `Characters/Enemies/RAEnemyBase.cpp` | `Tick` → `Enemy_ActorTick`; `UpdateHPBarVisibility` → `Enemy_HPBarVisibility`; `Die` → `Enemy_Death` | Actor 부대 비용·사망 이벤트 구분. HP 표시/사망/보상 정상 여부 |
| `Characters/Enemies/Components/EnemyAIComponent.cpp` | `TickComponent` → `Enemy_AI_Update`; `UpdateChase` → `Enemy_ChaseUpdate`; `UpdateBowSpacing` → `Enemy_BowSpacing` | AI 갱신과 추적 분기. 기존 이동·활 간격 유지 여부 |
| 같은 AI 파일 | `ChangeAIStateInternal` → `Enemy_StateTransition`; BeginOverlap → `Enemy_TargetDetection`; EndOverlap → `Enemy_TargetLost` | 전환 시도와 이벤트 구분. 감지/해제 후 기존 상태 전환 여부 |
| 같은 AI 파일 | `MoveToRandomCampLocation` → `Enemy_WanderMoveRequest`; `UpdateMovementStuckCheck` → `Enemy_StuckCheck`; `HandleMovementStuck` → `Enemy_StuckRecovery`; `ApplySeparationFromNearbyEnemies` → `Enemy_Separation` | 순찰 Nav 요청·정체 검사·실제 복구·주변 Query 구분. 순찰/장애물 복구 정상 여부 |
| `Characters/Enemies/Components/EnemyCombatComponent.cpp` | `TickComponent` → `Enemy_CombatUpdate`; `CanAttack` → `Enemy_CombatCheck`; `PerformAttack` → `Enemy_AttackStart` | 공격 가능 판단과 시작 비용 구분. 공격 범위/쿨다운/몽타주 정상 여부 |
| 같은 Combat 파일 | `ReleaseBowChargeAtTarget` → `Enemy_BowRelease`; `EndAttack` → `Enemy_AttackEnd`; `ApplyDamageToTarget` → `Enemy_ApplyDamage`; `FireArrowAtTarget` → `Enemy_FireArrow` | Tick 밖 Timer/Notify 비용 포함. 근접 타격/활 차징·발사/공격 종료 정상 여부 |

총 20개 Scope를 함수 단위로 추가했다. 실행되지 않은 분기의 Scope는 Trace에 나타나지 않는 것이 정상이다. 추가 계측 비용은 존재하며 아직 측정하지 않았다. Before/After에는 동일한 계측을 유지한다. 필요 시 동일 소스에서 CPU Trace on/off 대조 실행으로 계측 영향을 별도 점검한다.

### Scope 해석 주의

- `Enemy_AI_Update`는 AI Component Tick의 동기 CPU 비용이다. Combat Tick, Actor Tick, 이벤트 콜백, 엔진의 별도 Movement/Animation/비동기 작업까지 합친 'Enemy 전체 비용'이 아니다.
- `Enemy_ChaseUpdate`에는 상태 판단, 타깃 검증, 순찰 분기, 이동 요청이 함께 있다. 기존 구현에서 이 책임이 섞여 있으므로 순수 판단 시간처럼 이름 붙이지 않았다. `Enemy_CombatCheck`는 `CanAttack`만 포함하며 쿨다운 검사는 상위 CombatUpdate에 남는다.
- `Enemy_StateTransition`은 실패/동일 상태 전환 **시도**도 포함한다. 성공한 전환 수나 탐지 지연을 직접 의미하지 않는다.
- `Enemy_TargetDetection`/`Enemy_TargetLost`는 Overlap 콜백 처리 시간이다. Physics broadphase 및 Overlap 검출 비용 전체는 포함하지 않으며 비플레이어 Overlap의 조기 반환도 포함한다.
- `Enemy_Separation`은 Query와 결과 처리 및 입력 추가를 포함한다. Query만의 시간은 아니다. `Enemy_StuckRecovery`는 실제 복구 함수 호출 비용이며 자식 WanderMoveRequest를 포함할 수 있다.
- `Enemy_ApplyDamage`는 동기 VFX/SFX 요청과 타깃의 데미지 처리까지 포함할 수 있다. `Enemy_FireArrow`는 생성 비용이며 비행·충돌 수명 전체는 아니다. `Enemy_Death`에도 보상·드롭·월드 상태 갱신이 포함된다.
- 함수 진입부터 계측하므로 Dead 등 조기 반환도 호출 수에 포함한다. 함수 실행 시간과 게임플레이 반응 지연은 서로 다른 지표다.
- Insights에서 **GameThread와 동일 측정 구간**으로 범위를 한정한다. 부모 Inclusive에 자식 Inclusive를 더하면 중복 집계된다. `Enemy_AI_Update + Enemy_ChaseUpdate + Enemy_Separation`을 AI 총 비용으로 보고하지 않는다.
- `Enemy_AI_Update`의 선택 구간 Total Inclusive / 해당 구간 Game Frame 수 = 모든 Enemy의 프레임당 평균 AI Component CPU 시간. Total Inclusive / 호출 수 = 호출당 평균이며 서로 다른 지표다. Total 단위(s/ms)를 반드시 확인해 환산한다.
- Actor/Combat/이벤트 시간을 합산할 때도 호출 트리의 중첩을 먼저 확인한다. 전체 Enemy 비용은 엔진 경로까지 별도 분석해야 하며 `Enemy_*` 전체 합계로 대체하지 않는다.

## 3. 개발자가 실행할 검증 및 캡처

### 빌드와 기능 확인

1. 변경된 코드를 빌드한 바이너리인지 확인한다. Game Development 빌드와 Editor 모듈은 별개다. PIE/Standalone Editor 실행에는 Editor 모듈 재빌드 또는 해당 `.cpp` 변경의 Live Coding 적용이 필요하다.
2. 실제 사용하는 Enemy Blueprint/무기별로 탐지 → Chase → Attack, 타깃 해제 → Patrol/Idle을 확인한다. 현 구현에 없는 Return 상태 전환을 기대하지 않는다.
3. 순찰 영역 내 이동, 좁은 장애물/Enemy 밀집 시 Stuck Recovery, 사망 몽타주·보상·파괴까지 확인한다. Death 후 남아 있는 Tick은 현 Baseline 동작으로 기록한다.
4. CPU Trace에서 `Enemy_AI_Update`, `Enemy_CombatUpdate`, `Enemy_HPBarVisibility` 및 발생시킨 이벤트 Scope가 보이는지 확인한다. 화면 동작과 Trace 확인은 아직 개발자가 수행해야 한다.

### 캡처 절차

UE 5.5 설치 소스의 `CpuProfilerTrace.h` 및 `TraceAuxiliary.cpp`에서 Scope와 콘솔 명령을 확인했다. 채널/파일 캡처는 [Epic Unreal Insights Reference](https://dev.epicgames.com/documentation/en-us/unreal-engine/unreal-insights-reference-in-unreal-engine-5), Bookmark와 세션 분석은 [Epic Trace Quick Start](https://dev.epicgames.com/documentation/en-us/unreal-engine/trace-quick-start-guide-in-unreal-engine)를 참고한다. 웹 문서는 최신 버전으로 표시될 수 있으므로 로컬 5.5 소스와 Editor 명령 응답을 우선한다.

권장: 고정한 맵·시나리오를 Standalone 또는 Development Build에서 실행한다. 실행 인자로 `-trace=cpu,frame,bookmark -tracefile`을 사용하거나, 콘솔에서 다음과 같이 캡처한다. 파일명은 매 실행 다르게 지정한다.

```text
Trace.Status
Trace.File Enemy_Baseline_N050_R01.utrace cpu,frame,bookmark
stat unit
```

이미 캡처 중이면 먼저 `Trace.Stop`으로 기존 캡처를 끝낸 뒤 새 파일을 시작한다. 10초 Warm-up 후:

```text
Trace.Bookmark MeasureBegin_N050_R01
csvprofile start
```

30초 측정 후:

```text
Trace.Bookmark MeasureEnd_N050_R01
csvprofile stop
Trace.Stop
```

수동 입력 사이에는 시간 차이가 있다. 실제 Bookmark 타임스탬프와 CSV 캡처 구간을 확인하고, 이를 정확히 동기화된 자동 30초 테스트라고 보고하지 않는다. 시작·종료 콘솔 입력 경계 프레임 처리 규칙도 기록한다. 향후 PHASE 3에서 필요하면 측정 경계 자동화를 구현한다.

- Trace 파일은 콘솔이 출력한 실제 저장 경로를 기록한다. 기본 프로젝트 `Saved/Profiling` 및 CSV `Saved/Profiling/CSV`를 확인한다.
- `.utrace`를 Unreal Insights에서 열고 Timing Insights → GameThread → 측정 Bookmark 사이 구간 선택 → Timers에서 `Enemy_` 검색. Count, Total Inclusive, Total Exclusive와 호출 트리를 저장한다.
- 이번 CPU Scope는 CSV 열을 자동으로 생성하지 않는다. **AI 함수 시간은 Insights**, 프레임/스레드 통계는 해당 캡처에 실제 존재하는 CSV 열과 Insights에서 얻는다. CSV에 `Enemy_AI_Update`가 없다고 계측 실패로 판단하지 않는다.
- `stat unit`은 실행 중 확인용이며 P95/30초 평균의 원자료로 대체하지 않는다. 원본 프레임별 Frame Time에서 산술 평균과 P95를 계산하고, Game Thread 시간은 별도 지표로 유지한다. P95는 각 실행의 프레임 시간 정렬 후 nearest-rank(`ceil(0.95 * N)`) 등 선택한 방식을 Before/After에 동일 적용한다.
- 평균 함수 호출 시간의 P95는 P95 Frame Time이 아니다. 3회 반복 결과는 실행별 결과와 변동폭을 함께 보존한다.

### 고정·기록할 조건

- 연구 조건 초안: Enemy 10/25/50/100/150, 선택적으로 200; Warm-up 10초, Measurement 30초, 각 3회. 최종 조건은 개발자가 결정한다.
- 커밋/계측 버전, 엔진 버전, 빌드 종류, CPU/GPU/RAM, 실행 방식, 맵, Enemy BP/무기·애니메이션, 실제 생존 수, 배치·밀집도, 타깃 경로/속도, 카메라 위치, 해상도·Scalability, VSync/FPS 제한, 로그 정책, Trace 채널을 기록한다.
- 저장된 처치 상태와 Camp 설정, Player 사망/무적 조건, NavMesh 및 AIController 설정도 기록한다. 무적 Dummy는 피해/Hit Montage 경로를 바꿀 수 있으므로 원 Gameplay와 동일한 Attack 비용이라고 가정하지 않는다.
- Idle에서도 Separation과 CharacterMovement는 실행될 수 있다. 'Idle = FSM만의 순수 비용'으로 해석하지 않는다. 난수 순찰·복구와 프레임 간격으로 실행 편차가 있으므로 3회 반복하고 이를 한계로 남긴다.
- 탐지 → Chase 지연은 이번 CPU Duration만으로 측정되지 않는다. 현재 Overlap에서 동기 상태 전환하더라도 실제 범위 진입부터의 지연과 같지 않다. 추후 동일한 진입 경로와 대상 식별/타임스탬프 방식으로 별도 측정한다.

## 4. 변경 영향 및 작업 기록

| 항목 | 이번 작업 | 담당 |
| --- | --- | --- |
| 연구 질문·일정·측정 원칙 | 사용자 요구를 기준으로 기록 | 개발자 정의 |
| 구조 분석·비용 후보 | C++ 근거와 Blueprint 미확인 범위 구분 | AI 지원 |
| Instrumentation | 3개 `.cpp`, include 3줄 + 정적 Scope 20줄 | AI 작성 보조, 개발자 최종 Review |
| Gameplay 영향 | 제어 흐름 변경 없음. Trace 활성 시 기록 오버헤드 존재 | 코드 비교 + 개발자 플레이 검증 |
| 테스트 조건·Editor 설정 | 문서에 초안/확인 목록 제공 | 개발자 최종 결정 |
| Insights 실행·실측·적합성 | 미수행 | 개발자 |
| 병목 판정·대안·최적화 | Baseline 전달 전 보류 | AI 분석 보조, 개발자 방향 결정 |
| Before/After·반응성·포트폴리오 결과 | 실측 전 공란 유지 | 개발자 검증, AI 정리 보조 |

빌드/정적 검증 결과는 아래에 별도로 기록한다. 빌드 성공은 Editor Gameplay 회귀 검증이나 성능 개선의 증거가 아니다.

### 검증 기록

- `git diff --check`: 계측 추가 후 통과.
- Scope/include 줄을 제외한 세 C++ 파일을 HEAD와 비교: 모두 일치. Gameplay 코드 변경 없음 확인.
- `Build.bat RescueAnimal Win64 Development -Project=<프로젝트>/RescueAnimal.uproject -WaitMutex -NoHotReloadFromIDE`: 성공(exit code 0). 변경한 세 `.cpp` 컴파일 및 `RescueAnimal.exe` 링크 완료.
- Editor 모듈 빌드/Live Coding 적용은 수행하지 않음. 실행 중인 Editor를 종료하거나 Gameplay 측정을 대신 수행하지 않음.
- 실제 Editor 동작 및 Insights Scope 표시: 개발자 확인 대기.
- Baseline, 병목, 개선율, 반응성 실측: 데이터 없음.
