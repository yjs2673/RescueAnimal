#include "RACharacter.h"

#include "PortalActor.h"
#include "ShopActor.h"
#include "LobbyNPC.h"
#include "WeaponBase.h"
#include "ArrowProjectile.h"
#include "AnimalBase.h"
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

#include "RAGameInstance.h"
#include "RAStructTypes.h"

#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "InputActionValue.h"
#include "InputAction.h"

#include "Blueprint/UserWidget.h"

#include "CrosshairBowWidget.h"
#include "MainHUDWidget.h"
#include "RAPlayerController.h"

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

	PrimaryActorTick.bCanEverTick = true; // Tick() 함수를 사용하기 위해 true로 설정
	CurrentWeapon = nullptr; // 처음에는 무기를 들고 있지 않으므로 nullptr로 초기화

	StatComponent = CreateDefaultSubobject<UPlayerStatComponent>(TEXT("StatComponent"));
	InventoryComponent = CreateDefaultSubobject<UInventoryComponent>(TEXT("InventoryComponent"));
	QuickSlotComponent = CreateDefaultSubobject<UQuickSlotComponent>(TEXT("QuickSlotComponent"));
	PlayerSkillComponent = CreateDefaultSubobject<UPlayerSkillComponent>(TEXT("PlayerSkillComponent"));

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
		EnhancedInputComponent->BindAction(JumpAction, ETriggerEvent::Started, this, &ARACharacter::StartJump);
		EnhancedInputComponent->BindAction(JumpAction, ETriggerEvent::Completed, this, &ACharacter::StopJumping);

		// Moving
		EnhancedInputComponent->BindAction(MoveAction, ETriggerEvent::Triggered, this, &ARACharacter::Move);

		// Looking
		EnhancedInputComponent->BindAction(LookAction, ETriggerEvent::Triggered, this, &ARACharacter::Look);

		// Attacking
		// EnhancedInputComponent->BindAction(AttackAction, ETriggerEvent::Started, this, &ARACharacter::Attack);
		EnhancedInputComponent->BindAction(AttackAction, ETriggerEvent::Started, this, &ARACharacter::OnAttackPressed);
		EnhancedInputComponent->BindAction(AttackAction, ETriggerEvent::Completed, this, &ARACharacter::OnAttackReleased);
		EnhancedInputComponent->BindAction(AttackAction, ETriggerEvent::Canceled, this, &ARACharacter::OnAttackReleased);
		
		// Interacting
		EnhancedInputComponent->BindAction(InteractAction, ETriggerEvent::Started, this, &ARACharacter::Interact);

		// Dodging
		EnhancedInputComponent->BindAction(DodgeAction, ETriggerEvent::Started, this, &ARACharacter::Dodge);

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
			EnhancedInputComponent->BindAction(QuickSlot1Action, ETriggerEvent::Started, this, &ARACharacter::UseQuickSlotItem, 0);
		if (QuickSlot2Action)
			EnhancedInputComponent->BindAction(QuickSlot2Action, ETriggerEvent::Started, this, &ARACharacter::UseQuickSlotItem, 1);
		if (QuickSlot3Action)
			EnhancedInputComponent->BindAction(QuickSlot3Action, ETriggerEvent::Started, this, &ARACharacter::UseQuickSlotItem, 2);
		if (QuickSlot4Action)
			EnhancedInputComponent->BindAction(QuickSlot4Action, ETriggerEvent::Started, this, &ARACharacter::UseQuickSlotItem, 3);
		if (QuickSlot5Action)
			EnhancedInputComponent->BindAction(QuickSlot5Action, ETriggerEvent::Started, this, &ARACharacter::UseQuickSlotItem, 4);
		if (QuickSlot6Action)
			EnhancedInputComponent->BindAction(QuickSlot6Action, ETriggerEvent::Started, this, &ARACharacter::UseQuickSlotItem, 5);
		if (QuickSlot7Action)
			EnhancedInputComponent->BindAction(QuickSlot7Action, ETriggerEvent::Started, this, &ARACharacter::UseQuickSlotItem, 6);
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
		return RAPlayerController->IsPortalTransitionInputLocked();
	}

	return false;
}

/* Tick */
void ARACharacter::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	UpdateDodgeMovement(DeltaTime);

	if (bIsBowCharging) // 활을 당기는 중이면, 활의 방향과 충전 상태를 업데이트
	{
		UpdateBowFacing(DeltaTime);

		const float CurrentTime = GetWorld()->GetTimeSeconds();
		const float ChargeDuration = CurrentTime - BowChargeStartTime;

		const float ClampedCharge = FMath::Clamp(ChargeDuration, MinBowChargeTime, MaxBowChargeTime);
		const float ChargeAlpha =
			(MaxBowChargeTime > MinBowChargeTime)
			? (ClampedCharge - MinBowChargeTime) / (MaxBowChargeTime - MinBowChargeTime)
			: 1.0f;

		CachedBowChargeAlpha = FMath::Clamp(ChargeAlpha, 0.0f, 1.0f);

		if (CrosshairWidgetInstance)
		{
			CrosshairWidgetInstance->SetChargeAlpha(CachedBowChargeAlpha);

			if (CachedBowChargeAlpha >= 1.0f)
				CrosshairWidgetInstance->PlayFullChargeEffect();
		}
	}

	UpdateBowZoom(DeltaTime);
	UpdateBowCameraArm(DeltaTime);
}

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
		if (SpawnedWeapon)
			EquipWeapon(SpawnedWeapon);
	}

	//if (MainHUDClass) // MainHUDInstance를 생성하여 뷰포트에 추가
	//{
	//	APlayerController* PC = Cast<APlayerController>(GetController());
	//	if (PC)
	//	{
	//		MainHUDInstance = CreateWidget<UUserWidget>(PC, MainHUDClass);
	//		if (MainHUDInstance)
	//			MainHUDInstance->AddToViewport();
	//	}
	//}
	if (ARAPlayerController* RAPlayerController = Cast<ARAPlayerController>(GetController()))
	{
		if (UMainHUDWidget* HUDWidget = RAPlayerController->GetMainHUDWidget())
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
		RAPlayerController->ShowGameOverMessage();
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
void ARACharacter::StartJump()
{
	if (IsPortalTransitionInputLocked())
		return;

	if (bIsDodging)
		return;

	if (CanJump() && JumpSound)
	{
		UGameplayStatics::PlaySoundAtLocation(
			this,
			JumpSound,
			GetActorLocation()
		);
	}

	Jump();
}
void ARACharacter::Move(const FInputActionValue& Value)
{
	if (IsPortalTransitionInputLocked())
		return;

	if (StatComponent && StatComponent->IsDead())
		return;

	if (bIsDodging)
		return;

	if (bIsAttacking && !bIsBowCharging)
		return;

	StopHitMontage();

	// input is a Vector2D
	FVector2D MovementVector = Value.Get<FVector2D>();

	MoveInputX = MovementVector.X;
	MoveInputY = MovementVector.Y;

	if (Controller != nullptr)
	{
		// find out which way is forward
		const FRotator Rotation = Controller->GetControlRotation();
		const FRotator YawRotation(0, Rotation.Yaw, 0);

		// get forward vector
		const FVector ForwardDirection = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::X);
	
		// get right vector 
		const FVector RightDirection = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::Y);

		// add movement 
		AddMovementInput(ForwardDirection, MovementVector.Y);
		AddMovementInput(RightDirection, MovementVector.X);
	}
}

void ARACharacter::Look(const FInputActionValue& Value)
{
	if (IsPortalTransitionInputLocked())
		return;

	// input is a Vector2D
	FVector2D LookAxisVector = Value.Get<FVector2D>();

	if (Controller != nullptr)
	{
		// add yaw and pitch input to controller
		AddControllerYawInput(LookAxisVector.X);
		AddControllerPitchInput(LookAxisVector.Y);
	}
}

void ARACharacter::Interact()
{
	if (IsPortalTransitionInputLocked())
		return;

	if (StatComponent && StatComponent->IsDead())
		return;

	if (TryRescueNearbyAnimal())
		return;
	
	/*if (NearbyWeapon || CurrentWeapon)
	{
		HandleWeaponInteract();
		return;
	}*/

	if (CurrentShop)
	{
		CurrentShop->Interact(this);
		return;
	}

	if (CurrentLobbyNPC)
	{
		CurrentLobbyNPC->Interact();
		return;
	}

	if (CurrentPortal)
	{
		CurrentPortal->Interact(this);
		return;
	}
}

bool ARACharacter::TryRescueNearbyAnimal()
{
	AAnimalBase* RescueAnimal = FindNearbyRescueAnimal();
	if (!RescueAnimal)
	{
		return false;
	}

	if (!IsRescueKitEquipped())
	{
		UE_LOG(LogTemplateCharacter, Warning, TEXT("Animal rescue failed: rescue kit is not equipped."));
		return true;
	}

	if (!RescueAnimal->CanBeRescued())
	{
		UE_LOG(LogTemplateCharacter, Warning, TEXT("Animal rescue failed: camp is not cleared or animal is not trapped."));
		return true;
	}

	if (!RescueAnimal->Rescue())
	{
		UE_LOG(LogTemplateCharacter, Warning, TEXT("Animal rescue failed: Rescue() returned false."));
		return true;
	}

	UE_LOG(LogTemplateCharacter, Warning, TEXT("Animal rescued: %s"), *RescueAnimal->GetName());
	return true;
}

AAnimalBase* ARACharacter::FindNearbyRescueAnimal() const
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return nullptr;
	}

	AAnimalBase* BestAnimal = nullptr;
	float BestDistanceSquared = FMath::Square(AnimalRescueInteractDistance);

	for (TActorIterator<AAnimalBase> It(World); It; ++It)
	{
		AAnimalBase* Animal = *It;
		if (!IsValid(Animal) || !Animal->IsTrapped())
		{
			continue;
		}

		const float DistanceSquared = FVector::DistSquared(GetActorLocation(), Animal->GetActorLocation());
		if (DistanceSquared <= BestDistanceSquared)
		{
			BestDistanceSquared = DistanceSquared;
			BestAnimal = Animal;
		}
	}

	return BestAnimal;
}

bool ARACharacter::IsRescueKitEquipped() const
{
	return CurrentWeapon && CurrentWeapon->WeaponType == EWeaponType::Kit;
}

void ARACharacter::UseQuickSlotItem(int32 SlotIndex)
{
	if (IsPortalTransitionInputLocked())
		return;

	if (!QuickSlotComponent)
		return;

	const FName ItemID = QuickSlotComponent->GetSlotItem(SlotIndex);
	if (ItemID.IsNone())
	{
		UE_LOG(LogTemplateCharacter, Warning, TEXT("QuickSlot %d is empty"), SlotIndex + 1);
		return;
	}

	UseInventoryItem(ItemID);
}

bool ARACharacter::UseInventoryItem(FName ItemID)
{
	if (IsPortalTransitionInputLocked())
		return false;

	if (ItemID.IsNone())
		return false;

	URAGameInstance* RAGameInstance = GetGameInstance<URAGameInstance>();
	if (!RAGameInstance)
		return false;

	FItemData ItemData;
	if (!RAGameInstance->GetItemDataByID(ItemID, ItemData))
		return false;

	switch (ItemData.ItemType)
	{
	case EItemType::Consumable:
		return UseConsumableItem(ItemID);

	case EItemType::Weapon:
		return EquipWeaponFromInventory(ItemID);

	default:
		return false;
	}
}

bool ARACharacter::UseConsumableItem(FName ItemID)
{
	if (IsPortalTransitionInputLocked())
		return false;

	if (StatComponent && StatComponent->IsDead())
		return false;

	if (ItemID.IsNone())
		return false;

	if (!InventoryComponent || !InventoryComponent->HasItem(ItemID, 1))
	{
		UE_LOG(LogTemplateCharacter, Warning, TEXT("Consumable item not found in inventory: %s"), *ItemID.ToString());
		return false;
	}

	URAGameInstance* RAGameInstance = GetGameInstance<URAGameInstance>();
	if (!RAGameInstance)
	{
		UE_LOG(LogTemplateCharacter, Warning, TEXT("UseConsumableItem failed: RAGameInstance is null"));
		return false;
	}

	FItemData ItemData;
	if (!RAGameInstance->GetItemDataByID(ItemID, ItemData))
	{
		UE_LOG(LogTemplateCharacter, Warning, TEXT("UseConsumableItem failed: ItemData not found. ItemID=%s"), *ItemID.ToString());
		return false;
	}

	if (ItemData.ItemType != EItemType::Consumable)
	{
		UE_LOG(LogTemplateCharacter, Warning, TEXT("Item is not consumable: %s"), *ItemID.ToString());
		return false;
	}

	bool bUseSucceeded = false;

	switch (ItemData.ConsumableType)
	{
	case EConsumableType::Heal:
	{
		if (!StatComponent)
		{
			UE_LOG(LogTemplateCharacter, Warning, TEXT("Heal item failed: StatComponent is null"));
			return false;
		}

		const float OldHP = StatComponent->GetCurrentHP();

		StatComponent->Heal(ItemData.HealAmount);

		const float NewHP = StatComponent->GetCurrentHP();
		bUseSucceeded = true;

		UE_LOG(LogTemplateCharacter, Warning, TEXT("Heal item used: %s | HP %.1f -> %.1f"),
			*ItemID.ToString(),
			OldHP,
			NewHP);

		break;
	}

	case EConsumableType::Buff:
	{
		if (!StatComponent)
		{
			UE_LOG(LogTemplateCharacter, Warning, TEXT("Buff item failed: StatComponent is null"));
			return false;
		}

		if (!StatComponent->ApplyBuffItem(ItemData))
		{
			UE_LOG(LogTemplateCharacter, Warning, TEXT("Buff item failed: %s"), *ItemID.ToString());
			return false;
		}

		bUseSucceeded = true;

		UE_LOG(LogTemplateCharacter, Warning, TEXT("Buff item used: %s | Type=%d Value=%.2f Duration=%.2f"),
			*ItemID.ToString(),
			static_cast<uint8>(ItemData.BuffTargetType),
			ItemData.BuffValue,
			ItemData.BuffDuration);

		break;
	}

	case EConsumableType::Capture:
	{
		UE_LOG(LogTemplateCharacter, Warning, TEXT("Capture consumable is not implemented yet: %s"), *ItemID.ToString());
		return false;
	}

	default:
	{
		UE_LOG(LogTemplateCharacter, Warning, TEXT("Unknown consumable type: %s"), *ItemID.ToString());
		return false;
	}
	}

	if (!bUseSucceeded)
	{
		UE_LOG(LogTemplateCharacter, Warning, TEXT("Consumable use failed: %s"), *ItemID.ToString());
		return false;
	}

	if (!InventoryComponent->RemoveItem(ItemID, 1))
	{
		UE_LOG(LogTemplateCharacter, Warning, TEXT("Failed to consume inventory item after use: %s"), *ItemID.ToString());
		return false;
	}

	if (ConsumableUseSound)
	{
		UGameplayStatics::PlaySoundAtLocation(
			this,
			ConsumableUseSound,
			GetActorLocation()
		);
	}

	UE_LOG(LogTemplateCharacter, Warning, TEXT("Consumable item consumed: %s"), *ItemID.ToString());
	return true;
}

bool ARACharacter::UnequipCurrentWeaponToInventory()
{
	if (StatComponent && StatComponent->IsDead())
		return false;

	if (!CurrentWeapon || !InventoryComponent)
		return false;

	const FName PreviousWeaponID = CurrentWeapon->WeaponID;
	if (PreviousWeaponID.IsNone())
		return false;

	if (bIsBowCharging || bIsBowAiming)
	{
		EndBowAim();
		ApplyMovementStats();
	}

	AWeaponBase* OldWeapon = CurrentWeapon;
	CurrentWeapon = nullptr;
	OldWeapon->Destroy();

	if (!InventoryComponent->HasItem(PreviousWeaponID, 1))
	{
		InventoryComponent->AddItem(PreviousWeaponID, 1);
	}

	OnWeaponChanged.Broadcast(GetCurrentWeaponType());

	if (EquipmentSound)
	{
		UGameplayStatics::PlaySoundAtLocation(
			this,
			EquipmentSound,
			GetActorLocation()
		);
	}

	UE_LOG(LogTemplateCharacter, Warning, TEXT("Unequipped weapon to inventory: %s"), *PreviousWeaponID.ToString());
	return true;
}

bool ARACharacter::EquipWeaponFromInventory(FName ItemID)
{
	if (StatComponent && StatComponent->IsDead())
		return false;

	if (ItemID.IsNone())
		return false;

	if (CurrentWeapon && CurrentWeapon->WeaponID == ItemID)
	{
		return UnequipCurrentWeaponToInventory();
	}

	if (!InventoryComponent || !InventoryComponent->HasItem(ItemID, 1))
	{
		UE_LOG(LogTemplateCharacter, Warning, TEXT("EquipWeaponFromInventory failed: item not found. ItemID=%s"), *ItemID.ToString());
		return false;
	}

	URAGameInstance* RAGameInstance = GetGameInstance<URAGameInstance>();
	if (!RAGameInstance)
	{
		UE_LOG(LogTemplateCharacter, Warning, TEXT("EquipWeaponFromInventory failed: RAGameInstance is null"));
		return false;
	}

	FItemData ItemData;
	if (!RAGameInstance->GetItemDataByID(ItemID, ItemData) || ItemData.ItemType != EItemType::Weapon)
	{
		UE_LOG(LogTemplateCharacter, Warning, TEXT("EquipWeaponFromInventory failed: item is not weapon. ItemID=%s"), *ItemID.ToString());
		return false;
	}

	FWeaponData WeaponData;
	if (!RAGameInstance->GetWeaponDataByID(ItemID, WeaponData))
	{
		UE_LOG(LogTemplateCharacter, Warning, TEXT("EquipWeaponFromInventory failed: WeaponData not found. WeaponID=%s"), *ItemID.ToString());
		return false;
	}

	if (!WeaponData.WeaponClass)
	{
		UE_LOG(LogTemplateCharacter, Warning, TEXT("EquipWeaponFromInventory failed: WeaponClass is null. WeaponID=%s"), *ItemID.ToString());
		return false;
	}

	UWorld* World = GetWorld();
	if (!World)
		return false;

	AWeaponBase* NewWeapon = World->SpawnActor<AWeaponBase>(WeaponData.WeaponClass);
	if (!NewWeapon)
	{
		UE_LOG(LogTemplateCharacter, Warning, TEXT("EquipWeaponFromInventory failed: spawn failed. WeaponID=%s"), *ItemID.ToString());
		return false;
	}

	NewWeapon->WeaponID = WeaponData.WeaponID.IsNone() ? ItemID : WeaponData.WeaponID;
	NewWeapon->WeaponType = WeaponData.WeaponType;
	NewWeapon->AttackType = WeaponData.AttackType;


	if (CurrentWeapon)
	{

		if (bIsBowCharging || bIsBowAiming)
		{
			EndBowAim();
			ApplyMovementStats();
		}

		AWeaponBase* OldWeapon = CurrentWeapon;
		CurrentWeapon = nullptr;
		OldWeapon->Destroy();
	}


	EquipWeapon(NewWeapon);

	if (EquipmentSound)
	{
		UGameplayStatics::PlaySoundAtLocation(
			this,
			EquipmentSound,
			GetActorLocation()
		);
	}

	UE_LOG(LogTemplateCharacter, Warning, TEXT("Equipped weapon from inventory: %s"), *ItemID.ToString());
	return true;
}

void ARACharacter::ApplyMovementStats()
{
	if (!StatComponent || !GetCharacterMovement())
		return;

	GetCharacterMovement()->JumpZVelocity = StatComponent->GetFinalJumpZVelocity();

	if (!bIsBowCharging)
	{
		GetCharacterMovement()->MaxWalkSpeed = StatComponent->GetFinalMoveSpeed();
	}
}
bool ARACharacter::CanDodge() const
{
	if (bIsDodging || bIsAttacking)
		return false;

	if (StatComponent && StatComponent->IsDead())
		return false;

	if (!GetCharacterMovement() || GetCharacterMovement()->IsFalling())
		return false;

	return true;
}

void ARACharacter::Dodge()
{
	if (IsPortalTransitionInputLocked())
		return;

	UE_LOG(LogTemp, Warning, TEXT("Action Dodge"));

	if (!CanDodge())
		return;

	UAnimInstance* AnimInstance = GetMesh() ? GetMesh()->GetAnimInstance() : nullptr;
	if (!AnimInstance || !DodgeMontage)
		return;

	StopHitMontage();

	const float DodgeMontageDuration = AnimInstance->Montage_Play(DodgeMontage);
	if (DodgeMontageDuration <= 0.0f)
		return;

	bIsDodging = true;
	CurrentDodgeMontage = DodgeMontage;
	DodgeElapsedTime = 0.0f;
	DodgeDuration = DodgeMontageDuration;
	DodgeDirection = GetActorForwardVector();
	DodgeDirection.Z = 0.0f;
	DodgeDirection.Normalize();

	if (DodgeSound)
	{
		UGameplayStatics::PlaySoundAtLocation(
			this,
			DodgeSound,
			GetActorLocation()
		);
	}

	AnimInstance->OnMontageEnded.RemoveDynamic(this, &ARACharacter::OnDodgeMontageEnded);
	AnimInstance->OnMontageEnded.AddDynamic(this, &ARACharacter::OnDodgeMontageEnded);
}

void ARACharacter::UpdateDodgeMovement(float DeltaTime)
{
	if (!bIsDodging || DodgeDuration <= 0.0f || DodgeDirection.IsNearlyZero())
		return;

	const float RemainingTime = FMath::Max(DodgeDuration - DodgeElapsedTime, 0.0f);
	const float MoveDeltaTime = FMath::Min(DeltaTime, RemainingTime);
	const float MoveDistance = (DodgeDistance / DodgeDuration) * MoveDeltaTime;

	FHitResult HitResult;
	AddActorWorldOffset(DodgeDirection * MoveDistance, true, &HitResult);

	DodgeElapsedTime += MoveDeltaTime;

	if (HitResult.bBlockingHit)
	{
		DodgeElapsedTime = DodgeDuration;
	}
}

void ARACharacter::EndDodge()
{
	bIsDodging = false;
	CurrentDodgeMontage = nullptr;
	DodgeElapsedTime = 0.0f;
	DodgeDuration = 0.0f;
	DodgeDirection = FVector::ZeroVector;
}
#pragma endregion Base Action Func

#pragma region Equip Func
void ARACharacter::HandleWeaponInteract()
{
	if (bIsAttacking)
		return;

	if (NearbyWeapon)
	{
		if (!InventoryComponent)
			return;

		const FName WeaponItemID = NearbyWeapon->WeaponID;
		if (WeaponItemID.IsNone())
		{
			UE_LOG(LogTemplateCharacter, Warning, TEXT("Pickup weapon failed: WeaponID is None. Weapon=%s"), *GetNameSafe(NearbyWeapon));
			return;
		}

		URAGameInstance* RAGameInstance = GetGameInstance<URAGameInstance>();
		FItemData ItemData;
		if (!RAGameInstance || !RAGameInstance->GetItemDataByID(WeaponItemID, ItemData) || ItemData.ItemType != EItemType::Weapon)
		{
			UE_LOG(LogTemplateCharacter, Warning, TEXT("Pickup weapon failed: ItemData is not weapon. ItemID=%s"), *WeaponItemID.ToString());
			return;
		}

		AWeaponBase* WeaponToPickup = NearbyWeapon;
		NearbyWeapon = nullptr;

		InventoryComponent->AddItem(WeaponItemID, 1);
		WeaponToPickup->Destroy();

		if (EquipmentSound)
		{
			UGameplayStatics::PlaySoundAtLocation(
				this,
				EquipmentSound,
				GetActorLocation()
			);
		}

		UE_LOG(LogTemplateCharacter, Warning, TEXT("Picked up weapon item: %s"), *WeaponItemID.ToString());
		return;
	}

	if (CurrentWeapon)
	{
		DropCurrentWeapon();

		if (EquipmentSound)
		{
			UGameplayStatics::PlaySoundAtLocation(
				this,
				EquipmentSound,
				GetActorLocation()
			);
		}
	}
}
void ARACharacter::EquipWeapon(AWeaponBase* NewWeapon) // 무기 장착, 이미 장착 중이면 교체
{
	if (!NewWeapon)
	{
		UE_LOG(LogTemp, Warning, TEXT("EquipWeapon: NewWeapon is null"));
		return;
	}

	if (CurrentWeapon == NewWeapon)
	{
		UE_LOG(LogTemp, Warning, TEXT("EquipWeapon: Weapon is already equipped"));
		return;
	}

	if (CurrentWeapon)
	{
		UnequipWeapon();
	}

	CurrentWeapon = NewWeapon;
	CurrentWeapon->SetOwner(this);
	CurrentWeapon->SetPickupEnabled(false);
	CurrentWeapon->UpdateWeaponVisualState();

	if (CurrentWeapon->WeaponMesh)
	{
		CurrentWeapon->WeaponMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		CurrentWeapon->WeaponMesh->SetSimulatePhysics(false);
	}

	if (CurrentWeapon->WeaponSkeletalMesh)
	{
		CurrentWeapon->WeaponSkeletalMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		CurrentWeapon->WeaponSkeletalMesh->SetSimulatePhysics(false);
	}

	const FName AttachSocketName =
		(CurrentWeapon->WeaponType == EWeaponType::Bow)
		? LeftWeaponSocketName : RightWeaponSocketName;

	CurrentWeapon->AttachToComponent(
		GetMesh(),
		FAttachmentTransformRules::SnapToTargetNotIncludingScale,
		AttachSocketName
	);

	if (USceneComponent* ActiveVisual = CurrentWeapon->GetActiveVisualComponent())
	{
		ActiveVisual->SetRelativeLocation(CurrentWeapon->EquipRelativeLocation);
		ActiveVisual->SetRelativeRotation(CurrentWeapon->EquipRelativeRotation);
		ActiveVisual->SetRelativeScale3D(CurrentWeapon->EquipRelativeScale);
	}

	OnWeaponChanged.Broadcast(GetCurrentWeaponType());
	UE_LOG(LogTemp, Warning, TEXT("Weapon Changed Broadcast: %s"), *UEnum::GetValueAsString(GetCurrentWeaponType()));

	UE_LOG(LogTemp, Warning, TEXT("Equipped Weapon: %s | Socket: %s"),
		*CurrentWeapon->GetName(),
		*AttachSocketName.ToString());
}

void ARACharacter::UnequipWeapon() // 장착 해제
{
	if (!CurrentWeapon)
		return;

	CurrentWeapon->DetachFromActor(FDetachmentTransformRules::KeepWorldTransform);
	CurrentWeapon->SetOwner(nullptr);
	CurrentWeapon->UpdateWeaponVisualState();

	if (CurrentWeapon->WeaponMesh)
	{
		CurrentWeapon->WeaponMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		CurrentWeapon->WeaponMesh->SetSimulatePhysics(false);
	}

	if (CurrentWeapon->WeaponSkeletalMesh)
	{
		CurrentWeapon->WeaponSkeletalMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		CurrentWeapon->WeaponSkeletalMesh->SetSimulatePhysics(false);
	}

	CurrentWeapon->SetPickupEnabled(true);

	UE_LOG(LogTemp, Warning, TEXT("Unequipped Weapon: %s"), *CurrentWeapon->GetName());

	CurrentWeapon = nullptr;

	OnWeaponChanged.Broadcast(GetCurrentWeaponType());
}

void ARACharacter::DropCurrentWeapon() // 현재 장착한 무기를 떨어뜨리는 함수, 무기가 바닥에 떨어진 후의 위치와 회전을 계산하여 설정
{
	if (!CurrentWeapon)
		return;

	if (bIsBowCharging || bIsBowAiming)
	{
		EndBowAim();
		ApplyMovementStats();
	}

	AWeaponBase* WeaponToDrop = CurrentWeapon;

	WeaponToDrop->DetachFromActor(FDetachmentTransformRules::KeepWorldTransform);
	WeaponToDrop->SetOwner(nullptr);
	WeaponToDrop->UpdateWeaponVisualState();

	const FVector ForwardOffset = GetActorForwardVector() * 80.0f;
	const FVector TraceStart = GetActorLocation() + ForwardOffset + FVector(0.0f, 0.0f, 100.0f);
	const FVector TraceEnd = TraceStart - FVector(0.0f, 0.0f, 500.0f);

	FHitResult HitResult;
	FCollisionQueryParams QueryParams;
	QueryParams.AddIgnoredActor(this);
	QueryParams.AddIgnoredActor(WeaponToDrop);

	FVector DropLocation = GetActorLocation() + ForwardOffset + FVector(0.0f, 0.0f, 15.0f);

	if (GetWorld()->LineTraceSingleByChannel(HitResult, TraceStart, TraceEnd, ECC_Visibility, QueryParams))
	{
		DropLocation = HitResult.ImpactPoint + FVector(0.0f, 0.0f, 15.0f);
	}

	WeaponToDrop->SetActorLocation(DropLocation);
	WeaponToDrop->SetActorRotation(FRotator(0.0f, GetActorRotation().Yaw + 25.0f, 0.0f));

	if (WeaponToDrop->WeaponMesh)
	{
		WeaponToDrop->WeaponMesh->SetSimulatePhysics(false);
		WeaponToDrop->WeaponMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	}

	WeaponToDrop->SetPickupEnabled(true);
	WeaponToDrop->EnablePickupAfterDrop();

	CurrentWeapon = nullptr;

	OnWeaponChanged.Broadcast(GetCurrentWeaponType());

	UE_LOG(LogTemp, Warning, TEXT("Dropped Weapon: %s"), *WeaponToDrop->GetName());
}

void ARACharacter::SetNearbyWeapon(AWeaponBase* NewWeapon)
{
	NearbyWeapon = NewWeapon;
	UE_LOG(LogTemp, Warning, TEXT("Nearby Weapon Set: %s"), *GetNameSafe(NewWeapon));
}

void ARACharacter::ClearNearbyWeapon(AWeaponBase* WeaponToClear)
{
	if (NearbyWeapon == WeaponToClear)
	{
		NearbyWeapon = nullptr;
		UE_LOG(LogTemp, Warning, TEXT("Nearby Weapon Cleared"));
	}
}
#pragma endregion Equip Func

#pragma region Base Combat Func
void ARACharacter::Attack()
{
	if (IsPortalTransitionInputLocked())
		return;

	if (StatComponent && StatComponent->IsDead())
		return;

	if (bIsDodging)
		return;

	if (bIsAttacking && !bIsPunching)
	{
		UE_LOG(LogTemp, Warning, TEXT("Already Attacking"));
		return;
	}

	StopHitMontage();

	if (CurrentWeapon)
		AttackWithWeapon();
	else
		AttackUnarmed();
}

void ARACharacter::AttackUnarmed()
{
	if (!bIsPunching)
	{
		FaceAttackDirection();
		bIsAttacking = true;
		StartComboAttack();
		return;
	}

	QueueComboInput();
}

void ARACharacter::AttackWithWeapon()
{
	if (!CurrentWeapon)
	{
		UE_LOG(LogTemp, Warning, TEXT("AttackWithWeapon: No CurrentWeapon"));
		return;
	}

	FaceAttackDirection();
	bIsAttacking = true;

	UE_LOG(LogTemp, Warning, TEXT("Weapon Attack: %s"), *CurrentWeapon->GetName());

	if (CurrentWeapon->AttackMontage && GetMesh() && GetMesh()->GetAnimInstance())
	{
		float Duration = GetMesh()->GetAnimInstance()->Montage_Play(CurrentWeapon->AttackMontage);

		if (Duration > 0.0f)
		{
			FTimerHandle AttackEndTimerHandle;
			GetWorldTimerManager().SetTimer(
				AttackEndTimerHandle,
				this,
				&ARACharacter::EndAttack,
				Duration,
				false
			);
		}
		else
			EndAttack();
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("Weapon AttackMontage is missing"));
		EndAttack();
	}
}

void ARACharacter::FaceAttackDirection()
{
	if (!Controller)
		return;

	const FRotator ControlRot = Controller->GetControlRotation();
	const FRotator TargetRot(0.0f, ControlRot.Yaw, 0.0f);
	SetActorRotation(TargetRot);
}

bool ARACharacter::IsValidPlayerAttackTarget(const AActor* TargetActor) const
{
	return TargetActor && TargetActor != this && TargetActor->IsA<ARAEnemyBase>();
}

void ARACharacter::EndAttack()
{
	bIsAttacking = false;
	bIsBowCharging = false;
	UE_LOG(LogTemp, Warning, TEXT("Attack End"));
}

void ARACharacter::FireArrow()
{
	if (!CurrentWeapon)
	{
		UE_LOG(LogTemp, Warning, TEXT("FireArrow: No CurrentWeapon"));
		return;
	}

	if (CurrentWeapon->AttackType != EAttackType::Ranged)
	{
		UE_LOG(LogTemp, Warning, TEXT("FireArrow: CurrentWeapon is not ranged"));
		return;
	}

	if (!CurrentWeapon->ProjectileClass)
	{
		UE_LOG(LogTemp, Warning, TEXT("FireArrow: ProjectileClass is null"));
		return;
	}

	if (!GetMesh())
	{
		return;
	}

	if (!ConsumeArrowAmmo())
	{
		UE_LOG(LogTemp, Warning, TEXT("FireArrow failed: no Arrow ammo"));
		return;
	}

	const FVector SpawnLocation = GetMesh()->GetSocketLocation(TEXT("ArrowSpawnSocket"));
	const FRotator SpawnRotation = GetControlRotation();

	FActorSpawnParameters SpawnParams;
	SpawnParams.Owner = this;
	SpawnParams.Instigator = this;

	AArrowProjectile* Arrow = GetWorld()->SpawnActor<AArrowProjectile>(
		CurrentWeapon->ProjectileClass,
		SpawnLocation,
		SpawnRotation,
		SpawnParams
	);

	if (!Arrow)
	{
		RefundArrowAmmo();
		return;
	}

	Arrow->Damage = CurrentWeapon->AttackDamage;

	if (Arrow->ProjectileMovement)
	{
		Arrow->ProjectileMovement->InitialSpeed = CurrentWeapon->ProjectileSpeed;
		Arrow->ProjectileMovement->MaxSpeed = CurrentWeapon->ProjectileSpeed;
	}

	UE_LOG(LogTemp, Warning, TEXT("Arrow Fired"));
}
#pragma endregion Base Combat Func

#pragma region Punch Attack Func
void ARACharacter::PerformPunchHit(float damage, float range, float radius)
{
	if (!GetWorld())
		return;

	const FVector Start = GetActorLocation() + FVector(0.f, 0.f, 50.f);
	const FVector End = Start + (GetActorForwardVector() * range);

	FCollisionShape Sphere = FCollisionShape::MakeSphere(radius);

	FCollisionQueryParams QueryParams;
	QueryParams.AddIgnoredActor(this);

	FCollisionObjectQueryParams ObjectQueryParams;
	ObjectQueryParams.AddObjectTypesToQuery(ECC_Pawn);
	TArray<FHitResult> HitResults;
	GetWorld()->SweepMultiByObjectType(
		HitResults,
		Start,
		End,
		FQuat::Identity,
		ObjectQueryParams,
		Sphere,
		QueryParams
	);

	if (bDrawAttackDebug)
	{
		const bool bHitEnemy = HitResults.ContainsByPredicate([this](const FHitResult& HitResult)
			{
				return IsValidPlayerAttackTarget(HitResult.GetActor());
			});
		const FColor DebugColor = bHitEnemy ? FColor::Red : FColor::Green;
		DrawDebugCapsule(
			GetWorld(),
			(Start + End) * 0.5f,
			range * 0.5f,
			radius,
			FRotationMatrix::MakeFromX(End - Start).ToQuat(),
			DebugColor,
			false,
			1.5f
		);
	}

	for (const FHitResult& HitResult : HitResults)
	{
		AActor* HitActor = HitResult.GetActor();
		if (!IsValidPlayerAttackTarget(HitActor))
		{
			UE_LOG(LogTemplateCharacter, Warning, TEXT("Punch ignored non-enemy hit: %s"), *GetNameSafe(HitActor));
			continue;
		}

		UE_LOG(LogTemplateCharacter, Warning, TEXT("Hit Enemy: %s"), *HitActor->GetName());

		SpawnHitVFX(
			PunchHitVFX,
			HitResult.ImpactPoint,
			GetActorRotation(),
			PunchHitColor,
			PunchHitScale,
			PunchHitLifetime
		);

		USoundBase* SelectedHitSound = nullptr;
		const int32 SoundIndex = CurrentComboIndex - 1;

		if (PunchHitSounds.IsValidIndex(SoundIndex))
			SelectedHitSound = PunchHitSounds[SoundIndex];

		if (SelectedHitSound)
		{
			UGameplayStatics::PlaySoundAtLocation(
				this,
				SelectedHitSound,
				HitResult.ImpactPoint
			);
		}

		UGameplayStatics::ApplyDamage(
			HitActor,
			damage,
			GetController(),
			this,
			UDamageType::StaticClass()
		);

		break;
	}

	// TestAddItem("Potion", 2);
	// TestTakeDamage(20);
}

void ARACharacter::StartComboAttack()
{
	if (!PunchMontage || !GetMesh() || !GetMesh()->GetAnimInstance())
		return;

	UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance();
	if (!AnimInstance)
		return;

	bIsPunching = true;
	bIsAttacking = true;
	bComboInputBuffered = false;
	bCanAcceptComboInput = false;
	CurrentComboIndex = 1;

	AnimInstance->Montage_Play(PunchMontage);
	AnimInstance->Montage_JumpToSection(FName("Combo1"), PunchMontage);

	AnimInstance->OnMontageEnded.RemoveDynamic(this, &ARACharacter::OnPunchMontageEnded);
	AnimInstance->OnMontageEnded.AddDynamic(this, &ARACharacter::OnPunchMontageEnded);

	UE_LOG(LogTemplateCharacter, Warning, TEXT("Combo 1 Start"));
}

void ARACharacter::QueueComboInput()
{
	if (!bIsPunching)
		return;

	if (!bCanAcceptComboInput)
	{
		UE_LOG(LogTemplateCharacter, Warning, TEXT("Combo input ignored: not in combo window"));
		return;
	}

	if (CurrentComboIndex >= MaxComboCount)
	{
		UE_LOG(LogTemplateCharacter, Warning, TEXT("Already at max combo"));
		return;
	}

	bComboInputBuffered = true;
	UE_LOG(LogTemplateCharacter, Warning, TEXT("Combo input buffered"));
}
#pragma endregion Punch Attack Func

#pragma region Sword Attack Func
void ARACharacter::PerformSwordHit(float damage, float range, float radius)
{
	if (!GetWorld())
		return;

	const FVector Start = GetActorLocation() + FVector(0.f, 0.f, 50.f);
	const FVector End = Start + (GetActorForwardVector() * range);

	FCollisionShape Sphere = FCollisionShape::MakeSphere(radius);

	FCollisionQueryParams QueryParams;
	QueryParams.AddIgnoredActor(this);

	FCollisionObjectQueryParams ObjectQueryParams;
	ObjectQueryParams.AddObjectTypesToQuery(ECC_Pawn);
	TArray<FHitResult> HitResults;
	GetWorld()->SweepMultiByObjectType(
		HitResults,
		Start,
		End,
		FQuat::Identity,
		ObjectQueryParams,
		Sphere,
		QueryParams
	);

	if (bDrawAttackDebug)
	{
		const bool bHitEnemy = HitResults.ContainsByPredicate([this](const FHitResult& HitResult)
			{
				return IsValidPlayerAttackTarget(HitResult.GetActor());
			});
		const FColor DebugColor = bHitEnemy ? FColor::Red : FColor::Green;
		DrawDebugCapsule(
			GetWorld(),
			(Start + End) * 0.5f,
			range * 0.5f,
			radius,
			FRotationMatrix::MakeFromX(End - Start).ToQuat(),
			DebugColor,
			false,
			1.5f
		);
	}

	for (const FHitResult& HitResult : HitResults)
	{
		AActor* HitActor = HitResult.GetActor();
		if (!IsValidPlayerAttackTarget(HitActor))
		{
			UE_LOG(LogTemplateCharacter, Warning, TEXT("Sword ignored non-enemy hit: %s"), *GetNameSafe(HitActor));
			continue;
		}

		UE_LOG(LogTemplateCharacter, Warning, TEXT("Hit Enemy: %s"), *HitActor->GetName());

		SpawnHitVFX(
			SwordHitVFX,
			HitResult.ImpactPoint,
			GetActorRotation(),
			SwordHitColor,
			SwordHitScale,
			SwordHitLifetime
		);

		if (SwordHitSound)
		{
			UGameplayStatics::PlaySoundAtLocation(
				this,
				SwordHitSound,
				HitResult.ImpactPoint
			);
		}

		UGameplayStatics::ApplyDamage(
			HitActor,
			damage,
			GetController(),
			this,
			UDamageType::StaticClass()
		);

		break;
	}

	TestUsePotion();
}
#pragma endregion Sword Attack Func

#pragma region Bow Attack Func
void ARACharacter::OnAttackPressed()
{
	if (IsPortalTransitionInputLocked())
		return;

	if (StatComponent && StatComponent->IsDead())
		return;

	if (const ARAPlayerController* RAPlayerController = Cast<ARAPlayerController>(GetController()))
	{
		if (RAPlayerController->IsInventoryOpen() || RAPlayerController->IsShopOpen() || RAPlayerController->IsAnimalCollectionOpen())
			return;
	}

	if (bIsDodging)
		return;

	if (CurrentWeapon && CurrentWeapon->AttackType == EAttackType::Ranged)
	{
		if (!HasArrowAmmo())
		{
			UE_LOG(LogTemp, Warning, TEXT("Bow attack blocked: no Arrow ammo"));
			return;
		}

		StartBowCharge();
		return;
	}

	Attack();
}

void ARACharacter::OnAttackReleased()
{
	if (IsPortalTransitionInputLocked())
		return;

	if (StatComponent && StatComponent->IsDead())
		return;

	if (const ARAPlayerController* RAPlayerController = Cast<ARAPlayerController>(GetController()))
	{
		if (RAPlayerController->IsShopOpen())
			return;
	}

	if (bIsBowCharging)
		ReleaseBowCharge();
}

void ARACharacter::StartBowCharge()
{
	if (!CurrentWeapon || CurrentWeapon->AttackType != EAttackType::Ranged)
		return;

	if (!HasArrowAmmo())
	{
		UE_LOG(LogTemp, Warning, TEXT("StartBowCharge failed: no Arrow ammo"));
		return;
	}

	if (bIsAttacking || bIsBowCharging || bIsBowAiming)
		return;

	if (!CurrentWeapon->AttackMontage || !GetMesh() || !GetMesh()->GetAnimInstance())
		return;

	bIsAttacking = true;
	bIsBowCharging = true;
	bIsBowAiming = true;
	CachedBowChargeAlpha = 0.0f;
	BowChargeStartTime = GetWorld()->GetTimeSeconds();

	if (PlayerSkillComponent)
	{
		PlayerSkillComponent->CancelBowSkillPreparation();
	}

	GetCharacterMovement()->MaxWalkSpeed = 100.f;

	if (CrosshairWidgetInstance) // 조준선 위젯이 있다면 보이도록 설정하고 초기 알파값을 0으로 설정
	{
		CrosshairWidgetInstance->SetCrosshairVisible(true);
		CrosshairWidgetInstance->SetChargeAlpha(0.0f);
	}

	UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance();
	AnimInstance->Montage_Play(CurrentWeapon->AttackMontage);
	AnimInstance->Montage_JumpToSection(FName("Drawing"), CurrentWeapon->AttackMontage);

	PlayBowWeaponMontageSection(FName("Default"));

	if (BowDrawSound)
	{
		UGameplayStatics::PlaySoundAtLocation(
			this,
			BowDrawSound,
			GetActorLocation()
		);
	}

	ShowPreviewArrow();

	UE_LOG(LogTemp, Warning, TEXT("Bow Charge Start"));
}

void ARACharacter::ReleaseBowCharge()
{
	if (!bIsBowCharging)
		return;

	if (!CurrentWeapon || CurrentWeapon->AttackType != EAttackType::Ranged)
	{
		EndAttack();
		bIsBowCharging = false;
		return;
	}

	if (!CurrentWeapon->AttackMontage || !GetMesh() || !GetMesh()->GetAnimInstance())
	{
		EndAttack();
		bIsBowCharging = false;
		return;
	}

	bIsBowCharging = false;
	ApplyMovementStats();

	const float CurrentTime = GetWorld()->GetTimeSeconds();
	const float ChargeDuration = CurrentTime - BowChargeStartTime;

	const float ClampedCharge = FMath::Clamp(ChargeDuration, MinBowChargeTime, MaxBowChargeTime);

	CachedBowChargeAlpha =
		(MaxBowChargeTime > MinBowChargeTime)
		? (ClampedCharge - MinBowChargeTime) / (MaxBowChargeTime - MinBowChargeTime)
		: 1.0f;

	CachedBowChargeAlpha = FMath::Clamp(CachedBowChargeAlpha, 0.0f, 1.0f);

	UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance();
	AnimInstance->Montage_Play(CurrentWeapon->AttackMontage);
	AnimInstance->Montage_JumpToSection(FName("Releasing"), CurrentWeapon->AttackMontage);

	PlayBowWeaponMontageSection(FName("Release"));

	UE_LOG(LogTemp, Warning, TEXT("Bow Charge Released | Alpha=%.2f"), CachedBowChargeAlpha);
}

void ARACharacter::UpdateBowFacing(float DeltaTime)
{
	if (!Controller)
		return;

	const FRotator ControlRot = Controller->GetControlRotation();
	const FRotator TargetRot(0.0f, ControlRot.Yaw, 0.0f);
	SetActorRotation(TargetRot);
}

bool ARACharacter::HasArrowAmmo() const
{
	return InventoryComponent && InventoryComponent->HasItem(TEXT("Arrow"), 1);
}

bool ARACharacter::ConsumeArrowAmmo()
{
	return InventoryComponent && InventoryComponent->RemoveItem(TEXT("Arrow"), 1);
}

void ARACharacter::RefundArrowAmmo()
{
	if (InventoryComponent)
	{
		InventoryComponent->AddItem(TEXT("Arrow"), 1);
	}
}

void ARACharacter::FireChargedArrow()
{
	if (!CurrentWeapon || CurrentWeapon->AttackType != EAttackType::Ranged)
		return;

	TSubclassOf<AArrowProjectile> ProjectileClassToSpawn = CurrentWeapon->ProjectileClass;
	const bool bUseBowSkillProjectile = PlayerSkillComponent && PlayerSkillComponent->IsBowSkillPrepared();
	if (bUseBowSkillProjectile)
	{
		ProjectileClassToSpawn = PlayerSkillComponent->GetPreparedBowProjectileClass();
	}

	if (!ProjectileClassToSpawn)
	{
		if (PlayerSkillComponent)
		{
			PlayerSkillComponent->CancelBowSkillPreparation();
		}
		UE_LOG(LogTemp, Warning, TEXT("FireChargedArrow: ProjectileClassToSpawn is null"));
		return;
	}

	APlayerController* PC = Cast<APlayerController>(GetController());
	if (!PC)
		return;

	if (!GetMesh())
		return;

	HidePreviewArrow();

	int32 ViewportSizeX = 0;
	int32 ViewportSizeY = 0;
	PC->GetViewportSize(ViewportSizeX, ViewportSizeY);

	const FVector2D ScreenCenter(
		ViewportSizeX * 0.5f,
		ViewportSizeY * 0.5f
	);

	FVector WorldLocation;
	FVector WorldDirection;
	const bool bDeprojected = PC->DeprojectScreenPositionToWorld(
		ScreenCenter.X,
		ScreenCenter.Y,
		WorldLocation,
		WorldDirection
	);

	if (!bDeprojected)
	{
		UE_LOG(LogTemp, Warning, TEXT("FireChargedArrow: Deproject failed"));
		return;
	}

	const FVector TraceStart = WorldLocation;
	const FVector TraceEnd = TraceStart + (WorldDirection * 10000.0f);

	FHitResult HitResult;
	FCollisionQueryParams QueryParams;
	QueryParams.AddIgnoredActor(this);
	if (CurrentWeapon)
	{
		QueryParams.AddIgnoredActor(CurrentWeapon);
	}

	const bool bHit = GetWorld()->LineTraceSingleByChannel(
		HitResult,
		TraceStart,
		TraceEnd,
		ECC_Visibility,
		QueryParams
	);

	const FVector AimTargetLocation = bHit ? HitResult.ImpactPoint : TraceEnd;

	FVector SpawnLocation = FVector::ZeroVector;

	// 무기 Skeletal Mesh의 ArrowSpawnSocket
	if (CurrentWeapon->UsesSkeletalMesh() &&
		CurrentWeapon->WeaponSkeletalMesh &&
		CurrentWeapon->WeaponSkeletalMesh->DoesSocketExist(TEXT("ArrowSpawnSocket")))
	{
		SpawnLocation = CurrentWeapon->WeaponSkeletalMesh->GetSocketLocation(TEXT("ArrowSpawnSocket"));
	}
	// 무기 Static Mesh의 ArrowSpawnSocket
	else if (CurrentWeapon->WeaponMesh &&
		CurrentWeapon->WeaponMesh->DoesSocketExist(TEXT("ArrowSpawnSocket")))
	{
		SpawnLocation = CurrentWeapon->WeaponMesh->GetSocketLocation(TEXT("ArrowSpawnSocket"));
	}
	// 캐릭터 왼손 소켓
	else if (GetMesh()->DoesSocketExist(TEXT("LeftHandSocket")))
	{
		SpawnLocation = GetMesh()->GetSocketLocation(TEXT("LeftHandSocket"));
	}
	// fallback
	else
	{
		SpawnLocation = GetActorLocation() + GetActorForwardVector() * 50.0f + FVector(0.0f, 0.0f, 50.0f);
		UE_LOG(LogTemp, Warning, TEXT("FireChargedArrow: LeftHandSocket not found, using fallback location"));
	}

	const FVector ShootDirection = (AimTargetLocation - SpawnLocation).GetSafeNormal();
	const FRotator SpawnRotation = ShootDirection.Rotation();

	if (!ConsumeArrowAmmo())
	{
		HidePreviewArrow();
		if (PlayerSkillComponent)
		{
			PlayerSkillComponent->CancelBowSkillPreparation();
		}
		UE_LOG(LogTemp, Warning, TEXT("FireChargedArrow failed: no Arrow ammo"));
		return;
	}

	FActorSpawnParameters SpawnParams;
	SpawnParams.Owner = this;
	SpawnParams.Instigator = this;

	AArrowProjectile* Arrow = GetWorld()->SpawnActor<AArrowProjectile>(
		ProjectileClassToSpawn,
		SpawnLocation,
		SpawnRotation,
		SpawnParams
	);

	if (!Arrow)
	{
		RefundArrowAmmo();
		if (PlayerSkillComponent)
		{
			PlayerSkillComponent->CancelBowSkillPreparation();
		}
		return;
	}

	if (bUseBowSkillProjectile && PlayerSkillComponent)
	{
		PlayerSkillComponent->ApplyBowSkillHitEffects(Arrow);
		PlayerSkillComponent->CommitBowSkillRelease();
	}

	const float DamageMultiplier = FMath::Lerp(
		MinBowDamageMultiplier,
		MaxBowDamageMultiplier,
		CachedBowChargeAlpha
	);

	const float SpeedMultiplier = FMath::Lerp(
		MinBowSpeedMultiplier,
		MaxBowSpeedMultiplier,
		CachedBowChargeAlpha
	);

	float FinalBaseDamage = CurrentWeapon->AttackDamage;

	if (StatComponent)
	{
		FinalBaseDamage = StatComponent->GetFinalAttackPower(FinalBaseDamage);
	}

	Arrow->Damage = FinalBaseDamage * DamageMultiplier;

	if (Arrow->ProjectileMovement)
	{
		const float FinalSpeed = CurrentWeapon->ProjectileSpeed * SpeedMultiplier;
		Arrow->ProjectileMovement->InitialSpeed = FinalSpeed;
		Arrow->ProjectileMovement->MaxSpeed = FinalSpeed;
		Arrow->ProjectileMovement->Velocity = ShootDirection * FinalSpeed;
	}

#if WITH_EDITOR
	if (bDrawAttackDebug)
	{
		// DrawDebugLine(GetWorld(), TraceStart, AimTargetLocation, FColor::Green, false, 1.5f, 0, 1.5f);
		DrawDebugSphere(GetWorld(), AimTargetLocation, 12.0f, 12, FColor::Red, false, 1.5f);
		DrawDebugLine(GetWorld(), SpawnLocation, AimTargetLocation, FColor::Yellow, false, 1.5f, 0, 1.5f);
	}
#endif

	UE_LOG(LogTemp, Warning, TEXT("Charged Arrow Fired | Alpha=%.2f Damage=%.1f Spawn=%s"),
		CachedBowChargeAlpha,
		Arrow->Damage,
		*SpawnLocation.ToString());
}

void ARACharacter::UpdateBowZoom(float DeltaTime)
{
	if (!FollowCamera)
		return;

	const float TargetFOV = bIsBowAiming ? BowZoomFOV : DefaultFOV;

	const float NewFOV = FMath::FInterpTo(
		FollowCamera->FieldOfView,
		TargetFOV,
		DeltaTime,
		BowZoomInterpSpeed
	);

	FollowCamera->SetFieldOfView(NewFOV);
}

void ARACharacter::UpdateBowCameraArm(float DeltaTime)
{
	if (!CameraBoom)
		return;

	const float TargetArm = bIsBowAiming ? BowZoomArmLength : DefaultArmLength;

	const float NewArmLength = FMath::FInterpTo(
		CameraBoom->TargetArmLength,
		TargetArm,
		DeltaTime,
		BowArmInterpSpeed
	);

	CameraBoom->TargetArmLength = NewArmLength;
}

void ARACharacter::EndBowAim()
{
	if (PlayerSkillComponent)
	{
		PlayerSkillComponent->CancelBowSkillPreparation();
	}

	bIsBowCharging = false;
	bIsBowAiming = false;
	bIsAttacking = false;
	CachedBowChargeAlpha = 0.0f;

	if (CrosshairWidgetInstance)
	{
		CrosshairWidgetInstance->ResetCrosshair();
		CrosshairWidgetInstance->SetCrosshairVisible(false);
	}

	HidePreviewArrow();

	UE_LOG(LogTemp, Warning, TEXT("Bow Aim End"));
}

bool ARACharacter::CanPrepareBowSkill() const
{
	return bIsBowCharging &&
		bIsBowAiming &&
		CurrentWeapon &&
		CurrentWeapon->WeaponType == EWeaponType::Bow &&
		CurrentWeapon->AttackType == EAttackType::Ranged;
}

void ARACharacter::SetBowPreviewArrowStaticMesh(UStaticMesh* NewPreviewArrowStaticMesh)
{
	if (!PreviewArrowMesh || !NewPreviewArrowStaticMesh)
	{
		return;
	}

	PreviewArrowMesh->SetStaticMesh(NewPreviewArrowStaticMesh);
	PreviewArrowMesh->SetHiddenInGame(false);
}

void ARACharacter::ResetBowPreviewArrowStaticMesh()
{
	if (!PreviewArrowMesh || !PreviewArrowStaticMesh)
	{
		return;
	}

	PreviewArrowMesh->SetStaticMesh(PreviewArrowStaticMesh);
}

void ARACharacter::SetBowPreviewArrowVFX(
	UNiagaraSystem* NewPreviewArrowVFX,
	FVector RelativeLocation,
	FRotator RelativeRotation,
	FVector RelativeScale)
{
	if (!PreviewArrowVFXComponent || !NewPreviewArrowVFX)
	{
		return;
	}

	PreviewArrowVFXComponent->SetAsset(NewPreviewArrowVFX);
	PreviewArrowVFXComponent->SetRelativeLocation(RelativeLocation);
	PreviewArrowVFXComponent->SetRelativeRotation(RelativeRotation);
	PreviewArrowVFXComponent->SetRelativeScale3D(RelativeScale);
	PreviewArrowVFXComponent->SetHiddenInGame(false);
	PreviewArrowVFXComponent->Activate(true);
}

void ARACharacter::ClearBowPreviewArrowVFX()
{
	if (!PreviewArrowVFXComponent)
	{
		return;
	}

	PreviewArrowVFXComponent->Deactivate();
	PreviewArrowVFXComponent->SetAsset(nullptr);
	PreviewArrowVFXComponent->SetHiddenInGame(true);
}
#pragma	endregion Bow Attack Func

#pragma region Anim Montage Func
void ARACharacter::TriggerMeleeHit()
{
	UE_LOG(LogTemplateCharacter, Warning, TEXT("TriggerMeleeHit"));
	float Damage = PunchDamage;
	float Range = PunchRange;
	float Radius = PunchRadius;

	if (CurrentWeapon && CurrentWeapon->AttackType == EAttackType::Melee)
	{
		Damage = CurrentWeapon->AttackDamage;
		Range = CurrentWeapon->AttackRange;
		Radius = CurrentWeapon->AttackRadius;
	}

	if (StatComponent)
	{
		Damage = StatComponent->GetFinalAttackPower(Damage);
	}

	(CurrentWeapon && CurrentWeapon->WeaponType == EWeaponType::Sword)
		? PerformSwordHit(Damage, Range, Radius) : PerformPunchHit(Damage, Range, Radius);
}

void ARACharacter::TriggerSkillHit()
{
	if (PlayerSkillComponent)
	{
		PlayerSkillComponent->TriggerSkillHit();
	}
}

void ARACharacter::NormalRelease()
{
	if (PlayerSkillComponent && PlayerSkillComponent->IsBowSkillPrepared())
	{
		return;
	}

	if (BowReleaseSound)
	{
		UGameplayStatics::PlaySoundAtLocation(
			this,
			BowReleaseSound,
			GetActorLocation()
		);
	}
}

void ARACharacter::SkillRelease()
{
	if (PlayerSkillComponent)
	{
		PlayerSkillComponent->PlayBowSkillReleaseSound();
	}
}

void ARACharacter::ProceedCombo()
{
	if (!PunchMontage || !GetMesh() || !GetMesh()->GetAnimInstance())
		return;
	if (!bComboInputBuffered)
		return;
	if (CurrentComboIndex >= MaxComboCount)
		return;

	CurrentComboIndex++;
	bComboInputBuffered = false;
	bCanAcceptComboInput = false;

	FaceAttackDirection();

	UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance();
	const FName NextSectionName = FName(*FString::Printf(TEXT("Combo%d"), CurrentComboIndex));

	AnimInstance->Montage_JumpToSection(NextSectionName, PunchMontage);

	UE_LOG(LogTemplateCharacter, Warning, TEXT("Proceed to %s"), *NextSectionName.ToString());
}

void ARACharacter::EnableComboWindow()
{
	bCanAcceptComboInput = true;
	UE_LOG(LogTemplateCharacter, Warning, TEXT("Combo Window Open"));
}

void ARACharacter::DisableComboWindow()
{
	bCanAcceptComboInput = false;
	UE_LOG(LogTemplateCharacter, Warning, TEXT("Combo Window Closed"));
}

void ARACharacter::OnPunchMontageEnded(UAnimMontage* Montage, bool bInterrupted)
{
	if (Montage != PunchMontage)
		return;

	bIsPunching = false;
	bIsAttacking = false;
	bComboInputBuffered = false;
	bCanAcceptComboInput = false;
	CurrentComboIndex = 0;

	UE_LOG(LogTemplateCharacter, Warning, TEXT("Punch Montage Ended"));
}

void ARACharacter::OnDodgeMontageEnded(UAnimMontage* Montage, bool bInterrupted)
{
	if (Montage != CurrentDodgeMontage)
		return;

	if (UAnimInstance* AnimInstance = GetMesh() ? GetMesh()->GetAnimInstance() : nullptr)
	{
		AnimInstance->OnMontageEnded.RemoveDynamic(this, &ARACharacter::OnDodgeMontageEnded);
	}

	EndDodge();
}
void ARACharacter::PlayBowWeaponMontageSection(FName SectionName)
{
	if (!CurrentWeapon)
		return;

	if (!CurrentWeapon->UsesSkeletalMesh())
		return;

	if (!CurrentWeapon->WeaponSkeletalMesh)
		return;

	if (!CurrentWeapon->WeaponAnimMontage)
		return;

	UAnimInstance* WeaponAnimInstance = CurrentWeapon->WeaponSkeletalMesh->GetAnimInstance();
	if (!WeaponAnimInstance)
	{
		UE_LOG(LogTemp, Warning, TEXT("PlayBowWeaponMontageSection: WeaponAnimInstance is null"));
		return;
	}

	WeaponAnimInstance->Montage_Play(CurrentWeapon->WeaponAnimMontage);
	WeaponAnimInstance->Montage_JumpToSection(SectionName, CurrentWeapon->WeaponAnimMontage);

	UE_LOG(LogTemp, Warning, TEXT("Bow Weapon Montage Section: %s"), *SectionName.ToString());
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

#pragma region Delicate Func
EWeaponType ARACharacter::GetCurrentWeaponType() const
{
	if (!CurrentWeapon)
	{
		return EWeaponType::None;
	}

	return CurrentWeapon->WeaponType;
}

FName ARACharacter::GetCurrentWeaponItemID() const
{
	return CurrentWeapon ? CurrentWeapon->WeaponID : NAME_None;
}
#pragma endregion Delicate Func

bool ARACharacter::CanStartSkillAction(bool bAllowBowAiming) const
{
	if (IsPortalTransitionInputLocked())
		return false;

	if (StatComponent && StatComponent->IsDead())
		return false;

	if (const ARAPlayerController* RAPlayerController = Cast<ARAPlayerController>(GetController()))
	{
		if (RAPlayerController->IsInventoryOpen() || RAPlayerController->IsShopOpen() || RAPlayerController->IsAnimalCollectionOpen())
			return false;
	}

	if (bIsDodging)
		return false;

	const bool bBowAimException = bAllowBowAiming && bIsBowAiming;
	if (bIsAttacking && !bBowAimException)
		return false;

	return true;
}

void ARACharacter::BeginSkillAction()
{
	StopHitMontage();
	bIsAttacking = true;
}

void ARACharacter::EndSkillAction()
{
	bIsAttacking = false;
}

void ARACharacter::FaceSkillDirection()
{
	FaceAttackDirection();
}

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
	if (!NiagaraSystem)
		return;

	UNiagaraComponent* NiagaraComp = UNiagaraFunctionLibrary::SpawnSystemAtLocation(
		GetWorld(),
		NiagaraSystem,
		SpawnLocation,
		SpawnRotation,
		FVector(1.0f),
		true,
		true,
		ENCPoolMethod::None,
		true
	);

	if (!NiagaraComp)
		return;

	NiagaraComp->SetVariableLinearColor(TEXT("Color"), Color);
	NiagaraComp->SetVariableFloat(TEXT("Scale"), Scale);
	NiagaraComp->SetVariableFloat(TEXT("Lifetime"), Lifetime);
}
#pragma endregion VFX Func

#pragma region UI Func
void ARACharacter::ResetBowCrosshairUI()
{
	if (CrosshairWidgetInstance)
	{
		CrosshairWidgetInstance->ResetCrosshair();
		CrosshairWidgetInstance->SetCrosshairVisible(false);
	}
}
#pragma endregion UI Func

#pragma region Show & Hide Func
void ARACharacter::ShowPreviewArrow()
{
	if (!PreviewArrowMesh || !PreviewArrowStaticMesh || !CurrentWeapon)
		return;

	PreviewArrowMesh->SetStaticMesh(PreviewArrowStaticMesh);

	if (CurrentWeapon->UsesSkeletalMesh() &&
		CurrentWeapon->WeaponSkeletalMesh &&
		CurrentWeapon->WeaponSkeletalMesh->DoesSocketExist(TEXT("ArrowSocket")))
	{
		PreviewArrowMesh->AttachToComponent(
			CurrentWeapon->WeaponSkeletalMesh,
			FAttachmentTransformRules::SnapToTargetNotIncludingScale,
			TEXT("ArrowSocket")
		);
	}
	else if (GetMesh()->DoesSocketExist(TEXT("LeftHandSocket")))
	{
		PreviewArrowMesh->AttachToComponent(
			GetMesh(),
			FAttachmentTransformRules::SnapToTargetNotIncludingScale,
			TEXT("LeftHandSocket")
		);
	}
	else
	{
		PreviewArrowMesh->AttachToComponent(
			GetMesh(),
			FAttachmentTransformRules::SnapToTargetNotIncludingScale
		);
	}

	PreviewArrowMesh->SetRelativeLocation(FVector::ZeroVector);
	PreviewArrowMesh->SetRelativeRotation(FRotator::ZeroRotator);
	PreviewArrowMesh->SetRelativeScale3D(FVector(1.0f));
	PreviewArrowMesh->SetHiddenInGame(false);
}

void ARACharacter::HidePreviewArrow()
{
	if (!PreviewArrowMesh)
		return;

	ClearBowPreviewArrowVFX();

	PreviewArrowMesh->SetHiddenInGame(true);
	PreviewArrowMesh->DetachFromComponent(FDetachmentTransformRules::KeepWorldTransform);
}
#pragma endregion Show & Hide Func

#pragma region Interaction Function
void ARACharacter::SetCurrentPortal(APortalActor* NewPortal)
{
	CurrentPortal = NewPortal;
	UE_LOG(LogTemp, Warning, TEXT("Current Portal Set"));
}

void ARACharacter::ClearCurrentPortal(APortalActor* PortalToClear)
{
	if (CurrentPortal == PortalToClear)
	{
		CurrentPortal = nullptr;
		UE_LOG(LogTemp, Warning, TEXT("Current Portal Cleared"));
	}
}

void ARACharacter::SetCurrentShop(AShopActor* NewShop)
{
	CurrentShop = NewShop;
}

void ARACharacter::ClearCurrentShop(AShopActor* ShopToClear)
{
	if (CurrentShop == ShopToClear)
	{
		CurrentShop = nullptr;
	}
}

void ARACharacter::SetCurrentLobbyNPC(ALobbyNPC* NewLobbyNPC)
{
	CurrentLobbyNPC = NewLobbyNPC;
}

void ARACharacter::ClearCurrentLobbyNPC(ALobbyNPC* LobbyNPCToClear)
{
	if (CurrentLobbyNPC == LobbyNPCToClear)
	{
		CurrentLobbyNPC = nullptr;
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

void ARACharacter::TestTakeDamage(float DamageAmount)
{
	if (!StatComponent)
	{
		return;
	}

	StatComponent->ApplyDamage(DamageAmount);

	UE_LOG(LogTemplateCharacter, Warning, TEXT("TestTakeDamage: %.1f | Current HP: %.1f / %.1f"),
		DamageAmount,
		StatComponent->GetCurrentHP(),
		StatComponent->GetMaxHP());
}

void ARACharacter::TestHeal(float HealAmount)
{
	if (!StatComponent)
	{
		return;
	}

	StatComponent->Heal(HealAmount);

	UE_LOG(LogTemplateCharacter, Warning, TEXT("TestHeal: %.1f | Current HP: %.1f / %.1f"),
		HealAmount,
		StatComponent->GetCurrentHP(),
		StatComponent->GetMaxHP());
}

void ARACharacter::TestAddEXP(int32 EXPAmount)
{
	if (!StatComponent)
	{
		return;
	}

	StatComponent->AddEXP(EXPAmount);

	UE_LOG(LogTemplateCharacter, Warning, TEXT("TestAddEXP: %d | Level: %d | EXP: %d / %d"),
		EXPAmount,
		StatComponent->GetLevel(),
		StatComponent->GetCurrentEXP(),
		StatComponent->GetRequiredEXP());
}

void ARACharacter::TestAddItem(FName ItemID, int32 Count)
{
	if (!InventoryComponent)
	{
		return;
	}

	InventoryComponent->AddItem(ItemID, Count);

	UE_LOG(LogTemplateCharacter, Warning, TEXT("AddTestItem: %s x%d"),
		*ItemID.ToString(),
		Count);
}

bool ARACharacter::TestUseItem(FName ItemID, int32 Count)
{
	if (!InventoryComponent)
	{
		return false;
	}

	const bool bResult = InventoryComponent->RemoveItem(ItemID, Count);

	UE_LOG(LogTemplateCharacter, Warning, TEXT("UseTestItem: %s x%d | Result: %s | Remaining: %d"),
		*ItemID.ToString(),
		Count,
		bResult ? TEXT("True") : TEXT("False"),
		InventoryComponent->GetItemCount(ItemID));

	return bResult;
}

bool ARACharacter::TestUsePotion()
{
	if (!InventoryComponent || !StatComponent)
	{
		return false;
	}

	URAGameInstance* RAGameInstance = GetGameInstance<URAGameInstance>();
	if (!RAGameInstance)
	{
		UE_LOG(LogTemplateCharacter, Warning, TEXT("UsePotion Failed: RAGameInstance is null"));
		return false;
	}

	FItemData ItemData;
	if (!RAGameInstance->GetItemDataByID(TEXT("Potion"), ItemData))
	{
		UE_LOG(LogTemplateCharacter, Warning, TEXT("UsePotion Failed: ItemData not found for Potion"));
		return false;
	}

	if (!InventoryComponent->RemoveItem(TEXT("Potion"), 1))
	{
		UE_LOG(LogTemplateCharacter, Warning, TEXT("UsePotion Failed: No Potion in inventory"));
		return false;
	}

	StatComponent->Heal(ItemData.HealAmount);

	UE_LOG(LogTemplateCharacter, Warning, TEXT("UsePotion Success | HealAmount: %.1f | HP: %.1f / %.1f"),
		ItemData.HealAmount,
		StatComponent->GetCurrentHP(),
		StatComponent->GetMaxHP());

	return true;
}
