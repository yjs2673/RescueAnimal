#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "LevelTransitionComponent.generated.h"

class ARAPlayerController;

UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class RESCUEANIMAL_API ULevelTransitionComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	ULevelTransitionComponent();

	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	void HandleControllerBeginPlay();
	void StartLevelFadeIn();
	void PlayLevelFadeIn();
	void FinishLevelFadeIn();
	void TravelToLevelWithFade(FName TargetLevelName, float FadeOutDuration = -1.0f);
	void ReturnToTitleWithFade();
	void QuitGame();
	void OpenPendingFadeTravelLevel();
	void TryCreateGameFlowMenuWidget();
	bool IsTitleLevelName(const FString& LevelName) const;
	bool IsEndingLevelName(const FString& LevelName) const;
	bool IsGameFlowMenuLevel() const;
	void StartViewportFadeOverlay(float FromOpacity, float ToOpacity, float Duration, bool bRemoveWhenFinished);
	void TickViewportFadeOverlay();
	void EnsureViewportFadeOverlay(float InitialOpacity);
	void SetViewportFadeOverlayOpacity(float Opacity);
	void RemoveViewportFadeOverlay();

private:
	ARAPlayerController* GetOwnerController() const;
};
