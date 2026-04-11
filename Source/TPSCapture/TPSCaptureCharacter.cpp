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

#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "InputActionValue.h"

#include "Blueprint/UserWidget.h"
#include "CrosshairBowWidget.h"

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
	}
	else
	{
		UE_LOG(LogTemplateCharacter, Error, TEXT("'%s' Failed to find an Enhanced Input component! This template is built to use the Enhanced Input system. If you intend to use the legacy system, then you will need to update this C++ file."), *GetNameSafe(this));
	}
}
#pragma endregion Input Binding Func

/* Begin */
void ATPSCaptureCharacter::BeginPlay()
{
	Super::BeginPlay();

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
	if (CrosshairWidgetClass)
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

/* Tick */
void ATPSCaptureCharacter::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

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

#pragma region Base Action Func
void ATPSCaptureCharacter::Move(const FInputActionValue& Value)
{
	if (bIsAttacking && !bIsBowCharging)
		return;

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
#pragma endregion Base Action Func

#pragma region Equip Func
void ATPSCaptureCharacter::EquipWeapon(AWeaponBase* NewWeapon) // 새로운 무기 장착, 이미 장착되어 있다면 교체
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
		UnequipWeapon();

	CurrentWeapon = NewWeapon;

	CurrentWeapon->SetOwner(this);
	CurrentWeapon->SetPickupEnabled(false);

	if (CurrentWeapon->WeaponMesh)
	{
		CurrentWeapon->WeaponMesh->SetSimulatePhysics(false);
		CurrentWeapon->WeaponMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	}

	CurrentWeapon->AttachToComponent(
		GetMesh(),
		FAttachmentTransformRules::SnapToTargetNotIncludingScale,
		CurrentWeapon->WeaponType == EWeaponType::Bow? LeftWeaponSocketName : RightWeaponSocketName
	);

	if (CurrentWeapon->WeaponMesh)
	{
		CurrentWeapon->WeaponMesh->SetRelativeLocation(CurrentWeapon->EquipRelativeLocation);
		CurrentWeapon->WeaponMesh->SetRelativeRotation(CurrentWeapon->EquipRelativeRotation);
		CurrentWeapon->WeaponMesh->SetRelativeScale3D(CurrentWeapon->EquipRelativeScale);
	}

	UE_LOG(LogTemp, Warning, TEXT("Equipped Weapon: %s"), *CurrentWeapon->GetName());
}

void ATPSCaptureCharacter::UnequipWeapon() // 장착 해제
{
	if (!CurrentWeapon)
		return;

	CurrentWeapon->DetachFromActor(FDetachmentTransformRules::KeepWorldTransform);
	CurrentWeapon->SetOwner(nullptr);

	if (CurrentWeapon->WeaponMesh)
	{
		CurrentWeapon->WeaponMesh->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
		CurrentWeapon->WeaponMesh->SetSimulatePhysics(false);
	}

	CurrentWeapon->SetPickupEnabled(true);

	UE_LOG(LogTemp, Warning, TEXT("Unequipped Weapon: %s"), *CurrentWeapon->GetName());

	CurrentWeapon = nullptr;
}

void ATPSCaptureCharacter::HandleWeaponInteract()
{
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
	}
	else
	{
		if (CurrentWeapon)
			DropCurrentWeapon();
	}
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
	if (bIsAttacking && !bIsPunching)
	{
		UE_LOG(LogTemp, Warning, TEXT("Already Attacking"));
		return;
	}

	if (CurrentWeapon)
		AttackWithWeapon();
	else
		AttackUnarmed();
}

void ATPSCaptureCharacter::AttackUnarmed()
{
	if (!bIsPunching)
	{
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

void ATPSCaptureCharacter::TriggerMeleeHit()
{
	UE_LOG(LogTemplateCharacter, Warning, TEXT("TriggerMeleeHit"));
	float Damage = 20.0f;
	float Range = 150.0f;
	float Radius = 50.0f;

	if (CurrentWeapon && CurrentWeapon->AttackType == EAttackType::Melee)
	{
		Damage = CurrentWeapon->AttackDamage;
		Range = CurrentWeapon->AttackRange;
		Radius = CurrentWeapon->AttackRadius;
	}

	PerformPunchHit(Damage, Range, Radius);
}

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

		UGameplayStatics::ApplyDamage(
			HitResult.GetActor(),
			damage,
			GetController(),
			this,
			UDamageType::StaticClass()
		);
	}
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

#pragma region Bow Attack Func
void ATPSCaptureCharacter::OnAttackPressed()
{
	if (CurrentWeapon && CurrentWeapon->AttackType == EAttackType::Ranged)
	{
		StartBowCharge();
		return;
	}

	Attack();
}

void ATPSCaptureCharacter::OnAttackReleased()
{
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

	// 1) 화면 중앙 좌표 구하기
	int32 ViewportSizeX = 0;
	int32 ViewportSizeY = 0;
	PC->GetViewportSize(ViewportSizeX, ViewportSizeY);

	const FVector2D ScreenCenter(
		ViewportSizeX * 0.5f,
		ViewportSizeY * 0.5f
	);

	// 2) 화면 중앙을 월드 방향으로 변환
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

	// 3) 조준선 방향으로 라인트레이스
	const FVector TraceStart = WorldLocation;
	const FVector TraceEnd = TraceStart + (WorldDirection * 10000.0f);

	FHitResult HitResult;
	FCollisionQueryParams QueryParams;
	QueryParams.AddIgnoredActor(this);
	if (CurrentWeapon)
		QueryParams.AddIgnoredActor(CurrentWeapon);

	const bool bHit = GetWorld()->LineTraceSingleByChannel(
		HitResult,
		TraceStart,
		TraceEnd,
		ECC_Visibility,
		QueryParams
	);

	// 4) 실제 조준 목표 지점
	const FVector AimTargetLocation = bHit ? HitResult.ImpactPoint : TraceEnd;

	// 5) 화살은 손/활 소켓에서 생성
	const FVector SpawnLocation = GetMesh()->GetSocketLocation(TEXT("LeftHandSocket"));

	// 6) 스폰 위치에서 조준 목표를 향하도록 회전 계산
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

	Arrow->Damage = CurrentWeapon->AttackDamage * DamageMultiplier;

	if (Arrow->ProjectileMovement)
	{
		const float FinalSpeed = CurrentWeapon->ProjectileSpeed * SpeedMultiplier;
		Arrow->ProjectileMovement->InitialSpeed = FinalSpeed;
		Arrow->ProjectileMovement->MaxSpeed = FinalSpeed;
		Arrow->ProjectileMovement->Velocity = ShootDirection * FinalSpeed;
	}

#if WITH_EDITOR
	DrawDebugLine(GetWorld(), TraceStart, AimTargetLocation, FColor::Green, false, 1.5f, 0, 1.5f);
	DrawDebugSphere(GetWorld(), AimTargetLocation, 12.0f, 12, FColor::Red, false, 1.5f);
	DrawDebugLine(GetWorld(), SpawnLocation, AimTargetLocation, FColor::Yellow, false, 1.5f, 0, 1.5f);
#endif

	UE_LOG(LogTemp, Warning, TEXT("Charged Arrow Fired | Alpha=%.2f Damage=%.1f"),
		CachedBowChargeAlpha,
		Arrow->Damage);
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

	UE_LOG(LogTemp, Warning, TEXT("Bow Aim End"));
}
#pragma	endregion Bow Attack Func

#pragma region Anim Montage Func
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
#pragma endregion Anim Montage Func

void ATPSCaptureCharacter::ResetBowCrosshairUI()
{
	if (CrosshairWidgetInstance)
	{
		CrosshairWidgetInstance->ResetCrosshair();
		CrosshairWidgetInstance->SetCrosshairVisible(false);
	}
}

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