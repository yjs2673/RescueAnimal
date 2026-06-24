#include "GameProgressMessageWidget.h"

#include "Components/TextBlock.h"
#include "Kismet/GameplayStatics.h"
#include "Sound/SoundBase.h"

UGameProgressMessageWidget::UGameProgressMessageWidget(
	const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	PlainClearText = FText::FromString(TEXT("Clear Plain Island!"));
	SnowClearText = FText::FromString(TEXT("Clear Snow Island!"));
	DesertClearText = FText::FromString(TEXT("Clear Desert Island!"));
	GameOverText = FText::FromString(TEXT("Game Over"));
}

void UGameProgressMessageWidget::ShowFieldClearMessage(FName MapID)
{
	if (MapID == TEXT("MAP_Plain"))
	{
		ShowMessage(PlainClearText, FieldClearTextColor, MapClearSound);
	}
	else if (MapID == TEXT("MAP_Snow"))
	{
		ShowMessage(SnowClearText, FieldClearTextColor, MapClearSound);
	}
	else if (MapID == TEXT("MAP_Desert"))
	{
		ShowMessage(DesertClearText, FieldClearTextColor, MapClearSound);
	}
	else
	{
		UE_LOG(LogTemp, Warning,
			TEXT("[GameProgressWidget] Unknown field MapID: %s"),
			*MapID.ToString());
	}
}

void UGameProgressMessageWidget::ShowGameOverMessage()
{
	ShowMessage(GameOverText, GameOverTextColor, GameOverSound);
}

void UGameProgressMessageWidget::ShowMessage(
	const FText& Message,
	const FLinearColor& TextColor,
	USoundBase* Sound)
{
	if (!MessageText)
	{
		UE_LOG(LogTemp, Warning,
			TEXT("[GameProgressWidget] MessageText is not bound."));
		return;
	}

	MessageText->SetText(Message);
	MessageText->SetColorAndOpacity(FSlateColor(TextColor));

	MessageElapsedTime = 0.0f;
	bMessageActive = true;
	SetRenderOpacity(FadeInDuration > 0.0f ? 0.0f : 1.0f);
	SetVisibility(ESlateVisibility::HitTestInvisible);

	if (Sound)
	{
		UGameplayStatics::PlaySound2D(this, Sound);
	}
}

#pragma region Fade
void UGameProgressMessageWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);

	if (!bMessageActive)
	{
		return;
	}

	MessageElapsedTime += InDeltaTime;

	const float TotalDuration = FMath::Max(0.1f, MessageDuration);
	const float SafeFadeInDuration = FMath::Clamp(FadeInDuration, 0.0f, TotalDuration);
	const float SafeFadeOutDuration = FMath::Clamp(
		FadeOutDuration,
		0.0f,
		TotalDuration - SafeFadeInDuration
	);
	const float FadeOutStartTime = TotalDuration - SafeFadeOutDuration;

	float Opacity = 1.0f;

	if (SafeFadeInDuration > 0.0f && MessageElapsedTime < SafeFadeInDuration)
	{
		Opacity = MessageElapsedTime / SafeFadeInDuration;
	}
	else if (SafeFadeOutDuration > 0.0f && MessageElapsedTime >= FadeOutStartTime)
	{
		Opacity = (TotalDuration - MessageElapsedTime) / SafeFadeOutDuration;
	}

	SetRenderOpacity(FMath::Clamp(Opacity, 0.0f, 1.0f));

	if (MessageElapsedTime >= TotalDuration)
	{
		HideMessage();
	}
}
#pragma endregion Fade

void UGameProgressMessageWidget::HideMessage()
{
	bMessageActive = false;
	MessageElapsedTime = 0.0f;
	SetRenderOpacity(0.0f);
	SetVisibility(ESlateVisibility::Collapsed);
}

void UGameProgressMessageWidget::NativeDestruct()
{
	bMessageActive = false;
	Super::NativeDestruct();
}
