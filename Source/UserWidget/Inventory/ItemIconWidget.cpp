#include "ItemIconWidget.h"
#include "Blueprint/WidgetBlueprintLibrary.h"
#include "Components/Image.h"
#include "Components/TextBlock.h"

#include "PJ_Quiet_Protocol/Inventory/ItemDataAsset.h"
#include "PJ_Quiet_Protocol/Inventory/InventoryComponent.h"
#include "InventoryDragOperation.h"
#include "ItemDragVisualWidget.h"
#include "InventoryGridWidget.h"
#include "PJ_Quiet_Protocol/Character/QPCharacter.h"
#include "InventoryContextMenuWidget.h"
#include "Blueprint/WidgetLayoutLibrary.h"

void UItemIconWidget::Setup(UInventoryComponent* InInventory, UItemDataAsset* InItemData, int32 InQuantity, const FIntPoint& InFrom, const FIntPoint& InItemSize, float InCellSize, TSubclassOf<UUserWidget> InDragVisualClass, UInventoryGridWidget* InOwningGrid)
{
	Inventory = InInventory; // 소유한 인벤토리 컴포넌트 설정
	ItemData = InItemData; // 아이템 데이터 에셋 설정
	Quantity = InQuantity; // 수량 설정
	From = InFrom; // 그리드 내 위치 설정

	ItemSize = InItemSize; // 아이템 크기 설정
	ItemSize.X = FMath::Max(1, ItemSize.X); // 최소 크기 1로 설정
	ItemSize.Y = FMath::Max(1, ItemSize.Y); // 최소 크기 1로 설정

	CellSize = InCellSize; // 셀 크기 설정
	DragVisualClass = InDragVisualClass; // 드래그 비주얼 클래스 설정
	OwningGrid = InOwningGrid; // 소유한 그리드 위젯 설정

	ApplyVisual(); // 시각적 요소 적용
}

FReply UItemIconWidget::NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
	if (InMouseEvent.IsMouseButtonDown(EKeys::RightMouseButton)) //우클릭: 컨텍스트 메뉴 열기
	{
		if (!ContextMenuClass) return FReply::Handled(); // 컨텍스트 메뉴 클래스가 유효한지 확인

		if (OpenedMenu) // 이미 열려있는 메뉴가 있으면 닫기
		{
			OpenedMenu->RemoveFromParent(); // 메뉴 제거
			OpenedMenu = nullptr; // 포인터 초기화
		}

		OpenedMenu = CreateWidget<UInventoryContextMenuWidget>(GetOwningPlayer(), ContextMenuClass); // 컨텍스트 메뉴 위젯 생성
		if (!OpenedMenu) return FReply::Handled(); // 메뉴 생성 실패 시 처리

		OpenedMenu->InitMenu(From); // 메뉴 초기화

		OpenedMenu->OnEquip.BindLambda([this](const FIntPoint& Cell) //아이템 장착 콜백 바인딩
			{
				if (AQPCharacter* Character = Cast<AQPCharacter>(GetOwningPlayerPawn())) // 플레이어 폰을 QPCharacter로 캐스팅
				{
					Character->EquipInventoryItemAt(Cell); // 아이템 장착
				}
			});

		OpenedMenu->OnDrop.BindLambda([this](const FIntPoint& Cell) //아이템 버리기 콜백 바인딩
			{
				if (AQPCharacter* Character = Cast<AQPCharacter>(GetOwningPlayerPawn())) // 플레이어 폰을 QPCharacter로 캐스팅
				{
					Character->DropInventoryItemAt(Cell); // 아이템 버리기
				}
			});

		OpenedMenu->AddToViewport(9999); // 메뉴를 뷰포트에 추가

		APlayerController* PlayerController = GetOwningPlayer(); // 플레이어 컨트롤러 가져오기
		if (!PlayerController) return FReply::Handled(); // 플레이어 컨트롤러
		float X = 0.f, Y = 0.f;
		if(PlayerController->GetMousePosition(X, Y)) // 마우스 위치 가져오기
		{
			const FVector2D MousePosition(X, Y); // 마우스 위치 설정
			OpenedMenu->SetPositionInViewport(MousePosition, false); // 메뉴 위치 설정
		}

		OpenedMenu->SetKeyboardFocus(); // 메뉴에 키보드 포커스 설정
		return FReply::Handled(); // 처리 완료 반환
	}

	if (InMouseEvent.IsMouseButtonDown(EKeys::LeftMouseButton)) // 좌클릭: 드래그 시작
	{
		return UWidgetBlueprintLibrary::DetectDragIfPressed(InMouseEvent, this, EKeys::LeftMouseButton).NativeReply; // 드래그 감지 시작
	}

	return Super::NativeOnMouseButtonDown(InGeometry, InMouseEvent); // 기본 동작 호출
}

void UItemIconWidget::NativeOnDragDetected(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent, UDragDropOperation*& OutOperation)
{
	Super::NativeOnDragDetected(InGeometry, InMouseEvent, OutOperation); // 기본 동작 호출

	if (!Inventory || !ItemData) return; // 인벤토리 컴포넌트와 아이템 데이터가 유효한지 확인

	UInventoryDragOperation* DragOp = Cast<UInventoryDragOperation>(
		UWidgetBlueprintLibrary::CreateDragDropOperation(UInventoryDragOperation::StaticClass())
	); // 드래그 드롭 오퍼레이션 생성
	if (!DragOp) return;

	DragOp->SourceInventory = Inventory; // 드래그 출처 인벤토리 설정
	DragOp->FromCell = From; // 드래그 출처 셀 위치 설정
	DragOp->ItemData = ItemData; // 드래그 아이템 데이터 설정
	DragOp->Quantity = Quantity; // 드래그 수량 설정

	DragOp->DragLocalOffset = InGeometry.AbsoluteToLocal(InMouseEvent.GetScreenSpacePosition()); // 드래그 로컬 오프셋 설정
	DragOp->ItemPixelSize = FVector2D(ItemSize.X * CellSize, ItemSize.Y * CellSize); // 아이템 픽셀 크기 설정
	if (DragVisualClass) // 드래그 비주얼 클래스가 유효한지 확인
	{
		UUserWidget* Visual = CreateWidget<UUserWidget>(GetOwningPlayer(), DragVisualClass); // 드래그 비주얼 위젯 생성
		if (Visual) // 비주얼 위젯이 유효한지 확인
		{
			Visual->SetDesiredSizeInViewport(DragOp->ItemPixelSize); // 뷰포트 내 원하는 크기 설정
			if (UItemDragVisualWidget* DragVisual = Cast<UItemDragVisualWidget>(Visual)) // 비주얼 위젯을 아이템 드래그 비주얼 위젯으로 캐스팅
			{
				DragVisual->SetVisual(ItemData, Quantity, DragOp->ItemPixelSize); // 비주얼 설정
			}

			DragOp->DefaultDragVisual = Visual; // 드래그 비주얼 설정
		}
	}

	DragOp->Pivot = EDragPivot::MouseDown; // 드래그 피벗 설정
	OutOperation = DragOp; // 출력 오퍼레이션 설정
}


bool UItemIconWidget::NativeOnDrop(const FGeometry& InGeometry, const FDragDropEvent& InDragDropEvent, UDragDropOperation* InOperation)
{
	// 아이콘 위에 드랍해도 Grid가 처리하게 전달
	if (OwningGrid) // 소유한 그리드 위젯이 유효한지 확인
	{
		const FVector2D ScreenPos = InDragDropEvent.GetScreenSpacePosition(); // 스크린 위치 가져오기
		return OwningGrid->HandleDropFromScreenPos(InOperation, ScreenPos); // 드롭 처리
	}
	return false; // 드롭 처리 실패 반환
}

void UItemIconWidget::NativeOnDragCancelled(const FDragDropEvent& InDragDropEvent, UDragDropOperation* InOperation)
{
	Super::NativeOnDragCancelled(InDragDropEvent, InOperation); // 기본 동작 호출
}

void UItemIconWidget::ApplyVisual()
{
	if (ItemImage && ItemData && ItemData->ItemIcon) // 아이템 이미지와 데이터가 유효한지 확인
	{
		ItemImage->SetBrushFromTexture(ItemData->ItemIcon); // 아이템 이미지 설정
	}

	if (QuantityText) // 수량 텍스트가 유효한지 확인
	{
		if (Quantity > 1) // 수량이 0보다 큰지 확인
		{
			QuantityText->SetText(FText::AsNumber(Quantity)); // 수량 텍스트 설정
			QuantityText->SetVisibility(ESlateVisibility::HitTestInvisible); // 텍스트 가시성 설정
		}
		else
		{
			QuantityText->SetText(FText::GetEmpty()); // 수량 텍스트 비우기
			QuantityText->SetVisibility(ESlateVisibility::Collapsed); // 텍스트 가시성 설정
		}
	}
}
