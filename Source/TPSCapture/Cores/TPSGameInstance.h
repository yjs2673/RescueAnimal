#pragma once

#include "CoreMinimal.h"
#include "Engine/GameInstance.h"
#include "TPSStructTypes.h"
#include "TPSGameInstance.generated.h"

class UDataTable;

UCLASS()
class TPSCAPTURE_API UTPSGameInstance : public UGameInstance
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = "Data")
	bool GetItemDataByID(FName ItemID, FItemData& OutItemData) const;

	UFUNCTION(BlueprintCallable, Category = "Data")
	bool GetWeaponDataByID(FName WeaponID, FWeaponData& OutWeaponData) const;

protected:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Data")
	UDataTable* ItemDataTable;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Data")
	UDataTable* WeaponDataTable;

public:
	UFUNCTION(BlueprintPure, Category = "Data")
	UDataTable* GetItemDataTable() const { return ItemDataTable; }

	UFUNCTION(BlueprintPure, Category = "Data")
	UDataTable* GetWeaponDataTable() const { return WeaponDataTable; }
};
