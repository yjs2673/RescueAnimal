#include "MapProgressWidget.h"

#include "EnemyCampActor.h"
#include "TPSWorldStateManager.h"

#include "Components/Button.h"
#include "Components/TextBlock.h"
#include "EngineUtils.h"

void UMapProgressWidget::NativeConstruct()
{
	Super::NativeConstruct();

	if (CloseButton)
	{
		CloseButton->OnClicked.AddUniqueDynamic(this, &UMapProgressWidget::HandleCloseButtonClicked);
	}

	if (!MapNameText)
	{
		UE_LOG(LogTemp, Warning, TEXT("[MapProgressWidget] MapNameText is not bound."));
	}

	if (!CampProgressText)
	{
		UE_LOG(LogTemp, Warning, TEXT("[MapProgressWidget] CampProgressText is not bound."));
	}

	if (!AnimalProgressText)
	{
		UE_LOG(LogTemp, Warning, TEXT("[MapProgressWidget] AnimalProgressText is not bound."));
	}

	if (ProgressText)
	{
		ProgressText->SetText(CachedProgressText);
	}

	RefreshProgress();
}

void UMapProgressWidget::NativeDestruct()
{
	UnbindProgressSources();

	if (CloseButton)
	{
		CloseButton->OnClicked.RemoveDynamic(this, &UMapProgressWidget::HandleCloseButtonClicked);
	}

	Super::NativeDestruct();
}

void UMapProgressWidget::InitializeForWorldStateManager(ATPSWorldStateManager* InWorldStateManager)
{
	if (CachedWorldStateManager == InWorldStateManager)
	{
		RefreshProgress();
		return;
	}

	UnbindProgressSources();
	CachedWorldStateManager = InWorldStateManager;
	BindProgressSources();
	RefreshProgress();
}

void UMapProgressWidget::RefreshProgress()
{
	if (!CachedWorldStateManager)
	{
		return;
	}

	const FName MapID = CachedWorldStateManager->MapID;
	const int32 TotalCampCount = GetExpectedCampCount(MapID);
	int32 ClearedCampCount = 0;

	for (const TObjectPtr<AEnemyCampActor>& Camp : BoundCamps)
	{
		if (IsValid(Camp) && Camp->IsCampCleared())
		{
			++ClearedCampCount;
		}
	}

	ClearedCampCount = FMath::Clamp(ClearedCampCount, 0, TotalCampCount);
	const int32 RescuedAnimalCount = CachedWorldStateManager->GetRescuedAnimalCount();
	const int32 TotalAnimalCount = CachedWorldStateManager->TotalAnimalCount;

	if (MapNameText)
	{
		MapNameText->SetText(GetMapDisplayName(MapID));
	}

	if (CampProgressText)
	{
		CampProgressText->SetText(FText::Format(
			NSLOCTEXT("MapProgressWidget", "CampProgress", "클리어된 캠프: {0} / {1}"),
			FText::AsNumber(ClearedCampCount),
			FText::AsNumber(TotalCampCount)
		));
	}

	if (AnimalProgressText)
	{
		AnimalProgressText->SetText(FText::Format(
			NSLOCTEXT("MapProgressWidget", "AnimalProgress", "구조된 동물: {0} / {1}"),
			FText::AsNumber(RescuedAnimalCount),
			FText::AsNumber(TotalAnimalCount)
		));
	}

	SetProgressText(FText::Format(
		NSLOCTEXT("MapProgressWidget", "LegacyProgress", "{0}\n캠프: {1} / {2}\n동물: {3} / {4}"),
		GetMapDisplayName(MapID),
		FText::AsNumber(ClearedCampCount),
		FText::AsNumber(TotalCampCount),
		FText::AsNumber(RescuedAnimalCount),
		FText::AsNumber(TotalAnimalCount)
	));
}

void UMapProgressWidget::SetProgressText(const FText& InProgressText)
{
	CachedProgressText = InProgressText;

	if (ProgressText)
	{
		ProgressText->SetText(CachedProgressText);
	}
}

void UMapProgressWidget::RequestClose()
{
	OnCloseRequested.Broadcast();
}

void UMapProgressWidget::HandleCloseButtonClicked()
{
	RequestClose();
}

void UMapProgressWidget::HandleWorldProgressChanged()
{
	RefreshProgress();
}

void UMapProgressWidget::HandleCampCleared(AEnemyCampActor* ClearedCamp)
{
	RefreshProgress();
}

void UMapProgressWidget::BindProgressSources()
{
	if (CachedWorldStateManager)
	{
		CachedWorldStateManager->OnWorldProgressChanged.AddUniqueDynamic(
			this,
			&UMapProgressWidget::HandleWorldProgressChanged
		);
	}

	BoundCamps.Reset();
	if (!GetWorld())
	{
		return;
	}

	for (TActorIterator<AEnemyCampActor> It(GetWorld()); It; ++It)
	{
		AEnemyCampActor* Camp = *It;
		if (!IsValid(Camp))
		{
			continue;
		}

		BoundCamps.Add(Camp);
		Camp->OnEnemyCampCleared.AddUniqueDynamic(this, &UMapProgressWidget::HandleCampCleared);
	}
}

void UMapProgressWidget::UnbindProgressSources()
{
	if (CachedWorldStateManager)
	{
		CachedWorldStateManager->OnWorldProgressChanged.RemoveDynamic(
			this,
			&UMapProgressWidget::HandleWorldProgressChanged
		);
	}

	for (const TObjectPtr<AEnemyCampActor>& Camp : BoundCamps)
	{
		if (IsValid(Camp))
		{
			Camp->OnEnemyCampCleared.RemoveDynamic(this, &UMapProgressWidget::HandleCampCleared);
		}
	}

	BoundCamps.Reset();
}

FText UMapProgressWidget::GetMapDisplayName(FName MapID) const
{
	if (MapID == TEXT("MAP_Plain"))
	{
		return NSLOCTEXT("MapProgressWidget", "PlainMap", "초원 섬");
	}

	if (MapID == TEXT("MAP_Snow"))
	{
		return NSLOCTEXT("MapProgressWidget", "SnowMap", "설원 섬");
	}

	if (MapID == TEXT("MAP_Desert"))
	{
		return NSLOCTEXT("MapProgressWidget", "DesertMap", "사막 섬");
	}

	return FText::FromName(MapID);
}

int32 UMapProgressWidget::GetExpectedCampCount(FName MapID) const
{
	if (MapID == TEXT("MAP_Plain"))
	{
		return 3;
	}

	if (MapID == TEXT("MAP_Snow") || MapID == TEXT("MAP_Desert"))
	{
		return 2;
	}

	return BoundCamps.Num();
}
