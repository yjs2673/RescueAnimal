#include "BuffListWidget.h"

#include "BuffIconWidget.h"
#include "Components/PanelWidget.h"
#include "PlayerStatComponent.h"

void UBuffListWidget::BindPlayerStatComponent(UPlayerStatComponent* InStatComponent)
{
	if (CachedStatComponent == InStatComponent)
	{
		RefreshBuffs();
		return;
	}

	UnbindPlayerStatComponent();

	CachedStatComponent = InStatComponent;

	if (CachedStatComponent)
	{
		CachedStatComponent->OnActiveBuffsChanged.AddUniqueDynamic(this, &UBuffListWidget::RefreshBuffs);
	}

	RefreshBuffs();
}

void UBuffListWidget::UnbindPlayerStatComponent()
{
	if (CachedStatComponent)
	{
		CachedStatComponent->OnActiveBuffsChanged.RemoveDynamic(this, &UBuffListWidget::RefreshBuffs);
		CachedStatComponent = nullptr;
	}
}

void UBuffListWidget::RefreshBuffs()
{
	if (!BuffContainer)
		return;

	BuffContainer->ClearChildren();

	if (!CachedStatComponent || !BuffIconWidgetClass)
		return;

	const TArray<FActiveBuffInfo> ActiveBuffs = CachedStatComponent->GetActiveBuffs();

	for (const FActiveBuffInfo& BuffInfo : ActiveBuffs)
	{
		UBuffIconWidget* BuffIconWidget = CreateWidget<UBuffIconWidget>(GetOwningPlayer(), BuffIconWidgetClass);
		if (!BuffIconWidget)
			continue;

		BuffIconWidget->InitializeBuffIcon(CachedStatComponent, BuffInfo);
		BuffContainer->AddChild(BuffIconWidget);
	}
}

void UBuffListWidget::NativeDestruct()
{
	UnbindPlayerStatComponent();

	Super::NativeDestruct();
}