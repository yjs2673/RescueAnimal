# Enemy Performance Test Harness

실측 성능 결과가 아닌 테스트 지원 코드 및 사용법이다.

## 변경 범위

- `RescueAnimal/Tests/Performance/EnemyPerformanceHarness.h/.cpp`: 테스트 맵에 직접 배치하는 Actor.
- `RescueAnimal/Tests/Performance/EnemyPerfTarget.h/.cpp`: 기존 타깃 조건에 맞는 `ARACharacter` 파생 Dummy.
- 기존 Enemy, AI, Combat, Camp, GameMode, 계측 Scope는 수정하지 않는다.
- 실제 Enemy Blueprint를 그대로 Spawn하고 기존 `SetCampPatrolArea`, `ClearCampPatrolArea`, `SetTargetActor` API를 사용한다. 캠프는 Enemy 생성기가 아니므로 테스트 Actor에만 생성 기능을 추가한다.
- 이 클래스는 Runtime 모듈에 포함되며, 맵에 Actor를 배치했을 때만 실행한다.

## 주요 설정

| 설정 | 의미 |
| --- | --- |
| `EnemyCount` | 10/25/50/100 각각 설정 |
| `GridColumns`, `GridSpacing`, `GridOrigin` | 고정 격자 |
| `NavigationProjectionExtent` | 지면 후보를 NavMesh로 투영할 검색 범위 |
| `TargetClass` | 기본 `EnemyPerfTarget` |
| `TargetRoute` | Harness 기준 로컬 **Dummy Capsule 중심** 좌표 배열 |
| `bAssignInitialTarget` | Chase/Attack에서 시작 시 한 번 Dummy를 타깃으로 지정|
| `PatrolRadius` | Wander/Mixed에서 Harness 위치 중심의 순찰 반경 |

Spawn 후보별로 NavMesh 투영 후 `DontSpawnIfColliding`을 사용한다. 실패한 Enemy를 다른 위치로 임의 이동하거나 적은 수로 계속하지 않고 **전체 실행을 중단**한다. Output Log의 `LogEnemyPerformance`에서 실패 원인을 확인한다. AIController가 없으면 해당 Enemy의 `SpawnDefaultController`를 한 번 호출하고, 그래도 유효한 AIController가 없으면 중단한다.

시작 전에 실제 Spawn 좌표, 경로, Enemy BP, 수량 및 주요 설정을 로그에 남긴다. 추가로 BP 설정·테스트 맵·카메라·화질·빌드·로그/Trace 설정을 개발자가 기록한다. 실행 중 Details 변경은 새 조건으로 인정하지 않는다. 다시 Play하여 초기화한다.

## 시나리오

| Scenario | Harness 설정 동작 | 확인 조건 |
| --- | --- | --- |
| Idle | Dummy 없음, 순찰 영역 해제 | 측정 구간 Idle 수=전체. Separation/RVO/Movement Tick은 기존 그대로 |
| Wander | Dummy 없음, 순찰 영역 지정 | Patrol 상태와 실제 이동. Patrol 수만으로 이동 성공은 보장되지 않음 |
| Chase | Dummy 경로 반복, 선택적으로 초기 타깃 한 번 설정 | Chase 수·실제 이동 확인. 경로/속도/밀집도에 따라 Attack 또는 타깃 소실 가능 |
| Attack | Dummy를 TargetRoute 첫 점에 고정, 선택적으로 초기 타깃 설정 | 공격 가능한 배치·Montage. 큰 격자 전체가 동시에 공격 범위에 들어간다고 보장하지 않음 |

FSM을 강제로 고정하거나 Tick/공격을 끄지 않는다.
Chase/Attack의 초기 타깃 지정은 **범위 탐지를 우회하는 테스트 조건**이며, 이후에는 기존 Overlap Begin/End에 따른 타깃 획득/해제가 적용된다.
모든 Enemy를 계속 강제 재타깃하지 않는다.

## CSV / Insights에서 볼 항목

CSV의 `EnemyPerf/Phase`에서 **값 1인 행**이 측정 구간이다. 0=준비, 2=종료, -1=중단/미시작.
CPU Trace Scope가 CSV에 자동으로 생긴 것은 아니며, 아래 열은 Harness에서 별도로 추가한 Custom Stat이다.

| 열 | 해석 |
| --- | --- |
| `EnemyPerf/Scenario` | Idle=0, Wander=1, Chase=2, Attack=3 |
| `EnemyPerf/Spawned` | 시작 시 생성 성공 수 |
| `EnemyPerf/Alive` | 현재 Idle+Patrol+Chase+Attack 수 |
| `EnemyPerf/Idle`, `Patrol`, `Chase`, `Attack`, `Dead` | 현재 상태별 수 |
| `EnemyPerf/Missing` | 이미 파괴/무효화된 Enemy 수 |
| `EnemyPerf/MatchingState` | 지정 Scenario와 일치하는 상태 수. Mixed에서는 Alive와 동일 |
| `EnemyPerf/TargetPresent` | 유효한 Dummy 존재 여부. Idle/Wander는 0이 정상 |

`PerfHarness_Update`에는 매 프레임 상태 집계(O(N))와 Dummy 이동 및 Overlap 콜백 비용이 포함된다. 상태 집계와 CSV 기록 비용은 자식 `PerfHarness_Population`으로 별도 확인할 수 있다. 이 자체도 GT에 기여한다.
`Enemy_*` 이벤트가 Dummy 이동 도중 자식 Scope로 나타날 수 있으므로 PerfHarness 시간을 기계적으로 통째로 빼서 게임 비용을 계산하지 않는다.
Before/After에는 같은 Harness와 계측 설정을 유지한다.
