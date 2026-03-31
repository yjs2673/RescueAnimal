#include "TPSCaptureCharacter.h"

#include "PortalActor.h"
#include "WeaponBase.h"

#include "Engine/LocalPlayer.h"
#include "Camera/CameraComponent.h"

#include "Components/CapsuleComponent.h"
#include "Components/SkeletalMeshComponent.h"

#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "GameFramework/Controller.h"

#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "InputActionValue.h"

#include "TimerManager.h"
#include "DrawDebugHelpers.h"
#include "Kismet/GameplayStatics.h"
#include "Engine/World.h"
#include "Animation/AnimInstance.h"

DEFINE_LOG_CATEGORY(LogTemplateCharacter);

ATPSCaptureCharacter::ATPSCaptureCharacter()
{
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
	GetCharacterMovement()->JumpZVelocity = 700.f;
	GetCharacterMovement()->AirControl = 0.35f;
	GetCharacterMovement()->MaxWalkSpeed = 500.f;
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

	CurrentWeapon = nullptr; // 처음에는 무기를 들고 있지 않으므로 nullptr로 초기화
}

//////////////////////////////////////////////////////////////////////////
// Input

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

		// Punching
		EnhancedInputComponent->BindAction(AttackAction, ETriggerEvent::Started, this, &ATPSCaptureCharacter::Attack);
	
		// Interacting
		EnhancedInputComponent->BindAction(InteractAction, ETriggerEvent::Started, this, &ATPSCaptureCharacter::Interact);
	}
	else
	{
		UE_LOG(LogTemplateCharacter, Error, TEXT("'%s' Failed to find an Enhanced Input component! This template is built to use the Enhanced Input system. If you intend to use the legacy system, then you will need to update this C++ file."), *GetNameSafe(this));
	}
}

void ATPSCaptureCharacter::BeginPlay()
{
	Super::BeginPlay();

	if (StarterWeaponClass)
	{
		AWeaponBase* SpawnedWeapon = GetWorld()->SpawnActor<AWeaponBase>(StarterWeaponClass);
		if (SpawnedWeapon)
		{
			EquipWeapon(SpawnedWeapon);
		}
	}
}

void ATPSCaptureCharacter::Move(const FInputActionValue& Value)
{
	if (bIsAttacking) return;

	// input is a Vector2D
	FVector2D MovementVector = Value.Get<FVector2D>();

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

void ATPSCaptureCharacter::Attack()
{
	if (bIsAttacking && !bIsPunching)
	{
		UE_LOG(LogTemp, Warning, TEXT("Already Attacking"));
		return;
	}

	if (CurrentWeapon)
	{
		AttackWithWeapon();
	}
	else
	{
		AttackUnarmed();
	}
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
		{
			EndAttack();
		}
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
	UE_LOG(LogTemp, Warning, TEXT("Attack End"));
}

void ATPSCaptureCharacter::PerformPunchHit()
{
	if (!GetWorld())
	{
		return;
	}

	const FVector Start = GetActorLocation() + FVector(0.f, 0.f, 50.f);
	const FVector End = Start + (GetActorForwardVector() * PunchRange);

	FCollisionShape Sphere = FCollisionShape::MakeSphere(PunchRadius);

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
		PunchRange * 0.5f,
		PunchRadius,
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
			PunchDamage,
			GetController(),
			this,
			UDamageType::StaticClass()
		);
	}
}

void ATPSCaptureCharacter::TriggerPunchHit()
{
	UE_LOG(LogTemplateCharacter, Warning, TEXT("TriggerPunchHit"));
	PerformPunchHit();
}

void ATPSCaptureCharacter::StartComboAttack()
{
	if (!PunchMontage || !GetMesh() || !GetMesh()->GetAnimInstance())
	{
		return;
	}

	UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance();
	if (!AnimInstance)
	{
		return;
	}

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
	{
		return;
	}

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

void ATPSCaptureCharacter::ProceedCombo()
{
	if (!PunchMontage || !GetMesh() || !GetMesh()->GetAnimInstance())
	{
		return;
	}

	if (!bComboInputBuffered)
	{
		return;
	}

	if (CurrentComboIndex >= MaxComboCount)
	{
		return;
	}

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
	{
		return;
	}

	bIsPunching = false;
	bIsAttacking = false;
	bComboInputBuffered = false;
	bCanAcceptComboInput = false;
	CurrentComboIndex = 0;

	UE_LOG(LogTemplateCharacter, Warning, TEXT("Punch Montage Ended"));
}

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

void ATPSCaptureCharacter::Interact()
{
	if (CurrentPortal)
	{
		UE_LOG(LogTemp, Warning, TEXT("Interacting with Portal"));
		CurrentPortal->Interact(this);
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("No Portal to Interact With"));
	}
}

void ATPSCaptureCharacter::EquipWeapon(AWeaponBase* NewWeapon)
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

	if (CurrentWeapon->WeaponMesh)
	{
		CurrentWeapon->WeaponMesh->SetSimulatePhysics(false);
		CurrentWeapon->WeaponMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	}

	CurrentWeapon->AttachToComponent(
		GetMesh(),
		FAttachmentTransformRules::SnapToTargetNotIncludingScale,
		WeaponSocketName
	);

	UE_LOG(LogTemp, Warning, TEXT("Equipped Weapon: %s"), *CurrentWeapon->GetName());
}

void ATPSCaptureCharacter::UnequipWeapon()
{
	if (!CurrentWeapon)
	{
		return;
	}

	CurrentWeapon->DetachFromActor(FDetachmentTransformRules::KeepWorldTransform);
	CurrentWeapon->SetOwner(nullptr);

	if (CurrentWeapon->WeaponMesh)
	{
		CurrentWeapon->WeaponMesh->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
		CurrentWeapon->WeaponMesh->SetSimulatePhysics(false);
	}

	UE_LOG(LogTemp, Warning, TEXT("Unequipped Weapon: %s"), *CurrentWeapon->GetName());

	CurrentWeapon = nullptr;
}