#include "CurrentWeaponWidget.h"
#include "Components/Image.h"

void UCurrentWeaponWidget::NativeConstruct()
{
	Super::NativeConstruct();

	UpdateWeaponIcon(EWeaponType::None);
}

void UCurrentWeaponWidget::UpdateWeaponIcon(EWeaponType WeaponType)
{
	if (!WeaponIcon)
	{
		return;
	}

	UTexture2D* IconTexture = GetIconByWeaponType(WeaponType);

	if (IconTexture)
	{
		WeaponIcon->SetBrushFromTexture(IconTexture, true);
		WeaponIcon->SetVisibility(ESlateVisibility::Visible);
	}
	else
	{
		WeaponIcon->SetVisibility(ESlateVisibility::Hidden);
	}
}

UTexture2D* UCurrentWeaponWidget::GetIconByWeaponType(EWeaponType WeaponType) const
{
	switch (WeaponType)
	{
	case EWeaponType::Sword:
		return SwordIcon ? SwordIcon.Get() : FistIcon.Get();

	case EWeaponType::Bow:
		return BowIcon ? BowIcon.Get() : FistIcon.Get();

	case EWeaponType::None:
	default:
		return FistIcon.Get();
	}
}