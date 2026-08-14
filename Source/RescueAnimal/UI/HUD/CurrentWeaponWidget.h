#pragma once

#include "CoreMinimal.h"
#include "UI/Audio/RAUserWidget.h"
#include "RAGameEnums.h"
#include "CurrentWeaponWidget.generated.h"

class UImage;
class UTexture2D;
class ARACharacter;

UCLASS()
class RESCUEANIMAL_API UCurrentWeaponWidget : public URAUserWidget
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
	TObjectPtr<UTexture2D> NoneIcon;

private:
	UTexture2D* GetCurrentWeaponItemIcon() const;
	ARACharacter* GetPlayerCharacter() const;
};
