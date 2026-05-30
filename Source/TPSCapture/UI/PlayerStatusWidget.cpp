#include "PlayerStatusWidget.h"

#include "Components/TextBlock.h"
#include "Components/ProgressBar.h"

void UPlayerStatusWidget::UpdateHP(float CurrentHP, float MaxHP)
{
	const float Percent = MaxHP > 0.0f ? CurrentHP / MaxHP : 0.0f;

	if (HPProgressBar)
	{
		HPProgressBar->SetPercent(Percent);
	}

	if (HPText)
	{
		HPText->SetText(FText::FromString(
			FString::Printf(TEXT("%.0f / %.0f"), CurrentHP, MaxHP)
		));
	}
}

void UPlayerStatusWidget::UpdateEXP(int32 CurrentEXP, int32 RequiredEXP)
{
	const float Percent = RequiredEXP > 0 ? static_cast<float>(CurrentEXP) / static_cast<float>(RequiredEXP) : 0.0f;

	if (EXPProgressBar)
	{
		EXPProgressBar->SetPercent(Percent);
	}

	if (EXPText)
	{
		EXPText->SetText(FText::FromString(
			FString::Printf(TEXT("%d / %d"), CurrentEXP, RequiredEXP)
		));
	}
}

void UPlayerStatusWidget::UpdateLevel(int32 NewLevel)
{
	if (LevelText)
	{
		LevelText->SetText(FText::FromString(
			FString::Printf(TEXT("Lv. %d"), NewLevel)
		));
	}
}