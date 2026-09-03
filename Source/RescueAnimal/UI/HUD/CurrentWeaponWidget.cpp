#include "CurrentWeaponWidget.h"

#include "Components/Image.h"
#include "Kismet/GameplayStatics.h"

#include "RACharacter.h"
#include "RAGameInstance.h"
#include "RAStructTypes.h"
#include "PlayerEquipmentComponent.h"

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

	UTexture2D* IconTexture = WeaponType == EWeaponType::None
		? NoneIcon.Get()
		: GetCurrentWeaponItemIcon();

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

UTexture2D* UCurrentWeaponWidget::GetCurrentWeaponItemIcon() const
{
	const ARACharacter* PlayerCharacter = GetPlayerCharacter();
	if (!PlayerCharacter)
	{
		return NoneIcon.Get();
	}

	const UPlayerEquipmentComponent* PlayerEquipmentComponent = PlayerCharacter->GetPlayerEquipmentComponent();
	const FName WeaponItemID = PlayerEquipmentComponent
		? PlayerEquipmentComponent->GetCurrentWeaponItemID()
		: NAME_None;
	if (WeaponItemID.IsNone())
	{
		return NoneIcon.Get();
	}

	const URAGameInstance* RAGameInstance = GetWorld()
		? GetWorld()->GetGameInstance<URAGameInstance>()
		: nullptr;
	if (!RAGameInstance)
	{
		return NoneIcon.Get();
	}

	FItemData ItemData;
	if (!RAGameInstance->GetItemDataByID(WeaponItemID, ItemData))
	{
		return NoneIcon.Get();
	}

	if (ItemData.Icon)
	{
		return ItemData.Icon;
	}

	return ItemData.Image ? ItemData.Image : NoneIcon.Get();
}

ARACharacter* UCurrentWeaponWidget::GetPlayerCharacter() const
{
	APawn* OwningPawn = GetOwningPlayerPawn();

	if (ARACharacter* PlayerCharacter = Cast<ARACharacter>(OwningPawn))
	{
		return PlayerCharacter;
	}

	return Cast<ARACharacter>(
		UGameplayStatics::GetPlayerCharacter(GetWorld(), 0)
	);
}
