#pragma once

#include "CoreMinimal.h"
#include "TPSUserWidget.h"
#include "TPSStructTypes.h"
#include "AnimalCollectionWidget.generated.h"

class UButton;
class UImage;
class UTextBlock;
class UDataTable;
class UTexture2D;

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnAnimalCollectionCloseRequested);

USTRUCT(BlueprintType)
struct FAnimalCollectionPageData
{
	GENERATED_BODY()

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Animal Collection")
	FName AnimalID = NAME_None;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Animal Collection")
	FAnimalData AnimalData;
};

UCLASS()
class TPSCAPTURE_API UAnimalCollectionWidget : public UTPSUserWidget
{
	GENERATED_BODY()

public:
	UAnimalCollectionWidget(const FObjectInitializer& ObjectInitializer);

	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;

	UFUNCTION(BlueprintCallable, Category = "Animal Collection")
	void RefreshCollection();

	UPROPERTY(BlueprintAssignable, Category = "Animal Collection")
	FOnAnimalCollectionCloseRequested OnAnimalCollectionCloseRequested;

protected:
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UImage> AnimalImage;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> AnimalNameText;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> AnimalDescriptionText;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> PageNumberText;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UButton> PreviousButton;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UButton> NextButton;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UButton> CloseButton;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Animal Collection")
	TObjectPtr<UDataTable> AnimalDataTable;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Animal Collection")
	TObjectPtr<UTexture2D> LockedAnimalImage;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Animal Collection")
	FText LockedAnimalNameText = FText::FromString(TEXT("???"));

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Animal Collection")
	FText LockedAnimalDescriptionText = FText::FromString(TEXT("???"));

private:
	UFUNCTION()
	void HandlePreviousButtonClicked();

	UFUNCTION()
	void HandleNextButtonClicked();

	UFUNCTION()
	void HandleCloseButtonClicked();

	void BuildPages();
	void ShowCurrentPage();
	void ShowAnimalPage(const FAnimalData& AnimalData, bool bUnlocked);
	void ClearAnimalPage();
	bool IsAnimalUnlocked(FName AnimalID) const;
	UTexture2D* GetUnlockedAnimalImage(const FAnimalData& AnimalData) const;

	UPROPERTY()
	TArray<FAnimalCollectionPageData> Pages;

	int32 CurrentPageIndex = 0;
};
