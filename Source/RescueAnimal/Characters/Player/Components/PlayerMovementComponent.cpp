#include "PlayerMovementComponent.h"

#include "RACharacter.h"
#include "PlayerCombatComponent.h"
#include "PlayerStatComponent.h"

#include "Animation/AnimInstance.h"
#include "GameFramework/Controller.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "InputActionValue.h"
#include "Kismet/GameplayStatics.h"
#include "Sound/SoundBase.h"

UPlayerMovementComponent::UPlayerMovementComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
}

void UPlayerMovementComponent::BeginPlay()
{
	Super::BeginPlay();
}

void UPlayerMovementComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
	UpdateDodgeMovement(DeltaTime);
}

ARACharacter* UPlayerMovementComponent::GetOwnerCharacter() const
{
	return Cast<ARACharacter>(GetOwner());
}

void UPlayerMovementComponent::StartJump()
{
	ARACharacter* Character = GetOwnerCharacter();
	if (!Character || Character->IsPortalTransitionInputLocked() || Character->bIsDodging)
		return;

	if (Character->CanJump() && Character->JumpSound)
	{
		UGameplayStatics::PlaySoundAtLocation(Character, Character->JumpSound, Character->GetActorLocation());
	}

	Character->Jump();
}

void UPlayerMovementComponent::Move(const FInputActionValue& Value)
{
	ARACharacter* Character = GetOwnerCharacter();
	if (!Character || Character->IsPortalTransitionInputLocked())
		return;

	if (Character->StatComponent && Character->StatComponent->IsDead())
		return;

	if (Character->bIsDodging)
		return;

	if (Character->PlayerCombatComponent && Character->PlayerCombatComponent->IsAttacking() && !Character->PlayerCombatComponent->IsBowCharging())
		return;

	Character->StopHitMontage();

	const FVector2D MovementVector = Value.Get<FVector2D>();
	Character->MoveInputX = MovementVector.X;
	Character->MoveInputY = MovementVector.Y;

	if (AController* Controller = Character->GetController())
	{
		const FRotator Rotation = Controller->GetControlRotation();
		const FRotator YawRotation(0, Rotation.Yaw, 0);
		const FVector ForwardDirection = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::X);
		const FVector RightDirection = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::Y);

		Character->AddMovementInput(ForwardDirection, MovementVector.Y);
		Character->AddMovementInput(RightDirection, MovementVector.X);
	}
}

void UPlayerMovementComponent::Look(const FInputActionValue& Value)
{
	ARACharacter* Character = GetOwnerCharacter();
	if (!Character || Character->IsPortalTransitionInputLocked())
		return;

	const FVector2D LookAxisVector = Value.Get<FVector2D>();
	if (Character->GetController())
	{
		Character->AddControllerYawInput(LookAxisVector.X);
		Character->AddControllerPitchInput(LookAxisVector.Y);
	}
}

void UPlayerMovementComponent::ApplyMovementStats()
{
	ARACharacter* Character = GetOwnerCharacter();
	if (!Character || !Character->StatComponent || !Character->GetCharacterMovement())
		return;

	Character->GetCharacterMovement()->JumpZVelocity = Character->StatComponent->GetFinalJumpZVelocity();

	if (!Character->PlayerCombatComponent || !Character->PlayerCombatComponent->IsBowCharging())
	{
		Character->GetCharacterMovement()->MaxWalkSpeed = Character->StatComponent->GetFinalMoveSpeed();
	}
}

bool UPlayerMovementComponent::CanDodge() const
{
	const ARACharacter* Character = GetOwnerCharacter();
	if (!Character)
		return false;

	if (Character->bIsDodging || (Character->PlayerCombatComponent && Character->PlayerCombatComponent->IsAttacking()))
		return false;

	if (Character->StatComponent && Character->StatComponent->IsDead())
		return false;

	if (!Character->GetCharacterMovement() || Character->GetCharacterMovement()->IsFalling())
		return false;

	return true;
}

void UPlayerMovementComponent::Dodge()
{
	ARACharacter* Character = GetOwnerCharacter();
	if (!Character || Character->IsPortalTransitionInputLocked())
		return;

	UE_LOG(LogTemp, Warning, TEXT("Action Dodge"));

	if (!CanDodge())
		return;

	UAnimInstance* AnimInstance = Character->GetMesh() ? Character->GetMesh()->GetAnimInstance() : nullptr;
	if (!AnimInstance || !Character->DodgeMontage)
		return;

	Character->StopHitMontage();

	const float DodgeMontageDuration = AnimInstance->Montage_Play(Character->DodgeMontage);
	if (DodgeMontageDuration <= 0.0f)
		return;

	Character->bIsDodging = true;
	Character->CurrentDodgeMontage = Character->DodgeMontage;
	Character->DodgeElapsedTime = 0.0f;
	Character->DodgeDuration = DodgeMontageDuration;
	Character->DodgeDirection = Character->GetActorForwardVector();
	Character->DodgeDirection.Z = 0.0f;
	Character->DodgeDirection.Normalize();

	if (Character->DodgeSound)
	{
		UGameplayStatics::PlaySoundAtLocation(Character, Character->DodgeSound, Character->GetActorLocation());
	}

	AnimInstance->OnMontageEnded.RemoveDynamic(this, &UPlayerMovementComponent::OnDodgeMontageEnded);
	AnimInstance->OnMontageEnded.AddDynamic(this, &UPlayerMovementComponent::OnDodgeMontageEnded);
}

bool UPlayerMovementComponent::IsDodging() const
{
	const ARACharacter* Character = GetOwnerCharacter();
	return Character && Character->bIsDodging;
}

void UPlayerMovementComponent::UpdateDodgeMovement(float DeltaTime)
{
	ARACharacter* Character = GetOwnerCharacter();
	if (!Character || !Character->bIsDodging || Character->DodgeDuration <= 0.0f || Character->DodgeDirection.IsNearlyZero())
		return;

	const float RemainingTime = FMath::Max(Character->DodgeDuration - Character->DodgeElapsedTime, 0.0f);
	const float MoveDeltaTime = FMath::Min(DeltaTime, RemainingTime);
	const float MoveDistance = (Character->DodgeDistance / Character->DodgeDuration) * MoveDeltaTime;

	FHitResult HitResult;
	Character->AddActorWorldOffset(Character->DodgeDirection * MoveDistance, true, &HitResult);

	Character->DodgeElapsedTime += MoveDeltaTime;

	if (HitResult.bBlockingHit)
	{
		Character->DodgeElapsedTime = Character->DodgeDuration;
	}
}

void UPlayerMovementComponent::EndDodge()
{
	ARACharacter* Character = GetOwnerCharacter();
	if (!Character)
		return;

	Character->bIsDodging = false;
	Character->CurrentDodgeMontage = nullptr;
	Character->DodgeElapsedTime = 0.0f;
	Character->DodgeDuration = 0.0f;
	Character->DodgeDirection = FVector::ZeroVector;
}

void UPlayerMovementComponent::OnDodgeMontageEnded(UAnimMontage* Montage, bool bInterrupted)
{
	ARACharacter* Character = GetOwnerCharacter();
	if (!Character || Montage != Character->CurrentDodgeMontage)
		return;

	if (UAnimInstance* AnimInstance = Character->GetMesh() ? Character->GetMesh()->GetAnimInstance() : nullptr)
	{
		AnimInstance->OnMontageEnded.RemoveDynamic(this, &UPlayerMovementComponent::OnDodgeMontageEnded);
	}

	EndDodge();
}
