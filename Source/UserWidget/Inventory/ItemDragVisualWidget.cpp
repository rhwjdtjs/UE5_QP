#include "ItemDragVisualWidget.h"
#include "Components/Image.h"
#include "Components/TextBlock.h"
#include "PJ_Quiet_Protocol/Inventory/ItemDataAsset.h"
#include "Components/SizeBox.h"

void UItemDragVisualWidget::SetVisual(UItemDataAsset* InItemData, int32 InQuantity, const FVector2D& InPixelSize)
{
	if (RootSizeBox)
	{
		RootSizeBox->SetWidthOverride(InPixelSize.X); // 루트 사이즈 박스의 너비 설정
		RootSizeBox->SetHeightOverride(InPixelSize.Y); // 루트 사이즈 박스의 높이 설정
	}
	if(ItemImage && InItemData&& InItemData->ItemIcon)
	{
		ItemImage->SetBrushFromTexture(InItemData->ItemIcon); // 아이템 이미지 설정

		FSlateBrush Brush = ItemImage->GetBrush(); // 현재 브러시 가져오기
		Brush.ImageSize = InPixelSize; // 브러시 이미지 크기 설정
		ItemImage->SetBrush(Brush); // 브러시 적용
	}
	if(QuantityText)
	{
		if (InQuantity > 0)
		{
			QuantityText->SetText(FText::AsNumber(InQuantity)); // 수량 텍스트 설정
			QuantityText->SetVisibility(ESlateVisibility::Visible); // 텍스트 가시성 설정
		}
		else
		{
			QuantityText->SetText(FText::GetEmpty()); // 빈 텍스트 설정
			QuantityText->SetVisibility(ESlateVisibility::Hidden); // 텍스트 숨김
		}
	}
}
