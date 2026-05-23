#include "InventoryWidget.h"

#include "Components/Button.h"

void UInventoryWidget::NativeConstruct()
{
	Super::NativeConstruct();

	if (CloseButton)
	{
		CloseButton->OnClicked.AddDynamic(
			this,
			&UInventoryWidget::HandleCloseButtonClicked
		);
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("CloseButton is not bound in WBP_Inventory."));
	}
}

void UInventoryWidget::HandleCloseButtonClicked()
{
	OnInventoryCloseRequested.Broadcast();
}