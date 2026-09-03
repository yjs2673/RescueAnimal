#include "RACharacter.h"

#include "PortalActor.h"
#include "ShopActor.h"
#include "LobbyNPC.h"
#include "WeaponBase.h"
#include "ArrowProjectile.h"
#include "RAEnemyBase.h"

#include "Engine/LocalPlayer.h"
#include "Camera/CameraComponent.h"

#include "Components/CapsuleComponent.h"
#include "Components/SkeletalMeshComponent.h"

#include "GameFramework/PlayerController.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "GameFramework/Controller.h"
#include "GameFramework/ProjectileMovementComponent.h"

#include "PlayerStatComponent.h"
#include "InventoryComponent.h"
#include "QuickSlotComponent.h"
#include "PlayerSkillComponent.h"
#include "PlayerMovementComponent.h"
#include "PlayerCombatComponent.h"
#include "PlayerEquipmentComponent.h"
#include "PlayerInteractionComponent.h"

#include "RAGameInstance.h"
#include "RAStructTypes.h"

#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "InputAction.h"

#include "Blueprint/UserWidget.h"

#include "CrosshairBowWidget.h"
#include "MainHUDWidget.h"
#include "RAPlayerController.h"
#include "GameProgressUIComponent.h"
#include "PlayerUIFlowComponent.h"

#include "Sound/SoundBase.h"

#include "NiagaraFunctionLibrary.h"
#include "NiagaraComponent.h"

#include "TimerManager.h"
#include "DrawDebugHelpers.h"
#include "Kismet/GameplayStatics.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "Animation/AnimInstance.h"

DEFINE_LOG_CATEGORY(LogTemplateCharacter);

/* Constructor */
ARACharacter::ARACharacter()
{
#pragma region Base Setting
	// Set size for collision capsule
	GetCapsuleComponent()->InitCapsuleSize(42.f, 96.0f);
		
	// Don't rotate when the controller rotates. Let that just affect the camera.
	bUseControllerRotationPitch = false;
	bUseControllerRotationYaw = false;
	bUseControllerRotationRoll = false;

	// Configure character movement
	GetCharacterMovement()->bOrientRotationToMovement = true; // Character moves in the direction of input...	
	GetCharacterMovement()->RotationRate = FRotator(0.0f, 500.0f, 0.0f); // ...at this rotation rate

	// Note: For faster iteration times these variables, and many more, can be tweaked in the Character Blueprint
	// instead of recompiling to adjust them
	GetCharacterMovement()->JumpZVelocity = 400.f;
	GetCharacterMovement()->AirControl = 0.25f;
	GetCharacterMovement()->MaxWalkSpeed = 400.f;
	GetCharacterMovement()->MinAnalogWalkSpeed = 20.f;
	GetCharacterMovement()->BrakingDecelerationWalking = 2000.f;
	GetCharacterMovement()->BrakingDecelerationFalling = 1500.0f;

	// Create a camera boom (pulls in towards the player if there is a collision)
	CameraBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraBoom"));
	CameraBoom->SetupAttachment(RootComponent);
	CameraBoom->TargetArmLength = 400.0f; // The camera follows at this distance behind the character	
	CameraBoom->bUsePawnControlRotation = true; // Rotate the arm based on the controller

	// Create a follow camera
	FollowCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("FollowCamera"));
	FollowCamera->SetupAttachment(CameraBoom, USpringArmComponent::SocketName); // Attach the camera to the end of the boom and let the boom adjust to match the controller orientation
	FollowCamera->bUsePawnControlRotation = false; // Camera does not rotate relative to arm

	// Note: The skeletal mesh and anim blueprint references on the Mesh component (inherited from Character) 
	// are set in the derived blueprint asset named ThirdPersonCharacter (to avoid direct content references in C++)
#pragma endregion Base Setting

	PrimaryActorTick.bCanEverTick = false;
	CurrentWeapon = nullptr; // 처음에는 들고 있는 무기가 없으니 nullptr로 초기화

	StatComponent = CreateDefaultSubobject<UPlayerStatComponent>(TEXT("StatComponent"));
	InventoryComponent = CreateDefaultSubobject<UInventoryComponent>(TEXT("InventoryComponent"));
	QuickSlotComponent = CreateDefaultSubobject<UQuickSlotComponent>(TEXT("QuickSlotComponent"));
	PlayerSkillComponent = CreateDefaultSubobject<UPlayerSkillComponent>(TEXT("PlayerSkillComponent"));
	PlayerMovementComponent = CreateDefaultSubobject<UPlayerMovementComponent>(TEXT("PlayerMovementComponent"));
	PlayerCombatComponent = CreateDefaultSubobject<UPlayerCombatComponent>(TEXT("PlayerCombatComponent"));
	PlayerEquipmentComponent = CreateDefaultSubobject<UPlayerEquipmentComponent>(TEXT("PlayerEquipmentComponent"));
	PlayerInteractionComponent = CreateDefaultSubobject<UPlayerInteractionComponent>(TEXT("PlayerInteractionComponent"));

	// 차징 화살: 미리보기용 StaticMeshComponent 생성 및 설정
	PreviewArrowMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("PreviewArrowMesh"));
	PreviewArrowMesh->SetupAttachment(GetMesh());
	PreviewArrowMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	PreviewArrowMesh->SetGenerateOverlapEvents(false);
	PreviewArrowMesh->SetSimulatePhysics(false);
	PreviewArrowMesh->SetHiddenInGame(true);

	PreviewArrowVFXComponent = CreateDefaultSubobject<UNiagaraComponent>(TEXT("PreviewArrowVFXComponent"));
	PreviewArrowVFXComponent->SetupAttachment(PreviewArrowMesh);
	PreviewArrowVFXComponent->SetAutoActivate(false);
	PreviewArrowVFXComponent->SetHiddenInGame(true);
}

#pragma region Input Binding Func
void ARACharacter::NotifyControllerChanged()
{
	Super::NotifyControllerChanged();

	// Add Input Mapping Context
	if (APlayerController* PlayerController = Cast<APlayerController>(Controller))
	{
		if (UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(PlayerController->GetLocalPlayer()))
		{
			Subsystem->AddMappingContext(DefaultMappingContext, 0);
		}
	}
}

void ARACharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	// Set up action bindings
	if (UEnhancedInputComponent* EnhancedInputComponent = Cast<UEnhancedInputComponent>(PlayerInputComponent)) {
		
		// Jumping
		EnhancedInputComponent->BindAction(JumpAction, ETriggerEvent::Started, PlayerMovementComponent, &UPlayerMovementComponent::StartJump);
		EnhancedInputComponent->BindAction(JumpAction, ETriggerEvent::Completed, this, &ACharacter::StopJumping);

		// Moving
		EnhancedInputComponent->BindAction(MoveAction, ETriggerEvent::Triggered, PlayerMovementComponent, &UPlayerMovementComponent::Move);

		// Looking
		EnhancedInputComponent->BindAction(LookAction, ETriggerEvent::Triggered, PlayerMovementComponent, &UPlayerMovementComponent::Look);

		// Attacking
		// EnhancedInputComponent->BindAction(AttackAction, ETriggerEvent::Started, this, &ARACharacter::Attack);
		EnhancedInputComponent->BindAction(AttackAction, ETriggerEvent::Started, PlayerCombatComponent, &UPlayerCombatComponent::OnAttackPressed);
		EnhancedInputComponent->BindAction(AttackAction, ETriggerEvent::Completed, PlayerCombatComponent, &UPlayerCombatComponent::OnAttackReleased);
		EnhancedInputComponent->BindAction(AttackAction, ETriggerEvent::Canceled, PlayerCombatComponent, &UPlayerCombatComponent::OnAttackReleased);
		
		// Interacting
		EnhancedInputComponent->BindAction(InteractAction, ETriggerEvent::Started, PlayerInteractionComponent, &UPlayerInteractionComponent::Interact);

		// Dodging
		EnhancedInputComponent->BindAction(DodgeAction, ETriggerEvent::Started, PlayerMovementComponent, &UPlayerMovementComponent::Dodge);

		// Skill
		if (SkillAction && PlayerSkillComponent)
			EnhancedInputComponent->BindAction(SkillAction, ETriggerEvent::Started, PlayerSkillComponent, &UPlayerSkillComponent::HandleSkillInput);

		// Quick Slots
		UE_LOG(LogTemplateCharacter, Warning, TEXT("QuickSlot binding setup. Actions: 1=%s 2=%s 3=%s 4=%s 5=%s 6=%s 7=%s"),
			QuickSlot1Action ? TEXT("Set") : TEXT("None"),
			QuickSlot2Action ? TEXT("Set") : TEXT("None"),
			QuickSlot3Action ? TEXT("Set") : TEXT("None"),
			QuickSlot4Action ? TEXT("Set") : TEXT("None"),
			QuickSlot5Action ? TEXT("Set") : TEXT("None"),
			QuickSlot6Action ? TEXT("Set") : TEXT("None"),
			QuickSlot7Action ? TEXT("Set") : TEXT("None"));
		if (QuickSlot1Action)
			EnhancedInputComponent->BindAction(QuickSlot1Action, ETriggerEvent::Started, PlayerInteractionComponent, &UPlayerInteractionComponent::UseQuickSlotItem, 0);
		if (QuickSlot2Action)
			EnhancedInputComponent->BindAction(QuickSlot2Action, ETriggerEvent::Started, PlayerInteractionComponent, &UPlayerInteractionComponent::UseQuickSlotItem, 1);
		if (QuickSlot3Action)
			EnhancedInputComponent->BindAction(QuickSlot3Action, ETriggerEvent::Started, PlayerInteractionComponent, &UPlayerInteractionComponent::UseQuickSlotItem, 2);
		if (QuickSlot4Action)
			EnhancedInputComponent->BindAction(QuickSlot4Action, ETriggerEvent::Started, PlayerInteractionComponent, &UPlayerInteractionComponent::UseQuickSlotItem, 3);
		if (QuickSlot5Action)
			EnhancedInputComponent->BindAction(QuickSlot5Action, ETriggerEvent::Started, PlayerInteractionComponent, &UPlayerInteractionComponent::UseQuickSlotItem, 4);
		if (QuickSlot6Action)
			EnhancedInputComponent->BindAction(QuickSlot6Action, ETriggerEvent::Started, PlayerInteractionComponent, &UPlayerInteractionComponent::UseQuickSlotItem, 5);
		if (QuickSlot7Action)
			EnhancedInputComponent->BindAction(QuickSlot7Action, ETriggerEvent::Started, PlayerInteractionComponent, &UPlayerInteractionComponent::UseQuickSlotItem, 6);
	}
	else
	{
		UE_LOG(LogTemplateCharacter, Error, TEXT("'%s' Failed to find an Enhanced Input component! This template is built to use the Enhanced Input system. If you intend to use the legacy system, then you will need to update this C++ file."), *GetNameSafe(this));
	}
}
#pragma endregion Input Binding Func

bool ARACharacter::IsPortalTransitionInputLocked() const
{
	if (const ARAPlayerController* RAPlayerController = Cast<ARAPlayerController>(GetController()))
	{
		const UPlayerUIFlowComponent* PlayerUIFlowComponent = RAPlayerController->GetPlayerUIFlowComponent();
		return PlayerUIFlowComponent && PlayerUIFlowComponent->IsPortalTransitionInputLocked();
	}

	return false;
}

/* Tick */
//void ARACharacter::Tick(float DeltaTime)
//{
//	Super::Tick(DeltaTime);
//}

/* Begin */
void ARACharacter::BeginPlay()
{
	Super::BeginPlay();

	ApplyMovementStats();

	if (StatComponent)
	{
		StatComponent->OnDeath.AddDynamic(this, &ARACharacter::HandleCharacterDeath);
		StatComponent->OnMovementStatsChanged.AddUniqueDynamic(this, &ARACharacter::ApplyMovementStats);
	}

#pragma region Runtime Data BeginPlay
	const URAGameInstance* RAGameInstance = GetGameInstance<URAGameInstance>();
	const bool bShouldLoadPlayerRuntimeData = RAGameInstance && RAGameInstance->HasValidPlayerRuntimeData();
#pragma endregion Runtime Data BeginPlay

	if (!bShouldLoadPlayerRuntimeData && InventoryComponent && !StarterRescueKitItemID.IsNone() && !InventoryComponent->HasItem(StarterRescueKitItemID, 1))
	{
		InventoryComponent->AddItem(StarterRescueKitItemID, 1);
	}


	if (CameraBoom)
		DefaultArmLength = CameraBoom->TargetArmLength;

	if (FollowCamera)
		DefaultFOV = FollowCamera->FieldOfView;

	if (!bShouldLoadPlayerRuntimeData && StarterWeaponClass)
	{
		AWeaponBase* SpawnedWeapon = GetWorld()->SpawnActor<AWeaponBase>(StarterWeaponClass);
		if (SpawnedWeapon && PlayerEquipmentComponent)
		{
			PlayerEquipmentComponent->EquipWeapon(SpawnedWeapon);
		}
	}

	if (ARAPlayerController* RAPlayerController = Cast<ARAPlayerController>(GetController()))
	{
		UPlayerUIFlowComponent* PlayerUIFlowComponent = RAPlayerController->GetPlayerUIFlowComponent();
		if (UMainHUDWidget* HUDWidget = PlayerUIFlowComponent ? PlayerUIFlowComponent->GetMainHUDWidget() : nullptr)
		{
			CrosshairWidgetInstance = HUDWidget->GetCrosshairBowWidget();
			if (CrosshairWidgetInstance)
			{
				CrosshairWidgetInstance->SetCrosshairVisible(false);
			}
		}
	}

	if (!CrosshairWidgetInstance && CrosshairWidgetClass)
	{
		APlayerController* PC = Cast<APlayerController>(GetController());
		if (PC)
		{
			CrosshairWidgetInstance = CreateWidget<UCrosshairBowWidget>(PC, CrosshairWidgetClass);
			if (CrosshairWidgetInstance)
			{
				CrosshairWidgetInstance->AddToViewport();
				CrosshairWidgetInstance->SetCrosshairVisible(false);
			}
		}
	}

#pragma region Runtime Data BeginPlay
	if (bShouldLoadPlayerRuntimeData)
	{
		LoadRuntimeDataFromGameInstance();
	}
#pragma endregion Runtime Data BeginPlay
}

void ARACharacter::HandleCharacterDeath() // 플레이어 사망 처리: 공격 상태 초기화, 이동 불가, 입력 비활성화
{
#pragma region Game Progress
	if (bGameOverHandled)
	{
		return;
	}

	bGameOverHandled = true;
	UE_LOG(LogTemplateCharacter, Warning, TEXT("[GameProgress] GAME OVER: Player HP reached zero."));

	if (ARAPlayerController* RAPlayerController = Cast<ARAPlayerController>(GetController()))
	{
		if (UGameProgressUIComponent* GameProgressUIComponent = RAPlayerController->GetGameProgressUIComponent())
		{
			GameProgressUIComponent->ShowGameOverMessage();
		}
	}
	else
	{
		UE_LOG(LogTemplateCharacter, Warning, TEXT("[GameProgress] Game over message skipped: RAPlayerController is null."));
	}
#pragma endregion Game Progress

	bIsAttacking = false;
	bIsPunching = false;
	bIsBowCharging = false;
	bIsBowAiming = false;
	bIsDodging = false;
	bComboInputBuffered = false;
	bCanAcceptComboInput = false;

	UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance();
	AnimInstance->Montage_Play(DeathMontage);

	ResetBowCrosshairUI();
	HidePreviewArrow();

	GetCharacterMovement()->DisableMovement();

	if (APlayerController* PC = Cast<APlayerController>(GetController()))
	{
		DisableInput(PC);
	}

	UE_LOG(LogTemplateCharacter, Warning, TEXT("Character Died"));
}

#pragma region Base Action Func
void ARACharacter::ApplyMovementStats()
{
	if (PlayerMovementComponent)
	{
		PlayerMovementComponent->ApplyMovementStats();
	}
}
#pragma endregion Base Action Func

#pragma region Equip Func
#pragma endregion Equip Func

#pragma region Base Combat Func
void ARACharacter::Attack()
{
	if (PlayerCombatComponent)
	{
		PlayerCombatComponent->Attack();
	}
}

void ARACharacter::AttackUnarmed()
{
	if (PlayerCombatComponent)
	{
		PlayerCombatComponent->AttackUnarmed();
	}
}

void ARACharacter::AttackWithWeapon()
{
	if (PlayerCombatComponent)
	{
		PlayerCombatComponent->AttackWithWeapon();
	}
}

void ARACharacter::FaceAttackDirection()
{
	if (PlayerCombatComponent)
	{
		PlayerCombatComponent->FaceAttackDirection();
	}
}

bool ARACharacter::IsValidPlayerAttackTarget(const AActor* TargetActor) const
{
	return PlayerCombatComponent && PlayerCombatComponent->IsValidPlayerAttackTarget(TargetActor);
}

void ARACharacter::EndAttack()
{
	if (PlayerCombatComponent)
	{
		PlayerCombatComponent->EndAttack();
	}
}

void ARACharacter::FireArrow()
{
	if (PlayerCombatComponent)
	{
		PlayerCombatComponent->FireArrow();
	}
}
#pragma endregion Base Combat Func

#pragma region Punch Attack Func
void ARACharacter::PerformPunchHit(float damage, float range, float radius)
{
	if (PlayerCombatComponent)
	{
		PlayerCombatComponent->PerformPunchHit(damage, range, radius);
	}
}

void ARACharacter::StartComboAttack()
{
	if (PlayerCombatComponent)
	{
		PlayerCombatComponent->StartComboAttack();
	}
}

void ARACharacter::QueueComboInput()
{
	if (PlayerCombatComponent)
	{
		PlayerCombatComponent->QueueComboInput();
	}
}
#pragma endregion Punch Attack Func

#pragma region Sword Attack Func
void ARACharacter::PerformSwordHit(float damage, float range, float radius)
{
	if (PlayerCombatComponent)
	{
		PlayerCombatComponent->PerformSwordHit(damage, range, radius);
	}
}
#pragma endregion Sword Attack Func

#pragma region Bow Attack Func
void ARACharacter::OnAttackPressed()
{
	if (PlayerCombatComponent)
	{
		PlayerCombatComponent->OnAttackPressed();
	}
}

void ARACharacter::OnAttackReleased()
{
	if (PlayerCombatComponent)
	{
		PlayerCombatComponent->OnAttackReleased();
	}
}

void ARACharacter::StartBowCharge()
{
	if (PlayerCombatComponent)
	{
		PlayerCombatComponent->StartBowCharge();
	}
}

void ARACharacter::ReleaseBowCharge()
{
	if (PlayerCombatComponent)
	{
		PlayerCombatComponent->ReleaseBowCharge();
	}
}

void ARACharacter::UpdateBowFacing(float DeltaTime)
{
	if (PlayerCombatComponent)
	{
		PlayerCombatComponent->UpdateBowFacing(DeltaTime);
	}
}

bool ARACharacter::HasArrowAmmo() const
{
	return PlayerCombatComponent && PlayerCombatComponent->HasArrowAmmo();
}

bool ARACharacter::ConsumeArrowAmmo()
{
	return PlayerCombatComponent && PlayerCombatComponent->ConsumeArrowAmmo();
}

void ARACharacter::RefundArrowAmmo()
{
	if (PlayerCombatComponent)
	{
		PlayerCombatComponent->RefundArrowAmmo();
	}
}

void ARACharacter::FireChargedArrow()
{
	if (PlayerCombatComponent)
	{
		PlayerCombatComponent->FireChargedArrow();
	}
}

void ARACharacter::UpdateBowZoom(float DeltaTime)
{
	if (PlayerCombatComponent)
	{
		PlayerCombatComponent->UpdateBowZoom(DeltaTime);
	}
}

void ARACharacter::UpdateBowCameraArm(float DeltaTime)
{
	if (PlayerCombatComponent)
	{
		PlayerCombatComponent->UpdateBowCameraArm(DeltaTime);
	}
}

void ARACharacter::EndBowAim()
{
	if (PlayerCombatComponent)
	{
		PlayerCombatComponent->EndBowAim();
	}
}

#pragma	endregion Bow Attack Func

#pragma region Anim Montage Func
void ARACharacter::TriggerMeleeHit()
{
	if (PlayerCombatComponent)
	{
		PlayerCombatComponent->TriggerMeleeHit();
	}
}

void ARACharacter::TriggerSkillHit()
{
	if (PlayerCombatComponent)
	{
		PlayerCombatComponent->TriggerSkillHit();
	}
}

void ARACharacter::NormalRelease()
{
	if (PlayerCombatComponent)
	{
		PlayerCombatComponent->NormalRelease();
	}
}

void ARACharacter::SkillRelease()
{
	if (PlayerCombatComponent)
	{
		PlayerCombatComponent->SkillRelease();
	}
}

void ARACharacter::ProceedCombo()
{
	if (PlayerCombatComponent)
	{
		PlayerCombatComponent->ProceedCombo();
	}
}

void ARACharacter::EnableComboWindow()
{
	if (PlayerCombatComponent)
	{
		PlayerCombatComponent->EnableComboWindow();
	}
}

void ARACharacter::DisableComboWindow()
{
	if (PlayerCombatComponent)
	{
		PlayerCombatComponent->DisableComboWindow();
	}
}

void ARACharacter::OnPunchMontageEnded(UAnimMontage* Montage, bool bInterrupted)
{
	if (PlayerCombatComponent)
	{
		PlayerCombatComponent->OnPunchMontageEnded(Montage, bInterrupted);
	}
}

void ARACharacter::OnDodgeMontageEnded(UAnimMontage* Montage, bool bInterrupted)
{
	if (PlayerMovementComponent)
	{
		PlayerMovementComponent->OnDodgeMontageEnded(Montage, bInterrupted);
	}
}
void ARACharacter::PlayBowWeaponMontageSection(FName SectionName)
{
	if (PlayerCombatComponent)
	{
		PlayerCombatComponent->PlayBowWeaponMontageSection(SectionName);
	}
}

void ARACharacter::PlayHitMontage()
{
	if (StatComponent->IsDead())
		return;

	if (bIsAttacking || bIsBowCharging || bIsBowAiming)
		return;

	if (!GetMesh())
		return;

	UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance();
	if (!AnimInstance)
		return;

	//if (AnimInstance->IsAnyMontagePlaying()) // 다른 몽타주가 재생 중이면 멈추고 새로 재생
	//	AnimInstance->Montage_Stop(0.1f);

	const int32 RandomIndex = FMath::RandRange(0, HitMontages.Num() - 1);
	CurrentHitMontage = HitMontages[RandomIndex];

	if (CurrentHitMontage)
		AnimInstance->Montage_Play(CurrentHitMontage);
}

void ARACharacter::StopHitMontage()
{
	if (!CurrentHitMontage || !GetMesh())
		return;

	UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance();
	if (!AnimInstance)
		return;

	if (AnimInstance->Montage_IsPlaying(CurrentHitMontage))
		AnimInstance->Montage_Stop(0.1f, CurrentHitMontage);

	CurrentHitMontage = nullptr;
}
#pragma endregion Anim Montage Func

#pragma region Runtime Data Func
void ARACharacter::SaveRuntimeDataToGameInstance()
{
	if (URAGameInstance* RAGameInstance = GetGameInstance<URAGameInstance>())
	{
		RAGameInstance->SavePlayerRuntimeData(this);
		return;
	}

	UE_LOG(LogTemplateCharacter, Warning, TEXT("SaveRuntimeDataToGameInstance failed: RAGameInstance is null"));
}

void ARACharacter::LoadRuntimeDataFromGameInstance()
{
	URAGameInstance* RAGameInstance = GetGameInstance<URAGameInstance>();
	if (!RAGameInstance)
	{
		UE_LOG(LogTemplateCharacter, Warning, TEXT("LoadRuntimeDataFromGameInstance failed: RAGameInstance is null"));
		return;
	}

	if (!RAGameInstance->HasValidPlayerRuntimeData())
	{
		return;
	}

	RAGameInstance->LoadPlayerRuntimeData(this);
}
#pragma endregion Runtime Data Func

#pragma region VFX Func
void ARACharacter::SpawnHitVFX(
	UNiagaraSystem* NiagaraSystem,
	const FVector& SpawnLocation,
	const FRotator& SpawnRotation,
	const FLinearColor& Color,
	float Scale,
	float Lifetime)
{
	if (PlayerCombatComponent)
	{
		PlayerCombatComponent->SpawnHitVFX(NiagaraSystem, SpawnLocation, SpawnRotation, Color, Scale, Lifetime);
	}
}
#pragma endregion VFX Func

#pragma region UI Func
void ARACharacter::ResetBowCrosshairUI()
{
	if (PlayerCombatComponent)
	{
		PlayerCombatComponent->ResetBowCrosshairUI();
	}
}
#pragma endregion UI Func

#pragma region Show & Hide Func
void ARACharacter::ShowPreviewArrow()
{
	if (PlayerCombatComponent)
	{
		PlayerCombatComponent->ShowPreviewArrow();
	}
}

void ARACharacter::HidePreviewArrow()
{
	if (PlayerCombatComponent)
	{
		PlayerCombatComponent->HidePreviewArrow();
	}
}
#pragma endregion Show & Hide Func

#pragma region Interaction Function
void ARACharacter::SetCurrentPortal(APortalActor* NewPortal)
{
	if (PlayerInteractionComponent)
	{
		PlayerInteractionComponent->SetCurrentPortal(NewPortal);
	}
}

void ARACharacter::ClearCurrentPortal(APortalActor* PortalToClear)
{
	if (PlayerInteractionComponent)
	{
		PlayerInteractionComponent->ClearCurrentPortal(PortalToClear);
	}
}

void ARACharacter::SetCurrentShop(AShopActor* NewShop)
{
	if (PlayerInteractionComponent)
	{
		PlayerInteractionComponent->SetCurrentShop(NewShop);
	}
}

void ARACharacter::ClearCurrentShop(AShopActor* ShopToClear)
{
	if (PlayerInteractionComponent)
	{
		PlayerInteractionComponent->ClearCurrentShop(ShopToClear);
	}
}

void ARACharacter::SetCurrentLobbyNPC(ALobbyNPC* NewLobbyNPC)
{
	if (PlayerInteractionComponent)
	{
		PlayerInteractionComponent->SetCurrentLobbyNPC(NewLobbyNPC);
	}
}

void ARACharacter::ClearCurrentLobbyNPC(ALobbyNPC* LobbyNPCToClear)
{
	if (PlayerInteractionComponent)
	{
		PlayerInteractionComponent->ClearCurrentLobbyNPC(LobbyNPCToClear);
	}
}
#pragma endregion Interaction Function

float ARACharacter::TakeDamage(
	float DamageAmount,
	FDamageEvent const& DamageEvent,
	AController* EventInstigator,
	AActor* DamageCauser)
{
	if (!StatComponent || StatComponent->IsDead())
		return 0.f;

	if (bIsDodging)
	{
		UE_LOG(LogTemplateCharacter, Warning, TEXT("Damage ignored during dodge: %.1f"), DamageAmount);
		return 0.f;
	}

	const float OldHP = StatComponent->GetCurrentHP();
	StatComponent->ApplyDamage(DamageAmount);
	const float ActualDamage = OldHP - StatComponent->GetCurrentHP();

	UE_LOG(LogTemplateCharacter, Warning, TEXT("Player Took Damage: %.1f, Left HP: %f"), ActualDamage, StatComponent->GetCurrentHP());

	if (ActualDamage <= 0.f)
		return 0.f;

	if (StatComponent->IsDead())
		HandleCharacterDeath();
	else
		PlayHitMontage();

	return ActualDamage;
}
