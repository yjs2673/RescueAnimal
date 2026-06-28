#include "MapProgressWidget.h"

#include "Components/Button.h"
#include "Components/TextBlock.h"

void UMapProgressWidget::NativeConstruct()
{
	Super::NativeConstruct();

	if (CloseButton)
	{
		CloseButton->OnClicked.AddUniqueDynamic(this, &UMapProgressWidget::HandleCloseButtonClicked);
	}

	if (!ProgressText)
	{
		UE_LOG(LogTemp, Warning, TEXT("[MapProgressWidget] ProgressText is not bound."));
		return;
	}

	ProgressText->SetText(CachedProgressText);
}

void UMapProgressWidget::NativeDestruct()
{
	if (CloseButton)
	{
		CloseButton->OnClicked.RemoveDynamic(this, &UMapProgressWidget::HandleCloseButtonClicked);
	}

	Super::NativeDestruct();
}

void UMapProgressWidget::SetProgressText(const FText& InProgressText)
{
	CachedProgressText = InProgressText;

	if (ProgressText)
	{
		ProgressText->SetText(CachedProgressText);
	}
}

void UMapProgressWidget::RequestClose()
{
	OnCloseRequested.Broadcast();
}

void UMapProgressWidget::HandleCloseButtonClicked()
{
	RequestClose();
}
