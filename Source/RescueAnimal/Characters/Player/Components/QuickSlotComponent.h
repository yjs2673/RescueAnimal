#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "QuickSlotComponent.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(
	FOnQuickSlotChangedSignature,
	int32, SlotIndex,
	FName, ItemID
);

UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class RESCUEANIMAL_API UQuickSlotComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UQuickSlotComponent();

protected:
	virtual void BeginPlay() override;

public:
	UPROPERTY(BlueprintAssignable, Category = "Events")
	FOnQuickSlotChangedSignature OnQuickSlotChanged;

public:
	UFUNCTION(BlueprintPure, Category = "QuickSlot")
	int32 GetSlotCount() const;

	UFUNCTION(BlueprintPure, Category = "QuickSlot")
	FName GetSlotItem(int32 SlotIndex) const;

	UFUNCTION(BlueprintCallable, Category = "QuickSlot")
	bool SetSlotItem(int32 SlotIndex, FName ItemID);

	UFUNCTION(BlueprintCallable, Category = "QuickSlot")
	bool ClearSlot(int32 SlotIndex);

	UFUNCTION(BlueprintPure, Category = "QuickSlot")
	bool IsValidSlotIndex(int32 SlotIndex) const;

private:
	UPROPERTY(VisibleAnywhere, Category = "QuickSlot")
	TArray<FName> QuickSlots;

	static constexpr int32 MaxQuickSlotCount = 7;

private:
	void InitializeSlots();
};