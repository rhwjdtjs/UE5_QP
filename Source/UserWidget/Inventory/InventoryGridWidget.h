#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "InventoryGridWidget.generated.h"


UCLASS()
class PJ_QUIET_PROTOCOL_API UInventoryGridWidget : public UUserWidget
{
	GENERATED_BODY()
	
public:
	void SetInventory(class UInventoryComponent* InInventory); // 인벤토트 컴포넌트 설정
	void RefreshGrid(); // 그리드 새로고침(셀+아이템 재구성)
	bool HandleDropFromScreenPos(UDragDropOperation* Operation, const FVector2D& ScreenPos); // 화면 좌표에서 드롭 처리

protected:
	virtual void NativeConstruct() override; // 위젯 생성 시
	virtual bool NativeOnDrop(const FGeometry& InGeometry, const FDragDropEvent& InDragDropEvent, UDragDropOperation* InOperation) override; // 드롭 이벤트 처리

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<class USizeBox> GridSizeBox; // 있으면 자동 크기 맞춤

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<class UCanvasPanel> CellsLayer; // 셀 레이어

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<class UCanvasPanel> ItemsLayer; // 아이템 레이어

	// === BP에서 지정 ===
	UPROPERTY(EditDefaultsOnly, Category = "InventoryUI")
	float CellSize = 64.f; // 셀 크기

	UPROPERTY(EditDefaultsOnly, Category = "InventoryUI")
	TSubclassOf<UUserWidget> CellWidgetClass; // WBP_InventoryCell // 셀 위젯 클래스

	UPROPERTY(EditDefaultsOnly, Category = "InventoryUI")
	TSubclassOf<class UItemIconWidget> ItemIconWidgetClass; // WBP_ItemIcon(Parent=UQPItemIconWidget) // 아이템 아이콘 위젯 클래스

	UPROPERTY(EditDefaultsOnly, Category = "InventoryUI")
	TSubclassOf<UUserWidget> DragVisualClass; // WBP_ItemDragVisual(선택) // 드래그 비주얼 클래스

private:
	UPROPERTY()
	TObjectPtr<class UInventoryComponent> Inventory = nullptr; // 인벤토리 컴포넌트

	void BuildCells(); // 셀 구성
	void BuildItems(); // 아이템 구성

	bool ScreenToCell(const FVector2D& ScreenPos, FIntPoint& OutCell) const; // 화면 좌표 -> 셀 좌표 변환
};
