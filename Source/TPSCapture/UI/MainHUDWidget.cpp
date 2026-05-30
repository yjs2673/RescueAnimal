#include "MainHUDWidget.h"

#include "Components/Button.h"
#include "Components/Widget.h"
#include "Blueprint/WidgetTree.h"

#include "CrosshairBowWidget.h"
#include "QuickSlotBarWidget.h"
#include "CurrentWeaponWidget.h"

#include "TPSCaptureCharacter.h"
#include "Kismet/GameplayStatics.h"

void UMainHUDWidget::NativeConstruct()
{
	Super::NativeConstruct();

	if (InventoryButton)
	{
		InventoryButton->OnClicked.AddUniqueDynamic(
			this,
			&UMainHUDWidget::HandleInventoryButtonClicked
		);
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("MainHUDWidget: InventoryButton is not bound."));
	}

	if (QuickSlotBar)
	{
		QuickSlotBar->RefreshQuickSlots();
	}

	BindPlayerWeaponChanged();

	if (CachedPlayerCharacter && CurrentWeaponWidget)
	{
		CurrentWeaponWidget->UpdateWeaponIcon(
			CachedPlayerCharacter->GetCurrentWeaponType()
		);
	}
}

void UMainHUDWidget::NativeDestruct()
{
	UnbindPlayerWeaponChanged();

	if (InventoryButton)
	{
		InventoryButton->OnClicked.RemoveDynamic(
			this,
			&UMainHUDWidget::HandleInventoryButtonClicked
		);
	}

	Super::NativeDestruct();
}

void UMainHUDWidget::HandleInventoryButtonClicked()
{
	OnInventoryButtonClicked.Broadcast();
}

void UMainHUDWidget::BindPlayerWeaponChanged()
{
	CachedPlayerCharacter = GetPlayerCharacter();

	if (!CachedPlayerCharacter)
	{
		UE_LOG(LogTemp, Warning, TEXT("MainHUDWidget: Failed to find player character."));
		return;
	}

	CachedPlayerCharacter->OnWeaponChanged.AddUniqueDynamic(
		this,
		&UMainHUDWidget::HandleWeaponChanged
	);
}

void UMainHUDWidget::UnbindPlayerWeaponChanged()
{
	if (!CachedPlayerCharacter)
	{
		return;
	}

	CachedPlayerCharacter->OnWeaponChanged.RemoveDynamic(
		this,
		&UMainHUDWidget::HandleWeaponChanged
	);

	CachedPlayerCharacter = nullptr;
}

ATPSCaptureCharacter* UMainHUDWidget::GetPlayerCharacter() const
{
	APawn* OwningPawn = GetOwningPlayerPawn();

	if (ATPSCaptureCharacter* PlayerCharacter = Cast<ATPSCaptureCharacter>(OwningPawn))
	{
		return PlayerCharacter;
	}

	return Cast<ATPSCaptureCharacter>(
		UGameplayStatics::GetPlayerCharacter(GetWorld(), 0)
	);
}

void UMainHUDWidget::HandleWeaponChanged(EWeaponType NewWeaponType)
{
	if (CurrentWeaponWidget)
	{
		CurrentWeaponWidget->UpdateWeaponIcon(NewWeaponType);
	}
}

UCrosshairBowWidget* UMainHUDWidget::GetCrosshairBowWidget() const
{
	if (CrosshairBowWidget)
	{
		return CrosshairBowWidget;
	}

	if (!WidgetTree)
	{
		return nullptr;
	}

	UCrosshairBowWidget* FoundCrosshairWidget = nullptr;

	WidgetTree->ForEachWidget([&FoundCrosshairWidget](UWidget* Widget)
		{
			if (!FoundCrosshairWidget)
			{
				FoundCrosshairWidget = Cast<UCrosshairBowWidget>(Widget);
			}
		});

	return FoundCrosshairWidget;
}