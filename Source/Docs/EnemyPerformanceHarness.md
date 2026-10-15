# PHASE 3 — Enemy Performance Test Harness

2026-10-15. 실측 성능 결과가 아닌 테스트 지원 코드 및 사용법이다.

## 변경 범위

- `RescueAnimal/Tests/Performance/EnemyPerformanceHarness.h/.cpp`: 테스트 맵에 직접 배치하는 Actor.
- `RescueAnimal/Tests/Performance/EnemyPerfTarget.h/.cpp`: 기존 타깃 조건에 맞는 `ARACharacter` 파생 Dummy.
- 기존 Enemy, AI, Combat, Camp, GameMode, 계측 Scope는 수정하지 않는다. 맵 이름/경로 검사도 없다.
- 실제 Enemy Blueprint를 그대로 Spawn하고 기존 `SetCampPatrolArea`, `ClearCampPatrolArea`, `SetTargetActor` API를 사용한다. 캠프는 Enemy 생성기가 아니므로 테스트 Actor에만 생성 기능을 추가했다.
- 이 클래스는 Runtime 모듈에 포함된다. 자동 전역 등록은 없으며 **맵에 Actor를 배치했을 때만** 실행한다. Shipping 제외/맵 식별 시스템은 추가하지 않았다.

## Editor 최초 설정

새 UCLASS를 추가했으므로 Editor를 닫고 `RescueAnimalEditor / Win64 / Development`를 빌드한 후 다시 여는 방식을 권장한다. Game Development 빌드만으로 열린 Editor의 클래스 목록은 갱신되지 않는다.

1. 별도 Test Map에 충분히 넓은 평면 바닥과 `NavMeshBoundsVolume`을 배치하고 Navigation을 빌드한다. `P`로 격자 전체가 NavMesh 위인지 확인한다.
2. 테스트 전용 `GameModeBase` Blueprint를 만들고 **Default Pawn Class = SpectatorPawn**, **Player Controller Class = PlayerController**로 지정한다. Test Map의 World Settings → GameMode Override에만 적용한다. 기존 게임의 GameMode/PlayerController를 사용하면 Player Spawn/UI 흐름이 섞일 수 있다.
3. 테스트 맵에는 기존 Enemy, 플레이어 `ARACharacter`, Camp, WorldStateManager, 다른 Spawn Actor를 배치하지 않는다. Harness 한 개만 배치한다. 시작 시 기존 Enemy/RACharacter 및 중복 Harness는 검사하여 오류로 중단한다. Camp/WorldStateManager는 자동 제거하지 않으므로 직접 제외한다.
4. C++ Classes → RescueAnimal → Tests → Performance에서 `EnemyPerformanceHarness`를 배치한다. Actor 위치는 **지면 높이**, Scale=(1,1,1), Pitch/Roll=0으로 둔다. 배치는 Actor의 위치와 Yaw를 사용하며 Scale/Pitch/Roll은 적용하지 않는다.
5. `EnemyClass`에 **실제 사용하는 Enemy BP**를 지정한다. Mesh/AnimBP/무기·몽타주·AIController가 갖춰져 있어야 한다. 새 간소화 Enemy로 바꾸지 않는다.
6. 먼저 `Scenario=Idle`, `EnemyCount=10`, `GridColumns=10`, `GridSpacing=250`, Warmup=10, Measurement=30으로 동작 확인한다. 간격은 Enemy Capsule 크기보다 충분히 커야 한다.
7. `bAutoStart=true`이면 BeginPlay 다음 Tick에 시작한다. `false`이면 Play 후 런타임 Harness 인스턴스를 선택하여 Details의 **Start Test** 버튼을 누르거나 Blueprint에서 `StartTest`를 호출한다. Play 전 버튼은 Enemy를 생성하지 않는다.

Spectator는 AI 타깃이 아니다. 카메라는 고정하고 불필요하게 움직이지 않는다. 이 구성에서는 `GetPlayerCharacter`가 실제 RACharacter를 반환하지 않으므로 **Enemy HP Bar 표시 갱신이 조기 반환**한다. 따라서 이 테스트 결과를 실제 게임의 UI 포함 전체 비용과 동일시하지 않는다. 최종 실제 플레이어 회귀 검증은 기존 Gameplay 맵에서 별도로 수행한다.

## 주요 설정

| 설정 | 의미 |
| --- | --- |
| `EnemyCount` | 1~200. 연구 조건 10/25/50/100/150, 선택 200을 각각 설정 |
| `GridColumns`, `GridSpacing`, `GridOrigin` | 고정 격자. 개수가 늘어도 앞 N개 위치는 유지한다. 모든 수량에 동일 설정 사용 |
| `NavigationProjectionExtent` | 지면 후보를 NavMesh로 투영할 검색 범위. 실제 Spawn 좌표는 NavMesh 위 + Enemy Capsule 반높이 + 2cm |
| `TargetClass` | 기본 `EnemyPerfTarget`. 작은 구체로 보인다. 필요 시 이 클래스의 BP 사용, Auto Possess 비활성 유지 |
| `TargetRoute` | Harness 기준 로컬 **Dummy Capsule 중심** 좌표 배열. 마지막 점에서 첫 점으로 연결. Z는 바닥 높이가 아니다 |
| `TargetSpeed` | cm/s. 기본 450은 시작 설정일 뿐, 모든 Enemy가 순수 Chase를 유지하는 검증된 값은 아님 |
| `TargetPauseSeconds` | 각 점에서 머무는 시간. 첫 점에도 적용. Mixed에서 공격 구간을 만들 때 사용 |
| `bAssignInitialTarget` | Chase/Attack에서 시작 시 한 번 Dummy를 타깃으로 지정. 자연 탐지 지연 테스트에는 false |
| `PatrolRadius` | Wander/Mixed에서 Harness 위치 중심의 순찰 반경 |
| `RunLabel` | 예: `Baseline_Chase_N050_R01`. 매 실행 고유하게 지정 |

Spawn 후보별로 NavMesh 투영 후 `DontSpawnIfColliding`을 사용한다. 실패한 Enemy를 다른 위치로 임의 이동하거나 적은 수로 계속하지 않고 **전체 실행을 중단**한다. Output Log의 `LogEnemyPerformance`에서 실패 원인을 확인한다. AIController가 없으면 해당 Enemy의 `SpawnDefaultController`를 한 번 호출하고, 그래도 유효한 AIController가 없으면 중단한다.

시작 전에 실제 Spawn 좌표, 경로, Enemy BP, 수량 및 주요 설정을 로그에 남긴다. 추가로 BP 설정·테스트 맵·카메라·화질·빌드·로그/Trace 설정을 개발자가 기록한다. 실행 중 Details 변경은 새 조건으로 인정하지 않는다. 다시 Play하여 초기화한다.

## 시나리오

| Scenario | Harness 설정 동작 | 개발자가 확인할 조건 |
| --- | --- | --- |
| Idle | Dummy 없음, 순찰 영역 해제 | 측정 구간 Idle 수=전체. Separation/RVO/Movement Tick은 기존 그대로 |
| Wander | Dummy 없음, 순찰 영역 지정 | Patrol 상태와 실제 이동. Patrol 수만으로 이동 성공은 보장되지 않음 |
| Chase | Dummy 경로 반복, 선택적으로 초기 타깃 한 번 설정 | Chase 수·실제 이동 확인. 경로/속도/밀집도에 따라 Attack 또는 타깃 소실 가능 |
| Attack | Dummy를 TargetRoute 첫 점에 고정, 선택적으로 초기 타깃 설정 | 공격 가능한 배치·Montage. 큰 격자 전체가 동시에 공격 범위에 들어간다고 보장하지 않음 |
| Mixed | 순찰 영역 + Dummy 경로 반복. 강제 타깃 설정 없음 | 경로가 탐지 영역 안팎을 지나며 Patrol→Chase→Attack→Patrol/Idle 발생하는지 확인 |

FSM을 강제로 고정하거나 Tick/공격을 끄지 않는다. Chase/Attack의 초기 타깃 지정은 **범위 탐지를 우회하는 테스트 조건**이며, 이후에는 기존 Overlap Begin/End에 따른 타깃 획득/해제가 적용된다. 모든 Enemy를 계속 강제 재타깃하지 않는다. 초기 범위 밖 타깃도 한 번 지정할 수 있으므로 이 옵션과 경로를 반드시 기록한다.

Mixed 경로는 첫 점을 탐지 범위 밖에 두고, 중간 점은 Enemy 가까이, 다음 점은 다시 밖으로 배치한다. `TargetPauseSeconds`로 각 점의 체류 시간을 조절한다. 현재 FSM에 독립 Return 상태는 없으며, 타깃 소실 후 Patrol/Idle 복귀가 실제 검증 대상이다. 모든 상태를 자동으로 보장하는 스크립트는 아니다.

**Dummy의 의도적인 차이:**

- 상속한 CharacterMovement를 비활성화하고, Harness가 경과 시간으로 구간 위치를 직접 계산하여 이동한다. 속도는 프레임마다 더하는 방식 대신 절대 경과 시간에 따라 결정된다.
- 이동은 Sweep 없이 수행한다. 장애물 회피·지형 추종·물리 밀림은 없다. 경로 전체를 같은 높이의 열린 공간에 두고, 벽/바닥 관통과 큰 프레임 지연 시 탐지 영역 건너뜀을 점검한다.
- Pawn 충돌은 Overlap이며 무적이다. Enemy의 공격·몽타주·타이머·VFX/SFX 요청·화살은 실행하지만 **플레이어 HP/피격 몽타주/사망 처리 비용은 빠진다**. Attack 결과를 전체 전투 비용으로 보고하지 않는다.
- 일반 플레이어 컴포넌트와 부모 BeginPlay는 유지된다. 새 PIE/프로세스로 시작하여 이전 플레이어 런타임 상태가 유입되지 않게 한다. Dummy 비용 및 작은 구체 렌더링 비용은 해당 시나리오에서 공통이다.
- 순찰/복구 난수와 엔진 스케줄링은 변경하지 않았다. Spawn 배치와 목표 경로를 고정해도 모든 Enemy 궤적이 완전히 동일하지는 않다. 동일 조건 3회 반복한다.

## 측정 실행 순서

Harness는 **CSV/Trace 세션을 임의로 시작·종료하지 않는다.** 이미 사용한 콘솔 명령을 그대로 이용하며, 그 안에 측정 구간 표시를 추가한다.

1. 가장 확실한 방식은 `bAutoStart=false`로 Play → 캡처 시작 → Start Test이다. 또는 프로세스 시작부터 캡처하고 `bAutoStart=true`를 사용한다. Warm-up 도중 수동 캡처를 시작했다면 MeasureBegin 전에 시작됐는지 확인한다.
2. 콘솔에서 새 Trace와 CSV 시작:

   ```text
   Trace.File Enemy_Baseline_Chase_N050_R01.utrace cpu,frame,bookmark
   csvprofile start
   ```

   이미 Trace가 실행 중이면 기존 세션을 그대로 사용하거나 종료 후 새 파일을 시작한다.

3. `StartTest` 후 성공한 Spawn이 끝나면 Warm-up(기본 10초). 그 뒤 Measurement(기본 30초). 로그/Insights에 `EnemyPerf MeasureBegin`과 `EnemyPerf MeasureEnd`가 기록된다.
4. MeasureEnd 확인 후 `csvprofile stop`, `Trace.Stop`. `Phase=2`가 되어도 Enemy/Dummy는 계속 동작한다. 종료 순간 파괴 작업이 측정에 섞이는 것을 피하기 위한 동작이다.
5. Play 종료 또는 Stop Test로 Harness가 만든 Enemy/Controller/Dummy를 정리한다. 측정 중 Stop이면 `MeasureAborted` Bookmark를 남긴다. 해당 실행은 결과에서 제외한다.
6. **다음 조건/반복은 새 Play 또는 맵 재로드로 시작한다.** 동일 world에서 두 번째 Start는 거부한다. 난수·남아 있는 화살·월드 상태·이전 공격 타이머가 다음 조건에 섞이는 것을 줄이기 위함이다. 각 수량에 R01~R03을 수행한다.

타이밍은 게임 시간(`World::GetTimeSeconds`) 기준이며 경계는 Tick에 맞춰진다. 30초보다 한 프레임 정도 길 수 있다. 실제 MeasureEnd의 Seconds를 기록하고 고정 30.000초라고 가정하지 않는다. Pause, Time Dilation, 프레임별 수동 진행을 사용하지 않는다.

## CSV / Insights에서 볼 항목

CSV의 `EnemyPerf/Phase`에서 **값 1인 행**이 측정 구간이다. 0=Warm-up, 2=종료 후, -1=중단/미시작(중단 후 Tick은 비활성화되어 새 행을 계속 기록하지 않는다). CPU Trace Scope가 CSV에 자동으로 생긴 것은 아니며, 아래 열은 Harness에서 별도로 추가한 Custom Stat이다.

| 열 | 해석 |
| --- | --- |
| `EnemyPerf/Scenario` | Idle=0, Wander=1, Chase=2, Attack=3, Mixed=4 |
| `EnemyPerf/Spawned` | 시작 시 생성 성공 수 |
| `EnemyPerf/Alive` | 현재 Idle+Patrol+Chase+Attack 수 |
| `EnemyPerf/Idle`, `Patrol`, `Chase`, `Attack`, `Dead` | 현재 상태별 수 |
| `EnemyPerf/Missing` | 이미 파괴/무효화된 Enemy 수 |
| `EnemyPerf/MatchingState` | 지정 Scenario와 일치하는 상태 수. Mixed에서는 Alive와 동일 |
| `EnemyPerf/TargetPresent` | 유효한 Dummy 존재 여부. Idle/Wander는 0이 정상 |

Phase=1 구간에서 Alive=예정 수량, Missing=0을 먼저 확인한다. Chase는 Chase 수가 유지되는지 확인하고, 부족하다면 경로/속도/범위/배치를 조정한 후 **전체 실행을 다시 측정**한다. MatchingState는 상태 일치 지표일 뿐 경로 이동 성공이나 공격 명중 성공 판정이 아니다. Attack 쿨다운 중 다른 상태로 돌아가는 것은 기존 동작일 수 있다. Mixed는 상태 분포·전환을 직접 검토한다.

**원하는 상태인 프레임만 골라 성능 평균을 내지 않는다.** Phase 필터는 측정 구간 선택용이고, 상태 열은 시나리오가 제대로 성립했는지 진단하는 용도다. 특정 상태가 아닌 느린 프레임을 버리면 결과가 편향된다.

이후 `FrameTime` 평균/P95, `GameThreadTime` 평균을 구한다. Timing Insights에서는 동일 Run의 Begin~End Bookmark 구간을 선택하고 GameThread의 `Enemy_` Scope를 확인한다. 경계 Bookmark는 Tick 도중 발생하고 CSV는 프레임 단위이므로 완전히 같은 시작/종료 순간은 아니다. 완전한 프레임만 비교할 경우 양쪽 경계 프레임 제외 규칙을 통일하고 사용한 프레임 수를 기록한다.

`PerfHarness_Update`에는 매 프레임 상태 집계(O(N))와 Dummy 이동 및 Overlap 콜백 비용이 포함된다. 상태 집계와 CSV 기록 비용은 자식 `PerfHarness_Population`으로 별도 확인할 수 있다. 이 자체도 GT에 기여한다. `Enemy_*` 이벤트가 Dummy 이동 도중 자식 Scope로 나타날 수 있으므로 PerfHarness 시간을 기계적으로 통째로 빼서 게임 비용을 계산하지 않는다. Before/After에는 같은 Harness와 계측 설정을 유지한다.

## Editor 검증 체크리스트

- [ ] 클래스가 표시되고, Play 전 Start 버튼으로는 Actor가 생성되지 않음
- [ ] 지정 Enemy BP가 10/25/50/100/150개 생성되며 실제 수량이 CSV와 일치
- [ ] 같은 조건 재실행 시 로그의 Spawn 좌표가 일치하며 수량 증가 시 앞 N개 좌표 유지
- [ ] NavMesh 누락/좁은 배치/Controller 누락 시 오류가 표시되고 부분 측정으로 진행되지 않음
- [ ] Dummy 구체가 A→B→C→A로 반복 이동하며 장애물을 관통하는 경로가 없음
- [ ] Idle/Chase/Mixed에서 의도한 상태 분포와 이동·공격 동작이 발생
- [ ] Warm-up/Measurement/Complete가 0→1→2로 기록되고 Bookmark와 대응
- [ ] 측정 중 Stop 시 Abort 표시, 생성한 Enemy/Controller/Dummy 정리
- [ ] 일반 Gameplay 맵에서 탐지·추적·공격·타깃 해제·순찰·복구·사망이 기존처럼 동작

## 작업 역할 / 검증 상태

- AI: 기존 API 확인, Harness/Dummy 코드 작성, 계측과 사용 문서 작성, 빌드 검증.
- 개발자: Test Map·Enemy BP·NavMesh 준비, 시나리오 성립 판단과 최종 조건 결정, 직접 캡처, Gameplay 판단, 최종 코드 Review.
- Test Map/Blueprint 에셋은 이번 코드 작업에서 생성·수정하지 않았다.
- 컴파일: UE 5.5 UHT 통과 및 최종 `RescueAnimal Win64 Development` 빌드/링크 성공(exit code 0).
- 정적 확인: 신규 파일 공백 검사 통과, 기존 Characters/Camp/Creatures/Cores/Build.cs 변경 없음 확인.
- Editor 모듈 재빌드와 시나리오 실행·실제 성능/반응성 측정: 개발자 확인 대기. 컴파일 성공을 Gameplay 검증 또는 성능 결과로 간주하지 않는다.
