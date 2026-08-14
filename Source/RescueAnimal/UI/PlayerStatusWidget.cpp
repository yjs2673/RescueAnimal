#include "PlayerStatusWidget.h"

#include "Components/TextBlock.h"
#include "Components/ProgressBar.h"

void UPlayerStatusWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);

	if (HPProgressBar)
	{
		DisplayedHPPercent = FMath::FInterpTo(
			DisplayedHPPercent,
			TargetHPPercent,
			InDeltaTime,
			HPInterpSpeed
		);
		HPProgressBar->SetPercent(DisplayedHPPercent);
	}

	if (EXPProgressBar)
	{
		DisplayedEXPPercent = FMath::FInterpTo(
			DisplayedEXPPercent,
			TargetEXPPercent,
			InDeltaTime,
			EXPInterpSpeed
		);
		EXPProgressBar->SetPercent(DisplayedEXPPercent);
	}
}

void UPlayerStatusWidget::UpdateHP(float CurrentHP, float MaxHP)
{
	const float Percent = MaxHP > 0.0f ? FMath::Clamp(CurrentHP / MaxHP, 0.0f, 1.0f) : 0.0f;
	TargetHPPercent = Percent;

	if (HPProgressBar)
	{
		if (!bHasInitializedHPPercent)
		{
			DisplayedHPPercent = Percent;
			HPProgressBar->SetPercent(DisplayedHPPercent);
			bHasInitializedHPPercent = true;
		}
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
	const float Percent = RequiredEXP > 0
		? FMath::Clamp(static_cast<float>(CurrentEXP) / static_cast<float>(RequiredEXP), 0.0f, 1.0f)
		: 0.0f;
	TargetEXPPercent = Percent;

	if (EXPProgressBar)
	{
		if (!bHasInitializedEXPPercent)
		{
			DisplayedEXPPercent = Percent;
			EXPProgressBar->SetPercent(DisplayedEXPPercent);
			bHasInitializedEXPPercent = true;
		}
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
