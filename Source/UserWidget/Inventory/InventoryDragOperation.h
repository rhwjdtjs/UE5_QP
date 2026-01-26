#pragma once

#include "CoreMinimal.h"
#include "Blueprint/DragDropOperation.h"
#include "InventoryDragOperation.generated.h"


UCLASS()
class PJ_QUIET_PROTOCOL_API UInventoryDragOperation : public UDragDropOperation
{
	GENERATED_BODY()
	
public:
	UPROPERTY(BlueprintReadOnly)
	TObjectPtr<class UItemDataAsset> ItemData = nullptr; // 드래그하는 아이템 데이터 에셋

	UPROPERTY(BlueprintReadOnly)
	int32 Quantity = 1; // 드래그하는 아이템 수량

	UPROPERTY(BlueprintReadWrite, Category = "Inventory")
	TObjectPtr<class UInventoryComponent> SourceInventory = nullptr; // 드래그 출처 인벤토리 컴포넌트

	UPROPERTY(BlueprintReadWrite, Category = "Inventory")
	FIntPoint FromCell = FIntPoint(-1, -1); // 드래그 출처 셀 좌표

	UPROPERTY(BlueprintReadOnly, Category="Drag")
	FVector2D DragLocalOffset = FVector2D::ZeroVector; // 드래그 시작 시 로컬 오프셋

	UPROPERTY(BlueprintReadOnly, Category = "Drag")
	FVector2D ItemPixelSize = FVector2D::ZeroVector; // 아이템 픽셀 크기
};
