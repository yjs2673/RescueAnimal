#pragma once

#include "CoreMinimal.h"
#include "TPSUserWidget.h"
#include "TPSGameEnums.h"
#include "InventoryWidget.generated.h"

class UButton;
class UBorder;
class USizeBox;
class UUniformGridPanel;
class UInventorySlotWidget;
class UQuickSlotBarWidget;
class UInventoryComponent;

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnInventoryCloseRequested);

UCLASS()
class TPSCAPTURE_API UInventoryWidget : public UTPSUserWidget
{
	GENERATED_BODY()

public:
	UInventoryWidget(const FObjectInitializer& ObjectInitializer);

	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;

	virtual FReply NativeOnMouseButtonDown(
		const FGeometry& InGeometry,
		const FPointerEvent& InMouseEvent
	) override;

	virtual FReply NativeOnMouseButtonDoubleClick(
		const FGeometry& InGeometry,
		const FPointerEvent& InMouseEvent
	) override;

	virtual FReply NativeOnMouseMove(
		const FGeometry& InGeometry,
		const FPointerEvent& InMouseEvent
	) override;

	virtual FReply NativeOnMouseButtonUp(
		const FGeometry& InGeometry,
		const FPointerEvent& InMouseEvent
	) override;

public:
	UFUNCTION(BlueprintCallable, Category = "Inventory")
	void RefreshInventory();

	UFUNCTION(BlueprintCallable, Category = "Inventory")
	void SetInventoryFilter(EItemType NewItemType);

public:
	UPROPERTY(BlueprintAssignable, Category = "Inventory")
	FOnInventoryCloseRequested OnInventoryCloseRequested;

protected:
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<USizeBox> InventoryRootSizeBox;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UBorder> Upper_Border;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UButton> CloseButton;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UUniformGridPanel> InventoryGrid;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Inventory")
	TSubclassOf<UInventorySlotWidget> InventorySlotWidgetClass;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Inventory")
	int32 SlotColumnCount = 5;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Inventory")
	int32 MaxSlotCount = 20;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Inventory")
	EItemType CurrentFilterItemType = EItemType::Weapon;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UQuickSlotBarWidget> QuickSlotBar;

private:
	UFUNCTION()
	void HandleCloseButtonClicked();

	UFUNCTION()
	void HandleInventoryItemChanged(FName ItemID, int32 NewCount);

	void BuildEmptySlots();

	UInventoryComponent* GetInventoryComponent() const;
	void BindInventoryComponent();
	void UnbindInventoryComponent();

	bool IsMouseOverUpperBorder(const FPointerEvent& InMouseEvent) const;
	FVector2D GetInventoryPosition() const;
	void SetInventoryPosition(const FVector2D& NewPosition);

private:
	UPROPERTY()
	TArray<TObjectPtr<UInventorySlotWidget>> SlotWidgets;

	UPROPERTY()
	TObjectPtr<UInventoryComponent> CachedInventoryComponent;

	bool bIsDraggingInventory = false;
	FVector2D DragOffset = FVector2D::ZeroVector;

	static bool bHasSavedInventoryPosition;
	static FVector2D SavedInventoryPosition;
};
