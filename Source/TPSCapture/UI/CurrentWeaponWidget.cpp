#include "CurrentWeaponWidget.h"

#include "Components/Image.h"
#include "Kismet/GameplayStatics.h"

#include "TPSCaptureCharacter.h"
#include "TPSGameInstance.h"
#include "TPSStructTypes.h"

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
	const ATPSCaptureCharacter* PlayerCharacter = GetPlayerCharacter();
	if (!PlayerCharacter)
	{
		return NoneIcon.Get();
	}

	const FName WeaponItemID = PlayerCharacter->GetCurrentWeaponItemID();
	if (WeaponItemID.IsNone())
	{
		return NoneIcon.Get();
	}

	const UTPSGameInstance* TPSGameInstance = GetWorld()
		? GetWorld()->GetGameInstance<UTPSGameInstance>()
		: nullptr;
	if (!TPSGameInstance)
	{
		return NoneIcon.Get();
	}

	FItemData ItemData;
	if (!TPSGameInstance->GetItemDataByID(WeaponItemID, ItemData))
	{
		return NoneIcon.Get();
	}

	if (ItemData.Icon)
	{
		return ItemData.Icon;
	}

	return ItemData.Image ? ItemData.Image : NoneIcon.Get();
}

ATPSCaptureCharacter* UCurrentWeaponWidget::GetPlayerCharacter() const
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
