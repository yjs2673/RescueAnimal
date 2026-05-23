#include "MainHUDWidget.h"
#include "Components/Button.h"

void UMainHUDWidget::NativeConstruct()
{
    Super::NativeConstruct();

    if (InventoryButton)
    {
        InventoryButton->OnClicked.AddDynamic(this, &UMainHUDWidget::HandleInventoryButtonClicked);
    }
}

void UMainHUDWidget::HandleInventoryButtonClicked()
{
    OnInventoryButtonClicked.Broadcast();
}