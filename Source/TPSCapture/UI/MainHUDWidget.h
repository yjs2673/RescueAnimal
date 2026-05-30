#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "TPSGameEnums.h"
#include "MainHUDWidget.generated.h"

class UButton;
class UWidget;
class UQuickSlotBarWidget;
class UCrosshairBowWidget;
class UCurrentWeaponWidget;
class ATPSCaptureCharacter;

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnInventoryButtonClicked);

UCLASS()
class TPSCAPTURE_API UMainHUDWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;

public:
	UCrosshairBowWidget* GetCrosshairBowWidget() const;

	UPROPERTY(BlueprintAssignable, Category = "MainHUD")
	FOnInventoryButtonClicked OnInventoryButtonClicked;

protected:
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UButton> InventoryButton;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UQuickSlotBarWidget> QuickSlotBar;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UCrosshairBowWidget> CrosshairBowWidget;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UCurrentWeaponWidget> CurrentWeaponWidget;

private:
	UFUNCTION()
	void HandleInventoryButtonClicked();

	UFUNCTION()
	void HandleWeaponChanged(EWeaponType NewWeaponType);

	void BindPlayerWeaponChanged();
	void UnbindPlayerWeaponChanged();

	ATPSCaptureCharacter* GetPlayerCharacter() const;

private:
	UPROPERTY()
	TObjectPtr<ATPSCaptureCharacter> CachedPlayerCharacter;
};