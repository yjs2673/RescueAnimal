#include "ShopResultWidget.h"
#include "Components/Button.h"
#include "Components/TextBlock.h"

UShopResultWidget::UShopResultWidget(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	bPlayOpenCloseSounds = true;
}

void UShopResultWidget::NativeConstruct()
{
	Super::NativeConstruct();

	if (ConfirmButton)
	{
		ConfirmButton->OnClicked.AddUniqueDynamic(this, &UShopResultWidget::HandleConfirmClicked);
	}
}

void UShopResultWidget::SetupResult(const FText& InResultMessage)
{
	if (ResultText)
	{
		ResultText->SetText(InResultMessage);
	}
}

void UShopResultWidget::HandleConfirmClicked()
{
	RemoveFromParent();
}
