#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "DialogueWidget.generated.h"

class UButton;
class UTextBlock;

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnDialogueFinished);

UCLASS()
class TPSCAPTURE_API UDialogueWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;

	UFUNCTION(BlueprintCallable, Category = "Dialogue")
	void BeginDialogue(const TArray<FText>& InDialogueLines);

	UFUNCTION(BlueprintCallable, Category = "Dialogue")
	void AdvanceDialogue();

	UFUNCTION(BlueprintCallable, Category = "Dialogue")
	void FinishDialogue();

	UFUNCTION(BlueprintPure, Category = "Dialogue")
	bool IsDialogueActive() const { return bDialogueActive; }

	UPROPERTY(BlueprintAssignable, Category = "Dialogue")
	FOnDialogueFinished OnDialogueFinished;

protected:
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> DialogueText;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UButton> NextButton;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UButton> CloseButton;

private:
	UFUNCTION()
	void HandleNextButtonClicked();

	UFUNCTION()
	void HandleCloseButtonClicked();

	void RefreshDialogueText();

	UPROPERTY(Transient)
	TArray<FText> DialogueLines;

	int32 CurrentDialogueIndex = INDEX_NONE;
	bool bDialogueActive = false;
};
