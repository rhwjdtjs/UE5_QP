#include "InventoryRootWidget.h"
#include "InventoryGridWidget.h"
#include "PJ_Quiet_Protocol/Character/QPCharacter.h"
#include "PJ_Quiet_Protocol/Inventory/InventoryComponent.h"
#include "InventoryDragOperation.h" 

void UInventoryRootWidget::NativeConstruct()
{
	Super::NativeConstruct();

	AQPCharacter* Character = Cast<AQPCharacter>(GetOwningPlayerPawn()); // 소유한 플레이어 폰을 AQPCharacter로 캐스팅
	if (!Character || !InventoryGrid) return; // 캐스팅 실패 또는 인벤토리 그리드가 유효하지 않으면 종료

	CachedInventory = Character->GetInventoryComponent(); // 캐릭터의 인벤토리 컴포넌트 가져오기
	if (!CachedInventory) return; // 인벤토리 컴포넌트가 유효하지 않으면 종료

	// 델리게이트 바인딩
	CachedInventory->OnInventoryChanged.RemoveDynamic(this, &UInventoryRootWidget::HandleInventoryChanged); // 중복 바인딩 방지
	CachedInventory->OnInventoryChanged.AddDynamic(this, &UInventoryRootWidget::HandleInventoryChanged); // 인벤토리 변경 시 HandleInventoryChanged 호출
	// 최초 1회 그리기 
	InventoryGrid->SetInventory(CachedInventory); // 인벤토리 그리드에 인벤토리 설정
	InventoryGrid->RefreshGrid(); // 그리드 새로고침
}

void UInventoryRootWidget::NativeDestruct()
{
	if (CachedInventory) // 캐시된 인벤토리가 유효한 경우
	{
		CachedInventory->OnInventoryChanged.RemoveDynamic(this, &UInventoryRootWidget::HandleInventoryChanged); // 델리게이트 해제
		CachedInventory = nullptr; // 캐시된 인벤토리 포인터 초기화
	}
	Super::NativeDestruct(); // 부모 클래스의 NativeDestruct 호출
}

bool UInventoryRootWidget::NativeOnDrop(const FGeometry& InGeometry, const FDragDropEvent& InDragDropEvent, UDragDropOperation* InOperation)
{
	if (!InOperation) return false; // 오퍼레이션이 없으면 실패
	if (!InventoryGrid) return false; // 그리드가 없으면 실패

	UInventoryDragOperation* DragOp = Cast<UInventoryDragOperation>(InOperation); // 인벤 드래그인지 캐스팅
	if (!DragOp) return false; // 인벤 드래그가 아니면 실패
	if (!DragOp->SourceInventory) return false; // 소스 인벤이 없으면 실패

	const FVector2D ScreenPos = InDragDropEvent.GetScreenSpacePosition(); // 드랍된 화면 좌표

	const FGeometry GridGeo = InventoryGrid->GetCachedGeometry(); // 그리드의 지오메트리
	const bool bOverGrid = GridGeo.IsUnderLocation(ScreenPos); // 드랍 위치가 그리드 위인지 확인

	if (bOverGrid)
	{
		//그리드 위에 떨어졌으면 그리드가 정상적으로 Move 또는 Add 처리
		return InventoryGrid->HandleDropFromScreenPos(InOperation, ScreenPos); // 그리드로 전달
	}

	//그리드 밖으로 떨어졌으면 월드로 드랍 처리
	AQPCharacter* Character = Cast<AQPCharacter>(GetOwningPlayerPawn()); // 소유 캐릭터
	if (!Character) return false; // 캐릭터가 없으면 실패

	Character->DropInventoryItemAt(DragOp->FromCell); // 인벤 아이템을 월드로 드랍

	return true; // 드랍 처리 완료
}

void UInventoryRootWidget::HandleInventoryChanged()
{
	if (InventoryGrid) InventoryGrid->RefreshGrid(); // 인벤토리 그리드 새로고침
}
