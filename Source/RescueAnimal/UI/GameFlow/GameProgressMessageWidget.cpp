#include "GameProgressMessageWidget.h"

#include "Components/Button.h"
#include "Components/TextBlock.h"
#include "Kismet/GameplayStatics.h"
#include "Sound/SoundBase.h"
#include "RAPlayerController.h"
#include "LevelTransitionComponent.h"

UGameProgressMessageWidget::UGameProgressMessageWidget(
	const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	PlainClearText = FText::FromString(TEXT("Clear Plain Island!"));
	SnowClearText = FText::FromString(TEXT("Clear Snow Island!"));
	DesertClearText = FText::FromString(TEXT("Clear Desert Island!"));
	GameOverText = FText::FromString(TEXT("Game Over"));
}

void UGameProgressMessageWidget::NativeConstruct()
{
	Super::NativeConstruct();

	if (ReturnToTitleButton)
	{
		ReturnToTitleButton->OnClicked.AddUniqueDynamic(
			this,
			&UGameProgressMessageWidget::HandleReturnToTitleButtonClicked
		);
		ReturnToTitleButton->SetVisibility(ESlateVisibility::Collapsed);
	}
}

void UGameProgressMessageWidget::ShowFieldClearMessage(FName MapID)
{
	if (MapID == TEXT("MAP_Plain"))
	{
		ShowMessage(PlainClearText, FieldClearTextColor, MapClearSound, true);
	}
	else if (MapID == TEXT("MAP_Snow"))
	{
		ShowMessage(SnowClearText, FieldClearTextColor, MapClearSound, true);
	}
	else if (MapID == TEXT("MAP_Desert"))
	{
		ShowMessage(DesertClearText, FieldClearTextColor, MapClearSound, true);
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
	ShowMessage(GameOverText, GameOverTextColor, GameOverSound, false);

	if (ReturnToTitleButton)
	{
		ReturnToTitleButton->SetVisibility(ESlateVisibility::Visible);
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("[GameProgressWidget] ReturnToTitleButton is not bound."));
	}
}

void UGameProgressMessageWidget::ShowMessage(
	const FText& Message,
	const FLinearColor& TextColor,
	USoundBase* Sound,
	bool bAutoHide)
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
	bAutoHideMessage = bAutoHide;
	SetRenderOpacity(FadeInDuration > 0.0f ? 0.0f : 1.0f);
	SetVisibility(bAutoHideMessage ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Visible);

	if (ReturnToTitleButton)
	{
		ReturnToTitleButton->SetVisibility(bAutoHideMessage ? ESlateVisibility::Collapsed : ESlateVisibility::Visible);
	}

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

	if (!bAutoHideMessage)
	{
		const float SafeFadeInDuration = FMath::Max(0.0f, FadeInDuration);
		const float Opacity = SafeFadeInDuration > 0.0f && MessageElapsedTime < SafeFadeInDuration
			? MessageElapsedTime / SafeFadeInDuration
			: 1.0f;

		SetRenderOpacity(FMath::Clamp(Opacity, 0.0f, 1.0f));
		return;
	}

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
	bAutoHideMessage = true;
	MessageElapsedTime = 0.0f;
	SetRenderOpacity(0.0f);
	SetVisibility(ESlateVisibility::Collapsed);

	if (ReturnToTitleButton)
	{
		ReturnToTitleButton->SetVisibility(ESlateVisibility::Collapsed);
	}
}

void UGameProgressMessageWidget::NativeDestruct()
{
	if (ReturnToTitleButton)
	{
		ReturnToTitleButton->OnClicked.RemoveDynamic(
			this,
			&UGameProgressMessageWidget::HandleReturnToTitleButtonClicked
		);
	}

	bMessageActive = false;
	Super::NativeDestruct();
}

void UGameProgressMessageWidget::HandleReturnToTitleButtonClicked()
{
	ARAPlayerController* RAPlayerController = Cast<ARAPlayerController>(GetOwningPlayer());
	if (!RAPlayerController)
	{
		UE_LOG(LogTemp, Warning, TEXT("[GameProgressWidget] Return to title skipped: owning RAPlayerController is unavailable."));
		return;
	}

	UE_LOG(LogTemp, Log, TEXT("[GameProgressWidget] Return to title requested from game over."));
	if (ULevelTransitionComponent* LevelTransitionComponent = RAPlayerController->GetLevelTransitionComponent())
	{
		LevelTransitionComponent->ReturnToTitleWithFade();
	}
}
