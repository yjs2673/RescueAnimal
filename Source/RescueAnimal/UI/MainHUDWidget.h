#pragma once

#include "CoreMinimal.h"
#include "RAUserWidget.h"
#include "RAGameEnums.h"
#include "MainHUDWidget.generated.h"

class UButton;
class UWidget;
class UQuickSlotBarWidget;
class UCrosshairBowWidget;
class UCurrentWeaponWidget;
class UBuffListWidget;
class UPlayerStatusWidget;
class UPlayerStatComponent;
class ARACharacter;

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnInventoryButtonClicked);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnSettingButtonClicked);

UCLASS()
class RESCUEANIMAL_API UMainHUDWidget : public URAUserWidget
{
	GENERATED_BODY()

public:
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;

public:
	UCrosshairBowWidget* GetCrosshairBowWidget() const;

	UPROPERTY(BlueprintAssignable, Category = "MainHUD")
	FOnInventoryButtonClicked OnInventoryButtonClicked;

	UPROPERTY(BlueprintAssignable, Category = "MainHUD")
	FOnSettingButtonClicked OnSettingButtonClicked;

protected:
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UButton> InventoryButton;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UButton> SettingButton;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UQuickSlotBarWidget> QuickSlotBar;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UCrosshairBowWidget> CrosshairBowWidget;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UCurrentWeaponWidget> CurrentWeaponWidget;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UPlayerStatusWidget> PlayerStatusWidget;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UBuffListWidget> BuffListWidget;

private:
	UFUNCTION()
	void HandleInventoryButtonClicked();

	UFUNCTION()
	void HandleSettingButtonClicked();

	UFUNCTION()
	void HandleWeaponChanged(EWeaponType NewWeaponType);

	void BindPlayerWeaponChanged();
	void UnbindPlayerWeaponChanged();

	ARACharacter* GetPlayerCharacter() const;

	UFUNCTION()
	void HandleHPChanged(float CurrentHP, float MaxHP);

	UFUNCTION()
	void HandleEXPChanged(int32 CurrentEXP, int32 RequiredEXP);

	UFUNCTION()
	void HandleLevelChanged(int32 NewLevel);

	void BindPlayerStatComponent();
	void UnbindPlayerStatComponent();
	void RefreshPlayerStatus();

	UPlayerStatComponent* GetPlayerStatComponent() const;

	UPROPERTY()
	TObjectPtr<UPlayerStatComponent> CachedPlayerStatComponent;

private:
	UPROPERTY()
	TObjectPtr<ARACharacter> CachedPlayerCharacter;
};
