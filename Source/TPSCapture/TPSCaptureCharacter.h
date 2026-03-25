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

class APortalActor;

DECLARE_LOG_CATEGORY_EXTERN(LogTemplateCharacter, Log, All);

UCLASS(config=Game)
class ATPSCaptureCharacter : public ACharacter
{
	GENERATED_BODY()

	/** Camera boom positioning the camera behind the character */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Camera, meta = (AllowPrivateAccess = "true"))
	USpringArmComponent* CameraBoom;

	/** Follow camera */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Camera, meta = (AllowPrivateAccess = "true"))
	UCameraComponent* FollowCamera;
	
	/** MappingContext */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
	UInputMappingContext* DefaultMappingContext;

	/** Jump Input Action */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
	UInputAction* JumpAction;

	/** Move Input Action */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
	UInputAction* MoveAction;

	/** Look Input Action */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
	UInputAction* LookAction;

	/** Punch Input Action */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
	UInputAction* PunchAction;

	/** Interact Input Action */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
	UInputAction* InteractAction;

public:
	ATPSCaptureCharacter();
	
public:
	void SetCurrentPortal(APortalActor* NewPortal);			// 포탈에 들어갈 때 현재 포탈 설정
	void ClearCurrentPortal(APortalActor* PortalToClear);	// 포탈에서 나올 때 현재 포탈 해제
	
protected:
	/** Called for movement input */
	void Move(const FInputActionValue& Value);

	/** Called for looking input */
	void Look(const FInputActionValue& Value);

	/** Called for Punch input */
	void Punch(const FInputActionValue& Value);

	/** Called for Interact input */
	void Interact();

protected:
	/** Animation montage for the punch attack */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Animation", meta = (AllowPrivateAccess = "true"))
	UAnimMontage* PunchMontage;

	/** State to track if the character is currently punching */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "State", meta = (AllowPrivateAccess = "true"))
	bool bIsPunching = false;
	//FTimerHandle PunchTimerHandle;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Combat") // 한 번 때릴 때 데미지
	float PunchDamage = 20.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Combat") // 앞으로 얼마나 검사할지
	float PunchRange = 150.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Combat") // 판정 구 크기
	float PunchRadius = 50.0f;

	FTimerHandle PunchHitTimerHandle;
	void PerformPunchHit();

	UFUNCTION(BlueprintCallable) // 애니메이션 노티파이에서 호출할 Punch 함수
	void TriggerPunchHit();

	UFUNCTION() // 애니메이션 몽타주가 끝났을 때 호출되는 함수
	void OnPunchMontageEnded(UAnimMontage* Montage, bool bInterrupted);

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Combat")
	bool bComboInputBuffered = false;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Combat")
	bool bCanAcceptComboInput = false;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Combat")
	int32 CurrentComboIndex = 0;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Combat")
	int32 MaxComboCount = 3;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Portal")
	APortalActor* CurrentPortal = nullptr;
	
protected:
	void StartComboAttack();

	void QueueComboInput();

	UFUNCTION(BlueprintCallable)
	void EnableComboWindow();

	UFUNCTION(BlueprintCallable)
	void DisableComboWindow();

	UFUNCTION(BlueprintCallable)
	void ProceedCombo();

protected:

	virtual void NotifyControllerChanged() override;

	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

public:
	/** Returns CameraBoom subobject **/
	FORCEINLINE class USpringArmComponent* GetCameraBoom() const { return CameraBoom; }
	/** Returns FollowCamera subobject **/
	FORCEINLINE class UCameraComponent* GetFollowCamera() const { return FollowCamera; }
};

