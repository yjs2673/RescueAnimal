#pragma once

#include "CoreMinimal.h"
#include "UI/Audio/RAUserWidget.h"
#include "DialogueWidget.generated.h"

class UButton;
class UPanelWidget;
class UTextBlock;

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnDialogueFinished);

UENUM(BlueprintType)
enum class EDialogueChoice : uint8
{
	Tutorial UMETA(DisplayName = "Tutorial"),
	Progress UMETA(DisplayName = "Progress"),
	Ending UMETA(DisplayName = "Ending")
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(
	FOnDialogueChoiceSelected,
	EDialogueChoice,
	SelectedChoice
);

UCLASS()
class RESCUEANIMAL_API UDialogueWidget : public URAUserWidget
{
	GENERATED_BODY()

public:
	UDialogueWidget(const FObjectInitializer& ObjectInitializer);

	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;
	virtual FReply NativeOnPreviewKeyDown(
		const FGeometry& InGeometry,
		const FKeyEvent& InKeyEvent
	) override;

	UFUNCTION(BlueprintCallable, Category = "Dialogue")
	void BeginDialogue(const TArray<FText>& InDialogueLines);

	UFUNCTION(BlueprintCallable, Category = "Dialogue")
	void AdvanceDialogue();

	UFUNCTION(BlueprintCallable, Category = "Dialogue")
	void FinishDialogue();

	UFUNCTION(BlueprintCallable, Category = "Dialogue")
	bool ShowChoices(
		const FText& Prompt,
		const FText& TutorialChoiceText,
		const FText& ProgressChoiceText,
		const FText& EndingChoiceText
	);

	UFUNCTION(BlueprintCallable, Category = "Dialogue")
	void HideChoices();

	UFUNCTION(BlueprintPure, Category = "Dialogue")
	bool IsDialogueActive() const { return bDialogueActive; }

	UFUNCTION(BlueprintPure, Category = "Dialogue")
	bool IsWaitingForChoice() const { return bWaitingForChoice; }

	UPROPERTY(BlueprintAssignable, Category = "Dialogue")
	FOnDialogueFinished OnDialogueFinished;

	UPROPERTY(BlueprintAssignable, Category = "Dialogue")
	FOnDialogueChoiceSelected OnDialogueChoiceSelected;

protected:
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> DialogueText;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UButton> NextButton;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UButton> CloseButton;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UPanelWidget> ChoiceContainer;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UButton> ChoiceAButton;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UButton> ChoiceBButton;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UButton> ChoiceCButton;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> ChoiceAText;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> ChoiceBText;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> ChoiceCText;

private:
	UFUNCTION()
	void HandleNextButtonClicked();

	UFUNCTION()
	void HandleCloseButtonClicked();

	UFUNCTION()
	void HandleChoiceAButtonClicked();

	UFUNCTION()
	void HandleChoiceBButtonClicked();

	UFUNCTION()
	void HandleChoiceCButtonClicked();

	void RefreshDialogueText();
	void SelectChoice(EDialogueChoice SelectedChoice);
	void SetChoiceControlsVisibility(ESlateVisibility InVisibility);

	UPROPERTY(Transient)
	TArray<FText> DialogueLines;

	int32 CurrentDialogueIndex = INDEX_NONE;
	bool bDialogueActive = false;
	bool bWaitingForChoice = false;
};
