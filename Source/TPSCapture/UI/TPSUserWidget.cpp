#include "TPSUserWidget.h"

#include "Blueprint/WidgetTree.h"
#include "Components/Button.h"
#include "Components/Widget.h"
#include "Kismet/GameplayStatics.h"
#include "Sound/SoundBase.h"
#include "TPSAudioSubsystem.h"

UTPSUserWidget::UTPSUserWidget(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
}

void UTPSUserWidget::NativeConstruct()
{
	Super::NativeConstruct();

	BindButtonClickSounds();

	if (bPlayOpenCloseSounds)
	{
		PlayUISound(ResolvePopupOpenSound());
	}
}

void UTPSUserWidget::NativeDestruct()
{
	UnbindButtonClickSounds();

	if (bPlayOpenCloseSounds)
	{
		PlayUISound(ResolvePopupCloseSound());
	}

	Super::NativeDestruct();
}

void UTPSUserWidget::PlayUISound(USoundBase* Sound) const
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

void UTPSUserWidget::HandleAnyButtonClicked()
{
	PlayUISound(ResolveButtonClickSound());
}

void UTPSUserWidget::BindButtonClickSounds()
{
	if (!WidgetTree)
	{
		return;
	}

	WidgetTree->ForEachWidget([this](UWidget* Widget)
		{
			if (UButton* Button = Cast<UButton>(Widget))
			{
				Button->OnClicked.AddUniqueDynamic(this, &UTPSUserWidget::HandleAnyButtonClicked);
			}
		});
}

void UTPSUserWidget::UnbindButtonClickSounds()
{
	if (!WidgetTree)
	{
		return;
	}

	WidgetTree->ForEachWidget([this](UWidget* Widget)
		{
			if (UButton* Button = Cast<UButton>(Widget))
			{
				Button->OnClicked.RemoveDynamic(this, &UTPSUserWidget::HandleAnyButtonClicked);
			}
		});
}

USoundBase* UTPSUserWidget::ResolveButtonClickSound() const
{
	const UTPSAudioSubsystemSettings* AudioSettings = GetDefault<UTPSAudioSubsystemSettings>();
	return AudioSettings ? AudioSettings->UIButtonClickSound.LoadSynchronous() : nullptr;
}

USoundBase* UTPSUserWidget::ResolvePopupOpenSound() const
{
	const UTPSAudioSubsystemSettings* AudioSettings = GetDefault<UTPSAudioSubsystemSettings>();
	return AudioSettings ? AudioSettings->UIPopupOpenSound.LoadSynchronous() : nullptr;
}

USoundBase* UTPSUserWidget::ResolvePopupCloseSound() const
{
	const UTPSAudioSubsystemSettings* AudioSettings = GetDefault<UTPSAudioSubsystemSettings>();
	return AudioSettings ? AudioSettings->UIPopupCloseSound.LoadSynchronous() : nullptr;
}
