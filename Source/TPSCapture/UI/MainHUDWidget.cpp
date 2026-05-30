#include "MainHUDWidget.h"

#include "Components/Button.h"
#include "Components/Widget.h"
#include "Blueprint/WidgetTree.h"

#include "CrosshairBowWidget.h"
#include "QuickSlotBarWidget.h"
#include "CurrentWeaponWidget.h"

#include "PlayerStatusWidget.h"
#include "PlayerStatComponent.h"

#include "TPSCaptureCharacter.h"
#include "Kismet/GameplayStatics.h"

void UMainHUDWidget::NativeConstruct()
{
	Super::NativeConstruct();

	if (InventoryButton)
	{
		InventoryButton->OnClicked.AddUniqueDynamic(
			this,
			&UMainHUDWidget::HandleInventoryButtonClicked
		);
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("MainHUDWidget: InventoryButton is not bound."));
	}

	if (QuickSlotBar)
	{
		QuickSlotBar->RefreshQuickSlots();
	}

	BindPlayerWeaponChanged();

	if (CachedPlayerCharacter && CurrentWeaponWidget)
	{
		CurrentWeaponWidget->UpdateWeaponIcon(
			CachedPlayerCharacter->GetCurrentWeaponType()
		);
	}

	BindPlayerStatComponent();
	RefreshPlayerStatus();
}

void UMainHUDWidget::NativeDestruct()
{
	UnbindPlayerWeaponChanged();

	if (InventoryButton)
	{
		InventoryButton->OnClicked.RemoveDynamic(
			this,
			&UMainHUDWidget::HandleInventoryButtonClicked
		);
	}

	UnbindPlayerStatComponent();

	Super::NativeDestruct();
}

void UMainHUDWidget::HandleInventoryButtonClicked()
{
	OnInventoryButtonClicked.Broadcast();
}

void UMainHUDWidget::BindPlayerWeaponChanged()
{
	CachedPlayerCharacter = GetPlayerCharacter();

	if (!CachedPlayerCharacter)
	{
		UE_LOG(LogTemp, Warning, TEXT("MainHUDWidget: Failed to find player character."));
		return;
	}

	CachedPlayerCharacter->OnWeaponChanged.AddUniqueDynamic(
		this,
		&UMainHUDWidget::HandleWeaponChanged
	);
}

void UMainHUDWidget::UnbindPlayerWeaponChanged()
{
	if (!CachedPlayerCharacter)
	{
		return;
	}

	CachedPlayerCharacter->OnWeaponChanged.RemoveDynamic(
		this,
		&UMainHUDWidget::HandleWeaponChanged
	);

	CachedPlayerCharacter = nullptr;
}

ATPSCaptureCharacter* UMainHUDWidget::GetPlayerCharacter() const
{
	APawn* OwningPawn = GetOwningPlayerPawn();

	if (ATPSCaptureCharacter* PlayerCharacter = Cast<ATPSCaptureCharacter>(OwningPawn))
	{
		return PlayerCharacter;
	}

	return Cast<ATPSCaptureCharacter>(
		UGameplayStatics::GetPlayerCharacter(GetWorld(), 0)
	);
}

void UMainHUDWidget::HandleWeaponChanged(EWeaponType NewWeaponType)
{
	if (CurrentWeaponWidget)
	{
		CurrentWeaponWidget->UpdateWeaponIcon(NewWeaponType);
	}
}

UCrosshairBowWidget* UMainHUDWidget::GetCrosshairBowWidget() const
{
	if (CrosshairBowWidget)
	{
		return CrosshairBowWidget;
	}

	if (!WidgetTree)
	{
		return nullptr;
	}

	UCrosshairBowWidget* FoundCrosshairWidget = nullptr;

	WidgetTree->ForEachWidget([&FoundCrosshairWidget](UWidget* Widget)
		{
			if (!FoundCrosshairWidget)
			{
				FoundCrosshairWidget = Cast<UCrosshairBowWidget>(Widget);
			}
		});

	return FoundCrosshairWidget;
}

void UMainHUDWidget::BindPlayerStatComponent()
{
	CachedPlayerStatComponent = GetPlayerStatComponent();

	if (!CachedPlayerStatComponent)
	{
		UE_LOG(LogTemp, Warning, TEXT("MainHUDWidget: PlayerStatComponent not found."));
		return;
	}

	CachedPlayerStatComponent->OnHPChanged.AddUniqueDynamic(
		this,
		&UMainHUDWidget::HandleHPChanged
	);

	CachedPlayerStatComponent->OnEXPChanged.AddUniqueDynamic(
		this,
		&UMainHUDWidget::HandleEXPChanged
	);

	CachedPlayerStatComponent->OnLevelChanged.AddUniqueDynamic(
		this,
		&UMainHUDWidget::HandleLevelChanged
	);
}

void UMainHUDWidget::UnbindPlayerStatComponent()
{
	if (!CachedPlayerStatComponent)
	{
		return;
	}

	CachedPlayerStatComponent->OnHPChanged.RemoveDynamic(
		this,
		&UMainHUDWidget::HandleHPChanged
	);

	CachedPlayerStatComponent->OnEXPChanged.RemoveDynamic(
		this,
		&UMainHUDWidget::HandleEXPChanged
	);

	CachedPlayerStatComponent->OnLevelChanged.RemoveDynamic(
		this,
		&UMainHUDWidget::HandleLevelChanged
	);

	CachedPlayerStatComponent = nullptr;
}

UPlayerStatComponent* UMainHUDWidget::GetPlayerStatComponent() const
{
	APawn* OwningPawn = GetOwningPlayerPawn();

	if (!OwningPawn)
	{
		return nullptr;
	}

	return OwningPawn->FindComponentByClass<UPlayerStatComponent>();
}

void UMainHUDWidget::RefreshPlayerStatus()
{
	if (!CachedPlayerStatComponent)
	{
		CachedPlayerStatComponent = GetPlayerStatComponent();
	}

	if (!CachedPlayerStatComponent || !PlayerStatusWidget)
	{
		return;
	}

	PlayerStatusWidget->UpdateHP(
		CachedPlayerStatComponent->GetCurrentHP(),
		CachedPlayerStatComponent->GetMaxHP()
	);

	PlayerStatusWidget->UpdateEXP(
		CachedPlayerStatComponent->GetCurrentEXP(),
		CachedPlayerStatComponent->GetRequiredEXP()
	);

	PlayerStatusWidget->UpdateLevel(
		CachedPlayerStatComponent->GetLevel()
	);
}

void UMainHUDWidget::HandleHPChanged(float CurrentHP, float MaxHP)
{
	if (PlayerStatusWidget)
	{
		PlayerStatusWidget->UpdateHP(CurrentHP, MaxHP);
	}
}

void UMainHUDWidget::HandleEXPChanged(int32 CurrentEXP, int32 RequiredEXP)
{
	if (PlayerStatusWidget)
	{
		PlayerStatusWidget->UpdateEXP(CurrentEXP, RequiredEXP);
	}
}

void UMainHUDWidget::HandleLevelChanged(int32 NewLevel)
{
	if (PlayerStatusWidget)
	{
		PlayerStatusWidget->UpdateLevel(NewLevel);
	}

	RefreshPlayerStatus();
}