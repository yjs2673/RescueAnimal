#pragma once

#include "CoreMinimal.h"
#include "TPSUserWidget.h"
#include "TPSGameEnums.h"
#include "CurrentWeaponWidget.generated.h"

class UImage;
class UTexture2D;
class ATPSCaptureCharacter;

UCLASS()
class TPSCAPTURE_API UCurrentWeaponWidget : public UTPSUserWidget
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
	ATPSCaptureCharacter* GetPlayerCharacter() const;
};
