#include "MainHUDWidget.h"

#include "Components/Button.h"
#include "QuickSlotBarWidget.h"

void UMainHUDWidget::NativeConstruct()
{
	Super::NativeConstruct();

	if (InventoryButton)
	{
		InventoryButton->OnClicked.AddDynamic(
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
}

void UMainHUDWidget::HandleInventoryButtonClicked()
{
	OnInventoryButtonClicked.Broadcast();
}