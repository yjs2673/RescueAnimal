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
class UBuffListWidget;
class UPlayerStatusWidget;
class UPlayerStatComponent;
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

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UPlayerStatusWidget> PlayerStatusWidget;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UBuffListWidget> BuffListWidget;

private:
	UFUNCTION()
	void HandleInventoryButtonClicked();

	UFUNCTION()
	void HandleWeaponChanged(EWeaponType NewWeaponType);

	void BindPlayerWeaponChanged();
	void UnbindPlayerWeaponChanged();

	ATPSCaptureCharacter* GetPlayerCharacter() const;

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
	TObjectPtr<ATPSCaptureCharacter> CachedPlayerCharacter;
};
