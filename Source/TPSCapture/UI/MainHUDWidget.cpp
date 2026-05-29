#include "MainHUDWidget.h"

#include "Components/Button.h"
#include "Components/Widget.h"
#include "Blueprint/WidgetTree.h"
#include "CrosshairBowWidget.h"
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
