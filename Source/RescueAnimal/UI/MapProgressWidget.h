#pragma once

#include "CoreMinimal.h"
#include "RAUserWidget.h"
#include "MapProgressWidget.generated.h"

class UButton;
class UTextBlock;
class AEnemyCampActor;
class ARAWorldStateManager;

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnMapProgressCloseRequested);

UCLASS()
class RESCUEANIMAL_API UMapProgressWidget : public URAUserWidget
{
	GENERATED_BODY()

public:
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;

	UFUNCTION(BlueprintCallable, Category = "Map Progress")
	void SetProgressText(const FText& InProgressText);

	UFUNCTION(BlueprintCallable, Category = "Map Progress")
	void InitializeForWorldStateManager(ARAWorldStateManager* InWorldStateManager);

	UFUNCTION(BlueprintCallable, Category = "Map Progress")
	void RefreshProgress();

	UFUNCTION(BlueprintCallable, Category = "Map Progress")
	void RequestClose();

	UPROPERTY(BlueprintAssignable, Category = "Map Progress")
	FOnMapProgressCloseRequested OnCloseRequested;

protected:
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> ProgressText;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> MapNameText;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> CampProgressText;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> AnimalProgressText;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UButton> CloseButton;

private:
	UFUNCTION()
	void HandleCloseButtonClicked();

	UFUNCTION()
	void HandleWorldProgressChanged();

	UFUNCTION()
	void HandleCampCleared(AEnemyCampActor* ClearedCamp);

	void BindProgressSources();
	void UnbindProgressSources();
	FText GetMapDisplayName(FName MapID) const;
	int32 GetExpectedCampCount(FName MapID) const;

	UPROPERTY(Transient)
	FText CachedProgressText;

	UPROPERTY(Transient)
	TObjectPtr<ARAWorldStateManager> CachedWorldStateManager;

	UPROPERTY(Transient)
	TArray<TObjectPtr<AEnemyCampActor>> BoundCamps;
};
