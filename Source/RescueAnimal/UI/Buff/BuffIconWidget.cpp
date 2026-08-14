#include "BuffIconWidget.h"

#include "BuffTooltipWidget.h"
#include "Components/Image.h"

void UBuffIconWidget::InitializeBuffIcon(UPlayerStatComponent* InStatComponent, const FActiveBuffInfo& InBuffInfo)
{
	CachedStatComponent = InStatComponent;
	BuffInfo = InBuffInfo;

	SetupIcon();
	SetupTooltip();
	UpdateBlinkOpacity();
}

void UBuffIconWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);

	UpdateBlinkOpacity();
}

void UBuffIconWidget::SetupIcon()
{
	if (BuffIconImage && BuffInfo.Icon)
	{
		BuffIconImage->SetBrushFromTexture(BuffInfo.Icon);
	}
}

void UBuffIconWidget::SetupTooltip()
{
	if (!TooltipWidgetClass)
		return;

	UBuffTooltipWidget* TooltipWidget = CreateWidget<UBuffTooltipWidget>(GetOwningPlayer(), TooltipWidgetClass);
	if (!TooltipWidget)
		return;

	TooltipWidget->InitializeTooltip(
		CachedStatComponent,
		BuffInfo.BuffType,
		BuffInfo.BuffName,
		BuffInfo.BuffValue,
		BuffInfo.bBuffValueIsPercent
	);

	SetToolTip(TooltipWidget);
}

void UBuffIconWidget::UpdateBlinkOpacity()
{
	if (!CachedStatComponent)
	{
		SetRenderOpacity(1.0f);
		return;
	}

	const float RemainingTime = CachedStatComponent->GetBuffRemainingTime(BuffInfo.BuffType);
	if (RemainingTime <= 0.0f)
	{
		SetRenderOpacity(0.0f);
		return;
	}

	if (RemainingTime > BlinkStartTime)
	{
		SetRenderOpacity(1.0f);
		return;
	}

	const UWorld* World = GetWorld();
	const float CurrentTime = World ? World->GetTimeSeconds() : 0.0f;
	const float BlinkAlpha = 0.5f + 0.5f * FMath::Sin(CurrentTime * BlinkSpeed);
	const float Opacity = FMath::Lerp(MinBlinkOpacity, 1.0f, BlinkAlpha);

	SetRenderOpacity(Opacity);
}