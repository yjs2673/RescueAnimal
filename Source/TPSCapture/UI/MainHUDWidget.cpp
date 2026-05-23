#include "MainHUDWidget.h"

#include "Components/Button.h"

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
		UE_LOG(LogTemp, Warning, TEXT("InventoryButton is not bound in WBP_MainHUD."));
	}
}

void UMainHUDWidget::HandleInventoryButtonClicked()
{
	OnInventoryButtonClicked.Broadcast();
}