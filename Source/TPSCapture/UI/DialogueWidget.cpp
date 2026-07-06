#include "DialogueWidget.h"

#include "Components/Button.h"
#include "Components/TextBlock.h"
#include "Input/Reply.h"
#include "InputCoreTypes.h"

UDialogueWidget::UDialogueWidget(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	bIsFocusable = true;
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

	if (!DialogueText)
	{
		UE_LOG(LogTemp, Warning, TEXT("[DialogueWidget] DialogueText is not bound."));
	}

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

	bDialogueActive = false;
	Super::NativeDestruct();
}

FReply UDialogueWidget::NativeOnPreviewKeyDown(
	const FGeometry& InGeometry,
	const FKeyEvent& InKeyEvent)
{
	if (bDialogueActive && InKeyEvent.GetKey() == EKeys::SpaceBar)
	{
		if (!InKeyEvent.IsRepeat())
		{
			AdvanceDialogue();
		}

		return FReply::Handled();
	}

	return Super::NativeOnPreviewKeyDown(InGeometry, InKeyEvent);
}

void UDialogueWidget::BeginDialogue(const TArray<FText>& InDialogueLines)
{
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
	if (!bDialogueActive)
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
	OnDialogueFinished.Broadcast();
}

void UDialogueWidget::HandleNextButtonClicked()
{
	AdvanceDialogue();
}

void UDialogueWidget::HandleCloseButtonClicked()
{
	FinishDialogue();
}

void UDialogueWidget::RefreshDialogueText()
{
	if (!DialogueText || !DialogueLines.IsValidIndex(CurrentDialogueIndex))
	{
		return;
	}

	DialogueText->SetText(DialogueLines[CurrentDialogueIndex]);
}
