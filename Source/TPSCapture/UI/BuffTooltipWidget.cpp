#include "BuffTooltipWidget.h"

#include "Components/TextBlock.h"
#include "PlayerStatComponent.h"

void UBuffTooltipWidget::InitializeTooltip(UPlayerStatComponent* InStatComponent, EBuffType InBuffType, const FText& InBuffName, float InBuffValue, bool bInBuffValueIsPercent)
{
	CachedStatComponent = InStatComponent;
	BuffType = InBuffType;
	BuffName = InBuffName;
	BuffValue = InBuffValue;
	bBuffValueIsPercent = bInBuffValueIsPercent;

	RefreshTexts();
}

void UBuffTooltipWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);

	RefreshTexts();
}

void UBuffTooltipWidget::RefreshTexts()
{
	if (BuffNameText)
	{
		BuffNameText->SetText(BuffName);
	}

	if (BuffValueText)
	{
		BuffValueText->SetText(MakeBuffValueText());
	}

	if (RemainingTimeText)
	{
		const float RemainingTime = CachedStatComponent
			? CachedStatComponent->GetBuffRemainingTime(BuffType)
			: 0.0f;

		RemainingTimeText->SetText(FText::Format(
			NSLOCTEXT("BuffTooltip", "RemainingTimeFormat", "Remaining: {0}s"),
			FText::AsNumber(FMath::CeilToInt(FMath::Max(0.0f, RemainingTime)))
		));
	}
}

FText UBuffTooltipWidget::MakeBuffValueText() const
{
	if (bBuffValueIsPercent)
	{
		return FText::Format(
			NSLOCTEXT("BuffTooltip", "BuffValuePercentFormat", "+{0}%"),
			FText::AsNumber(BuffValue)
		);
	}

	return FText::Format(
		NSLOCTEXT("BuffTooltip", "BuffValueFormat", "x{0}"),
		FText::AsNumber(BuffValue)
	);
}