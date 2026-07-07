#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "GameProgressMessageWidget.generated.h"

class UTextBlock;
class UButton;
class USoundBase;

UCLASS()
class TPSCAPTURE_API UGameProgressMessageWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UGameProgressMessageWidget(const FObjectInitializer& ObjectInitializer);

	virtual void NativeConstruct() override;

	UFUNCTION(BlueprintCallable, Category = "Game Progress")
	void ShowFieldClearMessage(FName MapID);

	UFUNCTION(BlueprintCallable, Category = "Game Progress")
	void ShowGameOverMessage();

protected:
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;
	virtual void NativeDestruct() override;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> MessageText;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UButton> ReturnToTitleButton;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Game Progress|Text")
	FText PlainClearText;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Game Progress|Text")
	FText SnowClearText;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Game Progress|Text")
	FText DesertClearText;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Game Progress|Text")
	FText GameOverText;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Game Progress", meta = (ClampMin = "0.1"))
	float MessageDuration = 3.0f;

#pragma region Fade
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Game Progress|Fade", meta = (ClampMin = "0.0"))
	float FadeInDuration = 0.5f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Game Progress|Fade", meta = (ClampMin = "0.0"))
	float FadeOutDuration = 0.5f;
#pragma endregion Fade

#pragma region Color
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Game Progress|Color")
	FLinearColor FieldClearTextColor = FLinearColor::White;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Game Progress|Color")
	FLinearColor GameOverTextColor = FLinearColor::Red;
#pragma endregion Color

#pragma region SFX
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Game Progress|SFX")
	TObjectPtr<USoundBase> MapClearSound = nullptr;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Game Progress|SFX")
	TObjectPtr<USoundBase> GameOverSound = nullptr;
#pragma endregion SFX

private:
	UFUNCTION()
	void HandleReturnToTitleButtonClicked();

	void ShowMessage(const FText& Message, const FLinearColor& TextColor, USoundBase* Sound, bool bAutoHide);
	void HideMessage();

	float MessageElapsedTime = 0.0f;
	bool bMessageActive = false;
	bool bAutoHideMessage = true;
};
