#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "PlayerMovementComponent.generated.h"

struct FInputActionValue;
class ARACharacter;
class UAnimMontage;

UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class RESCUEANIMAL_API UPlayerMovementComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UPlayerMovementComponent();

	virtual void BeginPlay() override;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	void StartJump();
	void Move(const FInputActionValue& Value);
	void Look(const FInputActionValue& Value);
	void ApplyMovementStats();
	bool CanDodge() const;
	void Dodge();
	bool IsDodging() const;
	void UpdateDodgeMovement(float DeltaTime);
	void EndDodge();

	UFUNCTION()
	void OnDodgeMontageEnded(UAnimMontage* Montage, bool bInterrupted);

private:
	ARACharacter* GetOwnerCharacter() const;
};
