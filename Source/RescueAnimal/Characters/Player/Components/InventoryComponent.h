#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "RAStructTypes.h"
#include "InventoryComponent.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnItemChangedSignature, FName, ItemID, int32, NewCount);

UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class RESCUEANIMAL_API UInventoryComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UInventoryComponent();

protected:
	virtual void BeginPlay() override;

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Inventory")
	TArray<FInventoryEntry> Items;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Inventory", meta = (ClampMin = "1"))
	int32 MaxSlotCount = 20;

public:
	UPROPERTY(BlueprintAssignable, Category = "Events")
	FOnItemChangedSignature OnItemChanged;

public:
	UFUNCTION(BlueprintCallable, Category = "Inventory")
	void AddItem(FName ItemID, int32 Count);

	UFUNCTION(BlueprintCallable, Category = "Inventory")
	bool RemoveItem(FName ItemID, int32 Count);

	UFUNCTION(BlueprintPure, Category = "Inventory")
	int32 GetItemCount(FName ItemID) const;

	UFUNCTION(BlueprintPure, Category = "Inventory")
	bool HasItem(FName ItemID, int32 Count = 1) const;

	UFUNCTION(BlueprintPure, Category = "Inventory")
	bool CanAddItem(FName ItemID, int32 Count) const;

	UFUNCTION(BlueprintPure, Category = "Inventory")
	const TArray<FInventoryEntry>& GetAllItems() const { return Items; }

#pragma region Runtime Data
	UFUNCTION(BlueprintCallable, Category = "Runtime")
	void SetItems(const TArray<FInventoryEntry>& NewItems);
#pragma endregion Runtime Data

protected:
	FInventoryEntry* FindEntry(FName ItemID);
	const FInventoryEntry* FindEntry(FName ItemID) const;
};
