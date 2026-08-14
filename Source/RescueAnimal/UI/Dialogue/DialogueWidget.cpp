#include "DialogueWidget.h"

#include "Components/Button.h"
#include "Components/PanelWidget.h"
#include "Components/TextBlock.h"
#include "Input/Reply.h"
#include "InputCoreTypes.h"

UDialogueWidget::UDialogueWidget(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	SetIsFocusable(true);
	bPlayOpenCloseSounds = true;
}

void UDialogueWidget::NativeConstruct()
{
	Super::NativeConstruct();

	if (NextButton)
	{
		NextButton->OnClicked.AddUniqueDynamic(this, &UDialogueWidget::HandleNextButtonClicked);
	}

	if (CloseButton)
	{
		CloseButton->OnClicked.AddUniqueDynamic(this, &UDialogueWidget::HandleCloseButtonClicked);
	}

	if (ChoiceAButton)
	{
		ChoiceAButton->OnClicked.AddUniqueDynamic(this, &UDialogueWidget::HandleChoiceAButtonClicked);
	}

	if (ChoiceBButton)
	{
		ChoiceBButton->OnClicked.AddUniqueDynamic(this, &UDialogueWidget::HandleChoiceBButtonClicked);
	}

	if (ChoiceCButton)
	{
		ChoiceCButton->OnClicked.AddUniqueDynamic(this, &UDialogueWidget::HandleChoiceCButtonClicked);
	}

	if (!DialogueText)
	{
		UE_LOG(LogTemp, Warning, TEXT("[DialogueWidget] DialogueText is not bound."));
	}

	HideChoices();
	RefreshDialogueText();
}

void UDialogueWidget::NativeDestruct()
{
	if (NextButton)
	{
		NextButton->OnClicked.RemoveDynamic(this, &UDialogueWidget::HandleNextButtonClicked);
	}

	if (CloseButton)
	{
		CloseButton->OnClicked.RemoveDynamic(this, &UDialogueWidget::HandleCloseButtonClicked);
	}

	if (ChoiceAButton)
	{
		ChoiceAButton->OnClicked.RemoveDynamic(this, &UDialogueWidget::HandleChoiceAButtonClicked);
	}

	if (ChoiceBButton)
	{
		ChoiceBButton->OnClicked.RemoveDynamic(this, &UDialogueWidget::HandleChoiceBButtonClicked);
	}

	if (ChoiceCButton)
	{
		ChoiceCButton->OnClicked.RemoveDynamic(this, &UDialogueWidget::HandleChoiceCButtonClicked);
	}

	bDialogueActive = false;
	bWaitingForChoice = false;
	Super::NativeDestruct();
}

FReply UDialogueWidget::NativeOnPreviewKeyDown(
	const FGeometry& InGeometry,
	const FKeyEvent& InKeyEvent)
{
	if (bDialogueActive && InKeyEvent.GetKey() == EKeys::SpaceBar)
	{
		if (!bWaitingForChoice && !InKeyEvent.IsRepeat())
		{
			AdvanceDialogue();
		}

		return FReply::Handled();
	}

	return Super::NativeOnPreviewKeyDown(InGeometry, InKeyEvent);
}

void UDialogueWidget::BeginDialogue(const TArray<FText>& InDialogueLines)
{
	HideChoices();
	DialogueLines = InDialogueLines;
	CurrentDialogueIndex = DialogueLines.IsEmpty() ? INDEX_NONE : 0;
	bDialogueActive = CurrentDialogueIndex != INDEX_NONE;

	if (!bDialogueActive)
	{
		UE_LOG(LogTemp, Warning, TEXT("[DialogueWidget] Dialogue cannot start: DialogueLines is empty."));
		OnDialogueFinished.Broadcast();
		return;
	}

	SetVisibility(ESlateVisibility::Visible);
	RefreshDialogueText();
	SetKeyboardFocus();
}

void UDialogueWidget::AdvanceDialogue()
{
	if (!bDialogueActive || bWaitingForChoice)
	{
		return;
	}

	if (DialogueLines.IsValidIndex(CurrentDialogueIndex + 1))
	{
		++CurrentDialogueIndex;
		RefreshDialogueText();
		return;
	}

	FinishDialogue();
}

void UDialogueWidget::FinishDialogue()
{
	if (!bDialogueActive)
	{
		return;
	}

	bDialogueActive = false;
	bWaitingForChoice = false;
	HideChoices();
	OnDialogueFinished.Broadcast();
}

bool UDialogueWidget::ShowChoices(
	const FText& Prompt,
	const FText& TutorialChoiceText,
	const FText& ProgressChoiceText,
	const FText& EndingChoiceText)
{
	if (!DialogueText || !ChoiceAButton || !ChoiceBButton || !ChoiceCButton)
	{
		UE_LOG(
			LogTemp,
			Warning,
			TEXT("[DialogueWidget] Choice UI is incomplete. Bind DialogueText and ChoiceAButton/ChoiceBButton/ChoiceCButton in the Widget Blueprint.")
		);
		return false;
	}

	DialogueLines.Reset();
	CurrentDialogueIndex = INDEX_NONE;
	bDialogueActive = true;
	bWaitingForChoice = true;

	DialogueText->SetText(Prompt);
	if (ChoiceAText)
	{
		ChoiceAText->SetText(TutorialChoiceText);
	}
	if (ChoiceBText)
	{
		ChoiceBText->SetText(ProgressChoiceText);
	}
	if (ChoiceCText)
	{
		ChoiceCText->SetText(EndingChoiceText);
	}

	if (NextButton)
	{
		NextButton->SetVisibility(ESlateVisibility::Collapsed);
	}
	SetChoiceControlsVisibility(ESlateVisibility::Visible);
	SetVisibility(ESlateVisibility::Visible);
	SetKeyboardFocus();

	return true;
}

void UDialogueWidget::HideChoices()
{
	bWaitingForChoice = false;
	SetChoiceControlsVisibility(ESlateVisibility::Collapsed);

	if (NextButton)
	{
		NextButton->SetVisibility(ESlateVisibility::Visible);
	}
}

void UDialogueWidget::HandleNextButtonClicked()
{
	AdvanceDialogue();
}

void UDialogueWidget::HandleCloseButtonClicked()
{
	FinishDialogue();
}

void UDialogueWidget::HandleChoiceAButtonClicked()
{
	SelectChoice(EDialogueChoice::Tutorial);
}

void UDialogueWidget::HandleChoiceBButtonClicked()
{
	SelectChoice(EDialogueChoice::Progress);
}

void UDialogueWidget::HandleChoiceCButtonClicked()
{
	SelectChoice(EDialogueChoice::Ending);
}

void UDialogueWidget::RefreshDialogueText()
{
	if (!DialogueText || !DialogueLines.IsValidIndex(CurrentDialogueIndex))
	{
		return;
	}

	DialogueText->SetText(DialogueLines[CurrentDialogueIndex]);
}

void UDialogueWidget::SelectChoice(EDialogueChoice SelectedChoice)
{
	if (!bDialogueActive || !bWaitingForChoice)
	{
		return;
	}

	bDialogueActive = false;
	HideChoices();
	OnDialogueChoiceSelected.Broadcast(SelectedChoice);
}

void UDialogueWidget::SetChoiceControlsVisibility(ESlateVisibility InVisibility)
{
	if (ChoiceContainer)
	{
		ChoiceContainer->SetVisibility(InVisibility);
	}

	if (ChoiceAButton)
	{
		ChoiceAButton->SetVisibility(InVisibility);
	}
	if (ChoiceBButton)
	{
		ChoiceBButton->SetVisibility(InVisibility);
	}
	if (ChoiceCButton)
	{
		ChoiceCButton->SetVisibility(InVisibility);
	}
}
