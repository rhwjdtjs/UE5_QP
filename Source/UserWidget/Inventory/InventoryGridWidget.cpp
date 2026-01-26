#include "InventoryGridWidget.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/SizeBox.h"
#include "PJ_Quiet_Protocol/Inventory/InventoryComponent.h"
#include "PJ_Quiet_Protocol/Inventory/InventoryHeaders/InventoryItem.h"
#include "PJ_Quiet_Protocol/Inventory/ItemDataAsset.h"
#include "ItemIconWidget.h"
#include "InventoryDragOperation.h"

void UInventoryGridWidget::NativeConstruct()
{
	Super::NativeConstruct(); // 부모 클래스의 NativeConstruct 호출
}

void UInventoryGridWidget::SetInventory(UInventoryComponent* InInventory)
{
	Inventory = InInventory; // 인벤토리 컴포넌트 설정
}

void UInventoryGridWidget::RefreshGrid()
{
	if (!Inventory || !CellsLayer || !ItemsLayer) return; // 인벤토리와 레이어가 유효한지 확인
	UE_LOG(LogTemp, Warning, TEXT("[Grid1] Inv=%s Cells=%s Items=%s SizeBox=%s W=%d H=%d Cell=%.1f"),
		Inventory ? TEXT("OK") : TEXT("NULL"),
		CellsLayer ? TEXT("OK") : TEXT("NULL"),
		ItemsLayer ? TEXT("OK") : TEXT("NULL"),
		GridSizeBox ? TEXT("OK") : TEXT("NULL"),
		Inventory ? Inventory->Width : -1,
		Inventory ? Inventory->Height : -1,
		CellSize
	);
	if (GridSizeBox) // GridSizeBox가 유효한지 확인
	{
		GridSizeBox->SetWidthOverride(Inventory->Width * CellSize); // 가로 크기 설정
		GridSizeBox->SetHeightOverride(Inventory->Height * CellSize); // 세로 크기 설정
	} 
	UE_LOG(LogTemp, Warning, TEXT("[Grid2] Inv=%s Cells=%s Items=%s SizeBox=%s W=%d H=%d Cell=%.1f"),
		Inventory ? TEXT("OK") : TEXT("NULL"),
		CellsLayer ? TEXT("OK") : TEXT("NULL"),
		ItemsLayer ? TEXT("OK") : TEXT("NULL"),
		GridSizeBox ? TEXT("OK") : TEXT("NULL"),
		Inventory ? Inventory->Width : -1,
		Inventory ? Inventory->Height : -1,
		CellSize
	);
	CellsLayer->ClearChildren(); // 셀 레이어 초기화
	ItemsLayer->ClearChildren(); // 아이템 레이어 초기화

	BuildCells(); // 셀 구성
	BuildItems(); // 아이템 구성
	UE_LOG(LogTemp, Warning, TEXT("[Grid3] Inv=%s Cells=%s Items=%s SizeBox=%s W=%d H=%d Cell=%.1f"),
		Inventory ? TEXT("OK") : TEXT("NULL"),
		CellsLayer ? TEXT("OK") : TEXT("NULL"),
		ItemsLayer ? TEXT("OK") : TEXT("NULL"),
		GridSizeBox ? TEXT("OK") : TEXT("NULL"),
		Inventory ? Inventory->Width : -1,
		Inventory ? Inventory->Height : -1,
		CellSize
	);
}

void UInventoryGridWidget::BuildCells()
{
	if (!Inventory || !CellsLayer || !CellWidgetClass) return; // 인벤토리와 레이어, 셀 위젯 클래스가 유효한지 확인

	for (int32 y = 0; y < Inventory->Height; ++y) // 세로 셀 반복
	{
		for (int32 x = 0; x < Inventory->Width; ++x) // 가로 셀 반복
		{
			UUserWidget* Cell = CreateWidget<UUserWidget>(GetOwningPlayer(), CellWidgetClass); // 셀 위젯 생성
			if (!Cell) continue; // 생성 실패 시 다음으로

			UCanvasPanelSlot* CellCanvasSlot = CellsLayer->AddChildToCanvas(Cell); // 셀 레이어에 추가
			if (!CellCanvasSlot) continue; // 추가 실패 시 다음으로

			CellCanvasSlot->SetAutoSize(false); // 자동 크기 조정 비활성화
			CellCanvasSlot->SetPosition(FVector2D(x * CellSize, y * CellSize)); // 위치 설정
			CellCanvasSlot->SetSize(FVector2D(CellSize, CellSize)); // 크기 설정
		}
	}
}

void UInventoryGridWidget::BuildItems() // 아이템 구성
{
	if (!Inventory || !ItemsLayer || !ItemIconWidgetClass) return; // 인벤토리와 레이어, 아이템 아이콘 위젯 클래스가 유효한지 확인

	for (const FInventorySlot& InvSlot : Inventory->Slots) // 인벤토리 슬롯 반복
	{
		if (!InvSlot.Item.ItemData) continue; // 아이템 데이터가 유효하지 않으면 다음으로

		UItemDataAsset* ItemData = InvSlot.Item.ItemData; // 아이템 데이터 가져오기

		FIntPoint Size = ItemData->ItemSize; // 아이템 크기 가져오기
		Size.X = FMath::Max(1, Size.X); // 최소 크기 1로 설정
		Size.Y = FMath::Max(1, Size.Y); // 최소 크기 1로 설정

		UItemIconWidget* Icon = CreateWidget<UItemIconWidget>(GetOwningPlayer(), ItemIconWidgetClass); // 아이템 아이콘 위젯 생성
		if (!Icon) continue; // 생성 실패 시 다음으로

		Icon->Setup(Inventory, ItemData, InvSlot.Item.Quantity, InvSlot.Position, Size, CellSize, DragVisualClass, this); // 아이템 아이콘 설정

		UCanvasPanelSlot* CanvasSlot = ItemsLayer->AddChildToCanvas(Icon); // 아이템 레이어에 추가
		if (!CanvasSlot) continue; // 추가 실패 시 다음으로

		CanvasSlot->SetAutoSize(false); // 자동 크기 조정 비활성화
		CanvasSlot->SetPosition(FVector2D(InvSlot.Position.X * CellSize, InvSlot.Position.Y * CellSize)); // 위치 설정
		CanvasSlot->SetSize(FVector2D(Size.X * CellSize, Size.Y * CellSize)); // 크기 설정
	}
}

bool UInventoryGridWidget::NativeOnDrop(const FGeometry& InGeometry, const FDragDropEvent& InDragDropEvent, UDragDropOperation* InOperation)
{
	return HandleDropFromScreenPos(InOperation, InDragDropEvent.GetScreenSpacePosition()); // 화면 좌표에서 드롭 처리
}

bool UInventoryGridWidget::HandleDropFromScreenPos(UDragDropOperation* Operation, const FVector2D& ScreenPos)
{
	if (!Inventory || !Operation || !CellsLayer) return false; // 인벤토리와 오퍼레이션, 셀 레이어가 유효한지 확인

	UInventoryDragOperation* DragOp = Cast<UInventoryDragOperation>(Operation); // 드래그 오퍼레이션 캐스트
	if (!DragOp || !DragOp->SourceInventory) return false; //	드래그 오퍼레이션과 소스 인벤토리가 유효한지 확인

	const FVector2D TopLeftScreen = ScreenPos - DragOp->DragLocalOffset; // 드롭할 아이템의 좌측 상단 스크린 좌표 계산

	// 드롭 위치(셀) 계산
	FIntPoint ToCell(0, 0); // 드롭할 셀 좌표 초기화
	{
		const FGeometry Geo = CellsLayer->GetCachedGeometry();
		const FVector2D Local = Geo.AbsoluteToLocal(TopLeftScreen);
		const int32 X = FMath::FloorToInt(Local.X / CellSize);
		const int32 Y = FMath::FloorToInt(Local.Y / CellSize);
		ToCell = FIntPoint(X, Y);
	} // 드롭 위치(셀) 계산 끝

	// 범위 체크(드롭 대상 = 이 그리드의 인벤토리 크기)
	if (ToCell.X < 0 || ToCell.Y < 0 || ToCell.X >= Inventory->Width || ToCell.Y >= Inventory->Height)
	{
		return false;
	}

	bool bResult = false; // 결과 플래그 초기화
	if (DragOp->SourceInventory == Inventory) bResult = Inventory->MoveItem(DragOp->FromCell, ToCell); // 같은 인벤토리 내에서 이동하는 경우
	else
	{
		// 다른 인벤토리에서 넘어오는 경우(월드/다른 그리드 등)
		if (Inventory->AddItemAt(DragOp->ItemData, DragOp->Quantity, ToCell))
		{
			DragOp->SourceInventory->RemoveItemAt(DragOp->FromCell); // 원본 인벤토리에서 아이템 제거
			bResult = true; // 성공 플래그 설정
		}
	}

	if (bResult) // 성공한 경우 그리드 새로고침
	{
		RefreshGrid(); // 그리드 새로고침
	}

	return bResult; // 결과 반환
	
}

bool UInventoryGridWidget::ScreenToCell(const FVector2D& ScreenPos, FIntPoint& OutCell) const
{
	if (!Inventory) return false; // 인벤토리가 유효한지 확인

	const FGeometry Geo = GetCachedGeometry(); // 위젯의 기하학 정보 가져오기
	const FVector2D Local = Geo.AbsoluteToLocal(ScreenPos); // 화면 좌표를 로컬 좌표로 변환

	const int32 X = FMath::FloorToInt(Local.X / CellSize); // 셀 X 좌표 계산
	const int32 Y = FMath::FloorToInt(Local.Y / CellSize); // 셀 Y 좌표 계산

	if (X < 0 || Y < 0 || X >= Inventory->Width || Y >= Inventory->Height) //	셀 좌표가 인벤토리 범위를 벗어나는지 확인
	{
		return false; // 범위를 벗어나면 false 반환
	}

	OutCell = FIntPoint(X, Y); // 출력 셀 좌표 설정
	return true; // 성공 반환
}
