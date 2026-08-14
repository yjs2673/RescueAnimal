#include "EnemyHPBarWidget.h"
#include "Components/ProgressBar.h"

void UEnemyHPBarWidget::SetHPPercent(float InPercent)
{
	if (!PB_HP)
	{
		return;
	}

	PB_HP->SetPercent(FMath::Clamp(InPercent, 0.f, 1.f));
}