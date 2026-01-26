// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "ItemDragVisualWidget.generated.h"

/**
 * 
 */
UCLASS()
class PJ_QUIET_PROTOCOL_API UItemDragVisualWidget : public UUserWidget
{
	GENERATED_BODY()
	
public:
	UFUNCTION(BlueprintCallable)
	void SetVisual(class UItemDataAsset* InItemData, int32 InQuantity, const FVector2D& InPixelSize); // 아이템 데이터와 수량을 설정하는 함수

protected:
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<class UImage> ItemImage; // 아이템 이미지를 표시하는 위젯

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<class UTextBlock> QuantityText; // 아이템 수량을 표시하는 위젯

	UPROPERTY(meta=(BindWidget))
	TObjectPtr<class USizeBox> RootSizeBox; // 아이템 크기를 조절
};
