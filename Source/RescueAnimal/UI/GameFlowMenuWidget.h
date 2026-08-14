#pragma once

#include "CoreMinimal.h"
#include "RAUserWidget.h"
#include "GameFlowMenuWidget.generated.h"

class UButton;

UCLASS()
class RESCUEANIMAL_API UGameFlowMenuWidget : public URAUserWidget
{
	GENERATED_BODY()

public:
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;

	UFUNCTION(BlueprintCallable, Category = "Game Flow")
	void StartGame();

	UFUNCTION(BlueprintCallable, Category = "Game Flow")
	void ReturnToTitle();

	UFUNCTION(BlueprintCallable, Category = "Game Flow")
	void QuitGame();

protected:
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UButton> StartGameButton;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UButton> TitleButton;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UButton> SettingButton;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UButton> QuitButton;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Game Flow")
	FName StartGameTargetLevelName = TEXT("MAP_Lobby");

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Game Flow")
	FName TitleTargetLevelName = TEXT("MAP_Title");

private:
	UFUNCTION()
	void HandleStartGameButtonClicked();

	UFUNCTION()
	void HandleTitleButtonClicked();

	UFUNCTION()
	void HandleSettingButtonClicked();

	UFUNCTION()
	void HandleQuitButtonClicked();
};
