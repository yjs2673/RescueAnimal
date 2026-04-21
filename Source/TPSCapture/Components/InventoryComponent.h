#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "InventoryComponent.generated.h"

USTRUCT(BlueprintType)
struct FInventoryEntry
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Inventory")
	FName ItemID = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Inventory")
	int32 Count = 0;
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnItemChangedSignature, FName, ItemID, int32, NewCount);

UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class TPSCAPTURE_API UInventoryComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UInventoryComponent();

protected:
	virtual void BeginPlay() override;

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Inventory")
	TArray<FInventoryEntry> Items;

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
	const TArray<FInventoryEntry>& GetAllItems() const { return Items; }

protected:
	FInventoryEntry* FindEntry(FName ItemID);
	const FInventoryEntry* FindEntry(FName ItemID) const;
};