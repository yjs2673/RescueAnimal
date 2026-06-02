#include "QuickSlotWidget.h"
#include "QuickSlotComponent.h"
#include "ItemDragDropOperation.h"
#include "Components/Image.h"
#include "Components/TextBlock.h"
#include "GameFramework/Pawn.h"
#include "TPSGameInstance.h"
#include "TPSCaptureCharacter.h"


void UQuickSlotWidget::NativeConstruct()
{
	Super::NativeConstruct();
}

FReply UQuickSlotWidget::NativeOnMouseButtonDown(
	const FGeometry& InGeometry,
	const FPointerEvent& InMouseEvent
)
{
	if (InMouseEvent.GetEffectingButton() == EKeys::LeftMouseButton && !ItemID.IsNone())
	{
		if (ATPSCaptureCharacter* PlayerCharacter = Cast<ATPSCaptureCharacter>(GetOwningPlayerPawn()))
		{
			PlayerCharacter->UseInventoryItem(ItemID);
		}

		return FReply::Handled();
	}

	return Super::NativeOnMouseButtonDown(InGeometry, InMouseEvent);
}

void UQuickSlotWidget::SetupQuickSlot(int32 InSlotIndex, FName InItemID, int32 InItemCount)
{
	SlotIndex = InSlotIndex;
	ItemID = InItemID;
	ItemCount = FMath::Max(0, InItemCount);

	if (SlotNumberText)
	{
		SlotNumberText->SetText(FText::AsNumber(SlotIndex + 1));
	}

	const bool bHasItem = !ItemID.IsNone();

	if (ItemIcon)
	{
		UTexture2D* ItemTexture = nullptr;

		if (bHasItem)
		{
			FItemData ItemData;
			const UTPSGameInstance* TPSGameInstance = GetGameInstance<UTPSGameInstance>();
			if (TPSGameInstance && TPSGameInstance->GetItemDataByID(ItemID, ItemData))
			{
				ItemTexture = ItemData.Image ? ItemData.Image : ItemData.Icon;
			}
		}

		if (ItemTexture)
		{
			ItemIcon->SetBrushFromTexture(ItemTexture, true);
			ItemIcon->SetVisibility(ESlateVisibility::Visible);
		}
		else
		{
			ItemIcon->SetVisibility(ESlateVisibility::Hidden);
		}
	}

	if (ItemCountText)
	{
		if (bHasItem && ItemCount > 0)
		{
			ItemCountText->SetText(FText::FromString(FString::Printf(TEXT("x%d"), ItemCount)));
			ItemCountText->SetVisibility(ESlateVisibility::Visible);
		}
		else
		{
			ItemCountText->SetText(FText::GetEmpty());
			ItemCountText->SetVisibility(ESlateVisibility::Hidden);
		}
	}
}

bool UQuickSlotWidget::NativeOnDrop(
	const FGeometry& InGeometry,
	const FDragDropEvent& InDragDropEvent,
	UDragDropOperation* InOperation
)
{
	UE_LOG(LogTemp, Warning, TEXT("QuickSlotWidget NativeOnDrop called. SlotIndex: %d"), SlotIndex);

	UItemDragDropOperation* ItemDragOperation = Cast<UItemDragDropOperation>(InOperation);

	if (!ItemDragOperation)
	{
		UE_LOG(LogTemp, Warning, TEXT("Drop failed: Operation is not ItemDragDropOperation."));
		return false;
	}

	if (ItemDragOperation->ItemID.IsNone())
	{
		UE_LOG(LogTemp, Warning, TEXT("Drop failed: ItemID is None."));
		return false;
	}

	APawn* OwningPawn = GetOwningPlayerPawn();
	if (!OwningPawn)
	{
		UE_LOG(LogTemp, Warning, TEXT("Drop failed: OwningPawn is null."));
		return false;
	}

	UQuickSlotComponent* QuickSlotComponent = OwningPawn->FindComponentByClass<UQuickSlotComponent>();
	if (!QuickSlotComponent)
	{
		UE_LOG(LogTemp, Warning, TEXT("Drop failed: QuickSlotComponent not found."));
		return false;
	}

	const bool bResult = QuickSlotComponent->SetSlotItem(SlotIndex, ItemDragOperation->ItemID);

	UE_LOG(LogTemp, Warning, TEXT("Drop result: %s / Slot: %d / Item: %s"),
		bResult ? TEXT("Success") : TEXT("Fail"),
		SlotIndex + 1,
		*ItemDragOperation->ItemID.ToString()
	);

	return bResult;
}
