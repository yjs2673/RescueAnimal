#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "TPSGameEnums.h"
#include "CurrentWeaponWidget.generated.h"

class UImage;
class UTexture2D;

UCLASS()
class TPSCAPTURE_API UCurrentWeaponWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	virtual void NativeConstruct() override;

public:
	UFUNCTION(BlueprintCallable, Category = "Weapon")
	void UpdateWeaponIcon(EWeaponType WeaponType);

protected:
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UImage> WeaponIcon;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Weapon Icon")
	TObjectPtr<UTexture2D> FistIcon;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Weapon Icon")
	TObjectPtr<UTexture2D> SwordIcon;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Weapon Icon")
	TObjectPtr<UTexture2D> BowIcon;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Weapon Icon")
	TObjectPtr<UTexture2D> KitIcon;

private:
	UTexture2D* GetIconByWeaponType(EWeaponType WeaponType) const;
};