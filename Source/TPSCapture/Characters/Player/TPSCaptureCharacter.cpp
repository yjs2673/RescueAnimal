#include "TPSCaptureCharacter.h"

#include "PortalActor.h"
#include "WeaponBase.h"
#include "ArrowProjectile.h"

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

#include "TPSGameInstance.h"
#include "TPSStructTypes.h"

#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "InputActionValue.h"
#include "InputAction.h"

#include "Blueprint/UserWidget.h"
#include "CrosshairBowWidget.h"
#include "MainHUDWidget.h"
#include "TPSPlayerController.h"

#include "Sound/SoundBase.h"

#include "NiagaraFunctionLibrary.h"
#include "NiagaraComponent.h"

#include "TimerManager.h"
#include "DrawDebugHelpers.h"
#include "Kismet/GameplayStatics.h"
#include "Engine/World.h"
#include "Animation/AnimInstance.h"

DEFINE_LOG_CATEGORY(LogTemplateCharacter);

/* Constructor */
ATPSCaptureCharacter::ATPSCaptureCharacter()
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

	// 차징 시 나오는 화살: 미리보기용 StaticMeshComponent 생성 및 설정
	PreviewArrowMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("PreviewArrowMesh"));
	PreviewArrowMesh->SetupAttachment(GetMesh());
	PreviewArrowMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	PreviewArrowMesh->SetGenerateOverlapEvents(false);
	PreviewArrowMesh->SetSimulatePhysics(false);
	PreviewArrowMesh->SetHiddenInGame(true);
}

#pragma region Input Binding Func
void ATPSCaptureCharacter::NotifyControllerChanged()
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

void ATPSCaptureCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	// Set up action bindings
	if (UEnhancedInputComponent* EnhancedInputComponent = Cast<UEnhancedInputComponent>(PlayerInputComponent)) {
		
		// Jumping
		EnhancedInputComponent->BindAction(JumpAction, ETriggerEvent::Started, this, &ACharacter::Jump);
		EnhancedInputComponent->BindAction(JumpAction, ETriggerEvent::Completed, this, &ACharacter::StopJumping);

		// Moving
		EnhancedInputComponent->BindAction(MoveAction, ETriggerEvent::Triggered, this, &ATPSCaptureCharacter::Move);

		// Looking
		EnhancedInputComponent->BindAction(LookAction, ETriggerEvent::Triggered, this, &ATPSCaptureCharacter::Look);

		// Attacking
		// EnhancedInputComponent->BindAction(AttackAction, ETriggerEvent::Started, this, &ATPSCaptureCharacter::Attack);
		EnhancedInputComponent->BindAction(AttackAction, ETriggerEvent::Started, this, &ATPSCaptureCharacter::OnAttackPressed);
		EnhancedInputComponent->BindAction(AttackAction, ETriggerEvent::Completed, this, &ATPSCaptureCharacter::OnAttackReleased);
		EnhancedInputComponent->BindAction(AttackAction, ETriggerEvent::Canceled, this, &ATPSCaptureCharacter::OnAttackReleased);
		
		// Interacting
		EnhancedInputComponent->BindAction(InteractAction, ETriggerEvent::Started, this, &ATPSCaptureCharacter::Interact);

		// Dodging
		EnhancedInputComponent->BindAction(DodgeAction, ETriggerEvent::Started, this, &ATPSCaptureCharacter::Dodge);
	}
	else
	{
		UE_LOG(LogTemplateCharacter, Error, TEXT("'%s' Failed to find an Enhanced Input component! This template is built to use the Enhanced Input system. If you intend to use the legacy system, then you will need to update this C++ file."), *GetNameSafe(this));
	}
}
#pragma endregion Input Binding Func

/* Tick */
void ATPSCaptureCharacter::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	UpdateDodgeMovement(DeltaTime);

	if (bIsBowCharging) // 활을 당기는 중이라면, 활의 방향과 충전 상태를 업데이트
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
void ATPSCaptureCharacter::BeginPlay()
{
	Super::BeginPlay();

	if (StatComponent)
		StatComponent->OnDeath.AddDynamic(this, &ATPSCaptureCharacter::HandleCharacterDeath);


	if (CameraBoom)
		DefaultArmLength = CameraBoom->TargetArmLength;

	if (FollowCamera)
		DefaultFOV = FollowCamera->FieldOfView;

	if (StarterWeaponClass)
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
	if (ATPSPlayerController* TPSPlayerController = Cast<ATPSPlayerController>(GetController()))
	{
		if (UMainHUDWidget* HUDWidget = TPSPlayerController->GetMainHUDWidget())
		{
			// CrosshairWidgetInstance = HUDWidget->GetCrosshairBowWidget();
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
}

void ATPSCaptureCharacter::HandleCharacterDeath() // 플레이어 사망 처리: 공격 상태 초기화, 이동 불가, 입력 비활성화
{
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
void ATPSCaptureCharacter::Move(const FInputActionValue& Value)
{
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

void ATPSCaptureCharacter::Look(const FInputActionValue& Value)
{
	// input is a Vector2D
	FVector2D LookAxisVector = Value.Get<FVector2D>();

	if (Controller != nullptr)
	{
		// add yaw and pitch input to controller
		AddControllerYawInput(LookAxisVector.X);
		AddControllerPitchInput(LookAxisVector.Y);
	}
}

void ATPSCaptureCharacter::Interact()
{
	if (StatComponent && StatComponent->IsDead())
		return;

	if (NearbyWeapon || CurrentWeapon) // 무가 상호작용을 포탈보다 우선시
	{
		HandleWeaponInteract();
		return;
	}

	if (CurrentPortal)
	{
		UE_LOG(LogTemp, Warning, TEXT("Interacting with Portal"));
		CurrentPortal->Interact(this);
	}
}

bool ATPSCaptureCharacter::CanDodge() const
{
	if (bIsDodging || bIsAttacking)
		return false;

	if (StatComponent && StatComponent->IsDead())
		return false;

	if (!GetCharacterMovement() || GetCharacterMovement()->IsFalling())
		return false;

	return true;
}

void ATPSCaptureCharacter::Dodge()
{
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

	AnimInstance->OnMontageEnded.RemoveDynamic(this, &ATPSCaptureCharacter::OnDodgeMontageEnded);
	AnimInstance->OnMontageEnded.AddDynamic(this, &ATPSCaptureCharacter::OnDodgeMontageEnded);
}

void ATPSCaptureCharacter::UpdateDodgeMovement(float DeltaTime)
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

void ATPSCaptureCharacter::EndDodge()
{
	bIsDodging = false;
	CurrentDodgeMontage = nullptr;
	DodgeElapsedTime = 0.0f;
	DodgeDuration = 0.0f;
	DodgeDirection = FVector::ZeroVector;
}
#pragma endregion Base Action Func

#pragma region Equip Func
void ATPSCaptureCharacter::HandleWeaponInteract()
{
	if (bIsAttacking)
		return;

	if (NearbyWeapon)
	{
		if (!CurrentWeapon)
		{
			EquipWeapon(NearbyWeapon);
			NearbyWeapon = nullptr;
		}
		else
		{
			AWeaponBase* WeaponToPickup = NearbyWeapon;
			DropCurrentWeapon();
			EquipWeapon(WeaponToPickup);
			NearbyWeapon = nullptr;
		}

		if (EquipmentSound)
		{
			UGameplayStatics::PlaySoundAtLocation(
				this,
				EquipmentSound,
				GetActorLocation()
			);
		}
	}
	else
	{
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
}

void ATPSCaptureCharacter::EquipWeapon(AWeaponBase* NewWeapon) // 무기 장착, 이미 장착 중이면 교체
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

	UE_LOG(LogTemp, Warning, TEXT("Equipped Weapon: %s | Socket: %s"),
		*CurrentWeapon->GetName(),
		*AttachSocketName.ToString());
}

void ATPSCaptureCharacter::UnequipWeapon() // 장착 해제
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
}

void ATPSCaptureCharacter::DropCurrentWeapon() // 현재 장착된 무기를 떨어뜨리는 함수, 무기가 바닥에 떨어질 때의 위치와 회전을 계산하여 설정
{
	if (!CurrentWeapon)
		return;

	if (bIsBowCharging || bIsBowAiming)
	{
		EndBowAim();
		GetCharacterMovement()->MaxWalkSpeed = 500.f;
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

	UE_LOG(LogTemp, Warning, TEXT("Dropped Weapon: %s"), *WeaponToDrop->GetName());
}

void ATPSCaptureCharacter::SetNearbyWeapon(AWeaponBase* NewWeapon)
{
	NearbyWeapon = NewWeapon;
	UE_LOG(LogTemp, Warning, TEXT("Nearby Weapon Set: %s"), *GetNameSafe(NewWeapon));
}

void ATPSCaptureCharacter::ClearNearbyWeapon(AWeaponBase* WeaponToClear)
{
	if (NearbyWeapon == WeaponToClear)
	{
		NearbyWeapon = nullptr;
		UE_LOG(LogTemp, Warning, TEXT("Nearby Weapon Cleared"));
	}
}
#pragma endregion Equip Func

#pragma region Base Combat Func
void ATPSCaptureCharacter::Attack()
{
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

void ATPSCaptureCharacter::AttackUnarmed()
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

void ATPSCaptureCharacter::AttackWithWeapon()
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
				&ATPSCaptureCharacter::EndAttack,
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

void ATPSCaptureCharacter::FaceAttackDirection()
{
	if (!Controller)
		return;

	const FRotator ControlRot = Controller->GetControlRotation();
	const FRotator TargetRot(0.0f, ControlRot.Yaw, 0.0f);
	SetActorRotation(TargetRot);
}
void ATPSCaptureCharacter::EndAttack()
{
	bIsAttacking = false;
	bIsBowCharging = false;
	UE_LOG(LogTemp, Warning, TEXT("Attack End"));
}

void ATPSCaptureCharacter::FireArrow()
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

	if (Arrow)
	{
		Arrow->Damage = CurrentWeapon->AttackDamage;

		if (Arrow->ProjectileMovement)
		{
			Arrow->ProjectileMovement->InitialSpeed = CurrentWeapon->ProjectileSpeed;
			Arrow->ProjectileMovement->MaxSpeed = CurrentWeapon->ProjectileSpeed;
		}

		UE_LOG(LogTemp, Warning, TEXT("Arrow Fired"));
	}
}
#pragma endregion Base Combat Func

#pragma region Punch Attack Func
void ATPSCaptureCharacter::PerformPunchHit(float damage, float range, float radius)
{
	if (!GetWorld())
		return;

	const FVector Start = GetActorLocation() + FVector(0.f, 0.f, 50.f);
	const FVector End = Start + (GetActorForwardVector() * range);

	FCollisionShape Sphere = FCollisionShape::MakeSphere(radius);

	FCollisionQueryParams QueryParams;
	QueryParams.AddIgnoredActor(this);

	FHitResult HitResult;
	const bool bHit = GetWorld()->SweepSingleByChannel(
		HitResult,
		Start,
		End,
		FQuat::Identity,
		ECC_Pawn,
		Sphere,
		QueryParams
	);

	const FColor DebugColor = bHit ? FColor::Red : FColor::Green;
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

	if (bHit && HitResult.GetActor())
	{
		UE_LOG(LogTemplateCharacter, Warning, TEXT("Hit: %s"), *HitResult.GetActor()->GetName());

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
			HitResult.GetActor(),
			damage,
			GetController(),
			this,
			UDamageType::StaticClass()
		);
	}

	// TestAddItem("Potion", 2);
	// TestTakeDamage(20);
}

void ATPSCaptureCharacter::StartComboAttack()
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

	AnimInstance->OnMontageEnded.RemoveDynamic(this, &ATPSCaptureCharacter::OnPunchMontageEnded);
	AnimInstance->OnMontageEnded.AddDynamic(this, &ATPSCaptureCharacter::OnPunchMontageEnded);

	UE_LOG(LogTemplateCharacter, Warning, TEXT("Combo 1 Start"));
}

void ATPSCaptureCharacter::QueueComboInput()
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
void ATPSCaptureCharacter::PerformSwordHit(float damage, float range, float radius)
{
	if (!GetWorld())
		return;

	const FVector Start = GetActorLocation() + FVector(0.f, 0.f, 50.f);
	const FVector End = Start + (GetActorForwardVector() * range);

	FCollisionShape Sphere = FCollisionShape::MakeSphere(radius);

	FCollisionQueryParams QueryParams;
	QueryParams.AddIgnoredActor(this);

	FHitResult HitResult;
	const bool bHit = GetWorld()->SweepSingleByChannel(
		HitResult,
		Start,
		End,
		FQuat::Identity,
		ECC_Pawn,
		Sphere,
		QueryParams
	);

	const FColor DebugColor = bHit ? FColor::Red : FColor::Green;
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

	if (bHit && HitResult.GetActor())
	{
		UE_LOG(LogTemplateCharacter, Warning, TEXT("Hit: %s"), *HitResult.GetActor()->GetName());

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
			HitResult.GetActor(),
			damage,
			GetController(),
			this,
			UDamageType::StaticClass()
		);
	}

	TestUsePotion();
}
#pragma endregion Sword Attack Func

#pragma region Bow Attack Func
void ATPSCaptureCharacter::OnAttackPressed()
{
	if (StatComponent && StatComponent->IsDead())
		return;

	if (bIsDodging)
		return;

	if (CurrentWeapon && CurrentWeapon->AttackType == EAttackType::Ranged)
	{
		StartBowCharge();
		return;
	}

	Attack();
}

void ATPSCaptureCharacter::OnAttackReleased()
{
	if (StatComponent && StatComponent->IsDead())
		return;

	if (bIsBowCharging)
		ReleaseBowCharge();
}

void ATPSCaptureCharacter::StartBowCharge()
{
	if (!CurrentWeapon || CurrentWeapon->AttackType != EAttackType::Ranged)
		return;

	if (bIsAttacking || bIsBowCharging || bIsBowAiming)
		return;

	if (!CurrentWeapon->AttackMontage || !GetMesh() || !GetMesh()->GetAnimInstance())
		return;

	bIsAttacking = true;
	bIsBowCharging = true;
	bIsBowAiming = true;
	CachedBowChargeAlpha = 0.0f;
	BowChargeStartTime = GetWorld()->GetTimeSeconds();

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

void ATPSCaptureCharacter::ReleaseBowCharge()
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
	GetCharacterMovement()->MaxWalkSpeed = 500.f;

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

void ATPSCaptureCharacter::UpdateBowFacing(float DeltaTime)
{
	if (!Controller)
		return;

	const FRotator ControlRot = Controller->GetControlRotation();
	const FRotator TargetRot(0.0f, ControlRot.Yaw, 0.0f);
	SetActorRotation(TargetRot);
}

void ATPSCaptureCharacter::FireChargedArrow()
{
	if (!CurrentWeapon || CurrentWeapon->AttackType != EAttackType::Ranged)
		return;

	if (!CurrentWeapon->ProjectileClass)
	{
		UE_LOG(LogTemp, Warning, TEXT("FireChargedArrow: ProjectileClass is null"));
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

	// 활 Skeletal Mesh의 ArrowSpawnSocket
	if (CurrentWeapon->UsesSkeletalMesh() &&
		CurrentWeapon->WeaponSkeletalMesh &&
		CurrentWeapon->WeaponSkeletalMesh->DoesSocketExist(TEXT("ArrowSpawnSocket")))
	{
		SpawnLocation = CurrentWeapon->WeaponSkeletalMesh->GetSocketLocation(TEXT("ArrowSpawnSocket"));
	}
	// 활 Static Mesh의 ArrowSpawnSocket
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
		return;

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
	// DrawDebugLine(GetWorld(), TraceStart, AimTargetLocation, FColor::Green, false, 1.5f, 0, 1.5f);
	DrawDebugSphere(GetWorld(), AimTargetLocation, 12.0f, 12, FColor::Red, false, 1.5f);
	DrawDebugLine(GetWorld(), SpawnLocation, AimTargetLocation, FColor::Yellow, false, 1.5f, 0, 1.5f);
#endif

	UE_LOG(LogTemp, Warning, TEXT("Charged Arrow Fired | Alpha=%.2f Damage=%.1f Spawn=%s"),
		CachedBowChargeAlpha,
		Arrow->Damage,
		*SpawnLocation.ToString());
}

void ATPSCaptureCharacter::UpdateBowZoom(float DeltaTime)
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

void ATPSCaptureCharacter::UpdateBowCameraArm(float DeltaTime)
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

void ATPSCaptureCharacter::EndBowAim()
{
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
#pragma	endregion Bow Attack Func

#pragma region Anim Montage Func
void ATPSCaptureCharacter::TriggerMeleeHit()
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

void ATPSCaptureCharacter::ProceedCombo()
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

void ATPSCaptureCharacter::EnableComboWindow()
{
	bCanAcceptComboInput = true;
	UE_LOG(LogTemplateCharacter, Warning, TEXT("Combo Window Open"));
}

void ATPSCaptureCharacter::DisableComboWindow()
{
	bCanAcceptComboInput = false;
	UE_LOG(LogTemplateCharacter, Warning, TEXT("Combo Window Closed"));
}

void ATPSCaptureCharacter::OnPunchMontageEnded(UAnimMontage* Montage, bool bInterrupted)
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

void ATPSCaptureCharacter::OnDodgeMontageEnded(UAnimMontage* Montage, bool bInterrupted)
{
	if (Montage != CurrentDodgeMontage)
		return;

	if (UAnimInstance* AnimInstance = GetMesh() ? GetMesh()->GetAnimInstance() : nullptr)
	{
		AnimInstance->OnMontageEnded.RemoveDynamic(this, &ATPSCaptureCharacter::OnDodgeMontageEnded);
	}

	EndDodge();
}
void ATPSCaptureCharacter::PlayBowWeaponMontageSection(FName SectionName)
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

void ATPSCaptureCharacter::PlayHitMontage()
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

void ATPSCaptureCharacter::StopHitMontage()
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

#pragma region VFX Func
void ATPSCaptureCharacter::SpawnHitVFX(
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
void ATPSCaptureCharacter::ResetBowCrosshairUI()
{
	if (CrosshairWidgetInstance)
	{
		CrosshairWidgetInstance->ResetCrosshair();
		CrosshairWidgetInstance->SetCrosshairVisible(false);
	}
}
#pragma endregion UI Func

#pragma region Show & Hide Func
void ATPSCaptureCharacter::ShowPreviewArrow()
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

void ATPSCaptureCharacter::HidePreviewArrow()
{
	if (!PreviewArrowMesh)
		return;

	PreviewArrowMesh->SetHiddenInGame(true);
	PreviewArrowMesh->DetachFromComponent(FDetachmentTransformRules::KeepWorldTransform);
}
#pragma endregion Show & Hide Func

#pragma region Interaction Function
void ATPSCaptureCharacter::SetCurrentPortal(APortalActor* NewPortal)
{
	CurrentPortal = NewPortal;
	UE_LOG(LogTemp, Warning, TEXT("Current Portal Set"));
}

void ATPSCaptureCharacter::ClearCurrentPortal(APortalActor* PortalToClear)
{
	if (CurrentPortal == PortalToClear)
	{
		CurrentPortal = nullptr;
		UE_LOG(LogTemp, Warning, TEXT("Current Portal Cleared"));
	}
}
#pragma endregion Interaction Function

float ATPSCaptureCharacter::TakeDamage(
	float DamageAmount,
	FDamageEvent const& DamageEvent,
	AController* EventInstigator,
	AActor* DamageCauser)
{
	if (!StatComponent || StatComponent->IsDead())
		return 0.f;

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

void ATPSCaptureCharacter::TestTakeDamage(float DamageAmount)
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

void ATPSCaptureCharacter::TestHeal(float HealAmount)
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

void ATPSCaptureCharacter::TestAddEXP(int32 EXPAmount)
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

void ATPSCaptureCharacter::TestAddItem(FName ItemID, int32 Count)
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

bool ATPSCaptureCharacter::TestUseItem(FName ItemID, int32 Count)
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

bool ATPSCaptureCharacter::TestUsePotion()
{
	if (!InventoryComponent || !StatComponent)
	{
		return false;
	}

	UTPSGameInstance* TPSGameInstance = GetGameInstance<UTPSGameInstance>();
	if (!TPSGameInstance)
	{
		UE_LOG(LogTemplateCharacter, Warning, TEXT("UsePotion Failed: TPSGameInstance is null"));
		return false;
	}

	FItemData ItemData;
	if (!TPSGameInstance->GetItemDataByID(TEXT("Potion"), ItemData))
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