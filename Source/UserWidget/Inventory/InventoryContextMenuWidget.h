#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "InventoryContextMenuWidget.generated.h"

DECLARE_DELEGATE_OneParam(FOnInventoryContextAction, const FIntPoint& /*Cell*/);

UCLASS()
class PJ_QUIET_PROTOCOL_API UInventoryContextMenuWidget : public UUserWidget
{
	GENERATED_BODY()
public:
	void InitMenu(const FIntPoint& InCell); // 초기화 함수

	FOnInventoryContextAction OnEquip; // 아이템 사용 델리게이트
	FOnInventoryContextAction OnDrop;  // 아이템 버리기 델리게이트

protected:
	virtual void NativeConstruct() override; // 위젯이 생성될 때 호출되는 함수
	virtual void NativeOnFocusLost(const FFocusEvent& InFocusEvent) override; // 포커스를 잃었을 때 호출되는 함수

private:
	UPROPERTY(meta=(BindWidget))
	TObjectPtr<class UButton> BtnEquip; // 장착 버튼
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<class UButton> BtnDrop;  // 버리기 버튼
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<class UButton> BtnClose; // 닫기 버튼

	UFUNCTION() void OnClickedBtnEquip(); // 장착 버튼 클릭 핸들러
	UFUNCTION() void OnClickedBtnDrop();  // 버리기 버튼 클릭 핸들러
	UFUNCTION() void OnClickedBtnClose(); // 닫기 버튼 클릭 핸들러

	FIntPoint Cell = FIntPoint(-1, -1); // 선택된 셀 위치
};
