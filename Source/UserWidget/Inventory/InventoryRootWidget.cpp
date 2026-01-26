#include "InventoryRootWidget.h"
#include "InventoryGridWidget.h"
#include "PJ_Quiet_Protocol/Character/QPCharacter.h"
#include "PJ_Quiet_Protocol/Inventory/InventoryComponent.h"


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

void UInventoryRootWidget::HandleInventoryChanged()
{
	if (InventoryGrid) InventoryGrid->RefreshGrid(); // 인벤토리 그리드 새로고침
}
