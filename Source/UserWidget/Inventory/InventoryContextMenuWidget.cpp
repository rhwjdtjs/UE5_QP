#include "InventoryContextMenuWidget.h"
#include "Components/Button.h"

void UInventoryContextMenuWidget::InitMenu(const FIntPoint& InCell) // 초기화 함수
{
	Cell = InCell; // 선택된 셀 위치 설정
}

void UInventoryContextMenuWidget::NativeConstruct()
{
	Super::NativeConstruct(); // 부모 클래스의 NativeConstruct 호출

	SetIsFocusable(true); // 위젯을 포커스 가능하게 설정

	if (BtnEquip) BtnEquip->OnClicked.AddDynamic(this, &UInventoryContextMenuWidget::OnClickedBtnEquip); // 장착 버튼 클릭 이벤트 바인딩
	if (BtnDrop)  BtnDrop->OnClicked.AddDynamic(this, &UInventoryContextMenuWidget::OnClickedBtnDrop);   // 버리기 버튼 클릭 이벤트 바인딩
	if (BtnClose) BtnClose->OnClicked.AddDynamic(this, &UInventoryContextMenuWidget::OnClickedBtnClose); // 닫기 버튼 클릭 이벤트 바인딩
}

void UInventoryContextMenuWidget::NativeOnFocusLost(const FFocusEvent& InFocusEvent)
{
	Super::NativeOnFocusLost(InFocusEvent); // 부모 클래스의 NativeOnFocusLost 호출
}

void UInventoryContextMenuWidget::OnClickedBtnEquip()
{
	if(OnEquip.IsBound()) OnEquip.Execute(Cell); // 아이템 사용 델리게이트 실행
	RemoveFromParent(); // 부모에서 제거하여 메뉴 닫기
}

void UInventoryContextMenuWidget::OnClickedBtnDrop()
{
	if (OnDrop.IsBound()) OnDrop.Execute(Cell); // 아이템 버리기 델리게이트 실행
	RemoveFromParent(); // 부모에서 제거하여 메뉴 닫기
}

void UInventoryContextMenuWidget::OnClickedBtnClose()
{
	RemoveFromParent(); // 부모에서 제거하여 메뉴 닫기
}
