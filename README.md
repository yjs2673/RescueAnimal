# RescueAnimal

> Unreal Engine 5.5 기반 3인칭 액션 어드벤처 프로젝트  
> 전투, 인벤토리, 상점, 동물 구조, 맵 진행도, 포탈 이동, 런타임 상태 복원을 C++ 중심으로 구현한 게임 클라이언트 포트폴리오입니다.

![Unreal Engine](https://img.shields.io/badge/Unreal%20Engine-5.5-0E1128?style=flat-square&logo=unrealengine)
![C++](https://img.shields.io/badge/C++-Gameplay%20Programming-00599C?style=flat-square&logo=cplusplus)
![UMG](https://img.shields.io/badge/UI-UMG-blue?style=flat-square)
![Enhanced Input](https://img.shields.io/badge/Input-Enhanced%20Input-purple?style=flat-square)

---

## 프로젝트 소개

`RescueAnimal`는 필드에 배치된 적을 처치하고, 갇힌 동물을 구조하며, 여러 맵을 탐험하는 3인칭 액션 게임입니다.

단순히 기능을 붙이는 것보다, 실제 게임 클라이언트 개발에서 자주 마주치는 문제를 직접 구조화하는 데 집중했습니다.

- 전투, 스탯, 버프, 인벤토리, 퀵슬롯을 컴포넌트 단위로 분리
- 데이터 테이블 기반 아이템/무기/동물 정보 관리
- UMG 위젯과 C++ 컨트롤러를 연결한 HUD 및 모달 UI 흐름 구현
- 맵 이동 후에도 플레이어 상태와 월드 상태가 이어지는 런타임 상태 시스템 구현
- 필드 클리어, 게임 오버, 엔딩 진입 등 게임 진행 흐름 구현

---

## 기술 스택

| 구분 | 내용 |
| --- | --- |
| Engine | Unreal Engine 5.5 |
| Language | C++ |
| Input | Enhanced Input |
| UI | UMG, UserWidget |
| Data | DataTable, FTableRowBase |
| AI/Movement | AIController, NavigationSystem |
| VFX/SFX | Niagara, SoundBase |
| Architecture | Actor, ActorComponent, GameInstance, PlayerController |

---

## 주요 구현

### 1. 플레이어 전투 시스템

플레이어는 무기 타입에 따라 다른 전투 방식을 사용합니다.

- 비무장 콤보 공격
- 검 공격
- 활 조준, 차징, 투사체 발사
- 회피 상태 중 피격 무시
- 피격, 사망, 레벨업, 버프 상태 처리
- 무기 장착/해제 및 현재 무기 UI 갱신

관련 클래스

- `ARACharacter`
- `AWeaponBase`
- `AArrowProjectile`
- `UPlayerStatComponent`

---

### 2. 스탯, 경험치, 버프 시스템

`UPlayerStatComponent`에서 플레이어의 전투 수치와 성장 데이터를 관리합니다.

- HP, 방어력, 이동속도, 점프력 관리
- 경험치 획득 및 레벨업
- 레벨 보너스와 버프 배율 분리
- 시간 제한 버프 적용 및 만료 처리
- 버프 변경 시 UI 갱신 이벤트 브로드캐스트

구현 포인트

- 최종 스탯 계산 로직을 컴포넌트 내부로 모아 캐릭터 코드의 책임을 줄였습니다.
- 버프 아이템은 `FItemData`의 설정값을 기반으로 처리되도록 구성했습니다.

---

### 3. 인벤토리, 퀵슬롯, 상점 시스템

아이템은 `ItemID`를 중심으로 관리하며, 실제 표시 정보는 데이터 테이블에서 조회합니다.

- 아이템 추가/제거
- 최대 슬롯 수 및 최대 스택 수 검사
- 퀵슬롯 등록 및 사용
- 인벤토리 드래그 앤 드롭
- 상점 구매 확인, 결과 UI, 재화 아이템 처리
- 아이템 툴팁, 아이콘, 수량 표시

관련 클래스

- `UInventoryComponent`
- `UQuickSlotComponent`
- `UInventoryWidget`
- `UInventorySlotWidget`
- `UQuickSlotWidget`
- `UShopWidget`
- `UShopItemSlotWidget`

---

### 4. 데이터 테이블 기반 게임 데이터

아이템, 무기, 동물 데이터를 구조체로 정의하고 DataTable에서 조회하도록 설계했습니다.

- `FItemData`
- `FWeaponData`
- `FAnimalData`
- `FShopItemData`
- `FDropItemData`

구현 포인트

- UI는 `ItemID`만 들고 있어도 `URAGameInstance::GetItemDataByID()`를 통해 이름, 설명, 아이콘, 타입을 표시할 수 있습니다.
- 아이템 로직과 UI 표시 데이터를 분리하여 확장성과 유지보수성을 높였습니다.

---

### 5. 동물 구조 및 컬렉션 시스템

필드의 동물은 일반 상태, 갇힘 상태, 구조 완료 상태를 가집니다.

- 동물 데이터 테이블 기반 초기화
- 갇힌 동물 구조
- 구조 도구 장착 여부 검사
- 적 캠프 클리어 여부에 따른 구조 가능 조건
- 구조 완료 동물 컬렉션 해금
- 컬렉션 UI 페이지 표시

관련 클래스

- `AAnimalBase`
- `AEnemyCampActor`
- `UAnimalCollectionWidget`
- `URAGameInstance`

구현 포인트

- 동물의 구조 상태는 `AAnimalBase`가 직접 관리하고, 해금 여부는 `GameInstance`에 저장했습니다.
- 캠프 소속 동물은 캠프 클리어 후 구조 가능하도록 게임 규칙을 분리했습니다.

---

### 6. 월드 상태 및 맵 진행도 시스템

맵을 다시 방문했을 때 이미 처치한 적, 구조한 동물, 획득한 아이템이 복원되도록 런타임 상태 시스템을 구현했습니다.

- 맵별 처치한 적 ID 저장
- 맵별 구조한 동물 ID 저장
- 맵별 획득한 아이템 ID 저장
- 런타임에 생성된 드롭 아이템 복원
- 맵 클리어 여부 저장
- 전체 필드 클리어 및 게임 클리어 판단

관련 클래스

- `URAGameInstance`
- `ARAWorldStateManager`
- `ADropItemActor`
- `ARAEnemyBase`
- `AAnimalBase`

구현 포인트

- `URAGameInstance`는 맵 이동 후에도 유지되는 런타임 저장소 역할을 합니다.
- `ARAWorldStateManager`는 현재 맵의 액터를 스캔하고 저장된 상태를 적용합니다.
- 정적 배치 액터와 런타임 생성 드롭 아이템을 구분해 복원했습니다.

---

### 7. 포탈 이동 및 게임 흐름

포탈 상호작용 시 페이드 아웃, 맵 이동, 페이드 인이 이어지도록 구현했습니다.

- 포탈 진입 사운드
- 페이드 아웃 후 `OpenLevel`
- 다음 맵 BeginPlay에서 페이드 인
- 이동 중 플레이어 입력 잠금
- 인벤토리/컬렉션/상점 등 모달 UI 정리
- 타이틀/엔딩 맵 전환

관련 클래스

- `APortalActor`
- `ARAPlayerController`
- `URAGameInstance`

---

### 8. UI/HUD 구조

플레이어 컨트롤러가 HUD와 모달 UI의 생성 및 입력 모드를 관리합니다.

- 메인 HUD
- 플레이어 상태 UI
- 현재 무기 UI
- 버프 리스트 UI
- 인벤토리 UI
- 퀵슬롯 UI
- 상점 UI
- 동물 컬렉션 UI
- 맵 진행도 UI
- 필드 클리어/게임 오버 메시지 UI

구현 포인트

- UI 생성과 입력 모드 전환을 `ARAPlayerController` 중심으로 관리했습니다.
- 여러 UI가 동시에 열릴 때 마우스 커서와 입력 모드가 어긋나지 않도록 상태를 분리했습니다.

---

## 시스템 구조

```mermaid
flowchart TD
    Player[ARACharacter]
    Stat[UPlayerStatComponent]
    Inventory[UInventoryComponent]
    QuickSlot[UQuickSlotComponent]
    Controller[ARAPlayerController]
    GI[URAGameInstance]
    WorldState[ARAWorldStateManager]
    UI[UMG Widgets]
    Data[DataTables]
    Portal[APortalActor]
    Enemy[ARAEnemyBase]
    Animal[AAnimalBase]
    Camp[AEnemyCampActor]

    Player --> Stat
    Player --> Inventory
    Player --> QuickSlot
    Player --> Controller

    Controller --> UI
    Controller --> GI

    Inventory --> Data
    QuickSlot --> Data
    UI --> Data
    GI --> Data

    Portal --> GI
    Portal --> Controller

    WorldState --> GI
    WorldState --> Enemy
    WorldState --> Animal
    WorldState --> Camp