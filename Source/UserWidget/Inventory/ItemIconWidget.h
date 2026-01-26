#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "ItemIconWidget.generated.h"


UCLASS()
class PJ_QUIET_PROTOCOL_API UItemIconWidget : public UUserWidget
{
	GENERATED_BODY()
	
public:
	// Grid가 생성 직후 호출해 세팅
	void Setup(class UInventoryComponent* InInventory, class UItemDataAsset* InItemData,int32 InQuantity,
		const FIntPoint& InFrom,
		const FIntPoint& InItemSize,
		float InCellSize,
		TSubclassOf<UUserWidget> InDragVisualClass,
		class UInventoryGridWidget* InOwningGrid
	); // Setup

protected:
	virtual FReply NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override; // 드래그 시작을 위해 필요
	virtual void NativeOnDragDetected(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent, UDragDropOperation*& OutOperation) override; // 드래그 시작
	virtual bool NativeOnDrop(const FGeometry& InGeometry, const FDragDropEvent& InDragDropEvent, UDragDropOperation* InOperation) override; // 드롭 처리

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<class UImage> ItemImage; // 아이템 이미지

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<class UTextBlock> QuantityText; // 수량 텍스트

private:
	UPROPERTY()
	TObjectPtr<class UInventoryComponent> Inventory = nullptr; // 소유한 인벤토리 컴포넌트

	UPROPERTY()
	TObjectPtr<class UItemDataAsset> ItemData = nullptr; // 아이템 데이터 에셋

	UPROPERTY()
	int32 Quantity = 1; // 수량

	UPROPERTY()
	FIntPoint From = FIntPoint::ZeroValue; // 그리드 내 위치

	UPROPERTY()
	FIntPoint ItemSize = FIntPoint(1, 1); // 아이템 크기 (셀 단위)

	UPROPERTY()
	float CellSize = 64.f; // 셀 크기 (픽셀 단위)

	UPROPERTY()
	TSubclassOf<UUserWidget> DragVisualClass; // 드래그 시 사용할 비주얼 클래스

	UPROPERTY()
	TObjectPtr<class UInventoryGridWidget> OwningGrid = nullptr; // 소유한 그리드 위젯

	void ApplyVisual(); // 시각적 요소 적용
};
