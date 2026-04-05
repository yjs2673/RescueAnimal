#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "Logging/LogMacros.h"

#include "Animation/AnimMontage.h"

#include "TPSCaptureCharacter.generated.h"

class USpringArmComponent;
class UCameraComponent;
class UInputMappingContext;
class UInputAction;
struct FInputActionValue;

class AWeaponBase;

#pragma region Interactive Object
class APortalActor;
#pragma endregion Interactive Object

DECLARE_LOG_CATEGORY_EXTERN(LogTemplateCharacter, Log, All);

UCLASS(config=Game)
class ATPSCaptureCharacter : public ACharacter
{
	GENERATED_BODY()

#pragma region Camera Mapping
	/** Camera positioning */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Camera, meta = (AllowPrivateAccess = "true"))
	USpringArmComponent* CameraBoom;

	/** Follow camera */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Camera, meta = (AllowPrivateAccess = "true"))
	UCameraComponent* FollowCamera;
	
	/** MappingContext */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
	UInputMappingContext* DefaultMappingContext;
#pragma endregion Camera Mapping

#pragma region Input Action
	/** Jump Input Action */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
	UInputAction* JumpAction;

	/** Move Input Action */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
	UInputAction* MoveAction;

	/** Look Input Action */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
	UInputAction* LookAction;

	/** Interact Input Action */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
	UInputAction* InteractAction;

	/** Attack Input Action */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
	UInputAction* AttackAction;
#pragma endregion Input Action

#pragma region Constructor & Tick & Begin
public:
	ATPSCaptureCharacter();
	virtual void Tick(float DeltaTime) override;
protected:
	virtual void BeginPlay() override;
#pragma endregion Constructor & Tick & Begin

/* Variations */
public:
#pragma region Equip Var
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Weapon") // 무기 장착 소켓
	FName WeaponSocketName = TEXT("RightHandSocket");

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Weapon")	// 현재 장착 무기
	AWeaponBase* CurrentWeapon;

	UPROPERTY(EditDefaultsOnly, Category = "Weapon") // 시작할 때 지급할 무기 클래스 (테스트용)
	TSubclassOf<AWeaponBase> StarterWeaponClass;
#pragma endregion Equip Var
	
protected:
#pragma region Combat Var
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Combat")
	bool bIsAttacking = false;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "State")
	bool bIsPunching = false;

#pragma region Punch Var
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Combat") // 펀치 데미지
	float PunchDamage = 20.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Combat") // 앞으로 얼마나 검사할지
	float PunchRange = 150.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Combat") // 판정 크기
	float PunchRadius = 50.0f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Combat")
	bool bComboInputBuffered = false;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Combat")
	bool bCanAcceptComboInput = false;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Combat")
	int32 CurrentComboIndex = 0;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Combat")
	int32 MaxComboCount = 3;
#pragma endregion Punch Var

#pragma endregion Combat Var

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Bow")
	bool bIsBowCharging = false;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Bow")
	float BowChargeStartTime = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Bow")
	float MinBowChargeTime = 0.1f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Bow")
	float MaxBowChargeTime = 1.5f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Bow")
	float MinBowDamageMultiplier = 0.7f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Bow")
	float MaxBowDamageMultiplier = 1.8f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Bow")
	float MinBowSpeedMultiplier = 0.7f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Bow")
	float MaxBowSpeedMultiplier = 1.8f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Bow")
	float CachedBowChargeAlpha = 0.0f;	// 차징값
#pragma region Montage & Interaction Var
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Animation", meta = (AllowPrivateAccess = "true"))
	UAnimMontage* PunchMontage;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Animation", meta = (AllowPrivateAccess = "true"))
	UAnimMontage* AttackMontage;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Portal")
	APortalActor* CurrentPortal = nullptr;
#pragma endregion Montage & Interaction Var

/* Functions */
protected:
#pragma region Input Binding Func
	virtual void NotifyControllerChanged() override;

	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

public:
	/** Returns CameraBoom subobject **/
	FORCEINLINE class USpringArmComponent* GetCameraBoom() const { return CameraBoom; }
	/** Returns FollowCamera subobject **/
	FORCEINLINE class UCameraComponent* GetFollowCamera() const { return FollowCamera; }
#pragma endregion Input Binding Func

protected:
#pragma region Base Action Func
	void Move(const FInputActionValue& Value);
	void Look(const FInputActionValue& Value);
	void Interact();
#pragma endregion Base Action Func

#pragma region Equip Func
	UFUNCTION(BlueprintCallable, Category = "Weapon") // 새로운 무기 장착, 이미 장착되어 있다면 교체
	void EquipWeapon(AWeaponBase* NewWeapon);

	UFUNCTION(BlueprintCallable, Category = "Weapon") // 장착 해제
	void UnequipWeapon();
#pragma endregion Equip Func

#pragma region Base Combat Func
	UFUNCTION(BlueprintCallable, Category = "Combat")
	void Attack();

	void AttackUnarmed();		// 맨손
	void AttackWithWeapon();	// 무기

	UFUNCTION(BlueprintCallable, Category = "Combat")
	void EndAttack();	
#pragma endregion Base Combat Func

#pragma region Punch Attack Func
	void PerformPunchHit(float damage, float range, float radius);
	void StartComboAttack();
	void QueueComboInput();
#pragma endregion Punch Attack Func

	UFUNCTION(BlueprintCallable, Category = "Combat")
	void OnAttackPressed();

	UFUNCTION(BlueprintCallable, Category = "Combat")
	void OnAttackReleased();

	void StartBowCharge();
	void ReleaseBowCharge();
	void UpdateBowFacing(float DeltaTime);

	UFUNCTION(BlueprintCallable, Category = "Combat")
	void FireChargedArrow();

#pragma region Anim Montage Func
	UFUNCTION(BlueprintCallable)
	void TriggerMeleeHit();

	UFUNCTION(BlueprintCallable)
	void ProceedCombo();

	UFUNCTION(BlueprintCallable)
	void EnableComboWindow();

	UFUNCTION(BlueprintCallable)
	void DisableComboWindow();

	UFUNCTION()
	void OnPunchMontageEnded(UAnimMontage* Montage, bool bInterrupted);

	UFUNCTION(BlueprintCallable)
	void FireArrow(); // 활
#pragma endregion Anim Montage 

public:
#pragma region Interaction Function
	void SetCurrentPortal(APortalActor* NewPortal);			// 포탈에 들어갈 때 현재 포탈 설정
	void ClearCurrentPortal(APortalActor* PortalToClear);	// 포탈에서 나올 때 현재 포탈 해제
#pragma endregion Interaction Func
};

