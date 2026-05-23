#include "InventoryWidget.h"
#include "Components/Button.h"

void UInventoryWidget::NativeConstruct()
{
    Super::NativeConstruct();

    if (CloseButton)
    {
        CloseButton->OnClicked.AddDynamic(this, &UInventoryWidget::HandleCloseButtonClicked);
    }
}

void UInventoryWidget::HandleCloseButtonClicked()
{
    OnInventoryCloseRequested.Broadcast();
}