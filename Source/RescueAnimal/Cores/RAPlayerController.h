#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "InputCoreTypes.h"
#include "TimerManager.h"
#include "RAPlayerController.generated.h"

class UMainHUDWidget;
class UInventoryWidget;
class UShopWidget;
class UAnimalCollectionWidget;
class UGameProgressMessageWidget;
class UMapProgressWidget;
class UGameFlowMenuWidget;
class USettingWidget;
class AShopActor;
class ARAWorldStateManager;
class SBorder;
class SWidget;

UCLASS()
class RESCUEANIMAL_API ARAPlayerController : public APlayerController
{
	GENERATED_BODY()

public:
	ARAPlayerController();

	virtual void BeginPlay() override;
	virtual void SetupInputComponent() override;

public:
	UFUNCTION()
	void ToggleInventory();

	UFUNCTION()
	void OpenInventory();

	UFUNCTION()
	void CloseInventory();

	UFUNCTION()
	void ToggleAnimalCollection();

	UFUNCTION()
	void OpenAnimalCollection();

	UFUNCTION()
	void CloseAnimalCollection();

	UMainHUDWidget* GetMainHUDWidget() const { return MainHUDWidget; }
	bool IsInventoryOpen() const { return bIsInventoryOpen; }

	UFUNCTION()
	void OpenShop(AShopActor* ShopActor);

	UFUNCTION()
	void CloseShop();

	UFUNCTION()
	void CloseUI();

	UFUNCTION()
	void ToggleSetting();

	UFUNCTION()
	void OpenSetting();

	UFUNCTION()
	void CloseSetting();

	bool IsShopOpen() const { return bIsShopOpen; }
	bool IsAnimalCollectionOpen() const { return bIsAnimalCollectionOpen; }
	void HideMainHUD();
	void ShowMainHUD();
	void SetPortalTransitionInputLocked(bool bLocked);
	bool IsPortalTransitionInputLocked() const { return bIsPortalTransitionInputLocked || bIsSettingOpen; }

	UFUNCTION(BlueprintCallable, Category = "Game Flow|Transition")
	void TravelToLevelWithFade(FName TargetLevelName, float FadeOutDuration = -1.0f);

	UFUNCTION(BlueprintCallable, Category = "Game Flow|Transition")
	void ReturnToTitleWithFade();

	UFUNCTION(BlueprintCallable, Category = "Game Flow")
	void QuitGame();

#pragma region Game Progress Message
	UFUNCTION(BlueprintCallable, Category = "Game Progress")
	void ShowFieldClearMessage(FName MapID);

	UFUNCTION(BlueprintCallable, Category = "Game Progress")
	void ShowGameOverMessage();
#pragma endregion Game Progress Message

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "UI|Map Progress")
	TSubclassOf<UMapProgressWidget> MapProgressWidgetClass;

protected:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "UI|Mouse Cursor")
	bool bShowMouseCursorInGame = false;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "UI|Mouse Cursor")
	TEnumAsByte<EMouseCursor::Type> NormalMouseCursor = EMouseCursor::Default;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "UI")
	TSubclassOf<UMainHUDWidget> MainHUDWidgetClass;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "UI")
	TSubclassOf<UInventoryWidget> InventoryWidgetClass;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "UI")
	TSubclassOf<UShopWidget> ShopWidgetClass;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "UI")
	TSubclassOf<UAnimalCollectionWidget> AnimalCollectionWidgetClass;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "UI")
	TSubclassOf<USettingWidget> SettingWidgetClass;

#pragma region Game Progress Message
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "UI|Game Progress")
	TSubclassOf<UGameProgressMessageWidget> GameProgressMessageWidgetClass;
#pragma endregion Game Progress Message

	UPROPERTY()
	TObjectPtr<UMapProgressWidget> MapProgressWidget;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Portal|Transition")
	float FadeInDuration = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Portal|Transition", meta = (ClampMin = "0.0"))
	float FadeInStartDelay = 0.05f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Game Flow|Transition", meta = (ClampMin = "0.0"))
	float DefaultFadeOutDuration = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Game Flow|Maps")
	FName TitleMapName = TEXT("MAP_Title");

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Game Flow|Maps")
	FName EndingMapName = TEXT("MAP_Ending");

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "UI|Game Flow")
	TSubclassOf<UGameFlowMenuWidget> TitleMenuWidgetClass;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "UI|Game Flow")
	TSubclassOf<UGameFlowMenuWidget> EndingMenuWidgetClass;

	UPROPERTY()
	TObjectPtr<AShopActor> CurrentShopActor;

private:
	UPROPERTY()
	TObjectPtr<UMainHUDWidget> MainHUDWidget;

	UPROPERTY()
	TObjectPtr<UInventoryWidget> InventoryWidget;

	UPROPERTY()
	bool bIsInventoryOpen = false;

	UPROPERTY()
	TObjectPtr<UShopWidget> ShopWidget;

	UPROPERTY()
	bool bIsShopOpen = false;

	UPROPERTY()
	TObjectPtr<UAnimalCollectionWidget> AnimalCollectionWidget;

	UPROPERTY()
	TObjectPtr<USettingWidget> SettingWidget;

#pragma region Game Progress Message
	UPROPERTY()
	TObjectPtr<UGameProgressMessageWidget> GameProgressMessageWidget;
#pragma endregion Game Progress Message

	UPROPERTY()
	bool bIsAnimalCollectionOpen = false;

	UPROPERTY()
	bool bIsSettingOpen = false;

	UPROPERTY()
	bool bIsPortalTransitionInputLocked = false;

	FTimerHandle FadeInTimerHandle;
	FTimerHandle FadeInStartTimerHandle;
	FTimerHandle FadeOutTimerHandle;
	FTimerHandle ViewportFadeOverlayTimerHandle;

	FName PendingFadeTravelLevelName;

	UPROPERTY()
	TObjectPtr<UGameFlowMenuWidget> ActiveGameFlowMenuWidget;

	TSharedPtr<SWidget> ViewportFadeOverlayRootWidget;
	TSharedPtr<SBorder> ViewportFadeOverlayBorderWidget;
	float ViewportFadeOverlayStartOpacity = 0.0f;
	float ViewportFadeOverlayTargetOpacity = 0.0f;
	float ViewportFadeOverlayDuration = 0.0f;
	float ViewportFadeOverlayElapsedTime = 0.0f;
	bool bRemoveViewportFadeOverlayWhenFinished = false;

private:
	UFUNCTION()
	void HandleInventoryButtonClicked();

	UFUNCTION()
	void HandleSettingButtonClicked();

	UFUNCTION()
	void HandleInventoryCloseRequested();

	UFUNCTION()
	void HandleSettingCloseRequested();

	UFUNCTION()
	void HandleAnimalCollectionCloseRequested();

	void SetGameInputMode();
	void SetUIInputMode();
	void SetSettingInputMode();
	void SetMenuInputMode();
	void InitializeMouseCursor();
	void SetMouseCursorType(EMouseCursor::Type NewMouseCursor);
	void StartLevelFadeIn();
	void PlayLevelFadeIn();
	void FinishLevelFadeIn();
	void OpenPendingFadeTravelLevel();
	void StartViewportFadeOverlay(float FromOpacity, float ToOpacity, float Duration, bool bRemoveWhenFinished);
	void TickViewportFadeOverlay();
	void EnsureViewportFadeOverlay(float InitialOpacity);
	void SetViewportFadeOverlayOpacity(float Opacity);
	void RemoveViewportFadeOverlay();
	void TryCreateMapProgressWidget();
	void TryCreateGameFlowMenuWidget();
	bool IsTitleLevelName(const FString& LevelName) const;
	bool IsEndingLevelName(const FString& LevelName) const;
	bool IsGameFlowMenuLevel() const;
};
