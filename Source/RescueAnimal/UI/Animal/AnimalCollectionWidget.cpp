#include "AnimalCollectionWidget.h"

#include "Components/Button.h"
#include "Components/Image.h"
#include "Components/TextBlock.h"
#include "Engine/DataTable.h"
#include "RAGameInstance.h"

UAnimalCollectionWidget::UAnimalCollectionWidget(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	bPlayOpenCloseSounds = true;
}

void UAnimalCollectionWidget::NativeConstruct()
{
	Super::NativeConstruct();

	if (PreviousButton)
	{
		PreviousButton->OnClicked.AddUniqueDynamic(
			this,
			&UAnimalCollectionWidget::HandlePreviousButtonClicked
		);
	}

	if (NextButton)
	{
		NextButton->OnClicked.AddUniqueDynamic(
			this,
			&UAnimalCollectionWidget::HandleNextButtonClicked
		);
	}

	if (CloseButton)
	{
		CloseButton->OnClicked.AddUniqueDynamic(
			this,
			&UAnimalCollectionWidget::HandleCloseButtonClicked
		);
	}

	RefreshCollection();
}

void UAnimalCollectionWidget::NativeDestruct()
{
	if (PreviousButton)
	{
		PreviousButton->OnClicked.RemoveDynamic(
			this,
			&UAnimalCollectionWidget::HandlePreviousButtonClicked
		);
	}

	if (NextButton)
	{
		NextButton->OnClicked.RemoveDynamic(
			this,
			&UAnimalCollectionWidget::HandleNextButtonClicked
		);
	}

	if (CloseButton)
	{
		CloseButton->OnClicked.RemoveDynamic(
			this,
			&UAnimalCollectionWidget::HandleCloseButtonClicked
		);
	}

	Super::NativeDestruct();
}

void UAnimalCollectionWidget::RefreshCollection()
{
	BuildPages();

	if (Pages.Num() <= 0)
	{
		CurrentPageIndex = 0;
	}
	else
	{
		CurrentPageIndex = FMath::Clamp(CurrentPageIndex, 0, Pages.Num() - 1);
	}

	ShowCurrentPage();
}

void UAnimalCollectionWidget::BuildPages()
{
	Pages.Reset();

	if (!AnimalDataTable)
	{
		UE_LOG(LogTemp, Warning, TEXT("AnimalCollectionWidget: AnimalDataTable is not assigned."));
		return;
	}

	const TMap<FName, uint8*>& RowMap = AnimalDataTable->GetRowMap();

	for (const TPair<FName, uint8*>& RowPair : RowMap)
	{
		const FAnimalData* AnimalData = reinterpret_cast<const FAnimalData*>(RowPair.Value);
		if (!AnimalData)
		{
			continue;
		}

		FAnimalCollectionPageData PageData;
		PageData.AnimalID = AnimalData->AnimalID.IsNone()
			? RowPair.Key
			: AnimalData->AnimalID;
		PageData.AnimalData = *AnimalData;

		Pages.Add(PageData);
	}

	Pages.Sort([](
		const FAnimalCollectionPageData& Left,
		const FAnimalCollectionPageData& Right)
		{
			if (Left.AnimalData.CollectionOrder != Right.AnimalData.CollectionOrder)
			{
				return Left.AnimalData.CollectionOrder < Right.AnimalData.CollectionOrder;
			}

			return Left.AnimalID.LexicalLess(Right.AnimalID);
		});
}

void UAnimalCollectionWidget::ShowCurrentPage()
{
	if (Pages.Num() <= 0)
	{
		ClearAnimalPage();

		if (PageNumberText)
		{
			PageNumberText->SetText(FText::FromString(TEXT("0 / 0")));
		}

		return;
	}

	const FAnimalCollectionPageData& CurrentPage = Pages[CurrentPageIndex];
	const bool bUnlocked = IsAnimalUnlocked(CurrentPage.AnimalID);

	ShowAnimalPage(CurrentPage.AnimalData, bUnlocked);

	if (PageNumberText)
	{
		PageNumberText->SetText(FText::FromString(
			FString::Printf(
				TEXT("%d / %d"),
				CurrentPageIndex + 1,
				Pages.Num()
			)
		));
	}

	if (PreviousButton)
	{
		PreviousButton->SetIsEnabled(CurrentPageIndex > 0);
	}

	if (NextButton)
	{
		NextButton->SetIsEnabled(CurrentPageIndex < Pages.Num() - 1);
	}
}

void UAnimalCollectionWidget::ShowAnimalPage(const FAnimalData& AnimalData, bool bUnlocked)
{
	if (AnimalImage)
	{
		UTexture2D* DisplayTexture = bUnlocked
			? GetUnlockedAnimalImage(AnimalData)
			: LockedAnimalImage.Get();

		if (DisplayTexture)
		{
			AnimalImage->SetBrushFromTexture(DisplayTexture, true);
			AnimalImage->SetVisibility(ESlateVisibility::Visible);
		}
		else
		{
			AnimalImage->SetVisibility(ESlateVisibility::Hidden);
		}
	}

	if (AnimalNameText)
	{
		AnimalNameText->SetText(
			bUnlocked
			? AnimalData.DisplayName
			: LockedAnimalNameText
		);
	}

	if (AnimalDescriptionText)
	{
		AnimalDescriptionText->SetText(
			bUnlocked
			? AnimalData.Description
			: LockedAnimalDescriptionText
		);
	}
}

void UAnimalCollectionWidget::ClearAnimalPage()
{
	if (AnimalImage)
	{
		AnimalImage->SetVisibility(ESlateVisibility::Hidden);
	}

	if (AnimalNameText)
	{
		AnimalNameText->SetText(LockedAnimalNameText);
	}

	if (AnimalDescriptionText)
	{
		AnimalDescriptionText->SetText(LockedAnimalDescriptionText);
	}
}

bool UAnimalCollectionWidget::IsAnimalUnlocked(FName AnimalID) const
{
	const URAGameInstance* RAGameInstance = GetWorld()
		? GetWorld()->GetGameInstance<URAGameInstance>()
		: nullptr;

	return RAGameInstance && RAGameInstance->IsAnimalUnlocked(AnimalID);
}

UTexture2D* UAnimalCollectionWidget::GetUnlockedAnimalImage(const FAnimalData& AnimalData) const
{
	if (AnimalData.CollectionImage)
	{
		return AnimalData.CollectionImage;
	}

	return AnimalData.Icon;
}

void UAnimalCollectionWidget::HandlePreviousButtonClicked()
{
	if (CurrentPageIndex <= 0)
	{
		return;
	}

	--CurrentPageIndex;
	ShowCurrentPage();
}

void UAnimalCollectionWidget::HandleNextButtonClicked()
{
	if (CurrentPageIndex >= Pages.Num() - 1)
	{
		return;
	}

	++CurrentPageIndex;
	ShowCurrentPage();
}

void UAnimalCollectionWidget::HandleCloseButtonClicked()
{
	OnAnimalCollectionCloseRequested.Broadcast();
}
