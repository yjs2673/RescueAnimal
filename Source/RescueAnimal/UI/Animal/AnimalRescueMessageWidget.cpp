#include "AnimalRescueMessageWidget.h"

#include "Components/Image.h"

void UAnimalRescueMessageWidget::NativeConstruct()
{
	Super::NativeConstruct();

	if (RescueImage && DefaultRescueTexture)
	{
		RescueImage->SetBrushFromTexture(DefaultRescueTexture, true);
		RescueImage->SetVisibility(ESlateVisibility::HitTestInvisible);
	}
}