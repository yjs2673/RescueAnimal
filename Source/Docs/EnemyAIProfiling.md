# Enemy AI 성능 연구

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
