#include "RAUserWidget.h"

#include "Blueprint/WidgetTree.h"
#include "Components/Button.h"
#include "Components/Widget.h"
#include "Kismet/GameplayStatics.h"
#include "Sound/SoundBase.h"
#include "RAAudioSubsystem.h"

URAUserWidget::URAUserWidget(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
}

void URAUserWidget::NativeConstruct()
{
	Super::NativeConstruct();

	BindButtonClickSounds();

	if (bPlayOpenCloseSounds)
	{
		PlayUISound(ResolvePopupOpenSound());
	}
}

void URAUserWidget::NativeDestruct()
{
	UnbindButtonClickSounds();

	if (bPlayOpenCloseSounds)
	{
		PlayUISound(ResolvePopupCloseSound());
	}

	Super::NativeDestruct();
}

void URAUserWidget::PlayUISound(USoundBase* Sound) const
{
	if (!Sound || IsDesignTime())
	{
		return;
	}

	UGameplayStatics::PlaySound2D(
		this,
		Sound,
		1.0f,
		1.0f,
		0.0f,
		nullptr,
		nullptr,
		true
	);
}

void URAUserWidget::HandleAnyButtonClicked()
{
	PlayUISound(ResolveButtonClickSound());
}

void URAUserWidget::BindButtonClickSounds()
{
	if (!WidgetTree)
	{
		return;
	}

	WidgetTree->ForEachWidget([this](UWidget* Widget)
		{
			if (UButton* Button = Cast<UButton>(Widget))
			{
				Button->OnClicked.AddUniqueDynamic(this, &URAUserWidget::HandleAnyButtonClicked);
			}
		});
}

void URAUserWidget::UnbindButtonClickSounds()
{
	if (!WidgetTree)
	{
		return;
	}

	WidgetTree->ForEachWidget([this](UWidget* Widget)
		{
			if (UButton* Button = Cast<UButton>(Widget))
			{
				Button->OnClicked.RemoveDynamic(this, &URAUserWidget::HandleAnyButtonClicked);
			}
		});
}

USoundBase* URAUserWidget::ResolveButtonClickSound() const
{
	const URAAudioSubsystemSettings* AudioSettings = GetDefault<URAAudioSubsystemSettings>();
	return AudioSettings ? AudioSettings->UIButtonClickSound.LoadSynchronous() : nullptr;
}

USoundBase* URAUserWidget::ResolvePopupOpenSound() const
{
	const URAAudioSubsystemSettings* AudioSettings = GetDefault<URAAudioSubsystemSettings>();
	return AudioSettings ? AudioSettings->UIPopupOpenSound.LoadSynchronous() : nullptr;
}

USoundBase* URAUserWidget::ResolvePopupCloseSound() const
{
	const URAAudioSubsystemSettings* AudioSettings = GetDefault<URAAudioSubsystemSettings>();
	return AudioSettings ? AudioSettings->UIPopupCloseSound.LoadSynchronous() : nullptr;
}
