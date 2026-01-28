#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "LootListEntryWidget.generated.h"

UCLASS()
class PJ_QUIET_PROTOCOL_API ULootListEntryWidget : public UUserWidget
{
	GENERATED_BODY()
	
public:
	void Setup(AActor* InSourceActor, class UItemDataAsset* InItemData, int32 InQuantity); // 설정 함수

protected:
	virtual FReply NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override; // 마우스 버튼 다운 이벤트
	virtual void NativeOnDragDetected(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent, UDragDropOperation*& OutOperation) override; // 드래그 감지 이벤트

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<class UImage> IconImage; // 아이콘 이미지

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<class UTextBlock> NameText; // 이름 텍스트

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<class UTextBlock> SizeText; // 크기 텍스트

	UPROPERTY(EditDefaultsOnly, Category = "Loot|Drag")
	TSubclassOf<UUserWidget> DragVisualClass; // 드래그 비주얼 클래스

	// 인벤 셀 크기(드래그 비주얼 크기 계산용). 프로젝트 기본 64로 시작.
	UPROPERTY(EditDefaultsOnly, Category = "Loot|Drag")
	float CellSize = 64.f; // 인벤토리 셀 크기

private:
	UPROPERTY()
	TWeakObjectPtr<AActor> SourceActor; // 출처 액터

	UPROPERTY()
	TObjectPtr<class UItemDataAsset> ItemData; // 아이템 데이터

	UPROPERTY()
	int32 Quantity = 1; // 수량
};
