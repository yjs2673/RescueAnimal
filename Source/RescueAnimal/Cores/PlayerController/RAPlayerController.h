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
class UGameProgressUIComponent;
class ULevelTransitionComponent;
class UPlayerUIFlowComponent;

UCLASS()
class RESCUEANIMAL_API ARAPlayerController : public APlayerController
{
	GENERATED_BODY()

	friend class UGameProgressUIComponent;
	friend class ULevelTransitionComponent;
	friend class UPlayerUIFlowComponent;

public:
	ARAPlayerController();

	virtual void BeginPlay() override;
	virtual void SetupInputComponent() override;

	UFUNCTION(BlueprintPure, Category = "Components")
	FORCEINLINE UPlayerUIFlowComponent* GetPlayerUIFlowComponent() const { return PlayerUIFlowComponent.Get(); }

	UFUNCTION(BlueprintPure, Category = "Components")
	FORCEINLINE UGameProgressUIComponent* GetGameProgressUIComponent() const { return GameProgressUIComponent.Get(); }

	UFUNCTION(BlueprintPure, Category = "Components")
	FORCEINLINE ULevelTransitionComponent* GetLevelTransitionComponent() const { return LevelTransitionComponent.Get(); }

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
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UPlayerUIFlowComponent> PlayerUIFlowComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UGameProgressUIComponent> GameProgressUIComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<ULevelTransitionComponent> LevelTransitionComponent;

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
};
